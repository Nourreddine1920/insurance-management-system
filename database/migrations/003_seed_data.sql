INSERT INTO customers (first_name, last_name, email, phone) VALUES
    ('Alice', 'Anderson', 'alice.anderson@example.com', '+15555550101'),
    ('Bob', 'Benson', 'bob.benson@example.com', '+15555550202');

INSERT INTO policies (customer_id, policy_type, premium, start_date, end_date, status) VALUES
    (1, 'AUTO', 700.50, '2025-01-01', '2026-01-01', 'ACTIVE'),
    (2, 'HOME', 1200.00, '2025-02-01', '2026-02-01', 'ACTIVE');

INSERT INTO claims (policy_id, description, amount, claim_date, status) VALUES
    (1, 'Rear bumper damage', 300.00, '2025-03-15', 'PENDING'),
    (2, 'Roof leak', 1500.00, '2025-04-20', 'APPROVED');
