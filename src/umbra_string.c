#include "gitissues/allocator.h"
#include <gitissues/umbra_string.h>
#include <stddef.h>
#include <stdio.h>

struct FastComparisonMetadata comparisonMetadata;

static inline bool areStringsEqual(char const *a, char const *b,
                                   uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
    if (a[i] != b[i])
      return false;
  }

  return true;
}

bool umbraCompare(struct UmbraString const a, struct UmbraString const b) {
  // Compare prefix first
  if (LIKELY(a.prefix != b.prefix)) {
    return false;
  }

  // Compare length
  if (a.size != b.size) {
    return false;
  }

  // If short-string
  if (a.size <= 12) {
    // Prefix already matches, just compare remaining data
    // We're going to assume an invariant; that with short-strings, we pad \0
    // bytes
    if (a.data != b.data) {
      return false;
    }

    return true;
  }

  // Compare ptr address for long strings (start at 4)
  for (uint32_t i = 4; i < a.size; i++) {
    if (a.ptr[i] != b.ptr[i]) {
      return false;
    }
  }

  return true;
}

bool umbraCompareString(struct UmbraString const a, char const *b) {
  uint32_t length = strlen(b);

  // Compare length
  if (a.size != length) {
    return false;
  }

  // We don't necessarily terminate the string, but we have the same length
  // field anyway
  char const *aPtr = a.size <= 12 ? (char const *)&a.prefix : a.ptr;

  if (strncmp(aPtr, b, length) == 0) {
    return true;
  }

  return false;
}

uint32_t umbraFirstIndexOf(struct UmbraString const string,
                           struct UmbraString const pattern, uint32_t offset) {
  // TODO: consider more optimised methods; maybe boyer-moore
  uint32_t start = offset;

  if (pattern.size == 0)
    return UINT32_MAX;

  if (pattern.size > string.size)
    return UINT32_MAX;

  uint8_t const *s = (uint8_t const *)string.ptr;
  if (string.size <= 12)
    s = (uint8_t const *)(&string.prefix);
  uint8_t const *p = (uint8_t const *)pattern.ptr;
  if (pattern.size <= 12)
    p = (uint8_t const *)(&pattern.prefix);

  for (size_t i = start; i < (string.size - pattern.size); ++i) {
    size_t j = 0;

    while (j < pattern.size && s[i + j] == p[j])
      ++j;

    if (j == pattern.size)
      return i;
  }

  return UINT32_MAX;
}

static inline bool _attemptInplaceConstruction(struct UmbraString *s,
                                               char const *value,
                                               uint32_t size) {
  s->size = size;
  s->prefix = 0;
  s->data = 0;

  // Always build the prefix
  for (uint32_t i = 0; i < 4 && i < s->size; i++) {
    s->prefix |= (uint32_t)value[i] << (8 * i);
  }

  // Failed in-place construction
  if (s->size > 12)
    return false;

  // Construct in-place
  for (uint32_t i = 4; i < s->size; i++) {
    s->data |= (uint64_t)value[i] << (8 * (i - 4));
  }

  return true;
}

struct UmbraString createUmbraStringNull(void) {
  struct UmbraString string;
  string.size = 0;
  string.prefix = 0;
  string.data = 0;
  string.ptr = NULL;

  return string;
}

struct UmbraString copyUmbraStringBlock(struct UmbraString const original,
                                        struct BlockAllocator *allocator) {
  struct UmbraString string;
  string.size = original.size;
  string.prefix = original.prefix;
  string.data = original.data;
  string.ptr = NULL;

  if (original.size <= 12) {
    return string;
  }

  string.ptr = allocateBlockAllocator(allocator, original.size * sizeof(char),
                                      alignof(char));
  DEBUG_ASSERT(string.ptr != NULL, "Failed to allocate for umbra string");

  memcpy((char *)string.ptr, original.ptr, sizeof(char) * original.size);

  return string;
}

