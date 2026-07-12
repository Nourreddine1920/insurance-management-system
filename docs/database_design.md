# Database Design Document

## 1. Relational Schema
We use SQLite3 to persist the data of our Insurance Management System. The schema consists of three core tables representing Customers, Policies, and Claims, forming a cascading relationship structure.

```
+---------------+        +--------------+        +------------+
|   customers   | 1    * |   policies   | 1    * |   claims   |
|---------------|------->|--------------|------->|------------|
| id (PK)       |        | id (PK)      |        | id (PK)    |
| first_name    |        | customer_id  |        | policy_id  |
| last_name     |        | policy_type  |        | description|
| email (UQ)    |        | premium      |        | amount     |
| phone         |        | start_date   |        | claim_date |
| created_at    |        | end_date     |        | status     |
+---------------+        | status       |        +------------+
                         +--------------+
```

---

## 2. Table Definitions

### 2.1 Table: `customers`
Stores demographic and contact details of policyholders.

| Field | Type | Constraints | Description |
|---|---|---|---|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Unique internal identifier. |
| `first_name` | `TEXT` | `NOT NULL` | Customer's first name. |
| `last_name` | `TEXT` | `NOT NULL` | Customer's last name. |
| `email` | `TEXT` | `NOT NULL UNIQUE` | Customer's primary contact email. Checked at runtime. |
| `phone` | `TEXT` | | Customer's contact phone number. |
| `created_at` | `DATETIME` | `DEFAULT CURRENT_TIMESTAMP` | System record creation timestamp. |

### 2.2 Table: `policies`
Represents insurance policies issued to customers.

| Field | Type | Constraints | Description |
|---|---|---|---|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Unique policy identifier. |
| `customer_id` | `INTEGER` | `NOT NULL`, `FOREIGN KEY` | Reference to `customers.id`. |
| `policy_type` | `TEXT` | `NOT NULL` | e.g. `HEALTH`, `AUTO`, `HOME`, `LIFE`. |
| `premium` | `REAL` | `NOT NULL`, `CHECK (premium >= 0)` | Annual or monthly premium cost. |
| `start_date` | `TEXT` | `NOT NULL` | ISO8601 YYYY-MM-DD start date. |
| `end_date` | `TEXT` | `NOT NULL` | ISO8601 YYYY-MM-DD end date. |
| `status` | `TEXT` | `NOT NULL`, `CHECK (status IN ('ACTIVE', 'EXPIRED', 'CANCELLED'))` | Current lifecycle state. |

### 2.3 Table: `claims`
Tracks insurance claims filed against specific policies.

| Field | Type | Constraints | Description |
|---|---|---|---|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Unique claim identifier. |
| `policy_id` | `INTEGER` | `NOT NULL`, `FOREIGN KEY` | Reference to `policies.id`. |
| `description` | `TEXT` | `NOT NULL` | Description of loss or incident. |
| `amount` | `REAL` | `NOT NULL`, `CHECK (amount >= 0)` | Requested payout amount. |
| `claim_date` | `TEXT` | `NOT NULL` | ISO8601 YYYY-MM-DD date of claim. |
| `status` | `TEXT` | `NOT NULL`, `CHECK (status IN ('PENDING', 'APPROVED', 'REJECTED'))` | Claims processing status. |

---

## 3. Database Normalization (Third Normal Form - 3NF)

The schema satisfies Third Normal Form (3NF) requirements:
1. **First Normal Form (1NF)**: All column values are atomic (e.g., names are split into `first_name` and `last_name`, dates are single ISO8601 strings, and list values are not stored inside a single column).
2. **Second Normal Form (2NF)**: The database is in 1NF, and all non-key columns depend entirely on the primary key of their respective tables. For instance, customer contact details depend on `customers.id`, not `policies.id`.
3. **Third Normal Form (3NF)**: The database is in 2NF, and there are no transitive dependencies. A change to a customer's phone number does not affect policy details, and policy status changes do not affect customer attributes.

---

## 4. Referential Integrity & Performance Indexes

### 4.1 Foreign Keys & Deletion Rules
* `policies.customer_id` references `customers.id` using `ON DELETE RESTRICT`. This prevents a customer record from being deleted while they still hold policies, avoiding orphaned policy records.
* `claims.policy_id` references `policies.id` using `ON DELETE RESTRICT`. A policy cannot be deleted if claims have been registered against it.

### 4.2 Indexes
SQLite creates implicit indexes for `PRIMARY KEY` and `UNIQUE` constraints (e.g., `customers.email`). However, foreign key columns are *not* indexed by default. To prevent expensive table scans during common joins:
* `idx_policies_customer_id` is created on `policies(customer_id)`.
* `idx_claims_policy_id` is created on `claims(policy_id)`.
* `idx_customers_last_first` is created on `customers(last_name, first_name)` to accelerate sorting and search queries by name.
