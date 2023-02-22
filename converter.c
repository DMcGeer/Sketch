#include "displayfull.h"
#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// ensures the maximum value can be handled by the system
int binaryInc(long maxVal){
    if (maxVal < 0) {
        return -1;
    }
    else if (maxVal >= 256 & maxVal < 65536) return 2;
    else if (maxVal >= 65536) return -1;
    else return 1;
}

// tests the algorithm which calculates the colour code 
unsigned int testColourAlg(unsigned char b){
    unsigned int c = (0 << 6) + (0x3f & (b >> 6));
    c = (c << 6) + (0x3f & b);
    for (int t = 0; t < 4; t++) c = (c << 6) + (0x3f & ((c >> 2) | (c << 6)));
    return c;
}

// sets up the colour in the file
void fileColour(FILE *sketchFile, unsigned short b){

    fputc(0xc0 | b >> 6, sketchFile);
    fputc(0xc0 | b, sketchFile);
    for (int t = 4; t > 0; t--){
       b = (b >> 2) | (b << 6);
       fputc(0xc0 | b, sketchFile);
    }

    // set the mode to colour
    fputc(0x83, sketchFile);
}

// checks the algorithm for increasing x at the end of a block
int testXCoord(int x, int xCount){
    // x coord cannot be -ve
    if (x < 0) return -1;
    while (xCount != 0){
        if (xCount >= 31) {
            x += 31;
            xCount -= 31;
        }
        else{
            x += xCount;
            xCount = 0;
        }
    }
    return x;
}

// moves the pixel to the right, and draws the pixel
void fileMove(FILE *sketchFile, int value){
    // set the mode to block
    fputc(0x82, sketchFile);

    // increase tx by value
    while (value != 0){
        if (value >= 31) {
            fputc(0x1f, sketchFile);
            value -= 31;
        }
        else {
            fputc(0x00 | value, sketchFile);
            value = 0;
        }
    }

    // increase ty by 1, makes block
    fputc(0x41, sketchFile);

    // set the mode to none
    fputc(0x80, sketchFile);

    // decrease ty by 1
    fputc(0x7f, sketchFile);
}

void resetLine(FILE *sketchFile){
    // set tx to 0
    fputc(0x84, sketchFile);

    // increase ty by 1
    fputc(0x41, sketchFile);
}

// ensures that the given file is in the pgm format
int testPgmFile(FILE *currentFile){
    char fileType[3];
    for (int t = 0; t < 2; t++){
        unsigned char b = fgetc(currentFile);
        fileType[t] = b;
    }
    fileType[2] = '\0';
    if (strcmp(fileType, "P5") == 0) return 0;
    else return -1;
}

// manages the body of the new pgm file
int Pgmbody(FILE *currentFile, FILE *sketchFile, int width, int height, int maxVal){
    int binaryIncrement = binaryInc(maxVal);
    if (binaryIncrement == -1) return -1;
    else if (binaryIncrement == 2) {
        unsigned short b = fgetc(currentFile);
        unsigned short prevInst;
    }
    else {
        unsigned char b = fgetc(currentFile);
        unsigned char prevInst = fgetc(currentFile);
    }
    fileColour(sketchFile, b);

    int total = 0;
    int xCount = 0;

    int yCount = 1;
    while (yCount <= height){
        unsigned char prevInst = b;
        for (int t = 0; t < binaryIncrement; t++){
            unsigned char c = fgetc(currentFile);
            b = b + (c << 8*t)
        }
        // if the width is reached, a new line is needed
        if (total == width){
            fileMove(sketchFile, xCount);
            resetLine(sketchFile);
            yCount++;
            xCount = 0;
            total = 0;
        }

        // if the colour is same to the prior, then the count is increased
        else if (prevInst == b) xCount++;

        // if the new colour is different, then a block should be made and the new colour set
        else{
            fileMove(sketchFile, xCount+1);
            fileColour(sketchFile, b);
            xCount = 0;
        }
        total++;
    }
    return 0;
}

