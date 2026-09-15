#include "gitissues/allocator.h"
#include "gitissues/defines.h"
#include "gitissues/ecs/registry.h"
#include "gitissues/ecs/string_map.h"
#include "gitissues/iff/schema.h"
#include "gitissues/umbra_string.h"
#include <gitissues/iff/iff.h>

#include <ctype.h>
#include <inttypes.h>
#include <time.h>

static void skipWhitespace(char const *fileContentPtr, uint32_t *pos) {
  while (isspace(fileContentPtr[*pos])) {
    (*pos)++;
  }
}

static void completeTagMetadataIfInvalid(struct Registry *registry,
                                         struct UmbraString const tagString,
                                         struct TagMetadata *tagMeta,
                                         enum SchemaPropertyTypes type) {
  if (tagMeta->tagID != _GITISSUES_COMPONENT_INVALID) {
    return;
  }

  DEBUG_ASSERT(!isRegistered(registry, tagString),
               "Expected tag to be unregistered");

  // TODO: attaching lifetime to registry is a hack
  struct UmbraString ownedTagString =
      copyUmbraStringBlock(tagString, &registry->lifetimeAllocations);

  ComponentID tagID = registerComponentID(registry, ownedTagString,
                                          getSizeOfSchemaProperty(type));

  tagMeta->tagID = tagID;
  tagMeta->type = type;
  tagMeta->name = ownedTagString;
}

static bool isTerminatorAtIndex(char const *fileContentPtr, uint32_t pos,
                                struct Schema const *schema) {
  return strncmp(getUmbraPtrConst(&schema->terminator), fileContentPtr + pos,
                 schema->terminator.size) == 0;
}

static void readIFFUnquotedString(char const *fileContentPtr, uint32_t *pos,
                                  struct BlockAllocator *allocator,
                                  struct Issue issue, struct Schema *schema,
                                  struct TagMetadata *tagMeta,
                                  struct UmbraString const tagString) {
  DEBUG_ASSERT(fileContentPtr[*pos] != '"',
               "Expected unquoted string, got quote");

  uint32_t start = *pos;

  // Read in until we find a whitespace or terminator
  // TODO: also have to stop reading at terminator
  while (!isspace(fileContentPtr[*pos]) && fileContentPtr[*pos] != '\0' &&
         !isTerminatorAtIndex(fileContentPtr, *pos, schema)) {
    (*pos)++;
  }

  struct UmbraString string;
  createUmbraStringBoundAllocate(&string, fileContentPtr + start, *pos - start,
                                 allocator);

  completeTagMetadataIfInvalid(&schema->registry, tagString, tagMeta,
                               SCHEMA_TYPE_STRING);

  addTagById(&schema->registry, issue, tagMeta->tagID, (uint8_t *)&string);
}

