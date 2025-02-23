/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define BLOCK_SIZE 16
#define KEY_ROUNDS 16
#define SALT_LENGTH 16

// Structure for encryption context
typedef struct {
    uint32_t key[KEY_ROUNDS];
    uint8_t salt[SALT_LENGTH];
    uint32_t rounds;
} CryptoContext;

// Custom secure random number generator using multiple seeds
uint32_t secure_rand(void) {
    static uint32_t state = 0;
    state = state * 1103515245 + 12345;
    state = state ^ (state >> 16) ^ (state << 8);
    state = state * 1664525 + 1013904223;
    return state;
}

// Function to generate a key from password
void generate_key(CryptoContext *ctx, const char *password) {
    uint32_t hash = 0x5A827999;  // Initial hash value (from SHA-1)
    size_t pass_len = strlen(password);
    
    // Generate random salt
    for (int i = 0; i < SALT_LENGTH; i++) {
        ctx->salt[i] = secure_rand() & 0xFF;
    }
    
    // Key expansion using password and salt
    for (int round = 0; round < KEY_ROUNDS; round++) {
        hash = 0;
        for (size_t i = 0; i < pass_len; i++) {
            hash += (password[i] + ctx->salt[i % SALT_LENGTH]) * (i + round + 1);
            hash = (hash << 7) | (hash >> 25);  // Rotate left by 7
            hash ^= ctx->salt[(i + round) % SALT_LENGTH];
        }
        
        // Additional mixing
        hash = hash ^ (hash >> 16);
        hash = hash * 0x85ebca6b;
        hash = hash ^ (hash >> 13);
        hash = hash * 0xc2b2ae35;
        hash = hash ^ (hash >> 16);
        
        ctx->key[round] = hash;
    }
}

// Custom block cipher round function
void cipher_round(uint8_t *block, uint32_t round_key) {
    uint32_t *block32 = (uint32_t *)block;
    
    // Mix the block with the round key
    for (int i = 0; i < BLOCK_SIZE/4; i++) {
        block32[i] ^= round_key;
        // Rotate and mix
        block32[i] = (block32[i] << 3) | (block32[i] >> 29);
        block32[i] ^= (block32[i] >> 7) & 0x01010101;
        // Additional substitution
        for (int j = 0; j < 4; j++) {
            uint8_t byte = (block32[i] >> (j * 8)) & 0xFF;
            // Custom S-box operation
            byte = ((byte * 7) ^ 0x3D) + (byte >> 5);
            block32[i] &= ~(0xFF << (j * 8));
            block32[i] |= (uint32_t)byte << (j * 8);
        }
    }
    
    // Block mixing
    for (int i = 0; i < BLOCK_SIZE - 1; i++) {
        block[i] ^= block[i + 1];
    }
    block[BLOCK_SIZE - 1] ^= block[0];
}

// Function to encrypt a block of data
void encrypt_block(CryptoContext *ctx, uint8_t *block) {
    // Initial mixing
    for (int i = 0; i < BLOCK_SIZE; i++) {
        block[i] ^= ctx->salt[i % SALT_LENGTH];
    }
    
    // Multiple rounds of encryption
    for (uint32_t round = 0; round < ctx->rounds; round++) {
        cipher_round(block, ctx->key[round % KEY_ROUNDS]);
        
        // Additional mixing between rounds
        if (round % 2 == 0) {
            // Rotate block right
            uint8_t last = block[BLOCK_SIZE - 1];
            for (int i = BLOCK_SIZE - 1; i > 0; i--) {
                block[i] = block[i - 1];
            }
            block[0] = last;
        } else {
            // Rotate block left
            uint8_t first = block[0];
            for (int i = 0; i < BLOCK_SIZE - 1; i++) {
                block[i] = block[i + 1];
            }
            block[BLOCK_SIZE - 1] = first;
        }
    }
}

