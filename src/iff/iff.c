#include "gitissues/allocator.h"
#include "gitissues/defines.h"
#include "gitissues/ecs/registry.h"
#include "gitissues/ecs/string_map.h"
#include "gitissues/iff/schema.h"
#include "gitissues/umbra_string.h"
#include <gitissues/iff/iff.h>

#include <ctype.h>
#include <inttypes.h>

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

  ComponentID tagID =
      registerComponentID(registry, tagString, getSizeOfSchemaProperty(type));

  tagMeta->tagID = tagID;
  tagMeta->type = type;
  tagMeta->name = tagString;
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

static bool readIFFFloat(char const *fileContentPtr, uint32_t *pos,
                         struct Issue issue, struct Registry *registry,
                         struct TagMetadata *tagMeta,
                         struct UmbraString const tagString) {
  NDEBUG_ASSERT(!isdigit(fileContentPtr) && fileContentPtr[*pos] != '-',
                "Expected digit at start of number");

  int64_t integer;
  int consumed;

  if (sscanf(fileContentPtr + (*pos), "%" SCNd64 "%n", &integer, &consumed) ==
      1) {
    *pos += consumed;
  } else {
    // Failed to read float, might be string?
    return false;
  }

  completeTagMetadataIfInvalid(registry, tagString, tagMeta,
                               SCHEMA_TYPE_FLOAT64);

  addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&integer);

  return true;
}

static bool readIFFInt(char const *fileContentPtr, uint32_t *pos,
                       struct Issue issue, struct Registry *registry,
                       struct TagMetadata *tagMeta,
                       struct UmbraString const tagString) {
  // TODO: support .132
  NDEBUG_ASSERT(!isdigit(fileContentPtr[*pos] && fileContentPtr[*pos] != '-'),
                "Expected digit at start of number");

  double floating;
  int consumed;

  // TODO: code duplication of readInt and readFloat
  // TODO: code duplication of JSON read functions
  // TODO: reparsing section again to test for floats
  if (sscanf(fileContentPtr + (*pos), "%lf%n", &floating, &consumed) == 1) {
    *pos += consumed;
  } else {
    // Failed to read int, might be float?

    return false;
  }

  completeTagMetadataIfInvalid(registry, tagString, tagMeta, SCHEMA_TYPE_INT64);

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
                         struct Issue issue, struct Registry *registry,
                         struct BlockAllocator *allocator,
                         struct TagMetadata *tagMeta,
                         struct UmbraString const tagString) {
  // Read in either a empty, string, int, float, boolean, or date

  // TODO: string has optional quotes

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
      readIFFString(fileContentPtr, pos, allocator, issue, registry, tagMeta,
                    tagString);
      return;
    case SCHEMA_TYPE_INT64:
      if (readIFFInt(fileContentPtr, pos, issue, registry, tagMeta, tagString))
        return;
      break;
    case SCHEMA_TYPE_FLOAT64:
      if (readIFFFloat(fileContentPtr, pos, issue, registry, tagMeta,
                       tagString))
        return;
      break;
    case SCHEMA_TYPE_BOOLEAN:
      if (readIFFBoolean(fileContentPtr, pos, issue, registry, tagMeta,
                         tagString))
        return;
      break;
    case SCHEMA_TYPE_DATE:
      // TODO: read date
      break;
    case SCHEMA_TYPE_EMPTY: {
      uint8_t empty = 0;
      // TODO: no official ECS support for empty components
      addTagById(registry, issue, tagMeta->tagID, (uint8_t *)&empty);
      return;
    }
    default:
      NDEBUG_ASSERT(false, "Unknown schema type");
      break;
    }

    return;
  }

  // We need to figure out the type, and register the tag
  if (fileContentPtr[*pos] == '"') {
    readIFFString(fileContentPtr, pos, allocator, issue, registry, tagMeta,
                  tagString);

    return;
  }

  if (isdigit(fileContentPtr[*pos]) || fileContentPtr[*pos] == '-') {
    if (readIFFInt(fileContentPtr, pos, issue, registry, tagMeta, tagString))
      return;
    if (readIFFFloat(fileContentPtr, pos, issue, registry, tagMeta, tagString))
      return;

    // TODO: read date
    // TODO: read a string or fail?
  }

  if (fileContentPtr[*pos] == 't' || fileContentPtr[*pos] == 'f') {
    if (readIFFBoolean(fileContentPtr, pos, issue, registry, tagMeta,
                       tagString))
      return;

    // TODO: read a string or fail?
  }

  // TODO: Read a string
  // TODO: Empty?

  NDEBUG_ASSERT(false, "Failed to read a value from IFF file");
}

