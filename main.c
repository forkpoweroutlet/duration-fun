#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long int findBytes(FILE *haystack, long int fileSize, unsigned char needle[], int needleSize) {

    unsigned char *readBuffer = NULL;

    // If for some godforsaken reason we can't allocate like 3 bytes
    if((readBuffer = malloc(needleSize)) == NULL) {
        return -2;
    }

    long int startAt = ftell(haystack);
    if(startAt < 0) {
        free(readBuffer);
        readBuffer = NULL;
        return -2;
    }

    for(int seekPos = startAt; seekPos < (fileSize - needleSize); seekPos++) {
        unsigned char currentChar = fgetc(haystack);

        if(currentChar == EOF) {
            free(readBuffer);
            readBuffer = NULL;
            return -2;

        // https://i.kym-cdn.com/entries/icons/original/000/029/264/cover8.jpg
        } else if(currentChar == needle[0]) {
            fseek(haystack, -1L, SEEK_CUR);

            if(needleSize != fread(readBuffer, 1, needleSize, haystack)) {
                free(readBuffer);
                readBuffer = NULL;
                return -2;

            } else if(memcmp(readBuffer, needle, needleSize) == 0) {

                free(readBuffer);
                readBuffer = NULL;

                if(0 == fseek(haystack, -1L * needleSize, SEEK_CUR)) {
                    return 0;
                    
                } else {
                    return -2;
                }

            } else if(0 != fseek(haystack, 1L - needleSize, SEEK_CUR)) {
                free(readBuffer);
                readBuffer = NULL;
                return -2;
            }

        }
    }

    free(readBuffer);
    return -1;
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Please supply a file\n");
        return EXIT_FAILURE;
    }

    FILE *inputFile;
    if((inputFile = fopen(argv[1], "rb")) == NULL) {
        fprintf(stderr, "Couldn't open the input file\n");
        return EXIT_FAILURE;
    }

    long int byteSize;
    fseek(inputFile, 0, SEEK_END);
    byteSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    FILE *outputFile;
    if((outputFile = fopen(argv[2], "ab")) == NULL) {
        fprintf(stderr, "Couldn't open the output file\n");
        return EXIT_FAILURE;
    }

    for(int seekPos = 0; seekPos < byteSize; seekPos++) {
        unsigned char byte = fgetc(inputFile);
        if(byte == EOF) {
            fprintf(stderr, "Problem reading from input file\n");
            fclose(inputFile);
            fclose(outputFile);
            return EXIT_FAILURE;
        }

        if(fputc(byte, outputFile) == EOF) {
            fclose(inputFile);
            fclose(outputFile);
            fprintf(stderr, "Problem writing to output file\n");
            return EXIT_FAILURE;
        }
    }
    fclose(inputFile);
    fclose(outputFile);

    outputFile = fopen(argv[2], "rb+");
    if(outputFile == NULL) {
        fprintf(stderr, "Problem writing to output file\n");
        return EXIT_FAILURE;
    }

    unsigned char timestampScaleBytes[3] = {0x2a, 0xd7, 0xb1};
    if(findBytes(outputFile, byteSize, timestampScaleBytes, 3) != 0) {
        fprintf(stderr, "Are you sure that's a valid WEBM?\n");
        fclose(outputFile);
        remove(argv[2]);
        return EXIT_FAILURE;
    }

    unsigned char durationBytes[3] = {0x44, 0x89, 0x88};
    if(findBytes(outputFile, byteSize, durationBytes, 3) != 0) {
        fprintf(stderr, "Are you sure that's a valid WEBM?\n");
        fclose(outputFile);
        remove(argv[2]);
        return EXIT_FAILURE;
    }

    fseek(outputFile, 3L, SEEK_CUR);
    unsigned char bytesToWrite[8] = {0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if(8 != fwrite(bytesToWrite, 1, 8, outputFile)) {
        fprintf(stderr, "Failed to change the required data - that's probably my problem\n");
        return EXIT_FAILURE;
    }

    fclose(outputFile);
    printf("Done! Saved file to %s\n", argv[2]);
    return EXIT_SUCCESS;
}