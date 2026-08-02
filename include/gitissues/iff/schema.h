#ifndef _GITISSUES_IFF_SCHEMA_H_
#define _GITISSUES_IFF_SCHEMA_H_

#include <gitissues/ecs/string_map.h>
#include <gitissues/umbra_string.h>

enum SchemaPropertyTypes { INT64, UINT64, FLOAT, STRING, DATE };

// TODO: move most of this to schema.h later
// TODO: add type info later
struct TagMetadata {
  bool isRequired;
};

struct Schema {
  struct UmbraString separator;
  struct UmbraString terminator;
  // Map symbol -> special tag (by componentID -- not ideal)
  struct StringMap aliases;

  struct {
    struct TagMetadata *data;
    uint32_t size;
    uint32_t capacity;
  } tagMeta;
};

void initSchema(void);
void terminateSchema(void);

struct Schema readSchema(char const *filename);
void dropSchema(struct Schema *schema);
#endif
