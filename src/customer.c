/*
 * customer.c — Business Logic + Data Access for Customer entities.
 *
 * Layer responsibilities in this file:
 *  - Validate input parameters (Business Logic Layer).
 *  - Construct and execute parameterised SQL queries (Data Access Layer).
 *  - Map SQLite column results back into Customer structs.
 *  - Propagate errors upward using error_t return codes.
 *
 * Why prepared statements?
 *  sprintf() to build SQL strings allows SQL injection. Even in internal
 *  tools, a customer name like "O'Brien" would corrupt a raw SQL string.
 *  sqlite3_prepare_v2() + sqlite3_bind_*() handles quoting, escaping,
 *  and type safety automatically. It also allows SQLite to cache the
 *  compiled query plan for repeated executions — a significant speedup.
 */

#include "customer.h"
#include "database.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Internal helper: safely copy a SQLite text column into a fixed      */
/* char buffer. sqlite3_column_text() returns (const unsigned char *). */
/* We cast to (const char *) and use snprintf to guard buffer bounds.  */
/* ------------------------------------------------------------------ */
static void copy_column_text(char *dest, size_t dest_size, sqlite3_stmt *stmt, int col) {
    const char *src = (const char *)sqlite3_column_text(stmt, col);
    if (src) {
        snprintf(dest, dest_size, "%s", src);
    } else {
        dest[0] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* Internal helper: map a prepared statement row into a Customer struct.*/
/* Column order MUST match the SELECT column order in the query.       */
/* ------------------------------------------------------------------ */
static void map_row_to_customer(sqlite3_stmt *stmt, Customer *out) {
    out->id = sqlite3_column_int(stmt, 0);
    copy_column_text(out->first_name, sizeof(out->first_name), stmt, 1);
    copy_column_text(out->last_name,  sizeof(out->last_name),  stmt, 2);
    copy_column_text(out->email,      sizeof(out->email),      stmt, 3);
    copy_column_text(out->phone,      sizeof(out->phone),      stmt, 4);
    copy_column_text(out->created_at, sizeof(out->created_at), stmt, 5);
}

/* ------------------------------------------------------------------ */
/* customer_create()                                                    */
/*                                                                      */
/* Inserts a new customer into the database.                           */
/*                                                                      */
/* Professional note: We do NOT set customer->id here. The caller      */
/* should call customer_get_by_email() after creation if they need the */
/* auto-generated ID. Alternatively, sqlite3_last_insert_rowid() can   */
/* be used, but only immediately after the INSERT on the same conn.    */
/* ------------------------------------------------------------------ */
error_t customer_create(const Customer *customer) {
    if (!customer) {
        LOG_ERROR("customer_create: NULL customer pointer received.");
        return ERR_INVALID_ARG;
    }
    if (customer->first_name[0] == '\0' || customer->last_name[0] == '\0') {
        LOG_ERROR("customer_create: first_name and last_name are required.");
        return ERR_VALIDATION;
    }
    if (customer->email[0] == '\0') {
        LOG_ERROR("customer_create: email is required.");
        return ERR_VALIDATION;
    }

    sqlite3 *db = db_get_connection();
    if (!db) {
        LOG_ERROR("customer_create: No active database connection.");
        return ERR_SQLITE;
    }

    const char *sql =
        "INSERT INTO customers (first_name, last_name, email, phone) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("customer_create: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    /* Bind parameters by position (1-indexed). SQLITE_STATIC means SQLite
     * will NOT copy the string — it trusts our pointer stays valid during
     * sqlite3_step(). Since customer is on our stack frame, this is safe. */
    sqlite3_bind_text(stmt, 1, customer->first_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, customer->last_name,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, customer->email,      -1, SQLITE_STATIC);

    /* Phone is optional. Bind NULL if empty. */
    if (customer->phone[0] != '\0') {
        sqlite3_bind_text(stmt, 4, customer->phone, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt); /* ALWAYS finalize — even on error — to free resources. */

    if (rc != SQLITE_DONE) {
        /* SQLITE_CONSTRAINT_UNIQUE fires if email already exists. */
        LOG_ERROR("customer_create: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    LOG_INFO("Customer created: %s %s <%s>",
             customer->first_name, customer->last_name, customer->email);
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* customer_get_by_id()                                                 */
/*                                                                      */
/* Fetches a single customer row by primary key.                       */
/* Returns ERR_NOT_FOUND if the ID doesn't exist in the database.      */
/* ------------------------------------------------------------------ */
error_t customer_get_by_id(int id, Customer *out_customer) {
    if (id <= 0) {
        LOG_ERROR("customer_get_by_id: invalid id %d.", id);
        return ERR_INVALID_ARG;
    }
    if (!out_customer) {
        LOG_ERROR("customer_get_by_id: NULL output pointer.");
        return ERR_INVALID_ARG;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql =
        "SELECT id, first_name, last_name, email, phone, created_at "
        "FROM customers WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("customer_get_by_id: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        map_row_to_customer(stmt, out_customer);
        sqlite3_finalize(stmt);
        LOG_DEBUG("customer_get_by_id: found customer id=%d", id);
        return ERR_OK;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        LOG_WARN("customer_get_by_id: no customer found with id=%d", id);
        return ERR_NOT_FOUND;
    }

    LOG_ERROR("customer_get_by_id: step error: %s", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return ERR_SQLITE;
}

/* ------------------------------------------------------------------ */
/* customer_update()                                                    */
/*                                                                      */
/* Updates an existing customer record. We identify the row by id      */
/* (from the struct) and overwrite all mutable fields.                 */
/* Note: created_at is immutable — it is never updated here.          */
/* ------------------------------------------------------------------ */
error_t customer_update(const Customer *customer) {
    if (!customer || customer->id <= 0) {
        LOG_ERROR("customer_update: invalid customer pointer or id.");
        return ERR_INVALID_ARG;
    }
    if (customer->first_name[0] == '\0' || customer->last_name[0] == '\0') {
        LOG_ERROR("customer_update: first_name and last_name are required.");
        return ERR_VALIDATION;
    }
    if (customer->email[0] == '\0') {
        LOG_ERROR("customer_update: email is required.");
        return ERR_VALIDATION;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql =
        "UPDATE customers SET first_name=?, last_name=?, email=?, phone=? "
        "WHERE id=?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("customer_update: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_text(stmt, 1, customer->first_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, customer->last_name,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, customer->email,      -1, SQLITE_STATIC);

    if (customer->phone[0] != '\0') {
        sqlite3_bind_text(stmt, 4, customer->phone, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    sqlite3_bind_int(stmt, 5, customer->id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("customer_update: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    /* sqlite3_changes() returns the number of rows modified. If 0,
     * the WHERE id=? matched nothing — the customer doesn't exist. */
    if (sqlite3_changes(db) == 0) {
        LOG_WARN("customer_update: no row updated — customer id=%d not found.", customer->id);
        return ERR_NOT_FOUND;
    }

    LOG_INFO("Customer id=%d updated successfully.", customer->id);
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* customer_delete()                                                    */
/*                                                                      */
/* Deletes a customer record.                                          */
/* The FOREIGN KEY ... ON DELETE RESTRICT on policies.customer_id      */
/* will cause this to fail if the customer still has policies.         */
/* This is by design — the business rule enforced at DB level.        */
/* ------------------------------------------------------------------ */
error_t customer_delete(int id) {
    if (id <= 0) {
        LOG_ERROR("customer_delete: invalid id %d.", id);
        return ERR_INVALID_ARG;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql = "DELETE FROM customers WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("customer_delete: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("customer_delete: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    if (sqlite3_changes(db) == 0) {
        LOG_WARN("customer_delete: no customer found with id=%d.", id);
        return ERR_NOT_FOUND;
    }

    LOG_INFO("Customer id=%d deleted.", id);
    return ERR_OK;
}
