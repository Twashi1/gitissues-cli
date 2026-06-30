#include <gitissues/tag.h>

uint32_t getSizeOfTag(enum TagType const tag) {
    uint32_t size = 0;

    //  enum TagType { UI32, I32, F32, STRING, BYTES, STRUCT, IS_ARRAY = 0x7000 };

    switch (tag & TAG_TYPE_BASE) {
        case TAG_TYPE_UI32:
        case TAG_TYPE_I32:
        case TAG_TYPE_F32:
            size = 4;
            break;
    }
}

struct Tag createTagUI32(char const* name, uint32_t value, struct BlockAllocator* allocator) {
    struct Tag tag;
    tag.size = sizeof(uint32_t);
    setModifiableString(&tag.name, name, allocator);
    tag.data = allocateBlockAllocator(allocator, sizeof(uint32_t), alignof(uint32_t));
    if (tag.data == NULL)
        return tag;

    *(uint32_t*)tag.data = value;

    return tag;
}

struct Tag createTagI32(char const* name, int32_t value, struct BlockAllocator* allocator) {
    struct Tag tag;
    tag.size = sizeof(int32_t);
    setModifiableString(&tag.name, name, allocator);
    tag.data = allocateBlockAllocator(allocator, sizeof(int32_t), alignof(int32_t));
    if (tag.data == NULL)
        return tag;

    *(int32_t*)tag.data = value;

    return tag;
}

struct Tag createTagF32(char const* name, float value, struct BlockAllocator* allocator) {
    struct Tag tag;
    tag.size = sizeof(float);
    setModifiableString(&tag.name, name, allocator);
    tag.data = allocateBlockAllocator(allocator, sizeof(float), alignof(float));
    if (tag.data == NULL)
        return tag;

    *(float*)tag.data = value;

    return tag;
}

struct Tag createTagString(char const* name, char const* value, struct BlockAllocator* allocator) {
    uint32_t size = (uint32_t)strlen(value);

    struct Tag tag;
    tag.size = size + 1;
    setModifiableString(&tag.name, name, allocator);
    tag.data = allocateBlockAllocator(allocator, size + 1, alignof(char));
    if (tag.data == NULL)
        return tag;

    memcpy(tag.data, value, size + 1);

    return tag;
}

struct Tag createTagBytes(char const* name, uint8_t const* value, uint32_t numBytes,
                          struct BlockAllocator* allocator) {
    struct Tag tag;
    tag.size = numBytes;
    setModifiableString(&tag.name, name, allocator);
    tag.data = allocateBlockAllocator(allocator, numBytes, alignof(uint8_t));
    if (tag.data == NULL)
        return tag;

    memcpy(tag.data, value, numBytes);

    return tag;
}

struct Tag createTagBytesMove(char const* name, uint8_t const* value, uint32_t numBytes,
                              struct BlockAllocator* allocator) {
    struct Tag tag;
    tag.size = numBytes;
    setModifiableString(&tag.name, name, allocator);
    tag.data = (void*)value;

    return tag;
}