static void readTagList(char const *fileContentPtr, uint32_t *pos,
                        struct Issue issue, struct Schema *schema,
                        struct Registry *registry,
                        struct BlockAllocator *allocator) {
  // TODO: read the tag list, write functions to read individual tags, deal with
  // aliases, also add schema info
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

  struct UmbraString tagName;

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
        NDEBUG_ASSERT(false,
                      "Expected alphanumeric or underscore or - tag name");
      }
      ++(*pos);
    }

    NDEBUG_ASSERT(fileContentPtr[*pos] == ':', "Expected colon after tag name");

    // Read from start to pos into a tag string
    createUmbraStringBoundParasitic(&tagName, fileContentPtr + start, *pos);

    // Move past colon
    ++(*pos);
  }

  // If tag is registered, use the existing metadata
  if (isRegistered(registry, tagName)) {
    ComponentID tagID = getComponentID(registry, tagName);

    // Get the tag metadata for this tag ID
    struct TagMetadata *tagMeta = &schema->tagMeta.data[tagID];

    // The tag metadata cannot be uninitialized if the string is already
    // registered. It would mean someone else registered this tag, but either
    // through a different schema, or not through a schema at all.
    NDEBUG_ASSERT(
        tagMeta->type != SCHEMA_TYPE_INVALID,
        "Tag was registered under a different schema/not under any schema, "
        "we have no knowledge of the type, thus cannot use this tag name");

    readIFFValue(fileContentPtr, pos, issue, registry, allocator, tagMeta,
                 tagName);

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
  readIFFValue(fileContentPtr, pos, issue, registry, allocator, &newMetadata,
               tagName);

  DEBUG_ASSERT(newMetadata.tagID != _GITISSUES_COMPONENT_INVALID,
               "Expected reading value to fill in tag metadata");
}

void readIFFFile(char const *filename, struct Registry *registry,
                 struct BlockAllocator *allocator, struct Issue **issues,
                 uint32_t *issuesSize, struct Schema *schema) {
  FILE *p = fopen(filename, "r");
  DEBUG_ASSERT(p != NULL, "Failed to open file for reading IFF");

  // TODO: there is no point in this, we should just read it in as a massive
  // allocation? the only benefit is we might double allocation size if we
  // create individual allocations for each tag we read store a lookup buffer of
  // size same as separator string make it a rolling buffer, which we
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

    struct Issue issue = createIssue(registry);

    // look for the separator character
    // TODO: what about escape characters?
    uint32_t separatorIndex =
        umbraFirstIndexOf(umbraFileContent, schema->separator, issueStart);

    NDEBUG_ASSERT(separatorIndex != UINT32_MAX, "Failed to find separator");

    // Get the text up to the separator
    uint32_t descriptionSize = separatorIndex - issueStart;
    struct UmbraString umbraDescription;
    createUmbraStringBoundAllocate(&umbraDescription, fileContent + issueStart,
                                   descriptionSize, allocator);
    // TODO: lifetime concerns, we attach the description of an issue to the
    // allocator passed, but does this allocator last as long as the registry?
    // Add description to issue
    addTagById(registry, issue, schema->descriptionID,
               (uint8_t *)&umbraDescription);

    // Get the tag list after the separator
    uint32_t tagListStart = separatorIndex + schema->separator.size;
    uint32_t tagListEnd = tagListStart;

    // Might modify schema data, adding more tag metadata for undeclared tags
    readTagList(fileContent, &tagListEnd, issue, schema, registry, allocator);

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
  }

  // TODO: check all required tags are present for each issue, do it at loading
  // time for speed reasons

  *issues = issueArray.data;
  *issuesSize = issueArray.size;

  free(fileContent);

  fclose(p);
}
