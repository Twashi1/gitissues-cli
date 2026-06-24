#include <gitissues/umbra_string.h>

bool compare(struct UmbraString const a, struct UmbraString const b) {
    // Compare prefix first
    if (LIKELY(a.prefix != b.prefix)) {
        return false;
    }

    // Compare length
    if (a.size != b.size) {
        return false;
    }

    // If short-string
    if (a.size <= 12) {
        // Prefix already matches, just compare remaining data
        // We're going to assume an invariant; that with short-strings, we pad \0 bytes
        if (a.data != b.data) {
            return false;
        }

        return true;
    }

    // Compare ptr address for long strings (start at 4)
    for (uint32_t i = 4; i < a.size; i++) {
        if (a.ptr[i] != b.ptr[i]) {
            return false;
        }
    }

    return true;
}

void setModifiableString(struct UmbraString* s, char const* value) {
    // Do it with 2-pass
    uint32_t size = (uint32_t)strlen(value);
    s->size = size;
    s->prefix = 0;
    s->data = 0;

    for (uint32_t i = 0; i < 4 && i < size; i++) {
        s->prefix |= (uint32_t)value[i] << (8 * i);
    }

    if (size <= 12) {
        for (uint32_t i = 0; i < size - 4; i++) {
            s->data |= (uint64_t)value[i + 4] << (8 * i);
        }

        return;
    }

    return;
}

void setImmutableString(struct UmbraString* s, char const* value) {
    return;
}
