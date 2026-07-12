#include "logger.h"
#include "cli.h"

int main(void) {
    logger_init("app.log");
    LOG_INFO("Application started in interactive mode");
    int result = cli_run("insurance.db");
    logger_close();
    return result;
}
