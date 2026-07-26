#include "validation.h"
#include <ctype.h>
#include <string.h>

static int is_digit_char(char ch) {
    return isdigit((unsigned char)ch);
}

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int validate_email(const char* email) {
    if (!email || email[0] == '\0') {
        return 0;
    }

    const char* at = strchr(email, '@');
    if (!at || at == email) {
        return 0;
    }

    const char* dot = strrchr(at + 1, '.');
    if (!dot || dot == at + 1 || dot[1] == '\0') {
        return 0;
    }

    for (const char* ch = email; *ch; ++ch) {
        if (*ch == ' ' || *ch == '\t' || *ch == '\r' || *ch == '\n') {
            return 0;
        }
    }

    return 1;
}

int validate_phone(const char* phone) {
    if (!phone || phone[0] == '\0') {
        return 0;
    }

    int digits_count = 0;
    while (*phone) {
        char ch = *phone;
        if (isdigit((unsigned char)ch)) {
            digits_count++;
        } else if (ch == '+' || ch == '-' || ch == ' ' || ch == '(' || ch == ')') {
            /* allowed formatting characters */
        } else {
            return 0;
        }
        phone++;
    }

    return digits_count >= 7;
}

int validate_iso_date(const char* date) {
    if (!date || strlen(date) != 10) {
        return 0;
    }
    if (date[4] != '-' || date[7] != '-') {
        return 0;
    }
    if (!is_digit_char(date[0]) || !is_digit_char(date[1]) || !is_digit_char(date[2]) ||
        !is_digit_char(date[3]) || !is_digit_char(date[5]) || !is_digit_char(date[6]) ||
        !is_digit_char(date[8]) || !is_digit_char(date[9])) {
        return 0;
    }

    int year =
        (date[0] - '0') * 1000 + (date[1] - '0') * 100 + (date[2] - '0') * 10 + (date[3] - '0');
    int month = (date[5] - '0') * 10 + (date[6] - '0');
    int day = (date[8] - '0') * 10 + (date[9] - '0');

    if (year < 1900 || month < 1 || month > 12 || day < 1) {
        return 0;
    }

    int max_day;
    switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            max_day = 31;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            max_day = 30;
            break;
        case 2:
            max_day = is_leap_year(year) ? 29 : 28;
            break;
        default:
            return 0;
    }

    return day <= max_day;
}

int validate_non_negative_double(double value) {
    return value >= 0.0;
}

int validate_non_empty_string(const char* value, size_t max_length) {
    if (!value || value[0] == '\0') {
        return 0;
    }
    return strlen(value) < max_length;
}
