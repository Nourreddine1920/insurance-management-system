/*
 * main.c — Interactive console application for the insurance workflow.
 *
 * The app demonstrates the business flow in a more understandable way:
 *   1. Create a customer
 *   2. Create a policy for that customer
 *   3. Create a claim for that policy
 *
 * It also includes a demo mode that runs the earlier integration scenario.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "database.h"
#include "customer.h"
#include "policy.h"
#include "claim.h"
#include "error.h"

static void print_separator(const char *title) {
    printf("\n----------------------------------------\n");
    if (title) printf("  %s\n", title);
    printf("----------------------------------------\n");
}

static void print_customer(const Customer *c) {
    printf("  [Customer #%d] %s %s | %s | %s\n",
           c->id, c->first_name, c->last_name, c->email,
           c->phone[0] ? c->phone : "(no phone)");
}

static void print_policy(const Policy *p) {
    printf("  [Policy #%d] customer=%d | type=%s | premium=%.2f | %s→%s | status=%s\n",
           p->id, p->customer_id, p->policy_type,
           p->premium, p->start_date, p->end_date, p->status);
}

static void print_claim(const Claim *c) {
    printf("  [Claim #%d] policy=%d | %.2f | date=%s | status=%s\n  desc: %s\n",
           c->id, c->policy_id, c->amount, c->claim_date, c->status, c->description);
}

static void print_banner(void) {
    printf("\n========================================\n");
    printf("  Insurance Management System\n");
    printf("  Console workflow demo\n");
    printf("========================================\n");
    printf("This app shows how a customer, policy, and claim are linked in the system.\n");
    printf("Think of it as: Customer -> Policy -> Claim\n");
}

static void print_menu(void) {
    printf("\nChoose an action:\n");
    printf("  1) Run the demo scenario\n");
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
    buffer[strcspn(buffer, "\r\n")] = '\0';
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

static error_t ensure_database_ready(void) {
    error_t err = db_initialize("insurance.db");
    if (err != ERR_OK) {
        printf("Failed to initialize database.\n");
    }
    return err;
}

static void run_demo_scenario(void) {
    print_separator("Demo scenario");
    printf("This demo creates a sample customer, policy, and claim in sequence.\n");

    remove("insurance.db");
    error_t err = ensure_database_ready();
    if (err != ERR_OK) {
        return;
    }

    Customer c1;
    memset(&c1, 0, sizeof(c1));
    snprintf(c1.first_name, sizeof(c1.first_name), "Alice");
    snprintf(c1.last_name,  sizeof(c1.last_name),  "Martin");
    snprintf(c1.email,      sizeof(c1.email),      "alice.martin@example.com");
    snprintf(c1.phone,      sizeof(c1.phone),      "+33-6-00-11-22-33");
    printf("\n[Step 1] Creating customer...\n");
    if (customer_create(&c1) == ERR_OK) {
        print_customer(&c1);
    }

    Policy p1;
    memset(&p1, 0, sizeof(p1));
    p1.customer_id = 1;
    snprintf(p1.policy_type, sizeof(p1.policy_type), "HEALTH");
    p1.premium = 1200.00;
    snprintf(p1.start_date, sizeof(p1.start_date), "2026-01-01");
    snprintf(p1.end_date,   sizeof(p1.end_date),   "2026-12-31");
    snprintf(p1.status,     sizeof(p1.status),     "ACTIVE");
    printf("\n[Step 2] Creating policy for that customer...\n");
    if (policy_create(&p1) == ERR_OK) {
        print_policy(&p1);
    }

    Claim cl1;
    memset(&cl1, 0, sizeof(cl1));
    cl1.policy_id = 1;
    snprintf(cl1.description, sizeof(cl1.description), "Hospital stay - appendix surgery");
    cl1.amount = 3500.00;
    snprintf(cl1.claim_date, sizeof(cl1.claim_date), "2026-04-15");
    snprintf(cl1.status,     sizeof(cl1.status),     "PENDING");
    printf("\n[Step 3] Creating claim for that policy...\n");
    if (claim_create(&cl1) == ERR_OK) {
        print_claim(&cl1);
    }

    printf("\nDemo complete. The workflow is now stored in the database.\n");
}

static void create_customer_interactive(void) {
    print_separator("Create Customer");
    Customer customer;
    memset(&customer, 0, sizeof(customer));
    read_text("First name: ", customer.first_name, sizeof(customer.first_name));
    read_text("Last name: ", customer.last_name, sizeof(customer.last_name));
    read_text("Email: ", customer.email, sizeof(customer.email));
    read_text("Phone (optional): ", customer.phone, sizeof(customer.phone));

    if (ensure_database_ready() != ERR_OK) {
        return;
    }

    error_t err = customer_create(&customer);
    if (err == ERR_OK) {
        printf("Customer created successfully.\n");
        print_customer(&customer);
    } else {
        printf("Customer creation failed: %s\n", error_to_string(err));
    }
}

static void create_policy_interactive(void) {
    print_separator("Create Policy");
    Policy policy;
    memset(&policy, 0, sizeof(policy));
    policy.customer_id = read_int("Customer ID: ");
    read_text("Policy type: ", policy.policy_type, sizeof(policy.policy_type));
    policy.premium = read_double("Premium: ");
    read_text("Start date (YYYY-MM-DD): ", policy.start_date, sizeof(policy.start_date));
    read_text("End date (YYYY-MM-DD): ", policy.end_date, sizeof(policy.end_date));
    read_text("Status (ACTIVE/EXPIRED/CANCELLED): ", policy.status, sizeof(policy.status));

    if (ensure_database_ready() != ERR_OK) {
        return;
    }

    error_t err = policy_create(&policy);
    if (err == ERR_OK) {
        printf("Policy created successfully.\n");
        print_policy(&policy);
    } else {
        printf("Policy creation failed: %s\n", error_to_string(err));
    }
}

static void create_claim_interactive(void) {
    print_separator("Create Claim");
    Claim claim;
    memset(&claim, 0, sizeof(claim));
    claim.policy_id = read_int("Policy ID: ");
    read_text("Description: ", claim.description, sizeof(claim.description));
    claim.amount = read_double("Amount: ");
    read_text("Claim date (YYYY-MM-DD): ", claim.claim_date, sizeof(claim.claim_date));
    read_text("Status (PENDING/APPROVED/REJECTED): ", claim.status, sizeof(claim.status));

    if (ensure_database_ready() != ERR_OK) {
        return;
    }

    error_t err = claim_create(&claim);
    if (err == ERR_OK) {
        printf("Claim created successfully.\n");
        print_claim(&claim);
    } else {
        printf("Claim creation failed: %s\n", error_to_string(err));
    }
}

static void reset_database(void) {
    print_separator("Reset database");
    remove("insurance.db");
    error_t err = ensure_database_ready();
    if (err == ERR_OK) {
        printf("Database was reset and recreated successfully.\n");
    }
}

int main(void) {
    logger_init("app.log");
    LOG_INFO("Application started in interactive mode");

    if (ensure_database_ready() != ERR_OK) {
        logger_close();
        return 1;
    }

    while (1) {
        print_banner();
        print_menu();
        int choice = read_int("Enter your choice: ");

        switch (choice) {
            case 1:
                run_demo_scenario();
                break;
            case 2:
                create_customer_interactive();
                break;
            case 3:
                create_policy_interactive();
                break;
            case 4:
                create_claim_interactive();
                break;
            case 5:
                reset_database();
                break;
            case 0:
                printf("\nGoodbye.\n");
                db_close();
                logger_close();
                return 0;
            default:
                printf("Unknown option. Please choose again.\n");
                break;
        }

        if (choice != 0) {
            wait_for_enter();
        }
    }
}
