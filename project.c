#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
void freeRecords() {
	
    struct Record* current = organizationList;
	
    while (current != NULL) {
        struct Record* next = current->next;
        free(current);
        current = next;
    }

    // Set organizationList to NULL to avoid double freeing
    organizationList = NULL;
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

    size_t organizationSize;

    // Read the organization size
    fscanf(inputFile, "%010lu", &organizationSize);

    char buffer[MAX_TOTAL_SIZE]; // Assuming MAX_TOTAL_SIZE is your maximum file size

    // Read and discard the organization section
    if (fgets(buffer, sizeof(buffer), inputFile) == NULL) {
        perror("Error reading organization section");
        exit(EXIT_FAILURE);
    }

    char* token = strtok(buffer, "|");

    while (token != NULL) {
        char fileName[MAX_PATH_LENGTH];
        char permissions[10];

        // Use size as a placeholder, as it's not stored in the structure
        size_t size = 0;

        sscanf(token, "%[^,],%[^,],%lu", fileName, permissions, &size);

        addRecord(head, fileName, permissions, size);

        token = strtok(NULL, "|");
    }
}


void splitFiles(int argc, char *argv[]) {

    if (argc != 4 || strcmp(argv[1], "-b") != 0) {
        printf("Usage: %s -b input.sau output_directory\n", argv[0]);
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
    splitFilesFromList(inputFile, &organizationList);


    // Write archived files to the output directory
    char outputDirectory[MAX_PATH_LENGTH + 20];
    snprintf(outputDirectory, sizeof(outputDirectory), "%s", argv[3]);


    struct Record *current = organizationList;

    int fileCount = 1;


    while (current != NULL) {
        if (current->size > 0) {
            char filePath[MAX_PATH_LENGTH + 30];  // Adjusted size
            snprintf(filePath, sizeof(filePath), "%s/file%d.txt", outputDirectory, fileCount);

            printf("DEBUG: Opening file %s\n", filePath);

            FILE *outputFile = fopen(filePath, "w");

            if (outputFile == NULL) {
                perror("Error opening output file");
                freeRecords(organizationList);
                fclose(inputFile);
                exit(EXIT_FAILURE);
            }


            char buffer[1024];
            size_t bytesRead = 0;


            while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
                // Check for the end of the organization section
                if (strncmp(buffer, "|", 1) == 0) {
                    break;
                }

                printf("DEBUG: Writing to file %s\n", filePath);  // Debug statement
                fputs(buffer, outputFile);
                bytesRead += strlen(buffer);
            }

            fclose(outputFile);
            fileCount++;
        }

        current = current->next;
    }


    // Free memory used by the linked list
    freeRecords(organizationList);
    fclose(inputFile);
}


// Function to read records from the input files and populate the global linked list
void mergeFiles(int argc, char *argv[]) {

    if (argc < 5 || argc > MAX_INPUT_FILES + 4) {
        printf("Usage: %s -a file1.txt file2.txt -o output.sau\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *outputFile = fopen(argv[argc - 1], "w");

    if (outputFile == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }


    for (int i = 2; i < argc - 2; i++) 
        FILE *inputFile = fopen(argv[i], "r");

        if (inputFile == NULL) {
            perror("Error opening input file");
            fclose(outputFile);
            exit(EXIT_FAILURE);
        }


        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
            fprintf(outputFile, "%s", buffer);
        }

        fclose(inputFile);
    }

    fclose(outputFile);
}


int main(int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: \n %s -a file1.txt file2.txt -o output.sau \n %s -b input.sau output_directory\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

   if (strcmp(argv[1], "-a") == 0) {
        mergeFiles(argc, argv);  // Uncomment this line if you want to implement mergeFiles
    } else if (strcmp(argv[1], "-b") == 0) {
        splitFiles(argc, argv);
    } else {
        printf("Invalid command\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
