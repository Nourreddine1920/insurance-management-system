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
#include <string.h>

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

/* ------------------------------------------------------------------ */
/* db_initialize()                                                      */
/*                                                                      */
/* Opens (or creates) the SQLite database file.                        */
/* Then enables foreign key enforcement — this is a per-connection     */
/* PRAGMA that SQLite resets each time a new connection is opened.     */
/* Finally, applies the schema via CREATE TABLE IF NOT EXISTS, which   */
/* is idempotent: safe to run on both first-launch and subsequent runs.*/
/* ------------------------------------------------------------------ */
error_t db_initialize(const char *db_path) {
    if (global_db != NULL) {
        LOG_WARN("db_initialize() called but a connection is already open. Ignoring.");
        return ERR_OK;
    }

    /* sqlite3_open() creates the file if it doesn't exist.
     * It returns SQLITE_OK even for a bad path (creation deferred),
     * so we must check with sqlite3_errmsg() after writing. */
    int rc = sqlite3_open(db_path, &global_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Cannot open database '%s': %s", db_path, sqlite3_errmsg(global_db));
        sqlite3_close(global_db);
        global_db = NULL;
        return ERR_SQLITE;
    }
    LOG_INFO("Database connection opened: %s", db_path);

    /* CRITICAL: SQLite disables foreign key enforcement by default.
     * This PRAGMA must be set on every new connection. */
    error_t err = db_exec_simple("PRAGMA foreign_keys = ON;");
    if (err != ERR_OK) {
        LOG_ERROR("Failed to enable foreign key enforcement.");
        db_close();
        return err;
    }

    /* Apply schema. CREATE TABLE IF NOT EXISTS is idempotent —
     * it is safe to run on an existing database. */
    const char *schema_sql =
        /* customers */
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

        /* policies */
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

        /* claims */
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

    err = db_exec_simple(schema_sql);
    if (err != ERR_OK) {
        LOG_ERROR("Failed to apply database schema.");
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
