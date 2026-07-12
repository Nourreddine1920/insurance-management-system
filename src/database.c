/*
 * database.c — Data Access Layer (DAL) Implementation
 *
 * This is the ONLY file that directly invokes sqlite3_* API calls.
 * All other modules (customer, policy, claim) call db_* functions
 * defined here. This enforces a clean abstraction boundary and means
 * that swapping out SQLite for another database requires changes to
 * only this file.
 *
 * Architecture Note:
 *   db_initialize() loads the schema from an embedded SQL string
 *   (or from a file). We use PRAGMA foreign_keys = ON because
 *   SQLite disables foreign key enforcement by default for historical
 *   compatibility — this MUST be re-enabled on every new connection.
 */

#include "database.h"
#include "logger.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif

/* ------------------------------------------------------------------ */
/* Module-private state — the single database connection handle.       */
/* Using a module-level static variable ensures there is exactly one   */
/* connection across the entire application (Singleton pattern).       */
/* ------------------------------------------------------------------ */
static sqlite3 *global_db = NULL;

/* ------------------------------------------------------------------ */
/* Internal helper — execute a raw SQL string with no parameters.      */
/* Used for one-shot commands like PRAGMA or BEGIN/COMMIT/ROLLBACK.    */
/* ------------------------------------------------------------------ */
static error_t db_exec_simple(const char *sql) {
    char *errmsg = NULL;
    int rc = sqlite3_exec(global_db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQLite exec failed [%s]: %s", sql, errmsg ? errmsg : "unknown error");
        sqlite3_free(errmsg);
        return ERR_SQLITE;
    }
    return ERR_OK;
}

static int compare_strings(const void *a, const void *b) {
    const char *const *sa = a;
    const char *const *sb = b;
    return strcmp(*sa, *sb);
}

static void free_string_array(char **files, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        free(files[i]);
    }
    free(files);
}

static error_t load_file_to_string(const char *path, char **out_sql) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("Failed to open migration file: %s", path);
        return ERR_SQLITE;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return ERR_SQLITE;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return ERR_SQLITE;
    }
    rewind(file);

    char *buffer = malloc((size_t)file_size + 1);
    if (!buffer) {
        fclose(file);
        return ERR_OUT_OF_MEMORY;
    }

    size_t read_size = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);
    if (read_size != (size_t)file_size) {
        free(buffer);
        return ERR_SQLITE;
    }

    buffer[read_size] = '\0';
    *out_sql = buffer;
    return ERR_OK;
}

static error_t ensure_migration_table(void) {
    return db_exec_simple(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version TEXT PRIMARY KEY,"
        "  applied_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");");
}

static error_t has_migration_been_applied(const char *version, int *applied) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT COUNT(1) FROM schema_migrations WHERE version = ?1;";
    int rc = sqlite3_prepare_v2(global_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ERR_SQLITE;
    }

    sqlite3_bind_text(stmt, 1, version, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return ERR_SQLITE;
    }

    *applied = sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return ERR_OK;
}

static error_t record_migration(const char *version) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO schema_migrations (version) VALUES (?1);";
    int rc = sqlite3_prepare_v2(global_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ERR_SQLITE;
    }

    sqlite3_bind_text(stmt, 1, version, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        return ERR_SQLITE;
    }
    return ERR_OK;
}

static const char *get_filename_from_path(const char *path) {
    const char *basename = strrchr(path, '/');
    if (!basename) {
        basename = strrchr(path, '\\');
    }
    return basename ? basename + 1 : path;
}

static error_t apply_migration_file(const char *path) {
    char *sql = NULL;
    error_t err = load_file_to_string(path, &sql);
    if (err != ERR_OK) {
        return err;
    }

    const char *version = get_filename_from_path(path);
    LOG_INFO("Applying migration %s", version);

    err = db_begin_transaction();
    if (err != ERR_OK) {
        free(sql);
        return err;
    }

    err = db_exec_simple(sql);
    if (err != ERR_OK) {
        db_rollback_transaction();
        free(sql);
        return err;
    }

    err = record_migration(version);
    if (err != ERR_OK) {
        db_rollback_transaction();
        free(sql);
        return err;
    }

    err = db_commit_transaction();
    if (err != ERR_OK) {
        db_rollback_transaction();
    }

    free(sql);
    return err;
}

