#include "gitissues/allocator.h"
#include "gitissues/defines.h"
#include "gitissues/ecs/registry.h"
#include "gitissues/ecs/string_map.h"
#include "gitissues/umbra_string.h"
#include <gitissues/iff/schema.h>
#include <gitissues/json/json.h>
#include <gitissues/uuid.h>

uint32_t getSizeOfSchemaProperty(enum SchemaPropertyTypes type) {
  switch (type) {
  case SCHEMA_TYPE_EMPTY:
    return sizeof(uint8_t);
  case SCHEMA_TYPE_INT64:
    return sizeof(int64_t);
  case SCHEMA_TYPE_FLOAT64:
    return sizeof(double);
  case SCHEMA_TYPE_STRING:
    return sizeof(struct UmbraString);
  case SCHEMA_TYPE_DATE:
    return sizeof(struct SchemaDate);
  case SCHEMA_TYPE_BOOLEAN:
    return sizeof(bool);
  case SCHEMA_TYPE_UUID:
    return sizeof(UUID7);
  default:
    NDEBUG_ASSERT(false, "Unknown schema type");
    return 0;
  }
}

static void addRegisteredTag(struct Schema *schema,
                             struct TagMetadata *metadata) {
  DEBUG_ASSERT(metadata->tagID != _GITISSUES_COMPONENT_INVALID,
               "Expected tag ID to be valid");

  GITISSUES_LOG_DEBUG(
      "Writing tag metadata to registered id at %d, should be size %d",
      metadata->tagID, ARRAY_GROWTH_PLUS_ONE(schema->tagMeta.capacity));

  uint32_t oldCapacity = schema->tagMeta.capacity;
  ARRAY_RESERVE(schema->tagMeta, (metadata->tagID + 1), ARRAY_GROWTH_PLUS_ONE);

  // Need to zero initialize all new elements
  for (uint32_t i = oldCapacity; i < schema->tagMeta.capacity; i++) {
    memset(&schema->tagMeta.data[i], 0, sizeof(struct TagMetadata));

    schema->tagMeta.data[i].tagID = _GITISSUES_COMPONENT_INVALID;
    schema->tagMeta.data[i].type = SCHEMA_TYPE_INVALID;
  }

  schema->tagMeta.data[metadata->tagID] = *metadata;
}

