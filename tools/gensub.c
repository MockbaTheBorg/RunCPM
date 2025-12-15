/*
    gensub - Generates a $$$.SUB file from a text file passed as argument and saves it to a destination also passed as argument.
    Usage: gensub <source> <destination>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 128

int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc < 3) {
        printf("Usage: %s <source> <destination>\n", argv[0]);
        return 1;
    }

    // Open the input file in binary mode
    FILE* file = fopen(argv[1], "rb");
    if (file == NULL) {
        printf("Error: Could not open input file %s\n", argv[1]);
        return 1;
    }
    // Count the number of lines in the file
    int lines = 0;
    char buffer[BUFSIZE];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        lines++;
    }
    // Reset the file pointer
    fseek(file, 0, SEEK_SET);

    // Check if the file is empty
    if (lines == 0) {
        printf("Error: File is empty\n");
        fclose(file);
        return 1;
    }

    // Allocate BUFSIZE bytes for each existing line
    char* subfile = (char*)calloc(lines, BUFSIZE);
    if (subfile == NULL) {
        printf("Error: Could not allocate memory\n");
        fclose(file);
        return 1;
    }

    // Read the file line by line and copy it to the subfile buffer in reverse order of BUFSIZE-byte blocks
    int offset = (lines - 1) * BUFSIZE;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        int len = (int)strlen(buffer);
        // Remove newline and carriage return
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[--len] = '\0';
        }
        if (len > 0 && buffer[len - 1] == '\r') {
            buffer[--len] = '\0';
        }
        if (len >= BUFSIZE - 1) {
            printf("Warning: Line too long, truncating to %d characters.\n", BUFSIZE - 2);
            len = BUFSIZE - 2;
            buffer[len] = '\0';
        }
        subfile[offset] = (char)len;
        memcpy(subfile + offset + 1, buffer, len);
        // Zero the rest of the block for safety
        memset(subfile + offset + 1 + len, 0, BUFSIZE - 1 - len);
        offset -= BUFSIZE;
    }

    // Close the input file
    fclose(file);

    // Write the subfile buffer to the destination file in binary mode
    FILE* dest = fopen(argv[2], "wb");
    if (dest == NULL) {
        printf("Error: Could not open output file %s\n", argv[2]);
        free(subfile);
        return 1;
    }

    fwrite(subfile, 1, lines * BUFSIZE, dest);

    fclose(dest);
    free(subfile);
    return 0;
}