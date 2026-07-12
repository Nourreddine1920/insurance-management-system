#include "cli.h"
#include "logger.h"
#include "database.h"
#include "customer.h"
#include "policy.h"
#include "claim.h"
#include "error.h"
#include "validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_separator(const char *title) {
    printf("\n----------------------------------------\n");
    if (title) {
        printf("  %s\n", title);
    }
    printf("----------------------------------------\n");
}

static void print_banner(void) {
    printf("\n========================================\n");
    printf("  Insurance Management System\n");
    printf("  Professional CLI Dashboard\n");
    printf("========================================\n");
}

static void print_menu(void) {
    printf("\nChoose an action:\n");
    printf("  1) Run demo scenario\n");
    printf("  2) Create a customer\n");
    printf("  3) Create a policy\n");
    printf("  4) Create a claim\n");
    printf("  5) Reset database\n");
    printf("  0) Exit\n");
}

static void wait_for_enter(void) {
    printf("\nPress Enter to continue...\n");
    getchar();
}

static void read_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\r\n") ] = '\0';
}

static int read_int(const char *prompt) {
    char buffer[64];
    int value = 0;
    while (1) {
        printf("%s", prompt);
        read_line(buffer, sizeof(buffer));
        if (sscanf(buffer, "%d", &value) == 1) {
            return value;
        }
        printf("  Please enter a valid integer.\n");
    }
}

static double read_double(const char *prompt) {
    char buffer[64];
    double value = 0.0;
    while (1) {
        printf("%s", prompt);
        read_line(buffer, sizeof(buffer));
        if (sscanf(buffer, "%lf", &value) == 1) {
            return value;
        }
        printf("  Please enter a valid number.\n");
    }
}

static void read_text(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    read_line(buffer, size);
}