static bool readIFFDate(char const *fileContentPtr, uint32_t *pos,
                        struct Issue issue, struct TagMetadata *tagMeta,
                        struct UmbraString const tagString,
                        struct Schema *schema) {
  // TODO: if any of these formats match a terminator, then do not consider
  struct SchemaDate date = {0};
  uint32_t curr = *pos;
  uint32_t digitCount[3] = {0};

  for (uint32_t i = 0; i < CARRAY_SIZE(schema->dateFormatParts); i++) {
    struct DateFormatPart const formatPart = schema->dateFormatParts[i];

    if (fileContentPtr[curr] == '\0') {
      return false;
    }

    if (formatPart.expectedLength == 0) {
      continue;
    }

    if (formatPart.part == DATE_PART_SEPARATOR) {
      // Expect the separator character
      if (fileContentPtr[curr] != formatPart.separator) {
        return false;
      }

      ++curr;

      continue;
    }

    // Expect a digit
    while (fileContentPtr[curr] != '\0' && isdigit(fileContentPtr[curr])) {
      uint32_t value = fileContentPtr[curr] - '0';

      switch (formatPart.part) {
      case DATE_PART_YEAR:
        date.year = date.year * 10 + value;
        ++digitCount[DATE_PART_YEAR];
        break;
      case DATE_PART_MONTH:
        date.month = date.month * 10 + value;
        ++digitCount[DATE_PART_MONTH];
        break;
      case DATE_PART_DAY:
        date.day = date.day * 10 + value;
        ++digitCount[DATE_PART_DAY];
        break;
      default:
        break; // unreachable
      }

      ++curr;
    }

    // Invalid date
    if (formatPart.expectedLength != 1 &&
        digitCount[formatPart.part] != (uint32_t)formatPart.expectedLength) {

      GITISSUES_LOG_DEBUG(
          "Invalid date format, got different number of digits to expected");
      return false;
    }
  }

  // Only day, month, fill in year
  if (date.year == 0) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    date.year = tm->tm_year + 1900;
  }

  // TODO: check against month, and leap years, if bothered
  NDEBUG_ASSERT(date.day <= 31, "Invalid day");
  NDEBUG_ASSERT(date.month <= 12, "Invalid month");

  GITISSUES_LOG_DEBUG("Reading date %d-%d-%d", date.year, date.month, date.day);

  completeTagMetadataIfInvalid(&schema->registry, tagString, tagMeta,
                               SCHEMA_TYPE_DATE);

  addTagById(&schema->registry, issue, tagMeta->tagID, (uint8_t *)&date);

  *pos = curr;

  return true;
}

static void readIFFString(char const *fileContentPtr, uint32_t *pos,
                          struct BlockAllocator *allocator, struct Issue issue,
                          struct Registry *registry,
                          struct TagMetadata *tagMeta,
                          struct UmbraString const tagString) {
  // TODO: this assert missed?
  NDEBUG_ASSERT(fileContentPtr[*pos] == '"',
                "Expected quote at start of string, got [%c]",
                fileContentPtr[*pos]);

  // Skip first quote, read until final quote
  uint32_t start = ++(*pos);

  // TODO: escaped strings
  while (fileContentPtr[*pos] != '\0' && fileContentPtr[*pos] != '"') {
    (*pos)++;
  }

  NDEBUG_ASSERT(fileContentPtr[*pos] == '"',
                "Never found end quotea; reached EOF");

  ++(*pos);

  struct UmbraString string;
  // TODO: check bounds
  createUmbraStringBoundAllocate(&string, fileContentPtr + start, *pos - start,
                                 allocator);

  completeTagMetadataIfInvalid(registry, tagString, tagMeta,
                               SCHEMA_TYPE_STRING);

  addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&string);
}

static bool readIFFInt(char const *fileContentPtr, uint32_t *pos,
                       struct Issue issue, struct Registry *registry,
                       struct TagMetadata *tagMeta,
                       struct UmbraString const tagString) {
  NDEBUG_ASSERT(isdigit(fileContentPtr[*pos]) || fileContentPtr[*pos] == '-',
                "Expected digit at start of number");

  int64_t integer;
  int consumed;

  if (sscanf(fileContentPtr + (*pos), "%" SCNd64 "%n ", &integer, &consumed) ==
      1) {
    if (!isspace(fileContentPtr[*pos + consumed])) {
      return false;
    }

    *pos += consumed;
  } else {
    // Failed to read int, might be something else
    return false;
  }

  completeTagMetadataIfInvalid(registry, tagString, tagMeta, SCHEMA_TYPE_INT64);

  addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&integer);

  return true;
}

