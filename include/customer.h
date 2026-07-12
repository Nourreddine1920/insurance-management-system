#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "error.h"

#define MAX_NAME_LEN 100
#define MAX_EMAIL_LEN 150
#define MAX_PHONE_LEN 50
#define MAX_DATE_LEN 30

/**
 * @brief Representation of a Customer entity.
 */
typedef struct {
    int id;                                 /**< Customer unique identifier (PK) */
    char first_name[MAX_NAME_LEN];          /**< Customer first name */
    char last_name[MAX_NAME_LEN];           /**< Customer last name */
    char email[MAX_EMAIL_LEN];              /**< Customer email (unique) */
    char phone[MAX_PHONE_LEN];              /**< Customer contact number */
    char created_at[MAX_DATE_LEN];          /**< Creation date (ISO8601 string) */
} Customer;

/**
 * @brief Inserts a new customer record.
 */
error_t customer_create(const Customer *customer);

/**
 * @brief Retrieves a customer record by ID.
 */
error_t customer_get_by_id(int id, Customer *out_customer);

/**
 * @brief Updates an existing customer record.
 */
error_t customer_update(const Customer *customer);

/**
 * @brief Deletes a customer record by ID.
 */
error_t customer_delete(int id);

#endif // CUSTOMER_H
