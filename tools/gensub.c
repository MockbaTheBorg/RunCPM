/*
    gensub - Generates a $$$.SUB file from a text file passed as argument and saves it to a destination also passed as argument.
    Usage: gensub <source> <destination>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 128

int main(int argc, char* argv[]) {
    // Check if the user passed a file as argument
    if (argc < 2) {
        printf("Usage: %s <filename> <destination>\n", argv[0]);
        return 1;
    }

    // Check if the user passed a destination as argument
    if (argc < 3) {
        printf("Usage: %s <filename> <destination>\n", argv[0]);
        return 1;
    }

    // Open the input file
    FILE* file = fopen(argv[1], "r");
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

    // Allocate 128 bytes for each axisting line
    char* subfile = (char*)malloc(lines * 128);
    if (subfile == NULL) {
        printf("Error: Could not allocate memory\n");
        fclose(file);
        return 1;
    }

    // Read the file line by line and copy it to the subfile buffer in reverse order of 128 byte blocks
    int offset = (lines - 1) * 128;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        int len = strlen(buffer);
        if (len > 0) {
            if (buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                len--;
            }
        }
        if (len > 0) {
            if (buffer[len - 1] == '\r') {
                buffer[len - 1] = '\0';
                len--;
            }
        }
        subfile[offset] = len;
        memcpy(subfile + offset + 1, buffer, len);
        offset -= 128;
    }

    // Close the input file
    fclose(file);

    // Write the subfile buffer to the destination file
    FILE* dest = fopen(argv[2], "w");
    if (dest == NULL) {
        printf("Error: Could not open output file %s\n", argv[2]);
        free(subfile);
        return 1;
    }

    // Write the subfile buffer to the destination file
    fwrite(subfile, 1, lines * 128, dest);

    // Close the destination file
    fclose(dest);

}