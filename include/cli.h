#ifndef CLI_H
#define CLI_H

#include "error.h"

/**
 * @brief Runs the interactive command-line interface.
 * @param db_path Path to the SQLite database file.
 * @return 0 on normal exit, non-zero on failure.
 */
int cli_run(const char *db_path);

#endif // CLI_H
