#ifndef _GITISSUES_API_API_H_
#define _GITISSUES_API_API_H_

#include "gitissues/defines.h"
#include "gitissues/ecs/string_map.h"
#include <gitissues/global.h>
#include <gitissues/iff/iff.h>
#include <gitissues/iff/schema.h>
#include <gitissues/issue.h>
#include <gitissues/uuid.h>

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
10. Remove a list of issues
11. Check if an issue is null
*/

void gitissuesInit(void);
void gitissuesTerminate(void);

struct Schema *gitissuesLoadSchema(char const *filename);

void gitissuesFreeSchema(struct Schema *schema);

void gitissuesLoadIFF(struct Schema *schema, char const *filename,
                      struct Issue **issues, uint32_t *issuesSize);
void gitissuesSaveIFF(struct Schema *schema, char const *filename,
                      struct Issue const *issues, uint32_t issuesSize);

// Issue lives in a semi-valid state until we provide required tags
struct Issue gitissuesCreateIssue(struct Schema *schema);
void gitissuesAttachTag(struct Schema *schema, struct Issue issue, void *data,
                        uint32_t sizeOfType, struct UmbraString const tag);
void gitissuesDetachTag(struct Schema *schema, struct Issue issue,
                        struct UmbraString const tag);
void *gitissuesGetTag(struct Schema *schema, struct Issue issue,
                      struct UmbraString const tag);
void *gitissuesGetTagByID(struct Schema *schema, struct Issue issue,
                          ComponentID tagID);
void gitissuesDetachTagByID(struct Schema *schema, struct Issue issue,
                            ComponentID tagID);
void gitissuesAttachTagByID(struct Schema *schema, struct Issue issue,
                            void *data, ComponentID tagID);
bool gitissuesHasTag(struct Schema *schema, struct Issue issue,
                     struct UmbraString const tag);
bool gitissuesHasTagByID(struct Schema *schema, struct Issue issue,
                         ComponentID tagID);

void gitissuesRemoveIssues(struct Schema *schema, struct Issue const *issues,
                           uint32_t issuesSize);
bool gitissuesIsNullIssue(struct Schema *schema, struct Issue const issue);
UUID7 gitissuesGetOrCreateUUID(struct Schema *schema, struct Issue issue);
#endif
