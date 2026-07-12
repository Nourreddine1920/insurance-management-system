#ifndef ERROR_H
#define ERROR_H

/**
 * @brief System-wide error codes.
 */
typedef enum {
    ERR_OK = 0,             /**< No error (success) */
    ERR_SQLITE,             /**< SQLite database operation failure */
    ERR_NOT_FOUND,          /**< Entity not found */
    ERR_VALIDATION,         /**< Business rule validation failed */
    ERR_OUT_OF_MEMORY,      /**< Dynamic memory allocation failed */
    ERR_INVALID_ARG,        /**< Invalid arguments passed to a function */
    ERR_GENERIC             /**< General uncategorized error */
} error_t;

/**
 * @brief Translates an error code into a human-readable string.
 * @param err The error code to translate.
 * @return A constant pointer to the error string.
 */
const char* error_to_string(error_t err);

#endif // ERROR_H
