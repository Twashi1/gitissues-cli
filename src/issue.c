#include <gitissues/issue.h>

void _fitTagList(struct Issue* issue, uint32_t newCapacity) {
    if (issue->tags.capacity >= newCapacity)
        return;

    uint32_t growSize = issue->tags.capacity + (issue->tags.capacity >> 1);
    newCapacity = newCapacity >= growSize ? newCapacity : growSize;

    void* p = realloc(issue->tags.data, sizeof(struct Tag) * newCapacity);
    issue->tags.data = p;

    if (issue->tags.data == NULL) {
        return;
    }

    issue->tags.capacity = newCapacity;
}

void attachTag(struct Issue* issue, struct Tag* tag) {
    // Check tag with same name doesn't already exist
    // TODO: could make this a hashmap
    for (uint32_t i = 0; i < issue->tags.size; i++) {
        struct Tag* existing = &issue->tags.data[i];

        // Tag already existed; update value instead
        if (compare(existing->name, tag->name)) {
            *existing = *tag;

            return;
        }
    }

    _fitTagList(issue, issue->tags.size + 1);

    issue->tags.data[issue->tags.size++] = *tag;
}