static void readTagMetadata(struct Schema *schema, struct JsonNode *node,
                            struct UmbraString const tagName) {
  DEBUG_ASSERT(node->type == JSON_OBJECT,
               "Expected tag metadata to be an object");

  struct TagMetadata metadata = {0};
  metadata.tagID = _GITISSUES_COMPONENT_INVALID; // TODO: register component
  metadata.alias = createUmbraStringNull();
  metadata.defaultValue = createUmbraStringNull();
  metadata.name = copyUmbraStringBlock(tagName, &schema->allocator);
  metadata.isRequired = false;
  metadata.type = SCHEMA_TYPE_INVALID;

  for (uint32_t i = 0; i < node->data.object.size; i++) {
    struct JsonPair pair = node->data.object.data[i];
    DEBUG_ASSERT(pair.key.size > 0, "Expected key to be non-empty");
    DEBUG_ASSERT(pair.value != NULL, "Expected value to be non-null");

    if (umbraCompareString(pair.key, "alias")) {
      DEBUG_ASSERT(pair.value->type == JSON_STRING,
                   "Expected alias to be a string");
      DEBUG_ASSERT(pair.value->data.string.size == 1,
                   "Alias must be a single character");

      uint32_t aliasTagIndex = schema->aliasTagNames.size;
      ARRAY_APPEND(schema->aliasTagNames, tagName, ARRAY_GROWTH_ONE_HALF);
      insertStringMap(
          &schema->aliases,
          copyUmbraStringBlock(pair.value->data.string, &schema->allocator),
          aliasTagIndex);

      metadata.alias =
          copyUmbraStringBlock(pair.value->data.string, &schema->allocator);

      continue;
    }

    if (umbraCompareString(pair.key, "default")) {
      DEBUG_ASSERT(pair.value->type == JSON_STRING,
                   "Expected default to be a string");
      metadata.defaultValue =
          copyUmbraStringBlock(pair.value->data.string, &schema->allocator);

      continue;
    }

    if (umbraCompareString(pair.key, "required")) {
      DEBUG_ASSERT(pair.value->type == JSON_BOOLEAN,
                   "Expected required to be a boolean");
      metadata.isRequired = pair.value->data.boolean;

      if (metadata.isRequired) {
        schema->numRequiredTags++;
      }

      continue;
    }

    if (umbraCompareString(pair.key, "type")) {
      DEBUG_ASSERT(pair.value->type == JSON_STRING,
                   "Expected type to be a string");

      if (umbraCompareString(pair.value->data.string, "int64")) {
        metadata.type = SCHEMA_TYPE_INT64;
      }

      else if (umbraCompareString(pair.value->data.string, "float64")) {
        metadata.type = SCHEMA_TYPE_FLOAT64;
      }

      else if (umbraCompareString(pair.value->data.string, "string")) {
        metadata.type = SCHEMA_TYPE_STRING;
      }

      else if (umbraCompareString(pair.value->data.string, "date")) {
        metadata.type = SCHEMA_TYPE_DATE;
      }

      else if (umbraCompareString(pair.value->data.string, "boolean")) {
        metadata.type = SCHEMA_TYPE_BOOLEAN;
      }

      else if (umbraCompareString(pair.value->data.string, "empty")) {
        metadata.type = SCHEMA_TYPE_EMPTY;
      }

      else {
        DEBUG_ASSERT(false, "Unknown schema type");
      }

      continue;
    }
  }

  NDEBUG_ASSERT(!isRegistered(&schema->registry, tagName),
                "Tag was already registered, duplicate within schema, or "
                "registry has been used before");

  // We have a type, so we can register the tag id
  if (metadata.type != SCHEMA_TYPE_INVALID) {
    metadata.tagID = registerComponentID(
        &schema->registry, tagName, getSizeOfSchemaProperty(metadata.type));

    addRegisteredTag(schema, &metadata);
  }
  // Add to unregistered tag metadata
  else {
    ARRAY_APPEND(schema->unregisteredTagMeta, metadata, ARRAY_GROWTH_ONE_HALF);
  }
}

static void readTagMetadatasSchema(struct Schema *schema,
                                   struct JsonNode *node) {
  DEBUG_ASSERT(node->type == JSON_OBJECT, "Expected tags to be an object");

  // Expecting list of tag name: { tag properties }
  for (uint32_t i = 0; i < node->data.object.size; i++) {
    struct JsonPair pair = node->data.object.data[i];
    DEBUG_ASSERT(pair.key.size > 0, "Expected key to be non-empty");
    DEBUG_ASSERT(pair.value != NULL, "Expected value to be non-null");

    struct UmbraString tagName = pair.key;

    NDEBUG_ASSERT(!umbraCompareString(tagName, "id"),
                  "Tag name 'id' is reserved, cannot be used");
    NDEBUG_ASSERT(!umbraCompareString(tagName, "description"),
                  "Tag name 'description' is reserved, cannot be used");

    readTagMetadata(schema, pair.value, tagName);
  }
}

static void readPropertiesOfSchema(struct Schema *schema,
                                   struct JsonNode *properties) {
  DEBUG_ASSERT(properties->type == JSON_OBJECT,
               "Expected properties values to be an object");

  for (uint32_t i = 0; i < properties->data.object.size; i++) {
    struct JsonPair pair = properties->data.object.data[i];
    DEBUG_ASSERT(pair.key.size > 0, "Expected key to be non-empty");
    DEBUG_ASSERT(pair.value != NULL, "Expected value to be non-null");

    if (umbraCompareString(pair.key, "sep")) {
      DEBUG_ASSERT(pair.value->type == JSON_STRING,
                   "Expected sep to be a string");
      schema->separator =
          copyUmbraStringBlock(pair.value->data.string, &schema->allocator);
    }

    if (umbraCompareString(pair.key, "term")) {
      DEBUG_ASSERT(pair.value->type == JSON_STRING,
                   "Expected term to be a string");
      schema->terminator =
          copyUmbraStringBlock(pair.value->data.string, &schema->allocator);
    }
  }
}

