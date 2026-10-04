#ifndef _GITISSUES_UUID_H_
#define _GITISSUES_UUID_H_

#include <stdint.h>

typedef struct {
  uint8_t bytes[16];
} UUID7;

typedef struct {
  uint64_t state;
  uint64_t inc;
} PCG32;

void UUID7Seed(PCG32 *rng);

// Generate something close enough to a UUIDv7.
// 48 bit timestamp, 4 bit version (0111), 12 bits rand, 2 bits variant (10), 62
// bits rand.
// Returns 0 on success.
int UUID7Generate(PCG32 *rng, UUID7 *u);

// Returns formatted as hex, with dashses between sections
void UUID7Format(UUID7 const *u, char out[37]);

#endif // _GITISSUES_UUID_H_