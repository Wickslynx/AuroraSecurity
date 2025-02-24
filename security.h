#ifndef SECURITY_H
#define SECURITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* (*decrypt)(const char* key, const char *input);
    char* (*encrypt)(const char* key, const char *input);
} securityf;

static securityf security;

char* encrypt(const char *key, const char *input);
char* decrypt(const char *key, const char *input);

__attribute__((constructor))
void init_security() {
    security.encrypt = encrypt;
    security.decrypt = decrypt;
}

char* decrypt(const char *key, const char *input) {
    size_t key_len = strlen(key);
    size_t input_len = strlen(input);

    char *output = malloc(input_len + 1);
    if (output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    for (size_t i = 0; i < input_len; i++) {
        output[i] = input[i] ^ key[i % key_len];
    }
    output[input_len] = '\0';

    return output;
}

char* encrypt(const char *key, const char *input) {
    return decrypt(key, input);
}

#endif
