#include "gitissues/allocator.h"
#include "gitissues/defines.h"
#include "gitissues/ecs/registry.h"
#include "gitissues/ecs/string_map.h"
#include "gitissues/umbra_string.h"
#include <gitissues/iff/schema.h>
#include <gitissues/json/json.h>

struct Schema defaultSchema;

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
  default:
    NDEBUG_ASSERT(false, "Unknown schema type");
    return 0;
  }
}

static void readTagMetadata(struct Schema *schema, struct JsonNode *node,
                            struct Registry *registry,
                            struct UmbraString const tagName) {
  DEBUG_ASSERT(node->type == JSON_OBJECT,
               "Expected tag metadata to be an object");

  struct TagMetadata metadata = {0};
  metadata.tagID = _GITISSUES_COMPONENT_INVALID; // TODO: register component
  metadata.alias = createUmbraStringNull();
  metadata.defaultValue = createUmbraStringNull();
  metadata.name = tagName;
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

  NDEBUG_ASSERT(!isRegistered(registry, tagName),
                "Tag was already registered, duplicate within schema, or "
                "registry has been used before");

  // We have a type, so we can register the tag id
  if (metadata.type != SCHEMA_TYPE_INVALID) {
    metadata.tagID = registerComponentID(
        registry, tagName, getSizeOfSchemaProperty(metadata.type));

    GITISSUES_LOG_DEBUG(
        "Writing tag metadata to registered id at %d, should be size %d",
        metadata.tagID, ARRAY_GROWTH_PLUS_ONE(schema->tagMeta.capacity));

    uint32_t oldCapacity = schema->tagMeta.capacity;
    ARRAY_RESERVE(schema->tagMeta, (metadata.tagID + 1), ARRAY_GROWTH_PLUS_ONE);

    // Need to zero initialize all new elements
    for (uint32_t i = oldCapacity; i < schema->tagMeta.capacity; i++) {
      memset(&schema->tagMeta.data[i], 0, sizeof(struct TagMetadata));

      schema->tagMeta.data[i].tagID = _GITISSUES_COMPONENT_INVALID;
      schema->tagMeta.data[i].type = SCHEMA_TYPE_INVALID;
    }

    schema->tagMeta.data[metadata.tagID] = metadata;
  }
  // Add to unregistered tag metadata
  else {
    ARRAY_APPEND(schema->unregisteredTagMeta, metadata, ARRAY_GROWTH_ONE_HALF);
  }
}

static void readTagMetadatasSchema(struct Schema *schema, struct JsonNode *node,
                                   struct Registry *registry) {
  DEBUG_ASSERT(node->type == JSON_OBJECT, "Expected tags to be an object");

  // Expecting list of tag name: { tag properties }

  for (uint32_t i = 0; i < node->data.object.size; i++) {
    struct JsonPair pair = node->data.object.data[i];
    DEBUG_ASSERT(pair.key.size > 0, "Expected key to be non-empty");
    DEBUG_ASSERT(pair.value != NULL, "Expected value to be non-null");

    // TODO: use name (key of the pair) to register component
    struct UmbraString tagName = pair.key;

    readTagMetadata(schema, pair.value, registry, tagName);
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

static void readRootOfSchema(struct Schema *schema, struct JsonNode *root,
                             struct Registry *registry) {
  DEBUG_ASSERT(root->type == JSON_OBJECT, "Expected root to be an object");

  for (uint32_t i = 0; i < root->data.object.size; i++) {
    struct JsonPair pair = root->data.object.data[i];
    DEBUG_ASSERT(pair.key.size > 0, "Expected key to be non-empty");
    DEBUG_ASSERT(pair.value != NULL, "Expected value to be non-null");

    if (umbraCompareString(pair.key, "properties")) {
      readPropertiesOfSchema(schema, pair.value);
    }

    if (umbraCompareString(pair.key, "tags")) {
      readTagMetadatasSchema(schema, pair.value, registry);
    }
  }
}

struct Schema readSchema(char const *filename, struct Registry *registry) {
  // schema format
  // declare separator, terminator
  struct Schema schema = {0};
  schema.allocator = createBlockAllocator(1024);
  schema.aliases = createStringMap();

  struct JsonReader p = jsonOpenFile(filename);
  struct ImplicitAllocator allocator = createImplicitAllocator();

  struct UmbraString descriptionTag;
  createUmbraStringParasitic(&descriptionTag, "description");
  NDEBUG_ASSERT(!isRegistered(registry, descriptionTag),
                "Must have 'description' component name unregistered");

  schema.descriptionID =
      registerComponentID(registry, descriptionTag, sizeof(struct UmbraString));

  struct JsonNode *root = jsonReadFile(&p, &allocator);

  readRootOfSchema(&schema, root, registry);

  freeJsonNode(root, &allocator);

  // TODO: would be nice, as part of validation, to check that all values
  // were properly cleaned up.
  freeImplicitAllocator(&allocator);
  jsonCloseFile(&p);

  return schema;
}

void dropSchema(struct Schema *schema) {
  freeBlockAllocator(&schema->allocator);
  freeStringMap(&schema->aliases);
}
