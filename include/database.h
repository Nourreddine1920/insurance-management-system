#ifndef DATABASE_H
#define DATABASE_H

#include "error.h"
#include <sqlite3.h>

/**
 * @brief Initializes and opens the database connection.
 * @param db_path Path to the SQLite database file.
 * @return error_t ERR_OK on success, ERR_SQLITE on connection/init error.
 */
error_t db_initialize(const char *db_path);

/**
 * @brief Closes the active database connection.
 * @return error_t ERR_OK on success, ERR_SQLITE if closure fails.
 */
error_t db_close(void);

/**
 * @brief Retrieves the active SQLite database handle.
 * @return A pointer to the sqlite3 handle.
 */
sqlite3* db_get_connection(void);

/**
 * @brief Begins a new database transaction.
 * @return error_t ERR_OK on success.
 */
error_t db_begin_transaction(void);

/**
 * @brief Commits the current database transaction.
 * @return error_t ERR_OK on success.
 */
error_t db_commit_transaction(void);

/**
 * @brief Rolls back the current database transaction.
 * @return error_t ERR_OK on success.
 */
error_t db_rollback_transaction(void);

#endif // DATABASE_H
