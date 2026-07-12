#include "error.h"

const char* error_to_string(error_t err) {
    switch (err) {
        case ERR_OK:
            return "Success";
        case ERR_SQLITE:
            return "Database operation failure (SQLite error)";
        case ERR_NOT_FOUND:
            return "Requested resource or entity not found";
        case ERR_VALIDATION:
            return "Business validation constraint violated";
        case ERR_OUT_OF_MEMORY:
            return "System out of memory";
        case ERR_INVALID_ARG:
            return "Invalid arguments provided";
        case ERR_GENERIC:
        default:
            return "Uncategorized generic error occurred";
    }
}
