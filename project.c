#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_INPUT_FILES 32
#define MAX_PATH_LENGTH 512
#define MAX_TOTAL_SIZE (200 * 1024 * 1024) // 200 MBytes

// Structure to represent a record in the organization section
struct Record {
    char fileName[MAX_PATH_LENGTH];
    char permissions[10];
    size_t size;
    struct Record* next;
    struct Record* prev;
};

// Global variable for the linked list
struct Record* organizationList = NULL;

// Function to add a record to the linked list
void addRecord(struct Record** head, const char* fileName, const char* permissions, size_t size) {

    struct Record* newRecord = (struct Record*)malloc(sizeof(struct Record));
    if (newRecord == NULL) {
        perror("Error allocating memory for record");
        exit(EXIT_FAILURE);
    }
	
    strncpy(newRecord->fileName, fileName, sizeof(newRecord->fileName));

    strncpy(newRecord->permissions, permissions, sizeof(newRecord->permissions));

    newRecord->size = size;

    newRecord->next = NULL;

    if (*head == NULL) {
        newRecord->prev = NULL;
        *head = newRecord;
   } else {
	   
        struct Record* current = *head;

        while (current->next != NULL) {
            current = current->next;
        }
        newRecord->prev = current;
        current->next = newRecord;
    }
}

// Function to free the memory used by the linked list
void freeRecords(struct Record** head) {
	
    struct Record* current = *head;

    while (current != NULL) {

        struct Record* next = current->next;

        free(current);
        current = next;

    }

    // Set the head pointer to NULL to avoid double freeing
    *head = NULL;
}

// Function to write records from the linked list to the output file
void writeRecordsToFile(FILE* outputFile, struct Record* head) {
    struct Record* current = head;
	
    while (current != NULL) {
        if (current->size > 0) {

            // Write the record to the output file
            fprintf(outputFile, "|%s,%s,%lu|", current->fileName, current->permissions, current->size);
        }
        current = current->next;
    }
}

// Function to read records from the input file and populate the linked list
void splitFilesFromList(FILE* inputFile, struct Record** head) {

    if (inputFile == NULL || head == NULL) {

        perror("Null pointer passed to splitFilesFromList");

        exit(EXIT_FAILURE);
    }

    size_t organizationSize;

    fscanf(inputFile, "%010lu", &organizationSize);

    char* buffer = malloc(MAX_TOTAL_SIZE);

    if (buffer == NULL) {
        perror("Error allocating memory for buffer");
        exit(EXIT_FAILURE);
    }

    if (fgets(buffer, MAX_TOTAL_SIZE, inputFile) == NULL) {

        perror("Error reading organization section");
        exit(EXIT_FAILURE);
    }

    char* token = strtok(buffer, "|");

    while (token != NULL) {
        char fileName[MAX_PATH_LENGTH];
        char permissions[10];
        size_t size = 0;

        sscanf(token, "%[^,],%[^,],%lu", fileName, permissions, &size);

        if (strlen(fileName) > 0) {
            addRecord(head, fileName, permissions, size);
        } else {
            fprintf(stderr, "ERROR: Invalid token: %s\n", token);
        }

        token = strtok(NULL, "|");
    }
    free(buffer);
}

void splitFiles(int argc, char *argv[]) {

    if (argc != 4 || strcmp(argv[1], "-a") != 0) {
        printf("Usage: %s -a input.sau output_directory\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *inputFile = fopen(argv[2], "r");

    if (inputFile == NULL) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Create a linked list for the organization section
    struct Record *organizationList = NULL;

    // Populate the linked list with records
    printf("DEBUG: Calling splitFilesFromList\n");

    splitFilesFromList(inputFile, &organizationList);

    // Add debug print statements
    printf("DEBUG: Linked list after splitFiles in splitFiles\n");

    struct Record* currentDebug = organizationList;

    while (currentDebug != NULL) {
        printf("DEBUG: Record: %s, %s, %lu\n", currentDebug->fileName, currentDebug->permissions, currentDebug->size);
        currentDebug = currentDebug->next;
    }

    // Write archived files to the output directory
    char outputDirectory[MAX_PATH_LENGTH + 20];
	
    snprintf(outputDirectory, sizeof(outputDirectory), "%s", argv[3]);

    // Check if the output directory exists, if not, create it
    struct stat st = {0};

    if (stat(outputDirectory, &st) == -1) {
        if (mkdir(outputDirectory, 0700) != 0) {
            perror("Error creating output directory");
            freeRecords(&organizationList);
            fclose(inputFile);
            exit(EXIT_FAILURE);
        }
    }

    struct Record *current = organizationList;

    int fileCount = 1;

    while (current != NULL) {
        if (current->size > 0) {
            char filePath[MAX_PATH_LENGTH + 30];
            snprintf(filePath, sizeof(filePath), "%s/file%d.txt", outputDirectory, fileCount);

            printf("DEBUG: Opening file %s\n", filePath);

            FILE *outputFile = fopen(filePath, "w");

            if (outputFile == NULL) {
                perror("Error opening output file");
                freeRecords(&organizationList);
                fclose(inputFile);
                exit(EXIT_FAILURE);
            }

            fseek(inputFile, 0, SEEK_SET); // Move to the beginning of the file

            char buffer[1024];

            size_t bytesRead = 0;

            // Skip the organization section
            while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
                if (strncmp(buffer, "|", 1) == 0) {
                    break;
                }
            }

            // Read and write individual files
            while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
                if (strncmp(buffer, "|", 1) == 0) {
                    break;  // End of the organization section
                }

                printf("DEBUG: Writing to file %s\n", filePath);
                fputs(buffer, outputFile);
                bytesRead += strlen(buffer);
            }

            fclose(outputFile);
            fileCount++;
        }

        // Move to the next node in the linked list
        struct Record *next = current->next;
        free(current);
        current = next;
    }

    // No need to set organizationList to NULL here
    fclose(inputFile);
}

// Function to read records from the input files and populate the global linked list
void mergeFiles(int argc, char *argv[]) {

    // Check if the user provided the output file name
    char* outputFileName = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFileName = argv[i + 1];
            break;
        }
    }

    // Set the default output file name if not provided
    if (outputFileName == NULL) {
        outputFileName = "a.sau";
    }

    FILE *outputFile = fopen(outputFileName, "w");

    if (outputFile == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    for (int i = 2; i < argc - 2; i++) {
        FILE *inputFile = fopen(argv[i], "r");

        if (inputFile == NULL) {
            perror("Error opening input file");
            fclose(outputFile);
            exit(EXIT_FAILURE);
        }

        char buffer[1024];

        // Read and write individual files
        while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
            fprintf(outputFile, "%s", buffer);
        }
        fclose(inputFile);
    }
    fclose(outputFile);
}

int main(int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: \n %s -b file1.txt file2.txt -o output.sau \n %s -a input.sau output_directory\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

   if (strcmp(argv[1], "-b") == 0) {
        mergeFiles(argc, argv);  // Uncomment this line if you want to implement mergeFiles
    } else if (strcmp(argv[1], "-a") == 0) {
        splitFiles(argc, argv);
    } else {
        printf("Invalid command\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
