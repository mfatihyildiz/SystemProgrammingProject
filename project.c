#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void mergeFiles(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s -a file1.txt file2.txt -o output.sau\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *outputFile = fopen(argv[argc - 1], "wb");

    if (outputFile == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    for (int i = 2; i < argc - 2; i++) {
        FILE *inputFile = fopen(argv[i], "r");

        if (inputFile == NULL) {
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }

        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        rewind(inputFile);

/*
        // Write the size of the first section to the output file
        fprintf(outputFile, "%010ld|", fileSize);
*/

        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
            fputs(buffer, outputFile);
        }

        fclose(inputFile);
    }

    fclose(outputFile);
}


void splitFiles(int argc, char *argv[]) {
    if (argc != 4 || strcmp(argv[1], "-b") != 0) {
        printf("Usage: %s -b archive.sau output_directory\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *inputFile = fopen(argv[2], "rb");

    if (inputFile == NULL) {
        perror("Error opening archive file");
        printf("Archive file is inappropriate or corrupt!\n");
        exit(EXIT_FAILURE);
    }

    char *outputDirectory = argv[3];

    // Check if the output directory exists, if not, create it
    #ifdef _WIN32
    _mkdir(outputDirectory);
    #else
    mkdir(outputDirectory, 0777);
    #endif

    // Read the size of the first section
    long firstSectionSize;
    fscanf(inputFile, "%010ld|", &firstSectionSize);

    FILE *outputFile = NULL;
    int fileCount = 1;
    char fileName[50];

    // Extracting files with the same permissions
    struct stat fileStat;

    for (long i = 0; i < firstSectionSize; i++) {
        int c = fgetc(inputFile);

        if (outputFile == NULL) {
            snprintf(fileName, sizeof(fileName), "%s/file%d.txt", outputDirectory, fileCount);
            outputFile = fopen(fileName, "wb");

            if (outputFile == NULL) {
                perror("Error opening output file");
                fclose(inputFile);
                exit(EXIT_FAILURE);
            }


            // Retrieve and set file permissions
            if (fstat(fileno(outputFile), &fileStat) == 0) {
                fchmod(fileno(outputFile), fileStat.st_mode);
            }
        }

        fputc(c, outputFile);

        if (c == '\n') {
            fclose(outputFile);
            outputFile = NULL;
            fileCount++;
        }
    }

    fclose(inputFile);
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s [-a file1.txt file2.txt -o output.sau] [-b input.sau output_directory]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-a") == 0) {
        mergeFiles(argc, argv);
    } else if (strcmp(argv[1], "-b") == 0) {
        splitFiles(argc, argv);
    } else {
        printf("Invalid command\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

