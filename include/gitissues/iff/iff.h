#ifndef _GITISSUES_IFF_IFF_H_
#define _GITISSUES_IFF_IFF_H_

#include <gitissues/ecs/string_map.h>
#include <gitissues/iff/schema.h>
#include <gitissues/issue.h>
#include <gitissues/umbra_string.h>

// Read IFF file format
// file: [issue]*
// issue: [text] [separator symbol] [taglist] [terminator symbol]
// taglist: [tag]*
// tag: [tagname] [tagvalue]
// tagname: alphanumeric or underscore or -
// tagvalue: "string", int, float, boolean (true/false), date

void readIFFFile(char const *filename, struct Registry *registry,
                 struct BlockAllocator *allocator, struct Issue **issues,
                 uint32_t *issuesSize, struct Schema *schema);

#endif
