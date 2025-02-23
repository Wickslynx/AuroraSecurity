#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char* decrypt(char* code) {
    int hash = ((strlen(code) - 3) / 3) + 2;
    char* decrypt = malloc(hash);
    char* toFree = decrypt;
    char* word = code;
    for (int ch = *code; ch != '\0'; ch = *(++code))
    {
        if((code - word + 2) % 3  == 1){
            *(decrypt++) = ch - (word - code + 1) - hash;
        }
    }
    *decrypt = '\0';
    return toFree;
}


char *encrypt(char* decrypt) {
    int length = strlen(decrypt) + 1;
    char* code = malloc(length * 3);
    
    int c = -1;
    for (int d = 0; d < length; d++)
    {
        code[++c] = '*'; 
        code[++c] = '*'; 
        code[++c] = decrypt[d] - c + length + 1;
    }

    code[c] = '\0';
    return code;
}
