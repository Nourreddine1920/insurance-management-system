#include "validation.h"

#include <stdio.h>

static int assert_equal(const char *name, int expected, int actual) {
    if (expected != actual) {
        printf("FAIL: %s expected=%d actual=%d\n", name, expected, actual);
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;

    failures += assert_equal("validate_email valid", 1, validate_email("user@example.com"));
    failures += assert_equal("validate_email invalid", 0, validate_email("invalid-email"));

    failures += assert_equal("validate_phone valid", 1, validate_phone("+1 (555) 123-4567"));
    failures += assert_equal("validate_phone invalid", 0, validate_phone("abc-123"));

    failures += assert_equal("validate_iso_date valid", 1, validate_iso_date("2025-02-13"));
    failures += assert_equal("validate_iso_date invalid", 0, validate_iso_date("2025-2-13"));

    failures += assert_equal("validate_non_negative_double valid", 1, validate_non_negative_double(42.0));
    failures += assert_equal("validate_non_negative_double invalid", 0, validate_non_negative_double(-3.14));

    failures += assert_equal("validate_non_empty_string valid", 1, validate_non_empty_string("hello", 10));
    failures += assert_equal("validate_non_empty_string invalid", 0, validate_non_empty_string("", 10));

    if (failures == 0) {
        printf("All validation tests passed.\n");
        return 0;
    }

    printf("%d validation test(s) failed.\n", failures);
    return 1;
}
