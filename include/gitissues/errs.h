#ifndef _GITISSUES_ERRS_H_
#define _GITISSUES_ERRS_H_

#include <gitissues/defines.h>

enum ErrorCode {
    GITISSUES_OK,
    GITISSUES_OUT_OF_MEMORY,
    GITISSUES_ENTITY_ALREADY_EXISTED,
    GITISSUES_ENTITY_DID_NOT_EXIST,
    GITISSUES_STRING_MAP_INSERTION_FAIL,
};

static inline char const* errorCodeString(enum ErrorCode ec) {
    switch (ec) {
        case GITISSUES_OK:
            return "OK";
        case GITISSUES_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case GITISSUES_ENTITY_DID_NOT_EXIST:
            return "ENTITY_DID_NOT_EXIST";
        case GITISSUES_ENTITY_ALREADY_EXISTED:
            return "ENTITY_ALREADY_EXISTED";
        case GITISSUES_STRING_MAP_INSERTION_FAIL:
            return "STRING_MAP_INSERTION_FAIL";
        default:
            return "Unknown";
    }
}

#define DEBUG_PRINT_ERROR(str, ec) \
    DEBUG_STATEMENT(if (ec != GITISSUES_OK) { printf(str, errorCodeString(ec)); })

#endif