static void createDateFormatParts(struct Schema *schema,
                                  struct UmbraString const dateFormat) {
  for (uint32_t i = 0; i < CARRAY_SIZE(schema->dateFormatParts); i++) {
    schema->dateFormatParts[i].part = DATE_PART_YEAR;
    schema->dateFormatParts[i].expectedLength = 0;
  }

  char const *validSeparators = "-./";
  char const *validParts = "YMD";
  char const *dateFormatPtr = getUmbraPtrConst(&dateFormat);

  uint32_t partIndex = 0;
  char lastChar = '\0';

  for (uint32_t i = 0; i < dateFormat.size; i++) {
    char current = dateFormatPtr[i];

    if (strchr(validSeparators, current) != NULL) {
      ++partIndex;
      schema->dateFormatParts[partIndex].separator = current;
      schema->dateFormatParts[partIndex].part = DATE_PART_SEPARATOR;
      schema->dateFormatParts[partIndex].expectedLength = 1;
      ++partIndex;

      NDEBUG_ASSERT(partIndex < 5, "Too many parts in date format");
      NDEBUG_ASSERT(strchr(validSeparators, lastChar) == NULL,
                    "Cannot have two separators in a row/empty date part");
    } else if (strchr(validParts, current) != NULL) {
      NDEBUG_ASSERT(
          lastChar == '\0' || lastChar == current ||
              (strchr(validSeparators, lastChar) != NULL),
          "Invalid date format, must have separator between groups of dates, "
          "e.g. cannot have YYYYMM, or MMDD, need YYYY-MM, or MM-DD");

      switch (current) {
      case 'Y':
        schema->dateFormatParts[partIndex].part = DATE_PART_YEAR;
        break;
      case 'M':
        schema->dateFormatParts[partIndex].part = DATE_PART_MONTH;
        break;
      case 'D':
        schema->dateFormatParts[partIndex].part = DATE_PART_DAY;
        break;
      }

      schema->dateFormatParts[partIndex].expectedLength++;
    } else {
      // Must not be a date, must be numbers separated by separators
      NDEBUG_ASSERT(false,
                    "Invalid character in date format, expect only -./ or YMD");
    }

    lastChar = current;
  }

  // Validation
  bool partsFound[3] = {false};
  int numValidParts = 0;

  for (uint32_t i = 0; i < CARRAY_SIZE(schema->dateFormatParts); i++) {
    if (schema->dateFormatParts[i].expectedLength == 0) {
      continue;
    }

    if (schema->dateFormatParts[i].part != DATE_PART_SEPARATOR) {
      ++numValidParts;
    }

    switch (schema->dateFormatParts[i].part) {
    case DATE_PART_YEAR:
      NDEBUG_ASSERT(schema->dateFormatParts[i].expectedLength <= 4,
                    "Year is at most 4 digits");
      NDEBUG_ASSERT(!partsFound[DATE_PART_YEAR],
                    "Cannot have multiple year parts");
      partsFound[DATE_PART_YEAR] = true;

      break;
    case DATE_PART_MONTH:
      NDEBUG_ASSERT(schema->dateFormatParts[i].expectedLength <= 2,
                    "Month is at most 2 digits");

      NDEBUG_ASSERT(!partsFound[DATE_PART_MONTH],
                    "Cannot have multiple month parts");
      partsFound[DATE_PART_MONTH] = true;
      break;
    case DATE_PART_DAY:
      NDEBUG_ASSERT(schema->dateFormatParts[i].expectedLength <= 2,
                    "Day is at most 2 digits");

      NDEBUG_ASSERT(!partsFound[DATE_PART_DAY],
                    "Cannot have multiple day parts");
      partsFound[DATE_PART_DAY] = true;
      break;
    case DATE_PART_SEPARATOR:
      break;
    }
  }

  NDEBUG_ASSERT(numValidParts >= 1, "Must have at least one part of date");
}

