/*
 * policy.c — Business Logic + Data Access for Insurance Policy entities.
 *
 * Business Rules enforced here (BLL):
 *   1. A policy MUST reference a valid, existing customer (customer_id > 0).
 *   2. premium must be >= 0.
 *   3. end_date must be strictly after start_date (chronological validity).
 *   4. status must be one of: ACTIVE, EXPIRED, CANCELLED.
 *   5. policy_type must not be empty.
 *
 * The FK constraint in SQLite (customer_id → customers.id) acts as a
 * final safety net, but we validate at the BLL first to give meaningful
 * error messages instead of raw SQLITE_CONSTRAINT codes.
 *
 * Data Access pattern: same as customer.c — prepared statements only,
 * column order in SELECT must match map_row_to_policy() column indices.
 */

#include "policy.h"
#include "database.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Valid status strings — used for BLL validation.                     */
/* ------------------------------------------------------------------ */
static const char * const VALID_POLICY_STATUSES[] = {
    "ACTIVE", "EXPIRED", "CANCELLED", NULL
};

/* ------------------------------------------------------------------ */
/* Internal helper: validate that a status string is one of the        */
/* allowed values. Returns 1 if valid, 0 if invalid.                  */
/* ------------------------------------------------------------------ */
static int is_valid_policy_status(const char *status) {
    for (int i = 0; VALID_POLICY_STATUSES[i] != NULL; i++) {
        if (strcmp(status, VALID_POLICY_STATUSES[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Internal helper: map a prepared statement row into a Policy struct. */
/* Column order MUST match the SELECT column order in each query.      */
/* ------------------------------------------------------------------ */
static void map_row_to_policy(sqlite3_stmt *stmt, Policy *out) {
    out->id          = sqlite3_column_int(stmt, 0);
    out->customer_id = sqlite3_column_int(stmt, 1);

    const char *col;

    col = (const char *)sqlite3_column_text(stmt, 2);
    snprintf(out->policy_type, sizeof(out->policy_type), "%s", col ? col : "");

    out->premium = sqlite3_column_double(stmt, 3);

    col = (const char *)sqlite3_column_text(stmt, 4);
    snprintf(out->start_date, sizeof(out->start_date), "%s", col ? col : "");

    col = (const char *)sqlite3_column_text(stmt, 5);
    snprintf(out->end_date, sizeof(out->end_date), "%s", col ? col : "");

    col = (const char *)sqlite3_column_text(stmt, 6);
    snprintf(out->status, sizeof(out->status), "%s", col ? col : "");
}

/* ------------------------------------------------------------------ */
/* policy_create()                                                      */
/*                                                                      */
/* Creates a new insurance policy and links it to an existing customer.*/
/*                                                                      */
/* Professional note: We verify the customer exists BEFORE issuing the */
/* INSERT. This gives a clean ERR_NOT_FOUND rather than an opaque      */
/* SQLITE_CONSTRAINT_FOREIGNKEY error to the caller.                   */
/* ------------------------------------------------------------------ */
error_t policy_create(const Policy *policy) {
    if (!policy) {
        LOG_ERROR("policy_create: NULL policy pointer.");
        return ERR_INVALID_ARG;
    }
    if (policy->customer_id <= 0) {
        LOG_ERROR("policy_create: invalid customer_id %d.", policy->customer_id);
        return ERR_INVALID_ARG;
    }
    if (policy->policy_type[0] == '\0') {
        LOG_ERROR("policy_create: policy_type is required.");
        return ERR_VALIDATION;
    }
    if (policy->premium < 0.0) {
        LOG_ERROR("policy_create: premium cannot be negative (got %.2f).", policy->premium);
        return ERR_VALIDATION;
    }
    if (policy->start_date[0] == '\0' || policy->end_date[0] == '\0') {
        LOG_ERROR("policy_create: start_date and end_date are required.");
        return ERR_VALIDATION;
    }
    /* Date comparison: ISO8601 strings (YYYY-MM-DD) compare correctly
     * as plain strings because their format is lexicographically ordered. */
    if (strcmp(policy->end_date, policy->start_date) <= 0) {
        LOG_ERROR("policy_create: end_date (%s) must be after start_date (%s).",
                  policy->end_date, policy->start_date);
        return ERR_VALIDATION;
    }
    if (!is_valid_policy_status(policy->status)) {
        LOG_ERROR("policy_create: invalid status '%s'. Must be ACTIVE, EXPIRED, or CANCELLED.",
                  policy->status);
        return ERR_VALIDATION;
    }

    sqlite3 *db = db_get_connection();
    if (!db) {
        LOG_ERROR("policy_create: No active database connection.");
        return ERR_SQLITE;
    }

    /* BLL pre-check: verify the customer exists before inserting */
    const char *check_sql = "SELECT 1 FROM customers WHERE id = ?;";
    sqlite3_stmt *check_stmt = NULL;
    if (sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("policy_create: customer check prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }
    sqlite3_bind_int(check_stmt, 1, policy->customer_id);
    int check_rc = sqlite3_step(check_stmt);
    sqlite3_finalize(check_stmt);

    if (check_rc != SQLITE_ROW) {
        LOG_ERROR("policy_create: customer_id=%d does not exist.", policy->customer_id);
        return ERR_NOT_FOUND;
    }

    /* Insert the policy */
    const char *sql =
        "INSERT INTO policies (customer_id, policy_type, premium, start_date, end_date, status) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("policy_create: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int   (stmt, 1, policy->customer_id);
    sqlite3_bind_text  (stmt, 2, policy->policy_type, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, policy->premium);
    sqlite3_bind_text  (stmt, 4, policy->start_date,  -1, SQLITE_STATIC);
    sqlite3_bind_text  (stmt, 5, policy->end_date,    -1, SQLITE_STATIC);
    sqlite3_bind_text  (stmt, 6, policy->status,      -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("policy_create: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    LOG_INFO("Policy created: type=%s customer_id=%d premium=%.2f status=%s",
             policy->policy_type, policy->customer_id, policy->premium, policy->status);
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* policy_get_by_id()                                                   */
/*                                                                      */
/* Retrieves a single policy by its primary key.                       */
/* ------------------------------------------------------------------ */
error_t policy_get_by_id(int id, Policy *out_policy) {
    if (id <= 0) {
        LOG_ERROR("policy_get_by_id: invalid id %d.", id);
        return ERR_INVALID_ARG;
    }
    if (!out_policy) {
        LOG_ERROR("policy_get_by_id: NULL output pointer.");
        return ERR_INVALID_ARG;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql =
        "SELECT id, customer_id, policy_type, premium, start_date, end_date, status "
        "FROM policies WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("policy_get_by_id: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        map_row_to_policy(stmt, out_policy);
        sqlite3_finalize(stmt);
        LOG_DEBUG("policy_get_by_id: found policy id=%d", id);
        return ERR_OK;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        LOG_WARN("policy_get_by_id: no policy found with id=%d", id);
        return ERR_NOT_FOUND;
    }

    LOG_ERROR("policy_get_by_id: step error: %s", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return ERR_SQLITE;
}

/* ------------------------------------------------------------------ */
/* policy_update_status()                                               */
/*                                                                      */
/* Updates only the status field of a policy.                          */
/* Status transitions should eventually be validated (e.g., you cannot */
/* reactivate a CANCELLED policy) — this is a future BLL enhancement. */
/* ------------------------------------------------------------------ */
error_t policy_update_status(int id, const char *status) {
    if (id <= 0) {
        LOG_ERROR("policy_update_status: invalid id %d.", id);
        return ERR_INVALID_ARG;
    }
    if (!status || !is_valid_policy_status(status)) {
        LOG_ERROR("policy_update_status: invalid status '%s'.", status ? status : "(null)");
        return ERR_VALIDATION;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql = "UPDATE policies SET status = ? WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("policy_update_status: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("policy_update_status: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }
    if (sqlite3_changes(db) == 0) {
        LOG_WARN("policy_update_status: no policy found with id=%d.", id);
        return ERR_NOT_FOUND;
    }

    LOG_INFO("Policy id=%d status updated to '%s'.", id, status);
    return ERR_OK;
}
