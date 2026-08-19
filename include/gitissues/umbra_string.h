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

// TODO: unused
struct FastComparisonMetadata {
  uint8_t *patternJump;
};

struct UmbraString {
  uint32_t size;
  uint32_t prefix;
  union {
    char *ptr;
    uint64_t data;
  };
};

_Static_assert(alignof(struct UmbraString) >= alignof(const char *),
               "UmbraString must satisfy pointer alignment requirements");
_Static_assert(alignof(struct UmbraString) >= alignof(uint32_t),
               "uint32_t alignment violated");

_Static_assert(alignof(struct UmbraString) >= alignof(char *),
               "pointer alignment violated");
_Static_assert(offsetof(struct UmbraString, size) == 0,
               "size must be first member");
_Static_assert(offsetof(struct UmbraString, data) == 8,
               "union must come after size");
_Static_assert(sizeof(struct UmbraString) == 16,
               "unexpected struct padding or layout change");
_Static_assert(sizeof(const char *) == alignof(const char *),
               "unexpected pointer ABI layout");
_Static_assert(alignof(struct UmbraString) == 8 ||
                   alignof(struct UmbraString) == 4,
               "unexpected platform alignment");

// TODO: create some init/terminate function that calls all the various
// init/terminate (like those for allocators too)
// void initUmbraString(void);
// void terminateUmbraString(void);

// TODO: functions should work on generic string ranges
bool umbraCompare(struct UmbraString const a, struct UmbraString const b);
bool umbraCompareString(struct UmbraString const a, char const *b);
uint32_t umbraFirstIndexOf(struct UmbraString const string,
                           struct UmbraString const substring, uint32_t offset);

// Semantics of this naming is unclear; should be take ownership
void createUmbraStringParasitic(struct UmbraString *s, char const *value);
void createUmbraStringBoundParasitic(struct UmbraString *s, char const *value,
                                     uint32_t valueLength);
struct UmbraString createUmbraStringNull(void);
struct UmbraString copyUmbraStringBlock(struct UmbraString const original,
                                        struct BlockAllocator *allocator);
struct UmbraString copyUmbraStringImplicit(struct UmbraString const original,
                                           struct ImplicitAllocator *allocator);

void createUmbraStringAllocate(struct UmbraString *s, char const *value,
                               struct BlockAllocator *allocator);
void createUmbraStringBoundAllocate(struct UmbraString *s, char const *value,
                                    uint32_t valueLength,
                                    struct BlockAllocator *allocator);
void createUmbraStringTransient(struct UmbraString *s, char const *value,
                                struct ImplicitAllocator *allocator);
void createUmbraStringBoundTransient(struct UmbraString *s, char const *value,
                                     uint32_t valueLength,
                                     struct ImplicitAllocator *allocator);
void freeUmbraStringTransient(struct UmbraString *s,
                              struct ImplicitAllocator *allocator);

void saveUmbraString(struct UmbraString const *s, FILE *p);
struct UmbraString loadUmbraString(struct BlockAllocator *allocator, FILE *p);

// Not null-terminated!
char *getUmbraPtr(struct UmbraString *s);
char const *getUmbraPtrConst(struct UmbraString const *s);

#endif
