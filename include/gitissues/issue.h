#ifndef _GITISSUES_ISSUE_H_
#define _GITISSUES_ISSUE_H_

#include <gitissues/ecs/registry.h>
#include <gitissues/ecs/string_map.h>
#include <gitissues/tag.h>
#include <stdint.h>

struct Issue {
  char const *title;
  Entity entity;

  struct {
    ComponentID *data;
    uint32_t size;
    uint32_t capacity;
  } tags;
};

struct Issue createIssue(struct Registry *registry, char const *title);
void freeIssue(struct Registry *registry, struct Issue *issue);

ComponentID registerTag(struct Registry *registry, struct UmbraString const tag,
                        uint32_t sizeOfType);
void addTagByName(struct Registry *registry, struct Issue *issue,
                  struct UmbraString const tag, uint8_t *data);
void addTagById(struct Registry *registry, struct Issue *issue, ComponentID id,
                uint8_t *data);

// TODO: load/save by text (all to JSON; provide utilities to write to JSON,
// then user-defined functions?)

#endif
