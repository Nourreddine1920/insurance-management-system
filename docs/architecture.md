# Architecture Design Document

## 1. Overview
The Insurance Policy Management System is a professional command-line application built in C11. It uses SQLite3 for persistent storage and is structured to showcase high-quality software engineering practices in a modular, low-level language.

## 2. Layered Architecture

To achieve clean separation of concerns and high testability, the system is divided into four main layers:

```
+-------------------------------------------------------------+
|                     Presentation Layer                      |
|              (CLI Menus, Input Validation, UI)              |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                    Business Logic Layer                     |
|           (Customer, Policy, & Claim Domain Rules)          |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                     Data Access Layer                       |
|           (SQLite Connection, Queries, Transactions)        |
+-------------------------------------------------------------+

===============================================================
       Utility Layer (Logging, Error Handling, Common Helpers)
===============================================================
```

### 2.1 Presentation Layer
* **Files**: `src/main.c`, `src/menu.c`
* **Rules**:
  * Must never construct or execute SQL queries.
  * Must never validate business rules (e.g., policy limits or claim dates).
  * Is responsible for reading from standard input safely (no `gets()` or unsafe `scanf()`).
  * Formats tabular data and handles user interactions.

### 2.2 Business Logic Layer (BLL)
* **Files**: `src/customer.c`, `src/policy.c`, `src/claim.c` (functions implementing rules)
* **Rules**:
  * Evaluates system constraints (e.g., checking if a customer exists before assigning a policy).
  * Handles logical state transitions (e.g., checking if a claim is valid for the policy status).
  * Returns structured errors if logic checks fail.

### 2.3 Data Access Layer (DAL)
* **Files**: `src/database.c` (and DB helpers within individual modules)
* **Rules**:
  * Directly references the `sqlite3` library.
  * Uses prepared statements to prevent SQL injection and speed up queries.
  * Maps SQLite rows to C structures.
  * Exposes clean transaction APIs (`db_begin_transaction()`, `db_commit()`, `db_rollback()`).

### 2.4 Utility Layer
* **Files**: `src/logger.c`, `src/error.c`
* **Rules**:
  * Cross-cutting concerns available to all layers.
  * Manages global application state (like log targets and configuration parameters).

---

## 3. Data Structures Flow

Data flows across layers using plain old C structures (structs). 

1. **Entities** (e.g., `Customer`, `Policy`, `Claim`) are defined in public headers (`include/*.h`).
2. **Creation DTOs** (Data Transfer Objects) or simply parameter lists are passed from the UI to the Business Logic Layer.
3. The Business Logic Layer validates the parameters, then passes them to the DAL.
4. The DAL binds them to prepared SQL queries, executes, and returns structured result codes.

---

## 4. Error Propagation System
Instead of printing errors to `stdout` from deep inside the DAL or BLL, we use a structured error return pattern:
* Every function returns an `error_t` code (an enum).
* If data is returned, it is passed via pointer parameters (out-parameters).
* Example:
  ```c
  error_t customer_get_by_id(int id, Customer *out_customer);
  ```
