#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h>

int validate_email(const char *email);
int validate_phone(const char *phone);
int validate_iso_date(const char *date);
int validate_non_negative_double(double value);
int validate_non_empty_string(const char *value, size_t max_length);

#endif // VALIDATION_H
