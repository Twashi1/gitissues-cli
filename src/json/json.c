#include "gitissues/defines.h"
#include <ctype.h>
#include <gitissues/allocator.h>
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

void jsonReadInt32(struct JsonReader *p, int32_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%d%n", value, &consumed) == 1) {
    p->pos += consumed;
    return;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
}

void jsonReadUInt32(struct JsonReader *p, uint32_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%u%n", value, &consumed) == 1) {
    p->pos += consumed;
    return;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
}

void jsonReadInt64(struct JsonReader *p, int64_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%" SCNd64 "%n", value, &consumed) == 1) {
    p->pos += consumed;

    return;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
}

void jsonReadUInt64(struct JsonReader *p, uint64_t *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading int");

  int consumed;

  if (sscanf(p->data + p->pos, "%" SCNu64 "%n", value, &consumed) == 1) {
    p->pos += consumed;
    return;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
}

void jsonReadFloat(struct JsonReader *p, float *value) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size, "Reached EOF before reading float");

  int consumed;

  if (sscanf(p->data + p->pos, "%f%n", value, &consumed) == 1) {
    p->pos += consumed;
    return;
  }

  DEBUG_ASSERT(false, "Failed to read integer from json");
}

void jsonReadStringLifetime(struct JsonReader *p,
                            struct BlockAllocator *allocator, char **value) {
  // TODO: also read strings that start with single quotes
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

  // We assume the user doesn't want the value
  if (value != NULL) {
    // We add 1 for the null terminator
    *value = allocateBlockAllocator(
        allocator, (stringLength + 1) * sizeof(char), alignof(char));
    DEBUG_ASSERT(value != NULL,
                 "Failed to allocate space for string JsonReader");
    memcpy(*value, p->data + start, stringLength * sizeof(char));
    (*value)[stringLength] = '\0';
  }

  p->pos = i;

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '"',
               "Expected quotation mark");
  p->pos++;
}

// TODO: terrible duplication
void jsonReadStringTransient(struct JsonReader *p,
                             struct ImplicitAllocator *allocator,
                             char **value) {
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

  // TODO: need to build the string character by character, so we can properly
  // deal with escape characters.

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

  // We assume the user doesn't want the value
  if (value != NULL) {
    // We add 1 for the null terminator
    *value = allocateImplicitAllocator(
        allocator, (stringLength + 1) * sizeof(char), alignof(char));
    DEBUG_ASSERT(value != NULL,
                 "Failed to allocate space for string JsonReader");
    memcpy(*value, p->data + start, stringLength * sizeof(char));
    (*value)[stringLength] = '\0';
  }

  p->pos = i;

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '"',
               "Expected quotation mark");
  p->pos++;
}

void jsonReadKeyTransient(struct JsonReader *p,
                          struct ImplicitAllocator *allocator, char **key) {
  jsonReadStringTransient(p, allocator, key);

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == ':',
               "Expected colon after reading key");
  p->pos++;
}

void jsonReadKeyLifetime(struct JsonReader *p, struct BlockAllocator *allocator,
                         char **key) {
  jsonReadStringLifetime(p, allocator, key);

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == ':',
               "Expected colon after reading key");
  p->pos++;
}

void jsonReadArrayBegin(struct JsonReader *p) {
  jsonSkipWhitespace(p);
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '[',
               "Expected [ at start of array");
  p->pos++;
}

void jsonReadArrayEnd(struct JsonReader *p) {
  jsonSkipWhitespace(p);
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == ']',
               "Expected ] at end of array");
  p->pos++;
}

void jsonReadObjectBegin(struct JsonReader *p) {
  jsonSkipWhitespace(p);
  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '{',
               "Expected { at start of array");
  p->pos++;
}

