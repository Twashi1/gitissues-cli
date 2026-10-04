#include "gitissues/ecs/registry.h"
#include <gitissues/api/api.h>

void gitissuesInit(void) { createGlobalContext(); }
void gitissuesTerminate(void) { freeGlobalContext(); }

struct Schema *gitissuesLoadSchema(char const *filename) {
  struct Schema *schema =
      transientAllocate(sizeof(struct Schema), alignof(struct Schema));
  *schema = readSchema(filename);

  return schema;
}

void gitissuesFreeSchema(struct Schema *schema) {
  freeSchema(schema);
  freeTransient(schema, sizeof(struct Schema));
}

void gitissuesLoadIFF(struct Schema *schema, char const *filename,
                      struct Issue **issues, uint32_t *issuesSize) {
  // TODO: if we do a bunch of re-syncs, we eventually run out of global space
  readIFFFile(filename, getGlobalLifetimeAllocator(), issues, issuesSize,
              schema);
}

void gitissuesSaveIFF(struct Schema *schema, char const *filename,
                      struct Issue const *issues, uint32_t issuesSize) {
  writeIFFFile(filename, issues, issuesSize, schema);
}

// Issue lives in a semi-valid state until we provide required tags
struct Issue gitissuesCreateIssue(struct Schema *schema) {
  return createIssue(&schema->registry);
}

void gitissuesAttachTag(struct Schema *schema, struct Issue issue, void *data,
                        uint32_t sizeOfType, struct UmbraString const tag) {
  struct UmbraString tagString = tag;
  if (!isRegistered(&schema->registry, tag)) {
    tagString =
        copyUmbraStringBlock(tag, &schema->registry.lifetimeAllocations);
  }

  ComponentID tagID = registerTag(&schema->registry, tagString, sizeOfType);
  addTagById(&schema->registry, issue, tagID, (uint8_t *)data);
}

void gitissuesDetachTag(struct Schema *schema, struct Issue issue,
                        struct UmbraString const tag) {
  DEBUG_ASSERT(isRegistered(&schema->registry, tag), "Tag was not registered");

  ComponentID tagID = getComponentID(&schema->registry, tag);
  removeTagById(&schema->registry, issue, tagID);
}

void *gitissuesGetTag(struct Schema *schema, struct Issue issue,
                      struct UmbraString const tag) {
  DEBUG_ASSERT(isRegistered(&schema->registry, tag), "Tag was not registered");

  ComponentID tagID = getComponentID(&schema->registry, tag);
  return getTagById(&schema->registry, issue, tagID);
}

void *gitissuesGetTagByID(struct Schema *schema, struct Issue issue,
                          ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");
  return getTagById(&schema->registry, issue, tagID);
}

void gitissuesDetachTagByID(struct Schema *schema, struct Issue issue,
                            ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");
  removeTagById(&schema->registry, issue, tagID);
}

void gitissuesAttachTagByID(struct Schema *schema, struct Issue issue,
                            void *data, ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");
  addTagById(&schema->registry, issue, tagID, (uint8_t *)data);
}

bool gitissuesHasTag(struct Schema *schema, struct Issue issue,
                     struct UmbraString const tag) {
  DEBUG_ASSERT(isRegistered(&schema->registry, tag), "Tag was not registered");

  ComponentID tagID = getComponentID(&schema->registry, tag);
  return hasComponent(&schema->registry, issue.entity, tagID);
}

bool gitissuesHasTagByID(struct Schema *schema, struct Issue issue,
                         ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");

  return hasComponent(&schema->registry, issue.entity, tagID);
}

void gitissuesRemoveIssues(struct Schema *schema, struct Issue const *issues,
                           uint32_t issuesSize) {
  // Should be O(n)
  for (uint32_t i = 0; i < issuesSize; i++) {
    struct Issue const issue = issues[i];

    freeEntity(&schema->registry, issue.entity);
  }
}

bool gitissuesIsNullIssue(struct Schema *schema, struct Issue const issue) {
  return isEntityNull(&schema->registry, issue.entity);
}

UUID7 gitissuesGetOrCreateUUID(struct Schema *schema, struct Issue issue) {
  if (hasComponent(&schema->registry, issue.entity, schema->identifierID)) {
    return *(UUID7 *)getComponent(&schema->registry, issue.entity,
                                  schema->identifierID);
  }

  UUID7 uuid;
  UUID7Generate(&schema->rng, &uuid);
  addComponent(&schema->registry, issue.entity, schema->identifierID,
               (uint8_t *)&uuid);

  return uuid;
}
