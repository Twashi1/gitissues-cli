#include "gitissues/defines.h"
#include "gitissues/ecs/registry.h"
#include "gitissues/json/json.h"
#include "gitissues/umbra_string.h"
#include <gitissues/issue.h>

struct Issue createIssue(struct Registry *registry) {
  struct Issue issue = {0};
  issue.entity = createEntity(registry);

  return issue;
}

// TODO: don't take pointer in most of these functions
void freeIssue(struct Registry *registry, struct Issue issue) {
  freeEntity(registry, issue.entity);
}

ComponentID registerTag(struct Registry *registry, struct UmbraString const tag,
                        uint32_t sizeOfType) {
  // TODO: confirm not already registered
  return registerComponentID(registry, tag, sizeOfType);
}

void addTagByName(struct Registry *registry, struct Issue issue,
                  struct UmbraString const tag, uint8_t *data) {
  ComponentID id = getComponentID(registry, tag);

  addTagById(registry, issue, id, data);
}

void addTagById(struct Registry *registry, struct Issue issue, ComponentID id,
                uint8_t *data) {
  addComponent(registry, issue.entity, id, data);
}

void removeTagByName(struct Registry *registry, struct Issue issue,
                     struct UmbraString const tag) {
  removeTagById(registry, issue, getComponentID(registry, tag));
}

void removeTagById(struct Registry *registry, struct Issue issue,
                   ComponentID id) {
  removeComponent(registry, issue.entity, id);
}

uint8_t *getTagByName(struct Registry *registry, struct Issue issue,
                      struct UmbraString tag) {
  return getTagById(registry, issue, getComponentID(registry, tag));
}

uint8_t *getTagById(struct Registry *registry, struct Issue issue,
                    ComponentID id) {
  return getComponent(registry, issue.entity, id);
}

uint8_t const *getTagByIdConst(struct Registry const *registry,
                               struct Issue issue, ComponentID id) {
  return getComponentConst(registry, issue.entity, id);
}

void jsonSaveIssues(struct Registry *registry, struct Issue *issues,
                    uint32_t count, char const *filename) {
  DEBUG_ASSERT(issues != NULL, "Passed null issues to save");

  FILE *p = fopen(filename, "w");
  DEBUG_ASSERT(p != NULL, "Failed to open file for writing");

  jsonWriteObjectBegin(p);

  for (uint32_t i = 0; i < count; i++) {
    if (i > 0) {
      jsonWriteNext(p);
    }

    saveEntityJson(registry, issues[i].entity, p);
  }

  jsonWriteObjectEnd(p);
}

void jsonLoadIssues(struct Registry *registry, struct Issue **issues,
                    uint32_t *count, struct JsonReader *p) {
  DEBUG_ASSERT(issues != NULL, "Passed null pointer to load issues into");
  DEBUG_ASSERT(count != NULL, "Passed null pointer to load issues into");

  struct {
    struct Issue *data;
    uint32_t size;
    uint32_t capacity;
  } issueArray;

  issueArray.data = NULL;
  issueArray.size = 0;
  issueArray.capacity = 0;

  ARRAY_RESERVE(issueArray, 8, ARRAY_GROWTH_PLUS_ONE);

  do {
    jsonReadObjectBegin(p);

    // Deal with empty list
    if (jsonPeekNext(p) == '}') {
      jsonReadObjectEnd(p);
      break;
    }

    Entity entity = loadEntityJson(registry, p);
    struct Issue issue;
    issue.entity = entity;

    ARRAY_APPEND(issueArray, issue, ARRAY_GROWTH_ONE_HALF);

    jsonReadObjectEnd(p);
  } while (jsonReadNext(p));

  *issues = issueArray.data;
  *count = issueArray.size;
}

void jsonWriteIssue(struct Registry *registry, struct Issue issue, FILE *p) {
  saveEntityJson(registry, issue.entity, p);
}

void jsonReadIssue(struct Registry *registry, struct JsonReader *p,
                   struct Issue *value) {
  value->entity = loadEntityJson(registry, p);
}
