#ifndef _GITISSUES_JSON_JSON_H_
#define _GITISSUES_JSON_JSON_H_

#include "gitissues/allocator.h"
#include <gitissues/defines.h>
#include <gitissues/log.h>
#include <gitissues/umbra_string.h>

enum JsonNodeType {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_INTEGER,
  JSON_FLOAT,
  JSON_BOOLEAN,
};

struct JsonReader {
  char *data;
  uint32_t size;
  uint32_t pos;
};

struct JsonNode;

struct JsonPair {
  struct UmbraString key;
  struct JsonNode *value;
};

struct JsonObject {
  struct JsonPair *data;
  uint32_t size;
  uint32_t capacity;
};

struct JsonArray {
  struct JsonNode *data;
  uint32_t size;
  uint32_t capacity;
};

union JsonData {
  int64_t integer;
  double floating;
  struct UmbraString string;
  struct JsonArray array;
  struct JsonObject object;
  uint8_t boolean;
};

struct JsonNode {
  union JsonData data;
  enum JsonNodeType type;
};

struct JsonReader jsonOpenFile(char const *filename);
void jsonCloseFile(struct JsonReader *reader);

void jsonWriteInt(int value, FILE *p);
void jsonWriteFloat(float value, FILE *p);
void jsonWriteString(char const *value, FILE *p);
void jsonWriteUmbraString(struct UmbraString const value, FILE *p);

void jsonWriteKey(char const *key, FILE *p);
void jsonWriteKeyUmbra(struct UmbraString const value, FILE *p);

void jsonWriteArrayBegin(FILE *p);
void jsonWriteArrayEnd(FILE *p);
void jsonWriteObjectBegin(FILE *p);
void jsonWriteObjectEnd(FILE *p);
void jsonWriteNext(FILE *p);

void jsonSkipWhitespace(struct JsonReader *p);
void jsonReadInt32(struct JsonReader *p, int32_t *value);
void jsonReadUInt32(struct JsonReader *p, uint32_t *value);
void jsonReadInt64(struct JsonReader *p, int64_t *value);
void jsonReadUInt64(struct JsonReader *p, uint64_t *value);
void jsonReadFloat(struct JsonReader *p, float *value);
void jsonReadStringLifetime(struct JsonReader *p,
                            struct BlockAllocator *allocator, char **value);
void jsonReadStringTransient(struct JsonReader *p,
                             struct ImplicitAllocator *allocator, char **value);
void jsonReadKeyLifetime(struct JsonReader *p, struct BlockAllocator *allocator,
                         char **key);
void jsonReadKeyTransient(struct JsonReader *p,
                          struct ImplicitAllocator *allocator, char **key);

void jsonReadArrayBegin(struct JsonReader *p);
void jsonReadArrayEnd(struct JsonReader *p);
void jsonReadObjectBegin(struct JsonReader *p);
void jsonReadObjectEnd(struct JsonReader *p);
// Returns true and consumes next element, or returns false
// TODO: make it explicit that it consumes the comma only if it is there
bool jsonReadNext(struct JsonReader *p);
char jsonPeekNext(struct JsonReader *p);

// Assuming current character is a {, read until we find the matching curly
size_t jsonGetLengthMatchObject(struct JsonReader *p);

struct JsonNode *jsonReadObject(struct JsonReader *p,
                                struct ImplicitAllocator *allocator);
struct JsonNode *jsonReadArray(struct JsonReader *p,
                               struct ImplicitAllocator *allocator);
struct JsonNode *jsonReadValue(struct JsonReader *p,
                               struct ImplicitAllocator *allocator);
struct JsonPair jsonReadPair(struct JsonReader *p,
                             struct ImplicitAllocator *allocator);
struct JsonNode *jsonReadFile(struct JsonReader *p,
                              struct ImplicitAllocator *allocator);

// Must be the root node
void freeJsonNode(struct JsonNode *node, struct ImplicitAllocator *allocator);

// TODO: corresponding write functions (not really needed; just if someone loads
// a json object and then operates on it)

#endif
