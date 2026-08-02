#ifndef _GITISSUES_JSON_JSON_H_
#define _GITISSUES_JSON_JSON_H_

#include <gitissues/defines.h>
#include <gitissues/log.h>
#include <gitissues/umbra_string.h>

struct JsonReader {
  char *data;
  uint32_t size;
  uint32_t pos;
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
bool jsonReadNext(struct JsonReader *p);
char jsonPeekNext(struct JsonReader *p);

// Assuming current character is a {, read until we find the matching curly
size_t jsonGetLengthMatchObject(struct JsonReader *p);

#endif
