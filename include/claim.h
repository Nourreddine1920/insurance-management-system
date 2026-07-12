#ifndef CLAIM_H
#define CLAIM_H

#include "error.h"
#include "customer.h" // For MAX_DATE_LEN

#define MAX_DESC_LEN 255
#define MAX_CLAIM_STATUS_LEN 20

/**
 * @brief Representation of an Insurance Claim entity.
 */
typedef struct {
    int id;                                     /**< Claim unique identifier (PK) */
    int policy_id;                              /**< Associated Policy identifier (FK) */
    char description[MAX_DESC_LEN];             /**< Claim incident description */
    double amount;                              /**< Claimed payout amount (amount >= 0) */
    char claim_date[MAX_DATE_LEN];              /**< Claim incident date (ISO8601 string) */
    char status[MAX_CLAIM_STATUS_LEN];          /**< Claim status ('PENDING', 'APPROVED', etc.) */
} Claim;

/**
 * @brief Submits a new claim.
 */
error_t claim_create(const Claim *claim);

/**
 * @brief Retrieves a claim by its ID.
 */
error_t claim_get_by_id(int id, Claim *out_claim);

/**
 * @brief Updates status of a claim.
 */
error_t claim_update_status(int id, const char *status);

/**
 * @brief Calculates total amount of all claims for a given policy.
 */
error_t claim_get_total_amount(int policy_id, double *out_total);

#endif // CLAIM_H