static bool readIFFFloat(char const *fileContentPtr, uint32_t *pos,
                         struct Issue issue, struct Registry *registry,
                         struct TagMetadata *tagMeta,
                         struct UmbraString const tagString) {
  // TODO: support .132
  NDEBUG_ASSERT(isdigit(fileContentPtr[*pos]) || fileContentPtr[*pos] == '-',
                "Expected digit at start of number");

  double floating;
  int consumed;

  // TODO: code duplication of readInt and readFloat
  // TODO: code duplication of JSON read functions
  // TODO: reparsing section again to test for floats
  if (sscanf(fileContentPtr + (*pos), "%lf%n ", &floating, &consumed) == 1) {
    if (!isspace(fileContentPtr[*pos + consumed])) {
      return false;
    }

    *pos += consumed;
  } else {
    // Failed to read float, might be something else
    return false;
  }

  completeTagMetadataIfInvalid(registry, tagString, tagMeta,
                               SCHEMA_TYPE_FLOAT64);

  addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&floating);

  return true;
}

// Returns success, not value
static bool readIFFBoolean(char const *fileContentPtr, uint32_t *pos,
                           struct Issue issue, struct Registry *registry,
                           struct TagMetadata *tagMeta,
                           struct UmbraString const tagString) {
  uint32_t curr = *pos;

  char trueChars[] = "true";
  char falseChars[] = "false";

  // Try read "true"
  for (uint32_t i = 0; i < 4; i++) {
    if (fileContentPtr[curr + i] == trueChars[i]) {
      curr++;
    } else {
      break;
    }

    if (i == 4) {
      // We got value "true"
      bool t = true;

      completeTagMetadataIfInvalid(registry, tagString, tagMeta,
                                   SCHEMA_TYPE_BOOLEAN);

      addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&t);

      *pos = curr;

      return true;
    }
  }

  // Try read "false"
  for (uint32_t i = 0; i < 5; i++) {
    if (fileContentPtr[curr + i] == falseChars[i]) {
      curr++;
    } else {
      break;
    }

    if (i == 5) {
      // We got value "false"
      bool f = false;

      completeTagMetadataIfInvalid(registry, tagString, tagMeta,
                                   SCHEMA_TYPE_BOOLEAN);

      addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&f);

      *pos = curr;

      return true;
    }
  }

  return false;
}

static void readIFFValue(char const *fileContentPtr, uint32_t *pos,
                         struct Issue issue, struct BlockAllocator *allocator,
                         struct TagMetadata *tagMeta, struct Schema *schema,
                         struct UmbraString const tagString) {
  // Read in either a empty, string, int, float, boolean, or date

  // Look at the type of the tag (if we have it)
  if (tagMeta->tagID != _GITISSUES_COMPONENT_INVALID) {
    // if tag not null, jump to the relevant readString/readInt/readFloat
    // function

    // quoted string -> string
    // otherwise, parse to next whitespace
    // only numbers -> int
    // numbers + decimal -> float
    // exactly "true" or "false" -> boolean
    // numbers with dash or dot or slash separators -> date (must be 3 parts)
    // everything else -> string
    switch (tagMeta->type) {
    case SCHEMA_TYPE_STRING:
      // TODO: deal with unquoted strings
      if (fileContentPtr[*pos] == '"') {

        readIFFString(fileContentPtr, pos, allocator, issue, &schema->registry,
                      tagMeta, tagString);
      } else {
        readIFFUnquotedString(fileContentPtr, pos, allocator, issue, schema,
                              tagMeta, tagString);
      }

      return;
    case SCHEMA_TYPE_INT64:
      NDEBUG_ASSERT(readIFFInt(fileContentPtr, pos, issue, &schema->registry,
                               tagMeta, tagString),
                    "Failed to read int when expected");
      return;
    case SCHEMA_TYPE_FLOAT64:
      NDEBUG_ASSERT(readIFFFloat(fileContentPtr, pos, issue, &schema->registry,
                                 tagMeta, tagString),
                    "Failed to read float when expected");
      return;
    case SCHEMA_TYPE_BOOLEAN:
      NDEBUG_ASSERT(readIFFBoolean(fileContentPtr, pos, issue,
                                   &schema->registry, tagMeta, tagString),
                    "Failed to read boolean when expected");
      return;
    case SCHEMA_TYPE_DATE:
      NDEBUG_ASSERT(
          readIFFDate(fileContentPtr, pos, issue, tagMeta, tagString, schema),
          "Failed to read date when expected");
      return;
    case SCHEMA_TYPE_EMPTY: {
      uint8_t empty = 0;
      // TODO: no official ECS support for empty components
      addTagById(&schema->registry, issue, tagMeta->tagID, (uint8_t *)&empty);
      return;
    }
    default:
      NDEBUG_ASSERT(false, "Unknown schema type");
      break;
    }

    NDEBUG_ASSERT(false, "Failed to read schema type, poorly formatted?");

    return;
  }

  // We need to figure out the type, and register the tag
  if (fileContentPtr[*pos] == '"') {
    readIFFString(fileContentPtr, pos, allocator, issue, &schema->registry,
                  tagMeta, tagString);

    return;
  }

  if (isdigit(fileContentPtr[*pos]) || fileContentPtr[*pos] == '-') {
    if (readIFFInt(fileContentPtr, pos, issue, &schema->registry, tagMeta,
                   tagString))
      return;
    if (readIFFFloat(fileContentPtr, pos, issue, &schema->registry, tagMeta,
                     tagString))
      return;
  }

  if (isdigit(fileContentPtr[*pos])) {
    if (readIFFDate(fileContentPtr, pos, issue, tagMeta, tagString, schema))
      return;
  }

  if (fileContentPtr[*pos] == 't' || fileContentPtr[*pos] == 'f') {
    if (readIFFBoolean(fileContentPtr, pos, issue, &schema->registry, tagMeta,
                       tagString))
      return;
  }

  // TODO: Empty?

  readIFFUnquotedString(fileContentPtr, pos, allocator, issue, schema, tagMeta,
                        tagString);

  NDEBUG_ASSERT(false, "Failed to read a value from IFF file");
}

