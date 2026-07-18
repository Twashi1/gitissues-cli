#ifndef _GITISSUES_ISSUE_H_
#define _GITISSUES_ISSUE_H_

#include <gitissues/ecs/registry.h>
#include <gitissues/ecs/string_map.h>
#include <gitissues/json/json.h>
#include <stdint.h>

struct Issue {
  Entity entity;
};

// TODO: don't pass in Issue*, just pass in Issue
struct Issue createIssue(struct Registry *registry);
void freeIssue(struct Registry *registry, struct Issue *issue);

ComponentID registerTag(struct Registry *registry, struct UmbraString const tag,
                        uint32_t sizeOfType);
void addTagByName(struct Registry *registry, struct Issue *issue,
                  struct UmbraString const tag, uint8_t *data);
void addTagById(struct Registry *registry, struct Issue *issue, ComponentID id,
                uint8_t *data);

void removeTagByName(struct Registry *registry, struct Issue *issue,
                     struct UmbraString const tag);
void removeTagById(struct Registry *registry, struct Issue *issue,
                   ComponentID id);

uint8_t *getTagByName(struct Registry *registry, struct Issue *issue,
                      struct UmbraString tag);
uint8_t *getTagById(struct Registry *registry, struct Issue *issue,
                    ComponentID id);

// TODO: load/save by text (all to JSON; provide utilities to write to JSON,
// then user-defined functions?)

void jsonWriteIssue(struct Registry *registry, struct Issue *issue, FILE *p);
enum ErrorCode jsonReadIssue(struct Registry *registry, struct JsonReader *p,
                             struct Issue *value);

#endif
