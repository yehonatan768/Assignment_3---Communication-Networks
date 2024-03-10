#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FILE_PATH "random_file.txt"   // Path for the output file
#define NUM_LINES 2000                // Number of lines to generate
#define MAX_LINE_LENGTH 3000          // Maximum length of each line

/**
 * Generates a random uppercase alphabet character.
 *
 * @return A random uppercase alphabet character ('A' to 'Z').
 */
char random_char() {
    return 'A' + rand() % 26;
}

/**
 * Generates random text content and writes it to the specified file.
 *
 * @param file Pointer to the file where the text content will be written.
 */
void generate_txt(FILE *file) {
    for (int i = 0; i < NUM_LINES; i++) {
        int line_length = MAX_LINE_LENGTH + 1;
        for (int j = 0; j < line_length; j++) {
            fputc(random_char(), file);  // Write random character to file
        }
        fputc('\n', file);  // Add newline character at the end of each line
    }
}

/**
 * Generates a random text file with specified parameters.
 * Prints the size of the generated file.
 */
void Generate_File() {
    srand(time(NULL));  // Initialize random number generator with current time

    FILE *file = fopen(FILE_PATH, "w");  // Open file for writing

    if (file == NULL) {  // Check if file opening is successful
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    generate_txt(file);  // Generate text content and write to file
    fseek(file, 0L, SEEK_END);  // Move file pointer to end to determine file size
    long size = ftell(file);     // Get current position of file pointer (file size)
    fclose(file);  // Close the file

    // Print the size of the generated file in megabytes
    printf("Random text file has been generated (size - %.2f MB)\n", ((double)size / (1024 * 1024)));
}

/**
 * Main function.
 * Calls the Generate_File() function to generate the random text file.
 *
 * @return 0 indicating successful execution of the program.
 */
int main() {
    Generate_File();  // Generate random text file
    return 0;
}