static void readRootOfSchema(struct Schema *schema, struct JsonNode *root) {
  DEBUG_ASSERT(root->type == JSON_OBJECT, "Expected root to be an object");

  for (uint32_t i = 0; i < root->data.object.size; i++) {
    struct JsonPair pair = root->data.object.data[i];
    DEBUG_ASSERT(pair.key.size > 0, "Expected key to be non-empty");
    DEBUG_ASSERT(pair.value != NULL, "Expected value to be non-null");

    if (umbraCompareString(pair.key, "properties")) {
      readPropertiesOfSchema(schema, pair.value);
    }

    if (umbraCompareString(pair.key, "tags")) {
      readTagMetadatasSchema(schema, pair.value);
    }

    if (umbraCompareString(pair.key, "date_format")) {
      DEBUG_ASSERT(pair.value->type == JSON_STRING,
                   "Expected date_format to be a string");
      createDateFormatParts(schema, pair.value->data.string);
    }
  }
}

struct Schema readSchema(char const *filename) {
  // schema format
  // declare separator, terminator
  struct Schema schema = {0};
  schema.allocator = createBlockAllocator(1024);
  schema.aliases = createStringMap();
  schema.numRequiredTags = 0;
  schema.registry = createRegistry();
  UUID7Seed(&schema.rng);

  struct UmbraString dateFormat;
  createUmbraStringParasitic(&dateFormat, "YYYY-MM-DD");
  createDateFormatParts(&schema, dateFormat);

  struct JsonReader p = jsonOpenFile(filename);
  struct ImplicitAllocator allocator = createImplicitAllocator();

  struct UmbraString descriptionTag;
  createUmbraStringParasitic(&descriptionTag, "description");
  NDEBUG_ASSERT(!isRegistered(&schema.registry, descriptionTag),
                "Must have 'description' component name unregistered");

  schema.descriptionID = registerComponentID(&schema.registry, descriptionTag,
                                             sizeof(struct UmbraString));
  struct TagMetadata descriptionMetadata = {0};
  descriptionMetadata.tagID = schema.descriptionID;
  descriptionMetadata.type = SCHEMA_TYPE_STRING;
  descriptionMetadata.name = descriptionTag;
  descriptionMetadata.isRequired = false;
  descriptionMetadata.alias = createUmbraStringNull();
  descriptionMetadata.defaultValue = createUmbraStringNull();
  addRegisteredTag(&schema, &descriptionMetadata);

  struct UmbraString idTag;
  createUmbraStringParasitic(&idTag, "id");
  NDEBUG_ASSERT(!isRegistered(&schema.registry, idTag),
                "Must have 'id' component name unregistered");

  schema.identifierID =
      registerComponentID(&schema.registry, idTag, sizeof(UUID7));
  struct TagMetadata idMetadata = {0};
  idMetadata.tagID = schema.identifierID;
  idMetadata.type = SCHEMA_TYPE_UUID;
  idMetadata.name = idTag;
  idMetadata.isRequired = false;
  idMetadata.alias = createUmbraStringNull();
  idMetadata.defaultValue = createUmbraStringNull();
  addRegisteredTag(&schema, &idMetadata);

  struct JsonNode *root = jsonReadFile(&p, &allocator);

  readRootOfSchema(&schema, root);

  freeJsonNode(root, &allocator);

  // TODO: would be nice, as part of validation, to check that all values
  // were properly cleaned up.
  freeImplicitAllocator(&allocator);
  jsonCloseFile(&p);

  return schema;
}

void freeSchema(struct Schema *schema) {
  freeBlockAllocator(&schema->allocator);
  freeStringMap(&schema->aliases);

  free(schema->aliasTagNames.data);
  free(schema->tagMeta.data);
  free(schema->unregisteredTagMeta.data);
}
