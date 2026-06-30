#ifndef _GITISSUES_ECS_ENTITY_H_
#define _GITISSUES_ECS_ENTITY_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t Entity;

#define _ECS_TOMBSTONE ((Entity)0xfff00000)
#define _ECS_NULL ((Entity)0x000fffff)
#define _ECS_DEAD ((Entity)(_ECS_TOMBSTONE | _ECS_NULL))
#define _ECS_ENTITY_TO_POS ((Entity)(~_ECS_TOMBSTONE))

static inline uint32_t entityToPos(Entity e) {
    return e & _ECS_ENTITY_TO_POS;
}

#endif
