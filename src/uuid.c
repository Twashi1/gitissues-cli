#include <gitissues/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint32_t PCG32Next(PCG32 *rng) {
  uint64_t oldstate = rng->state;

  rng->state =
      oldstate * UINT64_C(6364136223846793005) + (rng->inc | UINT64_C(1));

  uint32_t xorshifted = (uint32_t)(((oldstate >> 18) ^ oldstate) >> 27);

  uint32_t rot = (uint32_t)(oldstate >> 59);

  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static void PCG32Seed(PCG32 *rng, uint64_t state, uint64_t sequence) {
  rng->state = 0;
  rng->inc = (sequence << 1) | 1;

  (void)PCG32Next(rng);
  rng->state += state;
  (void)PCG32Next(rng);
}

static void PCG32GenerateBytes(PCG32 *rng, uint8_t *dst, size_t n) {
  while (n >= 4) {
    uint32_t x = PCG32Next(rng);

    dst[0] = (uint8_t)(x >> 24);
    dst[1] = (uint8_t)(x >> 16);
    dst[2] = (uint8_t)(x >> 8);
    dst[3] = (uint8_t)x;

    dst += 4;
    n -= 4;
  }

  if (n) {
    uint32_t x = PCG32Next(rng);

    while (n--) {
      *dst++ = (uint8_t)(x >> 24);
      x <<= 8;
    }
  }
}

/*
 * Return Unix time in milliseconds.
 *
 * C11 timespec_get() gives calendar time, but does not specify
 * the epoch explicitly. TIME_UTC is defined as UTC-based calendar
 * time, whose representation is compatible with Unix time on
 * normal C implementations.
 */
static uint64_t UUID7TimestampMS(void) {
  struct timespec ts;

  if (timespec_get(&ts, TIME_UTC) != TIME_UTC)
    return 0;

  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

void UUID7Seed(PCG32 *rng) {
  struct timespec ts;

  if (timespec_get(&ts, TIME_UTC) == TIME_UTC) {
    unsigned seed =
        (unsigned)ts.tv_nsec ^ (unsigned)ts.tv_sec ^ (unsigned)(uintptr_t)&ts;

    // TODO: look into an actual good starting value, maybe this will have
    // impact on distribution
    PCG32Seed(rng, seed, 0xdeadbeef);
  } else {
    PCG32Seed(rng, (unsigned)time(NULL), 0xdeadbeef);
  }
}

int UUID7Generate(PCG32 *rng, UUID7 *u) {
  uint64_t timestamp;
  uint8_t random[10];

  if (u == NULL)
    return -1;

  timestamp = UUID7TimestampMS();

  if (timestamp == 0)
    return -1;

  PCG32GenerateBytes(rng, random, sizeof random);

  // 48-bit timestamp
  u->bytes[0] = (uint8_t)(timestamp >> 40);
  u->bytes[1] = (uint8_t)(timestamp >> 32);
  u->bytes[2] = (uint8_t)(timestamp >> 24);
  u->bytes[3] = (uint8_t)(timestamp >> 16);
  u->bytes[4] = (uint8_t)(timestamp >> 8);
  u->bytes[5] = (uint8_t)timestamp;

  // Version 1110 + 12 random bits
  u->bytes[6] = (uint8_t)(0x70 | (random[0] & 0x0f));
  u->bytes[7] = random[1];

  // Variant 10 + 62 random bits
  u->bytes[8] = (uint8_t)(0x80 | (random[2] & 0x3f));
  u->bytes[9] = random[3];
  u->bytes[10] = random[4];
  u->bytes[11] = random[5];
  u->bytes[12] = random[6];
  u->bytes[13] = random[7];
  u->bytes[14] = random[8];
  u->bytes[15] = random[9];

  return 0;
}

void UUID7Format(UUID7 const *u, char out[37]) {
  static const char hex[] = "0123456789abcdef";

  int pos = 0;

  for (int i = 0; i < 16; ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10)
      out[pos++] = '-';

    out[pos++] = hex[u->bytes[i] >> 4];
    out[pos++] = hex[u->bytes[i] & 0x0f];
  }

  out[pos] = '\0';
}
