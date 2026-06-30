#ifndef _GITISSUES_TAG_H_
#define _GITISSUES_TAG_H_

#include <gitissues/allocator.h>
#include <gitissues/umbra_string.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>

enum TagType {
    TAG_TYPE_UI32,
    TAG_TYPE_I32,
    TAG_TYPE_F32,
    TAG_TYPE_STRING,
    TAG_TYPE_BYTES,
    TAG_TYPE_STRUCT,
    TAG_TYPE_ARRAY = 0x8000,
    TAG_TYPE_BASE = 0x7fff
};

struct Tag {
    void* data;
    struct UmbraString name;
    uint32_t size;
};

uint32_t getSizeType(enum TagType const tag);

struct Tag createTagUI32(char const* name, uint32_t value, struct BlockAllocator* allocator);
struct Tag createTagI32(char const* name, int32_t value, struct BlockAllocator* allocator);
struct Tag createTagF32(char const* name, float value, struct BlockAllocator* allocator);
struct Tag createTagString(char const* name, char const* value, struct BlockAllocator* allocator);
struct Tag createTagBytes(char const* name, uint8_t const* value, uint32_t numBytes,
                          struct BlockAllocator* allocator);
struct Tag createTagBytesMove(char const* name, uint8_t const* value, uint32_t numBytes,
                              struct BlockAllocator* allocator);

// TODO: requirements
// - save/load to text format
// - save/load to binary format
// - tag can store any arbitrary data, including structs/arrays/strings
// further requirements
// - assume information of what tags belong to what issues are lost (i.e. cannot recreate by calling
// the right functions)
//

// Create arbitrary data and serialise automatically?
// - need either type information of to pass in a function pointer
// - assume we want to serialise an array/struct?
// - for an array, assume same type
// -  createTagArray(name, pointer to array data, size of data, TagType, allocator)
// -  createTagArrayMove(...)

// User needs to know the required allocation for the data they're about to prepare

// Split into three steps
//  step 0
//  getSizeTag(type)
//  step 1 - writes data to pointer; pointer is written to based on size of tag
//  prepareTagData(value <implied type>, pointer)
//  prepareTagDataArray(value*, num, pointer)
//  step 2 - constructs tag, taking ownership of pointer (no copy; move)
//  createTag(name, data pointer, type)
//  createTagArray(name, data pointer, num elems, type)

_Static_assert(sizeof(float) == 4, "Size of float not 4 bytes");

#endif
