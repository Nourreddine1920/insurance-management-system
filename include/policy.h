#ifndef POLICY_H
#define POLICY_H

#include "error.h"
#include "customer.h"  // Needed for MAX_DATE_LEN definition

#define MAX_TYPE_LEN 50
#define MAX_STATUS_LEN 20

/**
 * @brief Representation of an Insurance Policy entity.
 */
typedef struct {
    int id;                         /**< Policy unique identifier (PK) */
    int customer_id;                /**< Associated Customer identifier (FK) */
    char policy_type[MAX_TYPE_LEN]; /**< Type of policy (e.g. 'HEALTH', 'AUTO') */
    double premium;                 /**< Policy premium price (amount >= 0) */
    char start_date[MAX_DATE_LEN];  /**< Policy start date (ISO8601 string) */
    char end_date[MAX_DATE_LEN];    /**< Policy end date (ISO8601 string) */
    char status[MAX_STATUS_LEN];    /**< Policy status ('ACTIVE', 'EXPIRED', etc.) */
} Policy;

/**
 * @brief Creates and assigns a new policy.
 */
error_t policy_create(const Policy* policy);

/**
 * @brief Retrieves a policy by its ID.
 */
error_t policy_get_by_id(int id, Policy* out_policy);

/**
 * @brief Updates status of a policy.
 */
error_t policy_update_status(int id, const char* status);

#endif  // POLICY_H