static void readTag(char const *fileContentPtr, uint32_t *pos,
                    struct Issue issue, struct Schema *schema,
                    struct BlockAllocator *allocator,
                    uint32_t *numRequiredTagsSeen) {
  // TODO: read the tag list, write functions to read individual tags, deal
  // with aliases, also add schema info
  // TODO: validate that required tags are present
  skipWhitespace(fileContentPtr, pos);

  // Look if current character matches an alias
  if (fileContentPtr[*pos] == '\0') {
    return;
  }

  char current = fileContentPtr[*pos];

  // Check for an alias
  struct UmbraString currentString;
  createUmbraStringBoundParasitic(&currentString, &current, 1);

  ComponentID tagIndex = getStringMap(&schema->aliases, currentString);

  struct UmbraString tagName = createUmbraStringNull();

  if (tagIndex != _GITISSUES_COMPONENT_INVALID) {
    // Advance past alias
    ++(*pos);

    // Get the full tag name
    // TODO: we never actually increment the size, because we add elements to
    // random slots
    DEBUG_ASSERT(tagIndex < schema->aliasTagNames.capacity,
                 "Invalid tag index, OOB");
    tagName = schema->aliasTagNames.data[tagIndex];
  } else {
    // Not an alias, should be name:value pair
    uint32_t start = *pos;

    // Read alnum with _ - until we reach :
    while (fileContentPtr[*pos] != ':' && fileContentPtr[*pos] != '\0') {
      if (!isalnum(fileContentPtr[*pos]) && fileContentPtr[*pos] != '_' &&
          fileContentPtr[*pos] != '-') {
        NDEBUG_ASSERT(
            false,
            "Expected alphanumeric or underscore or - tag name, but had %c",
            fileContentPtr[*pos]);
      }
      ++(*pos);
    }

    NDEBUG_ASSERT(fileContentPtr[*pos] == ':', "Expected colon after tag name");

    // Read from start to pos into a tag string
    createUmbraStringBoundParasitic(&tagName, fileContentPtr + start,
                                    *pos - start);

    // Move past colon
    ++(*pos);
  }

  // If tag is registered, use the existing metadata
  if (isRegistered(&schema->registry, tagName)) {
    ComponentID tagID = getComponentID(&schema->registry, tagName);

    // Get the tag metadata for this tag ID
    struct TagMetadata *tagMeta = &schema->tagMeta.data[tagID];

    // The tag metadata cannot be uninitialized if the string is already
    // registered. It would mean someone else registered this tag, but either
    // through a different schema, or not through a schema at all.
    NDEBUG_ASSERT(
        tagMeta->type != SCHEMA_TYPE_INVALID,
        "Tag was registered under a different schema/not under any schema, "
        "we have no knowledge of the type, thus cannot use this tag name");

    readIFFValue(fileContentPtr, pos, issue, allocator, tagMeta, schema,
                 tagName);

    if (tagMeta->isRequired) {
      (*numRequiredTagsSeen)++;
    }

    return;
  }

  // Construct tag metadata for unregistered tag
  struct TagMetadata newMetadata;
  newMetadata.tagID = _GITISSUES_COMPONENT_INVALID;
  newMetadata.alias = createUmbraStringNull();
  newMetadata.defaultValue = createUmbraStringNull();
  newMetadata.isRequired = false;
  newMetadata.name = tagName;
  newMetadata.type = SCHEMA_TYPE_INVALID;

  // Look for an unregistered tag, with semi-valid metadata
  for (uint32_t i = 0; i < schema->unregisteredTagMeta.size; i++) {
    struct TagMetadata *tagMeta = &schema->unregisteredTagMeta.data[i];

    // Found the metadata, remove from unregistered list
    if (umbraCompare(tagName, tagMeta->name)) {
      newMetadata = *tagMeta;

      ARRAY_REMOVE(schema->unregisteredTagMeta, i);
      break;
    }
  }

  // TODO: block allocator makes more sense? figure out which to use
  // Read value and infer the type of the tag
  readIFFValue(fileContentPtr, pos, issue, allocator, &newMetadata, schema,
               tagName);

  if (newMetadata.isRequired) {
    (*numRequiredTagsSeen)++;
  }

  DEBUG_ASSERT(newMetadata.tagID != _GITISSUES_COMPONENT_INVALID,
               "Expected reading value to fill in tag metadata");
}

