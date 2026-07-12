-- Database schema for Insurance Policy Management System
-- Target: SQLite3

-- Enable foreign keys check (SQLite requires this per-session, but good to document here)
PRAGMA foreign_keys = ON;

-- -------------------------------------------------------------
-- Table: customers
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS customers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    first_name TEXT NOT NULL,
    last_name TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    phone TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Index on email for fast lookups
CREATE UNIQUE INDEX IF NOT EXISTS idx_customers_email ON customers(email);
-- Composite index for name searches
CREATE INDEX IF NOT EXISTS idx_customers_name ON customers(last_name, first_name);

-- -------------------------------------------------------------
-- Table: policies
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS policies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    customer_id INTEGER NOT NULL,
    policy_type TEXT NOT NULL,
    premium REAL NOT NULL CHECK (premium >= 0),
    start_date TEXT NOT NULL, -- Format: YYYY-MM-DD
    end_date TEXT NOT NULL,   -- Format: YYYY-MM-DD
    status TEXT NOT NULL CHECK (status IN ('ACTIVE', 'EXPIRED', 'CANCELLED')),
    FOREIGN KEY (customer_id) REFERENCES customers(id) ON DELETE RESTRICT
);

-- Index on foreign key customer_id to avoid full table scan on joins
CREATE INDEX IF NOT EXISTS idx_policies_customer_id ON policies(customer_id);

-- -------------------------------------------------------------
-- Table: claims
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS claims (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    policy_id INTEGER NOT NULL,
    description TEXT NOT NULL,
    amount REAL NOT NULL CHECK (amount >= 0),
    claim_date TEXT NOT NULL, -- Format: YYYY-MM-DD
    status TEXT NOT NULL CHECK (status IN ('PENDING', 'APPROVED', 'REJECTED')),
    FOREIGN KEY (policy_id) REFERENCES policies(id) ON DELETE RESTRICT
);

-- Index on foreign key policy_id to avoid full table scan on joins
CREATE INDEX IF NOT EXISTS idx_claims_policy_id ON claims(policy_id);
