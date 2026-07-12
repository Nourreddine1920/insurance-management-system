# Insurance Policy Management System

A professional-grade C11 command-line application simulating an internal insurance company management system. It uses SQLite3 for persistent storage and is designed using clean layered architecture principles.

## Features
* **Customers Management**: CRUD operations, email-uniqueness constraint, indexing by email and name.
* **Policies Management**: CRUD operations, policy state validation, premium checks, constraint enforcement.
* **Claims Management**: Registrations, status tracking, aggregation query reporting.
* **Structured CLI**: Fast, sanitised, and safe standard input interface.
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
* Install Visual Studio with "Desktop development with C++" workload.
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
├── CMakeLists.txt         # CMake build configuration
├── README.md              # Project documentation
├── docs/                  # Architecture and database design docs
├── include/               # Public header files (*.h)
├── src/                   # Module source files (*.c)
├── database/              # SQL schema script
└── tests/                 # Unit tests (to be added)
```