// finds the width, height and max grey value of the file
int headerInfo(FILE *currentFile, FILE *sketchFile){
    int spaceCount = 0;
    unsigned char b;
    char charWidth[5];
    char charHeight[5];
    char maxValue[7];
    int count = 0;
    int count2 = 0;
    int count3 = 0;
    bool lineBreak = false;

    if (testPgmFile(currentFile) != 0) return -1;

    // deals with the header of the pgm file
    while (lineBreak == false){
        b = fgetc(currentFile);
        if (spaceCount == 1) {
            charWidth[count] = b;
            count++;
        }
        else if (spaceCount == 2){
            charHeight[count2] = b;
            count2++;
        }
        else if (spaceCount == 3) {
            if ((int)b == 10) lineBreak = true;
            else{
                maxValue[count3] = b;
                count3++;
            }
        }
        if (isspace(b) > 0) spaceCount++;
    }
    charWidth[4] = '\0';
    charHeight[4] = '\0';
    maxValue[6] = '\0';
    int width = atoi(charWidth);
    int height = atoi(charHeight);
    int maxVal = atoi(maxValue);

    int body = Pgmbody(currentFile, sketchFile, width, height, maxVal);

    return body;
}

// finds the width of an sk file
int findWidth(char fileName[]){
    FILE *currentFile = fopen(fileName, "rb");
    int count = 0;
    unsigned char b = fgetc(currentFile);
    while ((b != 0x84) & (!feof(currentFile))){
        // if the current instruction changes dx, then it should be added to the width
        if ((b & 0xc0) == 0){
            count += (b & 0x3f);
        }
        b = fgetc(currentFile);
    }
    return count;
}

// finds the height of an sk file
int findHeight(char fileName[]){
    FILE *currentFile = fopen(fileName, "rb");
    int count = 0;
    while (!feof(currentFile)){
        unsigned char b = fgetc(currentFile);
        if (b == 0x41) {
            count++;
        }
    }
    return count;
}

// writes the header of a new pgm file
void skHeader(char fileName[], char pgmFileName[]){
    FILE *pgmFile = fopen(pgmFileName, "wb");
    // P5 + space
    fputc(0x50, pgmFile);
    fputc(0x35, pgmFile);
    fputc(0x20, pgmFile);
    // 200 width
    fputc(0x32, pgmFile);
    fputc(0x30, pgmFile);
    fputc(0x30, pgmFile);
    // 200 height 
    fputc(0x20, pgmFile);
    fputc(0x32, pgmFile);
    fputc(0x30, pgmFile);
    fputc(0x30, pgmFile);
    // 255 maxVal
    fputc(0x20, pgmFile);
    fputc(0x32, pgmFile);
    fputc(0x35, pgmFile);
    fputc(0x35, pgmFile);
    // new line 
    fputc(0x0a, pgmFile);
}

// writes the body of the new pgm file
int skBody(char fileName[], char pgmFileName[]){
    unsigned int colour = 0;
    int yCount = 0;
    FILE *currentFile = fopen(fileName, "rb");
    FILE *pgmFile = fopen(pgmFileName, "wb");
    while (yCount <= (findHeight(fileName)-1)){
        unsigned char b = fgetc(currentFile);
        // if the instruction updates data, then it is relevant to the colour code
        if ((b & 0xc0) == 0xc0){
            while (b != 0x83){
                colour = (colour << 6) + (b & 0x3f);
                b = fgetc(currentFile);
            }
        }
        else if (b == 0x82) {
            int count = 1;
            b = fgetc(currentFile);
            if (b != 0x41){
                while (b != 0x41){
                    count += (int)(b & 0x3f);
                    b = fgetc(currentFile);
                }
                for (int t = 0; t < count; t++){
                    if (colour == 0x00000000) fputc(0x00, pgmFile);
                    else fputc((colour & 0xff), pgmFile);
                }
            }
            yCount++;
        }
        else if (b == 0x41) {
            yCount++;
        }
        if (yCount == 0){
            for (int t = 0; t < 14; t++){
                fputc(colour & 0xff, pgmFile);
            }
        }
    }
    return 0;
}