static void readTagList(char const *fileContentPtr, uint32_t *pos,
                        struct Issue issue, struct Schema *schema,
                        struct BlockAllocator *allocator) {
  // TODO: read the tag list, write functions to read individual tags, deal
  // with aliases, also add schema info
  // TODO: validate that required tags are present
  skipWhitespace(fileContentPtr, pos);

  uint32_t numRequiredTagsSeen = 0;

  // If we reach EOF, or we find the terminator, we must end
  while (fileContentPtr[*pos] != '\0' &&
         !isTerminatorAtIndex(fileContentPtr, *pos, schema)) {
    readTag(fileContentPtr, pos, issue, schema, allocator,
            &numRequiredTagsSeen);
    skipWhitespace(fileContentPtr, pos);
  }

  if (numRequiredTagsSeen != schema->numRequiredTags) {
    GITISSUES_LOG_ERROR("Number of required tags added was %d, but we expected "
                        "%d required tags",
                        numRequiredTagsSeen, schema->numRequiredTags);
  }
}

void readIFFFile(char const *filename, struct BlockAllocator *allocator,
                 struct Issue **issues, uint32_t *issuesSize,
                 struct Schema *schema) {
  FILE *p = fopen(filename, "r");
  DEBUG_ASSERT(p != NULL, "Failed to open file for reading IFF");

  // TODO: there is no point in this, we should just read it in as a massive
  // allocation? the only benefit is we might double allocation size if we
  // create individual allocations for each tag we read store a lookup buffer
  // of size same as separator string make it a rolling buffer, which we
  // continually add characters to end, then shift down
  DEBUG_ASSERT(schema->terminator.size <= 8,
               "Cannot have terminator larger than 8 characters");

  struct {
    struct Issue *data;
    uint32_t size;
    uint32_t capacity;
  } issueArray;

  issueArray.data = NULL;
  issueArray.size = 0;
  issueArray.capacity = 0;

  ARRAY_RESERVE(issueArray, 8, ARRAY_GROWTH_PLUS_ONE);

  // Read in the full file
  fseek(p, 0, SEEK_END);
  size_t fileSize = ftell(p);
  rewind(p);

  char *fileContent = malloc(fileSize + 1);
  DEBUG_ASSERT(fileContent != NULL, "Failed to allocate file content");

  size_t bytesRead = fread(fileContent, sizeof(char), fileSize, p);
  fileContent[bytesRead] = '\0';

  struct UmbraString umbraFileContent;
  createUmbraStringBoundParasitic(&umbraFileContent, fileContent,
                                  (uint32_t)bytesRead);

  size_t lastIssueEnd = 0;

  bool reachedEOF = false;
  bool isFirstIssue = true;

  while (!reachedEOF) {
    // On the first issue, there was no terminator beforehand.
    // Skip whitespace after terminator.
    uint32_t issueStart =
        lastIssueEnd + (isFirstIssue ? 0 : schema->terminator.size);
    isFirstIssue = false;
    skipWhitespace(fileContent, &issueStart);

    if (issueStart >= fileSize) {
      reachedEOF = true;
      break;
    }

    struct Issue issue = createIssue(&schema->registry);

    // look for the separator character
    // TODO: what about escape characters?
    uint32_t separatorIndex =
        umbraFirstIndexOf(umbraFileContent, schema->separator, issueStart);

    NDEBUG_ASSERT(separatorIndex != UINT32_MAX, "Failed to find separator");

    // Get the text up to the separator
    uint32_t descriptionSize = separatorIndex - issueStart;

    // Go backwards from separator index to remove whitespace
    for (uint32_t i = separatorIndex - 1; i >= issueStart; i--) {
      if (!isspace(fileContent[i])) {
        break;
      }

      descriptionSize--;
    }

    struct UmbraString umbraDescription;
    createUmbraStringBoundAllocate(&umbraDescription, fileContent + issueStart,
                                   descriptionSize, allocator);
    // TODO: lifetime concerns, we attach the description of an issue to the
    // allocator passed, but does this allocator last as long as the registry?
    // Add description to issue
    addTagById(&schema->registry, issue, schema->descriptionID,
               (uint8_t *)&umbraDescription);

    // Get the tag list after the separator
    uint32_t tagListStart = separatorIndex + schema->separator.size;
    uint32_t tagListEnd = tagListStart;

    // Might modify schema data, adding more tag metadata for undeclared tags
    readTagList(fileContent, &tagListEnd, issue, schema, allocator);

    // TODO: allows arbitrary data after tag list ending?
    // Read ahead from current pointer to look for separator
    uint32_t terminatorIndex =
        umbraFirstIndexOf(umbraFileContent, schema->terminator, tagListEnd);

    // Consider EOF to be a terminator
    if (terminatorIndex == UINT32_MAX) {
      terminatorIndex = bytesRead;
      reachedEOF = true;
    }

    lastIssueEnd = terminatorIndex;

    ARRAY_APPEND(issueArray, issue, ARRAY_GROWTH_ONE_HALF);
  }

  // TODO: check all required tags are present for each issue, do it at
  // loading time for speed reasons

  *issues = issueArray.data;
  *issuesSize = issueArray.size;

  free(fileContent);

  fclose(p);
}

