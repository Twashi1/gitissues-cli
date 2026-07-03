#include <gitissues/umbra_string.h>
#include <stddef.h>
#include <stdio.h>

bool compare(struct UmbraString const a, struct UmbraString const b) {
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

bool _attemptInplaceConstruction(struct UmbraString *s, char const *value) {
  // Do it with 2-pass
  s->size = (uint32_t)strlen(value);
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
  for (uint32_t i = 0; i < s->size - 4; i++) {
    s->data |= (uint64_t)value[i + 4] << (8 * i);
  }

  return true;
}

void setModifiableString(struct UmbraString *s, char const *value,
                         struct BlockAllocator *allocator) {
  if (_attemptInplaceConstruction(s, value))
    return;

  // Allocate larger block for string
  void *memory = allocateBlockAllocator(allocator, s->size, alignof(char));

  // TODO: return fail code?
  if (memory == NULL)
    return;

  // Note we don't include the \0 terminator
  memcpy(memory, value, s->size);
  s->ptr = memory;

  return;
}

void setImmutableString(struct UmbraString *s, char const *value) {
  if (_attemptInplaceConstruction(s, value))
    return;

  // If larger, just copy the actual pointer, we have guarantee on the lifetime
  s->ptr = value;

  return;
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