static error_t discover_migration_files(const char *directory, char ***out_paths, size_t *out_count) {
    char **paths = NULL;
    size_t count = 0;

#ifdef _WIN32
    char search_pattern[MAX_PATH];
    snprintf(search_pattern, sizeof(search_pattern), "%s\\*.sql", directory);

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(search_pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            *out_paths = NULL;
            *out_count = 0;
            return ERR_OK;
        }
        return ERR_SQLITE;
    }

    do {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        size_t path_length = strlen(directory) + 1 + strlen(find_data.cFileName) + 1;
        char *path = malloc(path_length);
        if (!path) {
            FindClose(handle);
            free_string_array(paths, count);
            return ERR_OUT_OF_MEMORY;
        }
        snprintf(path, path_length, "%s\\%s", directory, find_data.cFileName);
        char **next = realloc(paths, (count + 1) * sizeof(char *));
        if (!next) {
            free(path);
            FindClose(handle);
            free_string_array(paths, count);
            return ERR_OUT_OF_MEMORY;
        }
        paths = next;
        paths[count++] = path;
    } while (FindNextFileA(handle, &find_data) != 0);

    FindClose(handle);
#else
    DIR *dir = opendir(directory);
    if (!dir) {
        *out_paths = NULL;
        *out_count = 0;
        return ERR_OK;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        size_t len = strlen(name);
        if (len < 5 || strcmp(name + len - 4, ".sql") != 0) {
            continue;
        }

        size_t path_length = strlen(directory) + 1 + len + 1;
        char *path = malloc(path_length);
        if (!path) {
            closedir(dir);
            free_string_array(paths, count);
            return ERR_OUT_OF_MEMORY;
        }
        snprintf(path, path_length, "%s/%s", directory, name);
        char **next = realloc(paths, (count + 1) * sizeof(char *));
        if (!next) {
            free(path);
            closedir(dir);
            free_string_array(paths, count);
            return ERR_OUT_OF_MEMORY;
        }
        paths = next;
        paths[count++] = path;
    }
    closedir(dir);
#endif

    if (count > 0) {
        qsort(paths, count, sizeof(char *), compare_strings);
    }

    *out_paths = paths;
    *out_count = count;
    return ERR_OK;
}

static const char *builtin_schema_sql =
    "CREATE TABLE IF NOT EXISTS customers ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  first_name TEXT NOT NULL,"
    "  last_name  TEXT NOT NULL,"
    "  email      TEXT NOT NULL UNIQUE,"
    "  phone      TEXT,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE UNIQUE INDEX IF NOT EXISTS idx_customers_email"
    "  ON customers(email);"
    "CREATE INDEX IF NOT EXISTS idx_customers_name"
    "  ON customers(last_name, first_name);"
    "CREATE TABLE IF NOT EXISTS policies ("
    "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  customer_id INTEGER NOT NULL,"
    "  policy_type TEXT    NOT NULL,"
    "  premium     REAL    NOT NULL CHECK (premium >= 0),"
    "  start_date  TEXT    NOT NULL,"
    "  end_date    TEXT    NOT NULL,"
    "  status      TEXT    NOT NULL"
    "    CHECK (status IN ('ACTIVE','EXPIRED','CANCELLED'))," 
    "  FOREIGN KEY (customer_id) REFERENCES customers(id) ON DELETE RESTRICT"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_policies_customer_id"
    "  ON policies(customer_id);"
    "CREATE TABLE IF NOT EXISTS claims ("
    "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  policy_id   INTEGER NOT NULL,"
    "  description TEXT    NOT NULL,"
    "  amount      REAL    NOT NULL CHECK (amount >= 0),"
    "  claim_date  TEXT    NOT NULL,"
    "  status      TEXT    NOT NULL"
    "    CHECK (status IN ('PENDING','APPROVED','REJECTED'))," 
    "  FOREIGN KEY (policy_id) REFERENCES policies(id) ON DELETE RESTRICT"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_claims_policy_id"
    "  ON claims(policy_id);";