static int is_valid_enum(const char *value, const char * const valid_values[]) {
    if (!value || value[0] == '\0') {
        return 0;
    }
    for (int i = 0; valid_values[i] != NULL; ++i) {
        if (strcmp(value, valid_values[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int validate_optional_phone(const char *phone) {
    if (!phone || phone[0] == '\0') {
        return 1;
    }
    return validate_phone(phone);
}

static int validate_string_field(const char *value, const char *field_name, size_t max_length) {
    if (!validate_non_empty_string(value, max_length)) {
        printf("Invalid %s. Please enter a non-empty value shorter than %zu characters.\n", field_name, max_length);
        return 0;
    }
    return 1;
}

static error_t ensure_database_ready(const char *db_path) {
    error_t err = db_initialize(db_path);
    if (err != ERR_OK) {
        printf("Failed to initialize database.\n");
        LOG_ERROR("Failed to initialize database at %s", db_path);
    }
    return err;
}

static void print_customer(const Customer *c) {
    printf("  [Customer #%d] %s %s | %s | %s\n",
           c->id, c->first_name, c->last_name, c->email,
           c->phone[0] ? c->phone : "(no phone)");
}

static void print_policy(const Policy *p) {
    printf("  [Policy #%d] customer=%d | type=%s | premium=%.2f | %s → %s | status=%s\n",
           p->id, p->customer_id, p->policy_type, p->premium,
           p->start_date, p->end_date, p->status);
}

static void print_claim(const Claim *c) {
    printf("  [Claim #%d] policy=%d | %.2f | date=%s | status=%s\n  desc: %s\n",
           c->id, c->policy_id, c->amount, c->claim_date, c->status,
           c->description);
}

static void run_demo_scenario(const char *db_path) {
    print_separator("Demo scenario");
    printf("This demo creates a sample customer, policy, and claim in sequence.\n");

    db_close();
    remove(db_path);
    if (ensure_database_ready(db_path) != ERR_OK) {
        return;
    }

    Customer customer = {0};
    snprintf(customer.first_name, sizeof(customer.first_name), "Alice");
    snprintf(customer.last_name,  sizeof(customer.last_name),  "Martin");
    snprintf(customer.email,      sizeof(customer.email),      "alice.martin@example.com");
    snprintf(customer.phone,      sizeof(customer.phone),      "+33-6-00-11-22-33");

    printf("\n[Step 1] Creating customer...\n");
    if (customer_create(&customer) == ERR_OK) {
        print_customer(&customer);
    }

    Policy policy = {0};
    policy.customer_id = 1;
    snprintf(policy.policy_type, sizeof(policy.policy_type), "HEALTH");
    policy.premium = 1200.00;
    snprintf(policy.start_date, sizeof(policy.start_date), "2026-01-01");
    snprintf(policy.end_date,   sizeof(policy.end_date),   "2026-12-31");
    snprintf(policy.status,     sizeof(policy.status),     "ACTIVE");

    printf("\n[Step 2] Creating policy for that customer...\n");
    if (policy_create(&policy) == ERR_OK) {
        print_policy(&policy);
    }

    Claim claim = {0};
    claim.policy_id = 1;
    snprintf(claim.description, sizeof(claim.description), "Hospital stay - appendix surgery");
    claim.amount = 3500.00;
    snprintf(claim.claim_date, sizeof(claim.claim_date), "2026-04-15");
    snprintf(claim.status,     sizeof(claim.status),     "PENDING");

    printf("\n[Step 3] Creating claim for that policy...\n");
    if (claim_create(&claim) == ERR_OK) {
        print_claim(&claim);
    }

    printf("\nDemo complete. The workflow is now stored in the database.\n");
}

static void create_customer_interactive(void) {
    print_separator("Create Customer");

    Customer customer = {0};
    do {
        read_text("First name: ", customer.first_name, sizeof(customer.first_name));
    } while (!validate_string_field(customer.first_name, "first name", sizeof(customer.first_name)));

    do {
        read_text("Last name: ", customer.last_name, sizeof(customer.last_name));
    } while (!validate_string_field(customer.last_name, "last name", sizeof(customer.last_name)));

    do {
        read_text("Email: ", customer.email, sizeof(customer.email));
        if (!validate_email(customer.email)) {
            printf("Invalid email format. Please enter a valid email address.\n");
        }
    } while (!validate_email(customer.email));

    do {
        read_text("Phone (optional): ", customer.phone, sizeof(customer.phone));
        if (!validate_optional_phone(customer.phone)) {
            printf("Invalid phone number. Use digits, spaces, '+', '-', '(', or ')'.\n");
        }
    } while (!validate_optional_phone(customer.phone));

    error_t err = customer_create(&customer);
    if (err == ERR_OK) {
        printf("Customer created successfully.\n");
        print_customer(&customer);
    } else {
        printf("Customer creation failed: %s\n", error_to_string(err));
    }
}

static void create_policy_interactive(void) {
    static const char * const VALID_POLICY_STATUSES[] = {"ACTIVE", "EXPIRED", "CANCELLED", NULL};

    print_separator("Create Policy");

    Policy policy = {0};
    while (policy.customer_id <= 0) {
        policy.customer_id = read_int("Customer ID: ");
        if (policy.customer_id <= 0) {
            printf("Customer ID must be a positive integer.\n");
        }
    }

    do {
        read_text("Policy type: ", policy.policy_type, sizeof(policy.policy_type));
    } while (!validate_string_field(policy.policy_type, "policy type", sizeof(policy.policy_type)));

    do {
        policy.premium = read_double("Premium: ");
        if (!validate_non_negative_double(policy.premium)) {
            printf("Premium must be a non-negative number.\n");
        }
    } while (!validate_non_negative_double(policy.premium));

    do {
        read_text("Start date (YYYY-MM-DD): ", policy.start_date, sizeof(policy.start_date));
        if (!validate_iso_date(policy.start_date)) {
            printf("Invalid start date format. Use YYYY-MM-DD.\n");
        }
    } while (!validate_iso_date(policy.start_date));

    do {
        read_text("End date (YYYY-MM-DD): ", policy.end_date, sizeof(policy.end_date));
        if (!validate_iso_date(policy.end_date)) {
            printf("Invalid end date format. Use YYYY-MM-DD.\n");
        }
    } while (!validate_iso_date(policy.end_date));

    while (strcmp(policy.end_date, policy.start_date) <= 0) {
        printf("End date must be after start date.\n");
        read_text("End date (YYYY-MM-DD): ", policy.end_date, sizeof(policy.end_date));
    }

    do {
        read_text("Status (ACTIVE/EXPIRED/CANCELLED): ", policy.status, sizeof(policy.status));
        if (!is_valid_enum(policy.status, VALID_POLICY_STATUSES)) {
            printf("Invalid policy status. Choose ACTIVE, EXPIRED, or CANCELLED.\n");
        }
    } while (!is_valid_enum(policy.status, VALID_POLICY_STATUSES));

    error_t err = policy_create(&policy);
    if (err == ERR_OK) {
        printf("Policy created successfully.\n");
        print_policy(&policy);
    } else {
        printf("Policy creation failed: %s\n", error_to_string(err));
    }
}

static void create_claim_interactive(void) {
    static const char * const VALID_CLAIM_STATUSES[] = {"PENDING", "APPROVED", "REJECTED", NULL};

    print_separator("Create Claim");

    Claim claim = {0};
    while (claim.policy_id <= 0) {
        claim.policy_id = read_int("Policy ID: ");
        if (claim.policy_id <= 0) {
            printf("Policy ID must be a positive integer.\n");
        }
    }

    do {
        read_text("Description: ", claim.description, sizeof(claim.description));
    } while (!validate_string_field(claim.description, "description", sizeof(claim.description)));

    do {
        claim.amount = read_double("Amount: ");
        if (!validate_non_negative_double(claim.amount)) {
            printf("Claim amount must be a non-negative number.\n");
        }
    } while (!validate_non_negative_double(claim.amount));

    do {
        read_text("Claim date (YYYY-MM-DD): ", claim.claim_date, sizeof(claim.claim_date));
        if (!validate_iso_date(claim.claim_date)) {
            printf("Invalid claim date format. Use YYYY-MM-DD.\n");
        }
    } while (!validate_iso_date(claim.claim_date));

    do {
        read_text("Status (PENDING/APPROVED/REJECTED): ", claim.status, sizeof(claim.status));
        if (!is_valid_enum(claim.status, VALID_CLAIM_STATUSES)) {
            printf("Invalid claim status. Choose PENDING, APPROVED, or REJECTED.\n");
        }
    } while (!is_valid_enum(claim.status, VALID_CLAIM_STATUSES));

    error_t err = claim_create(&claim);
    if (err == ERR_OK) {
        printf("Claim created successfully.\n");
        print_claim(&claim);
    } else {
        printf("Claim creation failed: %s\n", error_to_string(err));
    }
}

static void reset_database(const char *db_path) {
    print_separator("Reset database");
    db_close();
    remove(db_path);
    if (ensure_database_ready(db_path) == ERR_OK) {
        printf("Database was reset and recreated successfully.\n");
    }
}

int cli_run(const char *db_path) {
    const char *database_path = db_path ? db_path : "insurance.db";
    if (ensure_database_ready(database_path) != ERR_OK) {
        return 1;
    }

    int should_exit = 0;
    while (!should_exit) {
        print_banner();
        print_menu();

        int choice = read_int("Enter your choice: ");
        switch (choice) {
            case 1:
                run_demo_scenario(database_path);
                wait_for_enter();
                break;
            case 2:
                create_customer_interactive();
                wait_for_enter();
                break;
            case 3:
                create_policy_interactive();
                wait_for_enter();
                break;
            case 4:
                create_claim_interactive();
                wait_for_enter();
                break;
            case 5:
                reset_database(database_path);
                wait_for_enter();
                break;
            case 0:
                should_exit = 1;
                break;
            default:
                printf("Invalid choice. Please select a valid option.\n");
                wait_for_enter();
                break;
        }
    }

    db_close();
    return 0;
}
