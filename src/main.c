#include "cli.h"
#include "database.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program_name) {
    printf("Usage: %s [--migrate]\n", program_name);
    printf("  --migrate   Apply database migrations and exit\n");
}

int main(int argc, char *argv[]) {
    const char *database_path = "insurance.db";
    int migrate_only = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "--migrate") == 0 || strcmp(argv[1], "-m") == 0) {
            migrate_only = 1;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    logger_init("app.log");
    if (migrate_only) {
        LOG_INFO("Starting database migration mode");
        error_t err = db_initialize(database_path);
        if (err != ERR_OK) {
            LOG_ERROR("Migration mode failed.");
            logger_close();
            return 1;
        }
        db_close();
        logger_close();
        LOG_INFO("Database migration completed successfully.");
        return 0;
    }

    LOG_INFO("Application started in interactive mode");
    int result = cli_run(database_path);
    logger_close();
    return result;
}