void jsonReadObjectEnd(struct JsonReader *p) {
  jsonSkipWhitespace(p);
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

char jsonPeekNext(struct JsonReader *p) {
  jsonSkipWhitespace(p);

  if (p->pos >= p->size) {
    return '\0';
  }

  return p->data[p->pos];
}

size_t jsonGetLengthMatchObject(struct JsonReader *p) {
  jsonSkipWhitespace(p);

  DEBUG_ASSERT(p->pos < p->size && p->data[p->pos] == '{',
               "Expected to start at curly");

  size_t curlySeen = 0;
  size_t start = p->pos;

  for (size_t i = p->pos; i < p->size; i++) {
    char c = p->data[i];

    if (c == '{')
      ++curlySeen;
    if (c == '}') {
      DEBUG_ASSERT(curlySeen > 0, "Mismatched {} braces");
      --curlySeen;
    }

    // Found the match
    if (curlySeen == 0) {
      return i - start;
    }
  }

  DEBUG_ASSERT(false, "Mismatched curlys");
  return 0;
}

struct JsonNode *jsonReadObject(struct JsonReader *p,
                                struct ImplicitAllocator *allocator) {
  jsonReadObjectBegin(p);

  struct JsonObject object = {NULL, 0, 0};

  while (jsonPeekNext(p) != '}') {
    struct JsonPair pair = jsonReadPair(p, allocator);
    ARRAY_APPEND(object, pair, ARRAY_GROWTH_ONE_HALF);

    // if comma is next, consume it
    jsonReadNext(p);
  }

  jsonReadObjectEnd(p);

  struct JsonNode *node = allocateImplicitAllocator(
      allocator, sizeof(struct JsonNode), alignof(struct JsonNode));
  DEBUG_ASSERT(node != NULL, "Failed to allocate space for JsonNode");

  node->data.object = object;
  node->type = JSON_OBJECT;

  return node;
}

struct JsonNode *jsonReadArray(struct JsonReader *p,
                               struct ImplicitAllocator *allocator) {
  jsonReadArrayBegin(p);

  struct JsonArray array = {NULL, 0, 0};
  while (jsonPeekNext(p) != ']') {
    struct JsonNode *value = jsonReadValue(p, allocator);
    ARRAY_APPEND(array, *value, ARRAY_GROWTH_ONE_HALF);
    freeFastAllocationImplicitAllocator(allocator, value,
                                        sizeof(struct JsonNode));

    // if comma is next, consume it
    jsonReadNext(p);
  }

  jsonReadArrayEnd(p);

  struct JsonNode *node = allocateImplicitAllocator(
      allocator, sizeof(struct JsonNode), alignof(struct JsonNode));
  DEBUG_ASSERT(node != NULL, "Failed to allocate space for JsonNode");

  node->data.array = array;
  node->type = JSON_ARRAY;

  return node;
}

struct JsonNode *jsonReadValue(struct JsonReader *p,
                               struct ImplicitAllocator *allocator) {
  char next = jsonPeekNext(p);

  // Check if is a string
  if (next == '"') {
    char *string = NULL;
    jsonReadStringTransient(p, allocator, &string);

    struct UmbraString stringUmbra;
    createUmbraStringParasitic(&stringUmbra, string);

    struct JsonNode *node = allocateImplicitAllocator(
        allocator, sizeof(struct JsonNode), alignof(struct JsonNode));
    DEBUG_ASSERT(node != NULL, "Failed to allocate space for JsonNode");

    node->data.string = stringUmbra;
    node->type = JSON_STRING;

    return node;
  }

  // Check if is an object
  if (next == '{') {
    return jsonReadObject(p, allocator);
  }

  // Check if is an array
  if (next == '[') {
    return jsonReadArray(p, allocator);
  }

  if (isdigit(next) || next == '-' || next == '.') {
    // TODO: check if we can use strtol
    int64_t integer;
    double floating;
    int consumed;
    bool wasInteger = false;

    // TODO: code duplication of readInt and readFloat
    if (sscanf(p->data + p->pos, "%lf%n", &floating, &consumed) == 1) {
      p->pos += consumed;
    }

    else if (sscanf(p->data + p->pos, "%" SCNd64 "%n", &integer, &consumed) ==
             1) {
      p->pos += consumed;
      wasInteger = true;
    }

    else {
      DEBUG_ASSERT(false, "Failed to read number from json");
    }

    struct JsonNode *node = allocateImplicitAllocator(
        allocator, sizeof(struct JsonNode), alignof(struct JsonNode));

    if (wasInteger) {
      node->data.integer = integer;
      node->type = JSON_INTEGER;
    } else {
      node->data.floating = floating;
      node->type = JSON_FLOAT;
    }

    return node;
  }

  // TODO: read bool function?
  if (next == 't' || next == 'f') {
    if (strncmp(p->data + p->pos, "true", strlen("true")) == 0) {
      p->pos += sizeof("true") - 1;

      struct JsonNode *node = allocateImplicitAllocator(
          allocator, sizeof(struct JsonNode), alignof(struct JsonNode));
      node->data.boolean = 1;
      node->type = JSON_BOOLEAN;

      return node;
    }

    if (strncmp(p->data + p->pos, "false", strlen("false")) == 0) {
      p->pos += sizeof("false") - 1;

      struct JsonNode *node = allocateImplicitAllocator(
          allocator, sizeof(struct JsonNode), alignof(struct JsonNode));
      node->data.boolean = 0;
      node->type = JSON_BOOLEAN;
      return node;
    }

    DEBUG_ASSERT(false, "Failed to read boolean from json");
  }

  DEBUG_ASSERT(false, "Failed to read any value from json");

  return NULL;
}

struct JsonPair jsonReadPair(struct JsonReader *p,
                             struct ImplicitAllocator *allocator) {
  char *string = NULL;
  jsonReadKeyTransient(p, allocator, &string);

  struct UmbraString key;
  createUmbraStringParasitic(&key, string);

  struct JsonNode *value = jsonReadValue(p, allocator);

  struct JsonPair pair;
  pair.key = key;
  pair.value = value;

  return pair;
}

struct JsonNode *jsonReadFile(struct JsonReader *p,
                              struct ImplicitAllocator *allocator) {
  return jsonReadValue(p, allocator);
}

void freeJsonNode(struct JsonNode *node, struct ImplicitAllocator *allocator) {
  switch (node->type) {
  case JSON_OBJECT:
    for (uint32_t i = 0; i < node->data.object.size; i++) {
      freeJsonNode(node->data.object.data[i].value, allocator);
    }

    break;

  case JSON_ARRAY:
    for (uint32_t i = 0; i < node->data.array.size; i++) {
      freeJsonNode(&node->data.array.data[i], allocator);
    }

    break;

  case JSON_STRING:
    freeUmbraStringTransient(&node->data.string, allocator);
    break;

  case JSON_INTEGER:
  case JSON_FLOAT:
  case JSON_BOOLEAN:
    break;
  default:
    DEBUG_ASSERT(false, "Unknown json node type");
    break;
  }

  // Free the node itself
  freeFastAllocationImplicitAllocator(allocator, node, sizeof(struct JsonNode));
}