static void writeIFFString(FILE *p, struct UmbraString const *string) {
  // TODO: write as unquoted in some situations?
  DEBUG_ASSERT(string != NULL && string->size > 0,
               "Expected non-zero, not-null string");

  // TODO: have to escape quotes and other characters?
  fputc('"', p);
  fwrite(getUmbraPtrConst(string), sizeof(char), string->size, p);
  fputc('"', p);
}

static void writeIFFInt(FILE *p, int64_t value) {
  fprintf(p, "%" PRId64, value);
}

static void writeIFFFloat(FILE *p, double value) { fprintf(p, "%lf", value); }
static void writeIFFBoolean(FILE *p, bool value) {
  fprintf(p, "%s", value ? "true" : "false");
}

static void writeIFFDate(FILE *p, struct SchemaDate const *date) {
  fprintf(p, "%02d-%02d-%04d", date->day, date->month, date->year);
}

static void writeIFFValue(FILE *p, struct TagMetadata const *tagMeta,
                          struct Issue const issue,
                          struct Schema const *schema) {
  switch (tagMeta->type) {
  case SCHEMA_TYPE_STRING:
    writeIFFString(p, (struct UmbraString const *)getTagByIdConst(
                          &schema->registry, issue, tagMeta->tagID));
    break;
  case SCHEMA_TYPE_INT64:
    writeIFFInt(p, *(int64_t const *)getTagByIdConst(&schema->registry, issue,
                                                     tagMeta->tagID));
    break;
  case SCHEMA_TYPE_FLOAT64:
    writeIFFFloat(p, *(double const *)getTagByIdConst(&schema->registry, issue,
                                                      tagMeta->tagID));
    break;
  case SCHEMA_TYPE_BOOLEAN:
    writeIFFBoolean(p, *(bool const *)getTagByIdConst(&schema->registry, issue,
                                                      tagMeta->tagID));
    break;
  case SCHEMA_TYPE_DATE:
    writeIFFDate(p, (struct SchemaDate const *)getTagByIdConst(
                        &schema->registry, issue, tagMeta->tagID));
    break;
  case SCHEMA_TYPE_EMPTY:
    // TODO: implementation
    break;
  default:
    break;
  }
}

