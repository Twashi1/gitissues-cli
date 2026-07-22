#include <gitissues/issue.h>

struct Issue createIssue(struct Registry *registry) {
  struct Issue issue = {0};
  issue.entity = createEntity(registry);

  return issue;
}

// TODO: don't take pointer in most of these functions
void freeIssue(struct Registry *registry, struct Issue *issue) {
  freeEntity(registry, issue->entity);
}

ComponentID registerTag(struct Registry *registry, struct UmbraString const tag,
                        uint32_t sizeOfType) {
  // TODO: confirm not already registered
  return registerComponentID(registry, tag, sizeOfType);
}

void addTagByName(struct Registry *registry, struct Issue *issue,
                  struct UmbraString const tag, uint8_t *data) {
  ComponentID id = getComponentID(registry, tag);

  addTagById(registry, issue, id, data);
}

void addTagById(struct Registry *registry, struct Issue *issue, ComponentID id,
                uint8_t *data) {
  addComponent(registry, issue->entity, id, data);
}

void removeTagByName(struct Registry *registry, struct Issue *issue,
                     struct UmbraString const tag) {
  removeTagById(registry, issue, getComponentID(registry, tag));
}

void removeTagById(struct Registry *registry, struct Issue *issue,
                   ComponentID id) {
  removeComponent(registry, issue->entity, id);
}

uint8_t *getTagByName(struct Registry *registry, struct Issue *issue,
                      struct UmbraString tag) {
  return getTagById(registry, issue, getComponentID(registry, tag));
}

uint8_t *getTagById(struct Registry *registry, struct Issue *issue,
                    ComponentID id) {
  return getComponent(registry, issue->entity, id);
}

void jsonWriteIssue(struct Registry *registry, struct Issue *issue, FILE *p) {
  saveEntityJson(registry, issue->entity, p);
}

enum ErrorCode jsonReadIssue(struct Registry *registry, struct JsonReader *p,
                             struct Issue *value) {
  value->entity = loadEntityJson(registry, p);

  return GITISSUES_OK;
}
