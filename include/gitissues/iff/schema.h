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

// Given a date format string, can be any sort of
// YYYY-MM-DD, DD-MM, DD-MM-YYYY, or YYYY-MM-DD
// or Y-M-D D-M, etc.
enum DatePart {
  DATE_PART_YEAR,
  DATE_PART_MONTH,
  DATE_PART_DAY,
  DATE_PART_SEPARATOR
};

struct DateFormatPart {
  enum DatePart part;
  int8_t expectedLength;
  char separator;
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
  struct DateFormatPart dateFormatParts[5];
  // Map symbol -> index into aliasTagNames
  // TODO: currently, all aliases expected to be single characters (if we force
  // this as a feature, then use a vector instead)
  struct StringMap aliases;
  struct Registry registry;
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

struct Schema readSchema(char const *filename);
void freeSchema(struct Schema *schema);
#endif