void writeIFFFile(char const *filename, struct Issue const *issues,
                  uint32_t issuesSize, struct Schema const *schema) {
  FILE *p = fopen(filename, "w");
  DEBUG_ASSERT(p != NULL, "Failed to open file for writing IFF");

  for (uint32_t i = 0; i < issuesSize; i++) {
    struct Issue const issue = issues[i];

    DEBUG_ASSERT(
        hasComponent(&schema->registry, issue.entity, schema->descriptionID),
        "Issue must have description component");

    struct UmbraString const *description =
        (struct UmbraString const *)getTagByIdConst(&schema->registry, issue,
                                                    schema->descriptionID);
    DEBUG_ASSERT(description != NULL, "Description data must exist");

    fwrite(getUmbraPtrConst(description), sizeof(char), description->size, p);
    fwrite(getUmbraPtrConst(&schema->separator), sizeof(char),
           schema->separator.size, p);

    bool isFirstTag = true;

    // Write tag list
    for (uint32_t i = 0; i < schema->tagMeta.capacity; i++) {
      if (schema->tagMeta.data[i].type == SCHEMA_TYPE_INVALID) {
        continue;
      }

      if (!isFirstTag) {
        fputc(' ', p);
      }
      isFirstTag = false;

      struct TagMetadata const *tagMeta = &schema->tagMeta.data[i];
      DEBUG_ASSERT(tagMeta->tagID != _GITISSUES_COMPONENT_INVALID,
                   "Expected tag ID to be valid when serialising");

      // Write alias if we have one
      if (tagMeta->alias.size > 0) {
        fwrite(getUmbraPtrConst(&tagMeta->alias), sizeof(char),
               tagMeta->alias.size, p);
      } else {
        fwrite(getUmbraPtrConst(&tagMeta->name), sizeof(char),
               tagMeta->name.size, p);
        fputc(':', p);
      }

      // Write the value in now
      writeIFFValue(p, tagMeta, issue, schema);
    }

    fwrite(getUmbraPtrConst(&schema->terminator), sizeof(char),
           schema->terminator.size, p);
  }

  fclose(p);
}
