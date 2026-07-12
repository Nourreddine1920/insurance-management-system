# Insurance Policy Management System

A professional-grade C11 command-line application simulating an internal insurance company management system. It uses SQLite3 for persistent storage and is designed using clean layered architecture principles.

## Features
* **Customers Management**: CRUD operations, email-uniqueness constraint, indexing by email and name.
* **Policies Management**: CRUD operations, policy state validation, premium checks, constraint enforcement.
* **Claims Management**: Registrations, status tracking, aggregation query reporting.
* **Structured CLI**: Interactive command-line workflows implemented in `src/cli.c` with clear menu navigation.
* **Robust Input Validation**: strict `YYYY-MM-DD` date parsing, email/phone validation, safe numeric input, and consistent user prompts.
* **Logger & Error Systems**: Uniform return codes and multi-level logging (INFO, DEBUG, ERROR).

## Prerequisites
* **C compiler** supporting C11 (GCC, Clang, or MSVC)
* **CMake** (v3.15 or higher)
* **SQLite3** library and headers

### Ubuntu/Debian Setup
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev Valgrind cppcheck clang-format
```

### Windows Setup
* Install Visual Studio with "Desktop development with C++" workload, or use MSYS2/MinGW with GCC.
* Install SQLite3 (you can use `vcpkg` or download precompiled binaries and map them in CMake).

## Building the Project
We enforce out-of-source builds to keep the repository clean.

### Linux/macOS
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows
If the shell fails with a Cygwin/MSYS runtime error such as "cygheap base mismatch", use the Windows wrapper instead:

```cmd
build.cmd
build.cmd cmake
build.cmd run
```

If you prefer PowerShell directly, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
powershell -ExecutionPolicy Bypass -File .\build.ps1 cmake
powershell -ExecutionPolicy Bypass -File .\build.ps1 run
```

## Folder Structure
```
insurance-management-system/
├── .github/               # GitHub Actions workflows for CI/CD and releases
├── assets/                # Supplemental diagrams, screenshots, and assets
├── config/                # Application and CI configuration files
├── database/              # SQLite database schema and migration scripts
│   ├── migrations/        # Versioned database migrations
│   └── schema.sql         # Baseline schema reference
├── docs/                  # Architecture and database design docs
├── include/               # Public header files (*.h)
├── scripts/               # Helper scripts for setup, migrations, and release tasks
├── src/                   # Module source files (*.c)
├── tests/                 # Automated unit tests
└── README.md              # Project documentation
```

## Database Migrations

This repository now uses versioned SQL migrations stored in `database/migrations/`.

- `001_create_tables.sql` creates the tables
- `002_indexes.sql` creates indexes
- `003_seed_data.sql` inserts starter data

### Migration modes

Run migrations directly with the application:

```bash
./insurance_system --migrate
```

This mode opens the database, applies any outstanding migrations, and exits.

### Helper scripts

- `scripts/init_db.sh` — Unix-like initialization script
- `scripts/init_db.ps1` — Windows PowerShell initialization script

These scripts build the project, remove an existing `insurance.db`, and bootstrap the database from migrations.

## Contribution Workflow

This repository includes GitHub templates for:

- Pull Requests
- Bug reports
- Feature requests
- Task tracking

Using templates helps maintain quality, speeds review, and ensures the team captures all relevant context.