static error_t db_apply_migrations(void) {
    char **migration_paths = NULL;
    size_t migration_count = 0;
    error_t err = discover_migration_files("database/migrations", &migration_paths, &migration_count);
    if (err != ERR_OK) {
        return err;
    }

    if (migration_count == 0) {
        LOG_WARN("No migration files found, falling back to built-in schema.");
        return db_exec_simple(builtin_schema_sql);
    }

    err = ensure_migration_table();
    if (err != ERR_OK) {
        free_string_array(migration_paths, migration_count);
        return err;
    }

    for (size_t i = 0; i < migration_count; ++i) {
        const char *version = get_filename_from_path(migration_paths[i]);
        int applied = 0;
        err = has_migration_been_applied(version, &applied);
        if (err != ERR_OK) {
            free_string_array(migration_paths, migration_count);
            return err;
        }
        if (applied) {
            LOG_DEBUG("Migration already applied: %s", version);
            continue;
        }
        err = apply_migration_file(migration_paths[i]);
        if (err != ERR_OK) {
            free_string_array(migration_paths, migration_count);
            return err;
        }
    }

    free_string_array(migration_paths, migration_count);
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* db_initialize()                                                      */
/*                                                                      */
/* Opens (or creates) the SQLite database file.                        */
/* Then enables foreign key enforcement — this is a per-connection     */
/* PRAGMA that SQLite resets each time a new connection is opened.     */
/* Finally, applies the database schema using migrations or fallback.  */
/* ------------------------------------------------------------------ */
error_t db_initialize(const char *db_path) {
    if (global_db != NULL) {
        LOG_WARN("db_initialize() called but a connection is already open. Ignoring.");
        return ERR_OK;
    }

    int rc = sqlite3_open(db_path, &global_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Cannot open database '%s': %s", db_path, sqlite3_errmsg(global_db));
        sqlite3_close(global_db);
        global_db = NULL;
        return ERR_SQLITE;
    }
    LOG_INFO("Database connection opened: %s", db_path);

    error_t err = db_exec_simple("PRAGMA foreign_keys = ON;");
    if (err != ERR_OK) {
        LOG_ERROR("Failed to enable foreign key enforcement.");
        db_close();
        return err;
    }

    err = db_apply_migrations();
    if (err != ERR_OK) {
        LOG_ERROR("Failed to apply database migrations.");
        db_close();
        return err;
    }

    LOG_INFO("Database schema verified / applied successfully.");
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* db_close()                                                           */
/*                                                                      */
/* Closes the database connection. sqlite3_close() returns SQLITE_BUSY */
/* if there are still outstanding prepared statements that have not     */
/* been finalized — a common bug in production. We log this and        */
/* return ERR_SQLITE so the caller knows something went wrong.         */
/* ------------------------------------------------------------------ */
error_t db_close(void) {
    if (global_db == NULL) {
        LOG_WARN("db_close() called but no connection is open.");
        return ERR_OK;
    }

    int rc = sqlite3_close(global_db);
    if (rc != SQLITE_OK) {
        /* SQLITE_BUSY here means prepared statements were not finalized.
         * This is a programmer error — not a user or runtime error. */
        LOG_ERROR("db_close() failed (SQLITE_BUSY?) — unfinalized statements? Code: %d", rc);
        return ERR_SQLITE;
    }

    global_db = NULL;
    LOG_INFO("Database connection closed cleanly.");
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* db_get_connection()                                                  */
/*                                                                      */
/* Returns the raw sqlite3* handle to modules that need to prepare     */
/* and execute their own queries. Modules MUST NOT call sqlite3_close   */
/* on this handle — only db_close() should do so.                      */
/* ------------------------------------------------------------------ */
sqlite3 *db_get_connection(void) {
    return global_db;
}

/* ------------------------------------------------------------------ */
/* Transaction Management                                               */
/*                                                                      */
/* Transactions are essential for any multi-step operation (e.g.,      */
/* creating a policy that also needs to verify a customer exists).      */
/* If any step fails, we ROLLBACK to leave the database in a clean     */
/* consistent state — the "all or nothing" guarantee.                  */
/* ------------------------------------------------------------------ */

error_t db_begin_transaction(void) {
    LOG_DEBUG("BEGIN TRANSACTION");
    return db_exec_simple("BEGIN TRANSACTION;");
}

error_t db_commit_transaction(void) {
    LOG_DEBUG("COMMIT TRANSACTION");
    return db_exec_simple("COMMIT;");
}

error_t db_rollback_transaction(void) {
    LOG_WARN("ROLLBACK TRANSACTION");
    return db_exec_simple("ROLLBACK;");
}
