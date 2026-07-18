#ifndef _GITISSUES_ECS_ENTITY_H_
#define _GITISSUES_ECS_ENTITY_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t Entity;

// TODO: instead of version numbers, use the extra bits as a condensed bitset to
// track the first 11 component pools with an id of 12 indicating more than 11
// pools (so manually search on free, less optimisations for checking)
// Requires syncing when we add a component to an entity

#define _ECS_TOMBSTONE ((Entity)0xfff00000)
#define _ECS_IDENTIFIER_BITS 20
#define _ECS_NULL ((Entity)0x000fffff)
#define _ECS_DEAD ((Entity)(_ECS_TOMBSTONE | _ECS_NULL))

static inline uint32_t entityToPos(Entity e) { return e & (~_ECS_TOMBSTONE); }
static inline uint32_t entityToVersion(Entity e) {
  return (e & _ECS_TOMBSTONE) >> _ECS_IDENTIFIER_BITS;
}
static inline Entity incrementEntityVersion(Entity e) {
  if ((e & _ECS_TOMBSTONE) == _ECS_TOMBSTONE) {
    return e;
  }

  return e + (1 << _ECS_IDENTIFIER_BITS);
}

#endif