// creates and manages a new pgm file
int setupSk(char fileName[]){
    //printf("Sk file implementation not fully complete\n");
    return -1;
    FILE *currentFile = fopen(fileName, "rb");
    if (currentFile == NULL) {
        printf("Can't open %s\n", fileName);
        fflush(stderr);
        exit(1);
    }

    char pgmFileName[strlen(fileName)+3];
    int t = 0;
    while (fileName[t] != '.'){
        pgmFileName[t] = fileName[t];
        t++;
    }
    pgmFileName[t] = '\0';
    FILE *pgmFile = fopen(strcat(pgmFileName, "1.pgm"), "wb");

    skHeader(fileName, pgmFileName);
    int value = skBody(fileName, pgmFileName);

    if (value == 0) printf("File %s has been written\n", pgmFileName);
    else printf("File couldn't be written\n");
    
    fclose(pgmFile);
    fclose(currentFile);
    return 0;
}

// makes and manages a new sk file
void setupPgm(char fileName[]){
    FILE *currentFile = fopen(fileName, "rb");
    if (currentFile == NULL) {
        printf("Can't open %s\n", fileName);
        fflush(stderr);
        exit(1);
    }

    char sketchFileName[strlen(fileName)+1];
    int t = 0;
    while(fileName[t] != '.'){
        sketchFileName[t] = fileName[t];
        t++;
    }
    sketchFileName[t] = '\0';
    FILE *sketchFile = fopen(strcat(sketchFileName, ".sk"), "wb");
    
    int value = headerInfo(currentFile, sketchFile);

    if (value == 0) printf("File %s has been written\n", sketchFileName);
    else printf("File couldn't be written\n");
    fclose(sketchFile);
    fclose(currentFile);
}

void functionPGMTesting(){
    // tests all non-file writing functions
    assert(binaryInc(-1) == -1);
    assert(binaryInc(0) == 1);
    assert(binaryInc(255) == 1);
    assert(binaryInc(256) == -1);
    assert(binaryInc(2147483647) == -1);
    assert(binaryInc(2147483648) == -1);

    assert(testColourAlg(0x1c) == 0x1c1c1c1c);
    assert(testColourAlg(0x00) == 0x00000000);
    assert(testColourAlg(0xff) == 0xffffffff);

    assert(testXCoord(0, 2) == 2);
    assert(testXCoord(-1, 3) == -1);
    assert(testXCoord(4, 45) == 49);

    FILE *firstFile = fopen("fractal.pgm", "rb");
    assert(testPgmFile(firstFile) == 0);
    FILE *secondFile = fopen("sketch00.sk", "rb");
    assert(testPgmFile(secondFile) == -1);
    assert(setupSk("sketch00.sk") == -1);
}

int fileSKTesting(){
    FILE *secondFile = fopen("bands1.pgm", "rb");
    FILE *compareFile = fopen("bands.pgm", "rb");
    int count = 0;
    while (!feof(secondFile)){
        count++;
        unsigned char b = fgetc(secondFile);
        unsigned char c = fgetc(compareFile);
        if (count == 1) printf("%0.2x\n", b);
        if (b != c) {
            return -1;
        }
    }
    return 0;
}

void functionSKTesting(){
    assert(fileSKTesting() == 0);
}

int main(int n, char *args[]){
    if(n == 1){
        functionPGMTesting();
        printf("All tests passed\n");
    }
    else{
        if (args[1][strlen(args[1])-2] == 's' & args[1][strlen(args[1])-1] == 'k') {
            if (setupSk(args[1]) == -1) printf("Sk file implementation not fully complete\n");
        }
        else if (args[1][strlen(args[1])-3] == 'p' & args[1][strlen(args[1])-2] == 'g' & args[1][strlen(args[1])-1] == 'm') setupPgm(args[1]);
        else printf("File type not supported\n");
    }
}