struct UmbraString
copyUmbraStringImplicit(struct UmbraString const original,
                        struct ImplicitAllocator *allocator) {
  struct UmbraString string;
  string.size = original.size;
  string.prefix = original.prefix;
  string.data = original.data;
  string.ptr = NULL;

  if (original.size <= 12) {
    return string;
  }

  string.ptr = allocateImplicitAllocator(
      allocator, original.size * sizeof(char), alignof(char));
  DEBUG_ASSERT(string.ptr != NULL, "Failed to allocate for umbra string");

  memcpy((char *)string.ptr, original.ptr, sizeof(char) * original.size);

  return string;
}

void createUmbraStringParasitic(struct UmbraString *s, char const *value) {
  createUmbraStringBoundParasitic(s, value, strlen(value));
}

void createUmbraStringBoundParasitic(struct UmbraString *s, char const *value,
                                     uint32_t valueLength) {
  if (_attemptInplaceConstruction(s, value, valueLength))
    return;

  s->ptr = value;
}

void createUmbraStringAllocate(struct UmbraString *s, char const *value,
                               struct BlockAllocator *allocator) {
  createUmbraStringBoundAllocate(s, value, strlen(value), allocator);
}

void createUmbraStringBoundAllocate(struct UmbraString *s, char const *value,
                                    uint32_t valueSize,
                                    struct BlockAllocator *allocator) {
  if (_attemptInplaceConstruction(s, value, valueSize))
    return;

  // Allocate larger block for string
  void *memory = allocateBlockAllocator(allocator, s->size, alignof(char));

  // Note we don't include the \0 terminator
  memcpy(memory, value, s->size);
  s->ptr = memory;
}

void createUmbraStringTransient(struct UmbraString *s, char const *value,
                                struct ImplicitAllocator *allocator) {
  createUmbraStringBoundTransient(s, value, strlen(value), allocator);
}

void createUmbraStringBoundTransient(struct UmbraString *s, char const *value,
                                     uint32_t valueLength,
                                     struct ImplicitAllocator *allocator) {
  if (_attemptInplaceConstruction(s, value, valueLength))
    return;

  // Allocate larger block for string
  void *memory = allocateImplicitAllocator(allocator, s->size, alignof(char));

  // TODO: return fail code?
  if (memory == NULL)
    return;

  // Note we don't include the \0 terminator
  memcpy(memory, value, s->size);
  s->ptr = memory;
}

void freeUmbraStringTransient(struct UmbraString *s,
                              struct ImplicitAllocator *allocator) {
  // Allocated in place
  if (s->size <= 12)
    return;

  freeFastAllocationImplicitAllocator(allocator, (void *)s->ptr, s->size);
}

void saveUmbraString(struct UmbraString const *s, FILE *p) {
  // TODO: note works of the assumption that size appears first at offset 0
  // Assume value is stored in place, just write entire object trivially
  fwrite(&s->size, sizeof(s->size), 1, p);
  fwrite(&s->prefix, sizeof(s->prefix), 1, p);
  if (s->size <= 12) {
    fwrite(&s->data, sizeof(s->data), 1, p);
    return;
  }

  // Write data stored at pointer
  fwrite(s->ptr, sizeof(char), s->size, p);
}

struct UmbraString loadUmbraString(struct BlockAllocator *allocator, FILE *p) {
  struct UmbraString string = {0};
  // Read in size
  fread(&string.size, sizeof(string.size), 1, p);
  fread(&string.prefix, sizeof(string.prefix), 1, p);

  if (string.size <= 12) {
    fread(&string.data, sizeof(string.data), 1, p);
  } else {
    string.ptr = allocateBlockAllocator(allocator, string.size * sizeof(char),
                                        alignof(char));
    DEBUG_ASSERT(string.ptr != NULL, "Failed to allocate for umbra string");
    // TODO: const cast here...
    fread((void *)string.ptr, sizeof(char), string.size, p);
  }

  return string;
}

char *getUmbraPtr(struct UmbraString *string) {
  if (string->size <= 12) {
    return (char *)(&string->prefix);
  }

  return string->ptr;
}

char const *getUmbraPtrConst(struct UmbraString const *string) {
  if (string->size <= 12) {
    return (char const *)(&string->prefix);
  }

  return string->ptr;
}
