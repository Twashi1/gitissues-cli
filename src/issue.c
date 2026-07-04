#include <gitissues/issue.h>

struct Issue createIssue(struct Registry *registry, char const *title) {
  struct Issue issue = {0};
  issue.entity = createEntity(registry);
  issue.title = title;

  return issue;
}

void freeIssue(struct Registry *registry, struct Issue *issue) {
  freeEntity(registry, issue->entity);
  free(issue->tags.data);
}

ComponentID registerTag(struct Registry *registry, struct UmbraString const tag,
                        uint32_t sizeOfType) {
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

  if (issue->tags.size >= issue->tags.capacity) {
    uint32_t newCapacity =
        issue->tags.capacity + (issue->tags.capacity >> 1) + 1;
    void *p = realloc(issue->tags.data, sizeof(ComponentID) * newCapacity);
    DEBUG_ASSERT(p != NULL, "Failed to allocate issue tags");
    issue->tags.data = p;
    issue->tags.capacity = newCapacity;
  }

  // TODO: maintaining this is annoying
  // ideally, for add/removes, we make this a small dense map?
  issue->tags.data[issue->tags.size++] = id;
}
