#ifndef _GITISSUES_TAG_H_
#define _GITISSUES_TAG_H_

#include <stdint.h>

struct Tag {
    void* data;
    char const* name;
    uint32_t size;
};

#endif
