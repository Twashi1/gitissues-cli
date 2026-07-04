#include "gitissues/allocator.h"
#include "gitissues/errs.h"
#include <ctype.h>
#include <gitissues/json/json.h>
#include <inttypes.h>

void jsonWriteInt(int value, FILE *p) { fprintf(p, "%d", value); }

void jsonWriteFloat(float value, FILE *p) { fprintf(p, "%f", value); }

void jsonWriteString(char const *value, FILE *p) {
  // TODO: can inject arbitrary keys from tags until we properly escape this
  // not a security risk (we allow arbitrary data anyway), but still bad
  fprintf(p, "\"%s\"", value);
}

void jsonWriteUmbraString(struct UmbraString const value, FILE *p) {
  char const *data = NULL;

  if (value.size <= 12) {
    data = (char const *)&value.prefix;
  } else {
    data = value.ptr;
  }

  fputc('"', p);
  fwrite(data, sizeof(char), value.size, p);
  fputc('"', p);
}

void jsonWriteKey(char const *key, FILE *p) {
  jsonWriteString(key, p);
  fputc(':', p);
}

void jsonWriteKeyUmbra(struct UmbraString const value, FILE *p) {
  jsonWriteUmbraString(value, p);
  fputc(':', p);
}

void jsonWriteArrayBegin(FILE *p) { fputc('[', p); }
void jsonWriteArrayEnd(FILE *p) { fputc(']', p); }
void jsonWriteObjectBegin(FILE *p) { fputc('{', p); }
void jsonWriteObjectEnd(FILE *p) { fputc('}', p); }
void jsonWriteNext(FILE *p) { fputc(',', p); }

struct JsonReader jsonOpenFile(char const *filename) {
  struct JsonReader reader;

  FILE *f = fopen(filename, "rb");
  DEBUG_ASSERT(f != NULL, "Couldn't open file");

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    DEBUG_ASSERT(false, "Failed to seek end of file");
  }

  long int size = ftell(f);
  if (size < 0) {
    fclose(f);
    DEBUG_ASSERT(false, "Failed to read size of file");
  }

  rewind(f);

  char *buf = malloc((size + 1) * sizeof(char));
  if (!buf) {
    fclose(f);
    DEBUG_ASSERT(false, "Failed to allocate for JsonReader");
  }

  uint32_t bytesRead = fread(buf, 1, size, f);
  fclose(f);

  buf[bytesRead] = '\0';

  reader.size = bytesRead;
  reader.pos = 0;
  reader.data = buf;

  // TODO: upon opening and processing file, tokenize
  // skip all whitespace tokens

  return reader;
}

void jsonCloseFile(struct JsonReader *reader) { free(reader->data); }

void jsonSkipWhitespace(struct JsonReader *p) {
  while (p->pos < p->size && isspace(p->data[p->pos])) {
    p->pos++;
  }
}

enum ErrorCode jsonReadInt32(struct JsonReader *p, int32_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%d%n", value, &consumed) == 1) {
    p->pos += consumed;
    return GITISSUES_OK;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
  return GITISSUES_JSON_FAIL_READ;
}

enum ErrorCode jsonReadUInt32(struct JsonReader *p, uint32_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%u%n", value, &consumed) == 1) {
    p->pos += consumed;
    return GITISSUES_OK;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
  return GITISSUES_JSON_FAIL_READ;
}

enum ErrorCode jsonReadInt64(struct JsonReader *p, int64_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%" SCNd64 "%n", value, &consumed) == 1) {
    p->pos += consumed;
    return GITISSUES_OK;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
  return GITISSUES_JSON_FAIL_READ;
}

enum ErrorCode jsonReadUInt64(struct JsonReader *p, uint64_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%" SCNu64 "%n", value, &consumed) == 1) {
    p->pos += consumed;
    return GITISSUES_OK;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
  return GITISSUES_JSON_FAIL_READ;
}

enum ErrorCode jsonReadFloat(struct JsonReader *p, float *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading float");

  int consumed;

  if (sscanf(p->data + p->pos, "%f%n", value, &consumed) == 1) {
    p->pos += consumed;
    return GITISSUES_OK;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
  return GITISSUES_JSON_FAIL_READ;
}

enum ErrorCode jsonReadString(struct JsonReader *p,
                              struct BlockAllocator *allocator, char **value) {

  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading string");

  // eat quotation mark
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '"',
               "Expected quotation mark");
  p->pos++;

  // read until unescaped quotation mark
  bool escaped = false;
  uint32_t start = p->pos;
  uint32_t i;

  // TODO: this doesn't properly deal with \n, \t, just \\ and \" (i think)
  for (i = start; i < p->size; i++) {
    if (p->data[i] == '"') {
      if (!escaped)
        break;
    }

    if (p->data[i] == '\\') {
      escaped = !escaped;
    } else {
      escaped = false;
    }
  }

  // We end when i == '"'
  uint32_t stringLength = i - start;
  // We add 1 for the null terminator
  *value = allocateBlockAllocator(allocator, (stringLength + 1) * sizeof(char),
                                  alignof(char));
  DEBUG_ASSERT(value != NULL, "Failed to allocate space for string JsonReader");
  memcpy(*value, p->data + start, stringLength * sizeof(char));
  (*value)[stringLength] = '\0';

  p->pos = i;

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '"',
               "Expected quotation mark");
  p->pos++;

  return GITISSUES_OK;
}
enum ErrorCode jsonReadKey(struct JsonReader *p,
                           struct BlockAllocator *allocator, char **key) {
  jsonReadString(p, allocator, key);

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == ':',
               "Expected colon after reading key");
  p->pos++;

  return GITISSUES_OK;
}

void jsonReadArrayBegin(struct JsonReader *p) {
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '[',
               "Expected [ at start of array");
  p->pos++;
}

void jsonReadArrayEnd(struct JsonReader *p) {
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == ']',
               "Expected ] at end of array");
  p->pos++;
}

void jsonReadObjectBegin(struct JsonReader *p) {
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '{',
               "Expected { at start of array");
  p->pos++;
}

void jsonReadObjectEnd(struct JsonReader *p) {
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '}',
               "Expected } at end of array");
  p->pos++;
}

bool jsonReadNext(struct JsonReader *p) {
  jsonSkipWhitespace(p);

  if (p->pos < p->size && p->data[p->pos] == ',') {
    p->pos++;
    return true;
  }

  return false;
}
