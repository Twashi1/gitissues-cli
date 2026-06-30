#ifndef _GITISSUES_STRING_MAP_H_
#define _GITISSUES_STRING_MAP_H_

#include <gitissues/defines.h>
#include <gitissues/errs.h>
#include <gitissues/umbra_string.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define _GITISSUES_HASH_MAP_MINIMUM ((uint32_t)8)  // 8 elements by default
#define _GITISSUES_STRING_MAP_LOAD_FACTOR ((float)0.85f)
#define _GITISSUES_COMPONENT_INVALID ((uint32_t)UINT32_MAX)

typedef uint32_t ComponentID;

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// Should be public domain, using wikipedia primes
static inline uint32_t fnv1aHash(char const* string) {
    DEBUG_ASSERT(string != NULL, "Passed null string to hash");
    DEBUG_ASSERT(string[0] != '\0', "Passed empty string to hash");

    uint32_t hash = 0x811c9dc5;

    while (*string != '\0') {
        hash = hash ^ (uint32_t)(*string);
        hash = hash * 0x01000193;

        ++string;
    }

    return hash;
}

static inline uint32_t fnv1aHashUmbra(struct UmbraString const string) {
    DEBUG_ASSERT(string.size > 0, "Passed empty string to hash");

    char const* ptr = string.ptr;

    // SSO
    if (string.size <= 12) {
        ptr = (char const*)&string.prefix;
    }

    uint32_t hash = 0x811c9dc5;

    printf("Hello in hash, string size: %d\n", string.size);

    for (uint32_t i = 0; i < string.size; i++) {
        hash = hash ^ (uint32_t)(ptr[i]);
        hash = hash * 0x01000193;
    }

    printf("Hashed string to get %d\n", hash);

    return hash;
}

// TODO: consider implementation of robin-hood hashing, but for now, simple linear probing will work
//  notably, no deletion, so tombstone elements aren't required
struct StringMapNode {
    struct UmbraString string;
    // TODO: prefer instead 3 bytes distance, 1 byte hash?
    // - correlates to ~1.6 million elements offset (enough)
    // notably we never expect to use the full component id, so we can encode an extra byte there
    // too?
    // compare just the high bits of the hash (low bits will determine index generally, and
    // thus are of lower importance)
    uint32_t stringHash;
    // the component id/actual value
    ComponentID value;
};

_Static_assert(alignof(struct StringMapNode) == alignof(void*),
               "StringMapNode should have pointer alignment");
_Static_assert(sizeof(struct StringMapNode) == 24, "StringMapNode had unexpected padding");

// TODO: should consider
// - we could store value types inline, with a vector storing the actual metadata?
// - at minimum, want a vector of just the component ids, easily iterable?
// - string map nodes instead just point to the component id? (extra indirection?)

struct StringMap {
    struct StringMapNode* data;
    uint32_t size;
    uint32_t capacity;
    uint32_t threshold;

    struct BlockAllocator stringAllocator;
};

// TODO: get rid of string allocator, we don't copy the UmbraStrings
// TODO: unit tests

struct StringMap createStringMap(void);
void freeStringMap(struct StringMap* map);

enum ErrorCode reserveStringMap(struct StringMap* map, uint32_t minCapacity);
// TODO: make it clear we take ownership of the string, but don't modify it
// TODO: use error code on both
enum ErrorCode insertStringMap(struct StringMap* map, struct UmbraString string, ComponentID value);
enum ErrorCode _insertUncheckedStringMap(struct StringMap* map, struct UmbraString string,
                                         ComponentID value);
ComponentID getStringMap(struct StringMap* map, struct UmbraString string);

#endif
