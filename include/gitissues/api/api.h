#ifndef _GITISSUES_API_API_H_
#define _GITISSUES_API_API_H_

#include "gitissues/defines.h"
#include "gitissues/ecs/string_map.h"
#include <gitissues/global.h>
#include <gitissues/iff/iff.h>
#include <gitissues/iff/schema.h>
#include <gitissues/issue.h>

// File defines the exact set of functions required for implementation of any
// binding.
// Any umbra string passed to the API may be transient.

/*
0. Init/terminate functions
1. Load an IFF file full of issues, given a schema, convert to list of issue IDs
- for now 1 schema, 1 file
2. Save a list of issues to an IFF file, given a schema
3. Create a new issue
4. Delete an issue
5. Attach tags to an issue (auto-register if not on schema)
6. Detatch tags from an issue
7. Get tag from an issue
8. Check if issue has tag
9. Load a schema
*/

inline void gitissuesInit(void) { createGlobalContext(); }
inline void gitissuesTerminate(void) { freeGlobalContext(); }

inline struct Schema *gitissuesLoadSchema(char const *filename) {
  struct Schema *schema =
      transientAllocate(sizeof(struct Schema), alignof(struct Schema));
  *schema = readSchema(filename);

  return schema;
}

inline void gitissuesFreeSchema(struct Schema *schema) {
  freeSchema(schema);
  freeTransient(schema, sizeof(struct Schema));
}

inline void gitissuesLoadIFF(struct Schema *schema, char const *filename,
                             struct Issue **issues, uint32_t *issuesSize) {
  // TODO: if we do a bunch of re-syncs, we eventually run out of global space
  readIFFFile(filename, getGlobalLifetimeAllocator(), issues, issuesSize,
              schema);
}
inline void gitissuesSaveIFF(struct Schema *schema, char const *filename,
                             struct Issue const *issues, uint32_t issuesSize) {
  writeIFFFile(filename, issues, issuesSize, schema);
}

// Issue lives in a semi-valid state until we provide required tags
inline struct Issue gitissuesCreateIssue(struct Schema *schema) {
  return createIssue(&schema->registry);
}
inline void gitissuesAttachTag(struct Schema *schema, struct Issue issue,
                               void *data, uint32_t sizeOfType,
                               struct UmbraString const tag) {
  struct UmbraString tagString = tag;
  if (!isRegistered(&schema->registry, tag)) {
    tagString =
        copyUmbraStringBlock(tag, &schema->registry.lifetimeAllocations);
  }

  ComponentID tagID = registerTag(&schema->registry, tagString, sizeOfType);
  addTagById(&schema->registry, issue, tagID, (uint8_t *)data);
}
inline void gitissuesDetachTag(struct Schema *schema, struct Issue issue,
                               struct UmbraString const tag) {
  DEBUG_ASSERT(isRegistered(&schema->registry, tag), "Tag was not registered");

  ComponentID tagID = getComponentID(&schema->registry, tag);
  removeTagById(&schema->registry, issue, tagID);
}
inline void *gitissuesGetTag(struct Schema *schema, struct Issue issue,
                             struct UmbraString const tag) {
  DEBUG_ASSERT(isRegistered(&schema->registry, tag), "Tag was not registered");

  ComponentID tagID = getComponentID(&schema->registry, tag);
  return getTagById(&schema->registry, issue, tagID);
}
inline void *gitissuesGetTagByID(struct Schema *schema, struct Issue issue,
                                 ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");
  return getTagById(&schema->registry, issue, tagID);
}
inline void gitissuesDetachTagByID(struct Schema *schema, struct Issue issue,
                                   ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");
  removeTagById(&schema->registry, issue, tagID);
}
inline void gitissuesAttachTagByID(struct Schema *schema, struct Issue issue,
                                   void *data, ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");
  addTagById(&schema->registry, issue, tagID, (uint8_t *)data);
}
inline bool gitissuesHasTag(struct Schema *schema, struct Issue issue,
                            struct UmbraString const tag) {
  DEBUG_ASSERT(isRegistered(&schema->registry, tag), "Tag was not registered");

  ComponentID tagID = getComponentID(&schema->registry, tag);
  return hasComponent(&schema->registry, issue.entity, tagID);
}
inline bool gitissuesHasTagByID(struct Schema *schema, struct Issue issue,
                                ComponentID tagID) {
  DEBUG_ASSERT(tagID != _GITISSUES_COMPONENT_INVALID, "Tag was not registered");

  return hasComponent(&schema->registry, issue.entity, tagID);
}

#endif