// Function to decrypt a block of data
void decrypt_block(CryptoContext *ctx, uint8_t *block) {
    // Reverse the encryption process
    for (int32_t round = ctx->rounds - 1; round >= 0; round--) {
        // Reverse the rotation
        if (round % 2 == 0) {
            // Rotate block left
            uint8_t first = block[0];
            for (int i = 0; i < BLOCK_SIZE - 1; i++) {
                block[i] = block[i + 1];
            }
            block[BLOCK_SIZE - 1] = first;
        } else {
            // Rotate block right
            uint8_t last = block[BLOCK_SIZE - 1];
            for (int i = BLOCK_SIZE - 1; i > 0; i--) {
                block[i] = block[i - 1];
            }
            block[0] = last;
        }
        
        // Reverse cipher round
        cipher_round(block, ctx->key[round % KEY_ROUNDS]);
    }
    
    // Remove initial mixing
    for (int i = 0; i < BLOCK_SIZE; i++) {
        block[i] ^= ctx->salt[i % SALT_LENGTH];
    }
}

// Function to encrypt a file
void encrypt_file(const char *password, const char *input_file, const char *output_file) {
    FILE *fin = fopen(input_file, "rb");
    FILE *fout = fopen(output_file, "wb");
    
    if (!fin || !fout) {
        printf("Error opening files!\n");
        return;
    }
    
    CryptoContext ctx;
    ctx.rounds = 32;  // Number of encryption rounds
    generate_key(&ctx, password);
    
    // Write salt to output file
    fwrite(ctx.salt, 1, SALT_LENGTH, fout);
    
    uint8_t block[BLOCK_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(block, 1, BLOCK_SIZE, fin)) > 0) {
        // Pad block if necessary
        if (bytes_read < BLOCK_SIZE) {
            memset(block + bytes_read, BLOCK_SIZE - bytes_read, BLOCK_SIZE - bytes_read);
        }
        
        encrypt_block(&ctx, block);
        fwrite(block, 1, BLOCK_SIZE, fout);
    }
    
    fclose(fin);
    fclose(fout);
}

// Function to decrypt a file
void decrypt_file(const char *password, const char *input_file, const char *output_file) {
    FILE *fin = fopen(input_file, "rb");
    FILE *fout = fopen(output_file, "wb");
    
    if (!fin || !fout) {
        printf("Error opening files!\n");
        return;
    }
    
    CryptoContext ctx;
    ctx.rounds = 32;
    
    // Read salt from input file
    fread(ctx.salt, 1, SALT_LENGTH, fin);
    generate_key(&ctx, password);
    
    uint8_t block[BLOCK_SIZE];
    size_t bytes_read;
    uint8_t last_byte;
    
    while ((bytes_read = fread(block, 1, BLOCK_SIZE, fin)) > 0) {
        decrypt_block(&ctx, block);
        
        // Handle padding in the last block
        if (bytes_read < BLOCK_SIZE) {
            last_byte = block[BLOCK_SIZE - 1];
            if (last_byte <= BLOCK_SIZE) {
                bytes_read = BLOCK_SIZE - last_byte;
            }
        }
        
        fwrite(block, 1, bytes_read, fout);
    }
    
    fclose(fin);
    fclose(fout);
}

int main() {
    char choice;
    char password[256];
    char input_file[256];
    char output_file[256];
    
    printf("Complex Encryption System\n");
    printf("1. Encrypt\n");
    printf("2. Decrypt\n");
    printf("Choice: ");
    scanf(" %c", &choice);
    getchar(); // Consume newline
    
    printf("Enter password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    printf("Enter input file: ");
    fgets(input_file, sizeof(input_file), stdin);
    input_file[strcspn(input_file, "\n")] = 0;
    
    printf("Enter output file: ");
    fgets(output_file, sizeof(output_file), stdin);
    output_file[strcspn(output_file, "\n")] = 0;
    
    if (choice == '1') {
        encrypt_file(password, input_file, output_file);
        printf("File encrypted successfully!\n");
    } else if (choice == '2') {
        decrypt_file(password, input_file, output_file);
        printf("File decrypted successfully!\n");
    } else {
        printf("Invalid choice!\n");
    }
    
    return 0;
}
*/







