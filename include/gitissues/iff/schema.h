#ifndef _GITISSUES_IFF_SCHEMA_H_
#define _GITISSUES_IFF_SCHEMA_H_

#include <gitissues/ecs/registry.h>
#include <gitissues/ecs/string_map.h>
#include <gitissues/umbra_string.h>

enum SchemaPropertyTypes {
  SCHEMA_TYPE_EMPTY,
  SCHEMA_TYPE_INT64,
  SCHEMA_TYPE_FLOAT64,
  SCHEMA_TYPE_STRING,
  SCHEMA_TYPE_DATE,
  SCHEMA_TYPE_BOOLEAN,
  SCHEMA_TYPE_INVALID,
};

struct SchemaDate {
  uint32_t year;
  uint32_t month;
  uint32_t day;
};

struct TagMetadata {
  struct UmbraString alias;
  struct UmbraString defaultValue;
  struct UmbraString name;
  enum SchemaPropertyTypes type;
  ComponentID tagID;
  bool isRequired;
};

struct Schema {
  struct BlockAllocator allocator;
  struct UmbraString separator;
  struct UmbraString terminator;
  // Map symbol -> index into aliasTagNames
  // TODO: cuirrently, all aliases expected to be single characters (if we force
  // this as a feature, then use a vector instead)
  struct StringMap aliases;
  uint32_t numRequiredTags;

  struct {
    struct UmbraString *data;
    uint32_t size;
    uint32_t capacity;
  } aliasTagNames;

  ComponentID descriptionID;

  struct {
    struct TagMetadata *data;
    uint32_t size;
    uint32_t capacity;
  } tagMeta;

  struct {
    struct TagMetadata *data;
    uint32_t size;
    uint32_t capacity;
  } unregisteredTagMeta;
};

uint32_t getSizeOfSchemaProperty(enum SchemaPropertyTypes type);

struct Schema readSchema(char const *filename, struct Registry *registry);
void dropSchema(struct Schema *schema);
#endif
