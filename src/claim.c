/*
 * claim.c — Business Logic + Data Access for Insurance Claim entities.
 *
 * Business Rules enforced here (BLL):
 *   1. A claim MUST reference a valid, existing policy (policy_id > 0).
 *   2. A claim can only be filed against an ACTIVE policy.
 *      (You cannot file new claims against EXPIRED or CANCELLED policies.)
 *   3. amount must be >= 0.
 *   4. description must not be empty.
 *   5. status must be one of: PENDING, APPROVED, REJECTED.
 *
 * SQL Features demonstrated:
 *   - Aggregation: SUM(amount) with NULL handling
 *   - INNER JOIN: claims joined to policies for reporting context
 *   - Scalar subquery for policy status validation before insert
 */

#include "claim.h"
#include "policy.h"   /* for MAX_STATUS_LEN */
#include "database.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Valid status strings for claims                                      */
/* ------------------------------------------------------------------ */
static const char * const VALID_CLAIM_STATUSES[] = {
    "PENDING", "APPROVED", "REJECTED", NULL
};

static int is_valid_claim_status(const char *status) {
    for (int i = 0; VALID_CLAIM_STATUSES[i] != NULL; i++) {
        if (strcmp(status, VALID_CLAIM_STATUSES[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Internal helper: map a prepared statement row into a Claim struct.  */
/* ------------------------------------------------------------------ */
static void map_row_to_claim(sqlite3_stmt *stmt, Claim *out) {
    out->id        = sqlite3_column_int(stmt, 0);
    out->policy_id = sqlite3_column_int(stmt, 1);

    const char *col;

    col = (const char *)sqlite3_column_text(stmt, 2);
    snprintf(out->description, sizeof(out->description), "%s", col ? col : "");

    out->amount = sqlite3_column_double(stmt, 3);

    col = (const char *)sqlite3_column_text(stmt, 4);
    snprintf(out->claim_date, sizeof(out->claim_date), "%s", col ? col : "");

    col = (const char *)sqlite3_column_text(stmt, 5);
    snprintf(out->status, sizeof(out->status), "%s", col ? col : "");
}

/* ------------------------------------------------------------------ */
/* claim_create()                                                       */
/*                                                                      */
/* Submits a new insurance claim.                                      */
/*                                                                      */
/* Key business rule: claims can only be filed against ACTIVE policies.*/
/* We query the policy's status BEFORE inserting the claim, and return */
/* ERR_VALIDATION if the policy is EXPIRED or CANCELLED. This is pure  */
/* BLL — the database schema cannot enforce this rule alone.           */
/* ------------------------------------------------------------------ */
error_t claim_create(const Claim *claim) {
    if (!claim) {
        LOG_ERROR("claim_create: NULL claim pointer.");
        return ERR_INVALID_ARG;
    }
    if (claim->policy_id <= 0) {
        LOG_ERROR("claim_create: invalid policy_id %d.", claim->policy_id);
        return ERR_INVALID_ARG;
    }
    if (claim->description[0] == '\0') {
        LOG_ERROR("claim_create: description is required.");
        return ERR_VALIDATION;
    }
    if (claim->amount < 0.0) {
        LOG_ERROR("claim_create: amount cannot be negative (got %.2f).", claim->amount);
        return ERR_VALIDATION;
    }
    if (claim->claim_date[0] == '\0') {
        LOG_ERROR("claim_create: claim_date is required.");
        return ERR_VALIDATION;
    }
    if (!is_valid_claim_status(claim->status)) {
        LOG_ERROR("claim_create: invalid status '%s'. Must be PENDING, APPROVED, or REJECTED.",
                  claim->status);
        return ERR_VALIDATION;
    }

    sqlite3 *db = db_get_connection();
    if (!db) {
        LOG_ERROR("claim_create: No active database connection.");
        return ERR_SQLITE;
    }

    /* BLL Rule: Verify the policy exists AND is ACTIVE.
     * We SELECT status (not just 1) so we can give a precise error message. */
    const char *check_sql = "SELECT status FROM policies WHERE id = ?;";
    sqlite3_stmt *check_stmt = NULL;
    if (sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("claim_create: policy status check prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int(check_stmt, 1, claim->policy_id);
    int check_rc = sqlite3_step(check_stmt);

    if (check_rc != SQLITE_ROW) {
        sqlite3_finalize(check_stmt);
        LOG_ERROR("claim_create: policy_id=%d does not exist.", claim->policy_id);
        return ERR_NOT_FOUND;
    }

    /* Read the policy status. We must copy it before finalize() frees it. */
    const char *policy_status_raw = (const char *)sqlite3_column_text(check_stmt, 0);
    char policy_status[MAX_STATUS_LEN];
    snprintf(policy_status, sizeof(policy_status), "%s",
             policy_status_raw ? policy_status_raw : "");
    sqlite3_finalize(check_stmt);

    if (strcmp(policy_status, "ACTIVE") != 0) {
        LOG_ERROR("claim_create: cannot file claim against policy_id=%d (status=%s, must be ACTIVE).",
                  claim->policy_id, policy_status);
        return ERR_VALIDATION;
    }

    /* Insert the claim */
    const char *sql =
        "INSERT INTO claims (policy_id, description, amount, claim_date, status) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("claim_create: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int   (stmt, 1, claim->policy_id);
    sqlite3_bind_text  (stmt, 2, claim->description, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, claim->amount);
    sqlite3_bind_text  (stmt, 4, claim->claim_date,  -1, SQLITE_STATIC);
    sqlite3_bind_text  (stmt, 5, claim->status,      -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("claim_create: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    LOG_INFO("Claim created: policy_id=%d amount=%.2f status=%s",
             claim->policy_id, claim->amount, claim->status);
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* claim_get_by_id()                                                    */
/* ------------------------------------------------------------------ */
error_t claim_get_by_id(int id, Claim *out_claim) {
    if (id <= 0) {
        LOG_ERROR("claim_get_by_id: invalid id %d.", id);
        return ERR_INVALID_ARG;
    }
    if (!out_claim) {
        LOG_ERROR("claim_get_by_id: NULL output pointer.");
        return ERR_INVALID_ARG;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql =
        "SELECT id, policy_id, description, amount, claim_date, status "
        "FROM claims WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("claim_get_by_id: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        map_row_to_claim(stmt, out_claim);
        sqlite3_finalize(stmt);
        LOG_DEBUG("claim_get_by_id: found claim id=%d", id);
        return ERR_OK;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        LOG_WARN("claim_get_by_id: no claim found with id=%d", id);
        return ERR_NOT_FOUND;
    }

    LOG_ERROR("claim_get_by_id: step error: %s", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return ERR_SQLITE;
}

/* ------------------------------------------------------------------ */
/* claim_update_status()                                                */
/*                                                                      */
/* Transitions a claim to APPROVED or REJECTED.                        */
/* ------------------------------------------------------------------ */
error_t claim_update_status(int id, const char *status) {
    if (id <= 0) {
        LOG_ERROR("claim_update_status: invalid id %d.", id);
        return ERR_INVALID_ARG;
    }
    if (!status || !is_valid_claim_status(status)) {
        LOG_ERROR("claim_update_status: invalid status '%s'.", status ? status : "(null)");
        return ERR_VALIDATION;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql = "UPDATE claims SET status = ? WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("claim_update_status: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("claim_update_status: step failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }
    if (sqlite3_changes(db) == 0) {
        LOG_WARN("claim_update_status: no claim found with id=%d.", id);
        return ERR_NOT_FOUND;
    }

    LOG_INFO("Claim id=%d status updated to '%s'.", id, status);
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* claim_get_total_amount()                                             */
/*                                                                      */
/* Uses SQL aggregation: SELECT SUM(amount) to calculate the total     */
/* payout amount for all claims against a given policy.                */
/*                                                                      */
/* Important: SUM() on an empty set returns NULL in SQL — not 0.       */
/* We check for SQLITE_NULL type and default to 0.0 in that case.     */
/* This is a subtle but common bug in database programming.            */
/* ------------------------------------------------------------------ */
error_t claim_get_total_amount(int policy_id, double *out_total) {
    if (policy_id <= 0) {
        LOG_ERROR("claim_get_total_amount: invalid policy_id %d.", policy_id);
        return ERR_INVALID_ARG;
    }
    if (!out_total) {
        LOG_ERROR("claim_get_total_amount: NULL output pointer.");
        return ERR_INVALID_ARG;
    }

    sqlite3 *db = db_get_connection();
    if (!db) { return ERR_SQLITE; }

    const char *sql =
        "SELECT SUM(amount) FROM claims WHERE policy_id = ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("claim_get_total_amount: prepare failed: %s", sqlite3_errmsg(db));
        return ERR_SQLITE;
    }

    sqlite3_bind_int(stmt, 1, policy_id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        /* Handle the NULL case: SUM on empty set = SQL NULL, not 0 */
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            *out_total = 0.0;
        } else {
            *out_total = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
        LOG_DEBUG("claim_get_total_amount: policy_id=%d total=%.2f", policy_id, *out_total);
        return ERR_OK;
    }

    LOG_ERROR("claim_get_total_amount: step error: %s", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return ERR_SQLITE;
}
