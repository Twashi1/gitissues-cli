#ifndef _GITISSUES_ISSUE_H_
#define _GITISSUES_ISSUE_H_

#include <gitissues/tag.h>
#include <stdint.h>

struct Issue {
    char const* title;
    char const* description;

    struct {
        struct Tag* data;
        uint32_t size;
        uint32_t capacity;
    } tags;
};

void _fitTagList(struct Issue* issue, uint32_t newCapacity);
void attachTag(struct Issue* issue, struct Tag* tag);
// void saveIssue(struct Issue* issue, )

#endif
