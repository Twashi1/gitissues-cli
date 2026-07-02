#ifndef _GITISSUES_UMBRA_STRING_H_
#define _GITISSUES_UMBRA_STRING_H_

#include <gitissues/allocator.h>
#include <gitissues/defines.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct UmbraString {
    uint32_t size;
    uint32_t prefix;
    union {
        char const* ptr;
        uint64_t data;
    };
};

_Static_assert(alignof(struct UmbraString) >= alignof(const char*),
               "UmbraString must satisfy pointer alignment requirements");
_Static_assert(alignof(struct UmbraString) >= alignof(uint32_t), "uint32_t alignment violated");

_Static_assert(alignof(struct UmbraString) >= alignof(char*), "pointer alignment violated");
_Static_assert(offsetof(struct UmbraString, size) == 0, "size must be first member");
_Static_assert(offsetof(struct UmbraString, data) == 8, "union must come after size");
_Static_assert(sizeof(struct UmbraString) == 16, "unexpected struct padding or layout change");
_Static_assert(sizeof(const char*) == alignof(const char*), "unexpected pointer ABI layout");
_Static_assert(alignof(struct UmbraString) == 8 || alignof(struct UmbraString) == 4,
               "unexpected platform alignment");

// TODO: evaluate whether or not we need such an optimized string construct; consider
// that modifiable string performance is poor if we need to make frequent changes
bool compare(struct UmbraString const a, struct UmbraString const b);
bool _attemptInplaceConstruction(struct UmbraString* s, char const* value);
void setModifiableString(struct UmbraString* s, char const* value,
                         struct BlockAllocator* allocator);
void setImmutableString(struct UmbraString* s, char const* value);

void saveUmbraString(struct UmbraString const* s, FILE* p);
struct UmbraString loadUmbraString(struct BlockAllocator* allocator, FILE* p);

#endif
