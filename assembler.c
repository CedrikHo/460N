#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>


#define MAX_OPCODE_SIZE 31

FILE* infile = NULL;
FILE* outfile = NULL;

char *opcodes[26] = {
    "add", 
    "and", 
    "br", 
    "brp",
    "brz",
    "brzp",
    "brn",
    "brnp",
    "brnzp",
    "halt",
    "jmp",
    "jsr",
    "jsrr",
    "ldb",
    "ldw",
    "lea",
    "nop",
    "not",
    "ret",
    "lshf",
    "rshfl",
    "rshfa",
    "rti",
    "stb",
    "trap",
    "xor"
};

int isOpcode(char *check) {
    for(int i = 0; i < 26; i++) {
        if(strcmp(check, opcodes[i]) == 0) {
            return 1;    
        }
    }
    return 0;   
}
int toNum(char* pStr) {
    char* t_ptr;
    char* orig_pStr;
    int t_length,k;
    int lNum, lNeg = 0;
    long int lNumLong;
    
    orig_pStr = pStr;
    if(*pStr == '#') { //if the number is decimal
        pStr++;
        if(*pStr == '-') { //if num is negative
            lNeg = 1;
            pStr++;
        }
        t_ptr = pStr;
        t_length = strlen(t_ptr);
        for(k = 0; k < t_length; k++) {
            if(!isdigit(*t_ptr)) {
                printf("Error: invalid decimal operand, %s\n", orig_pStr);
                exit(4);
            }
            t_ptr++;
        }
        lNum = atoi(pStr);
        if(lNeg)
            lNum*=-1;
        
        return lNum;
    } else if(*pStr == 'x') { //if num is in hex
        pStr++;
        if(*pStr == '-') { //if num is negative
            lNeg = 1;
            pStr++;
        }
        t_ptr = pStr;
        t_length = strlen(t_ptr);
        for(k = 0; k < t_length; k++) {
            if(!isxdigit(*t_ptr)) {
                printf("Error: invalid hex operand, %s\n", orig_pStr);
                exit(4);
            }
            t_ptr++;
        }
        lNumLong = strtol(pStr, NULL, 16); // convert hex string to int
        lNum = (lNumLong > INT_MAX) ? INT_MAX : lNumLong;
        if(lNeg)
            lNum *=-1;

        return lNum;
    } else {
        printf("Error: invalid operand, %s\n", orig_pStr);
        exit(4);
    }
}

#define MAX_LINE_LENGTH 255
enum {
    DONE, OK, EMPTY_LINE
};

int readAndParse(FILE * pInfile, char * pLine, char ** pLabel, char ** pOpcode, 
                    char ** pArg1, char ** pArg2, char ** pArg3, char ** pArg4) {
    char *lRet, *lPtr;
    int i;
    if(!fgets(pLine, MAX_LINE_LENGTH, pInfile)) 
        return( DONE );
    for( i = 0; i < strlen(pLine); i++) {
        pLine[i] = tolower( pLine[i] );
    }
    //make everything lowercase
    *pLabel = *pOpcode = *pArg1 = *pArg2 = *pArg3 = *pArg4 = pLine + strlen(pLine);

    // ignore the comments
    lPtr = pLine;

    while( *lPtr != ';' && *lPtr != '\0' && *lPtr != '\n')
        lPtr++;

    *lPtr = '\0';
    if(!(lPtr = strtok(pLine, "\t\n , ")))
        return (EMPTY_LINE);

    if(isOpcode(lPtr) == 0 && lPtr[0] != '.') {
        *pLabel = lPtr;
        
        if(!(lPtr = strtok(NULL, "\t\n ,"))) return (OK);
    }
    
    *pOpcode = lPtr;

    if(!(lPtr = strtok(NULL, "\t\n ,"))) return (OK);

    *pArg1 = lPtr;

    if(!(lPtr = strtok(NULL, "\t\n ,"))) return (OK);

    *pArg2 = lPtr;
   
    if(!(lPtr = strtok(NULL, "\t\n ,"))) return (OK);

    *pArg3 = lPtr;

    if(!(lPtr = strtok(NULL, "\t\n ,"))) return (OK);

    *pArg4 = lPtr;

    return (OK);
}
    

int main(int argc, char* argv[]) {
    
    char *prgName = NULL;
    char *iFileName = NULL;
    char *oFileName = NULL;

    prgName = argv[0];
    iFileName = argv[1];
    oFileName = argv[2];

    printf("program name = '%s'\n", prgName);
    printf("input file name = '%s'\n", iFileName);
    printf("output file name = '%s'\n", oFileName);

    infile = fopen(argv[1], "r");
    outfile = fopen(argv[2], "w");

    if(!infile) {
        printf("Error: Cannot open file %s\n", iFileName);
        exit(4);
    }
    if(!outfile) {
        printf("Error: Cannot open file %s\n", oFileName);  
        exit(4);
    }

    char lLine[MAX_LINE_LENGTH + 1], *lLabel, *lOpcode, *lArg1, 
        *lArg2, *lArg3, *lArg4;

    int lRet;

    do {
        lRet = readAndParse(infile, lLine, &lLabel, &lOpcode, &lArg1,
                                &lArg2, &lArg3, &lArg4);
        if(lRet != DONE && lRet != EMPTY_LINE)
        {
            printf("%s, %s, %s, %s, %s, %s. \n", lLabel, lOpcode, lArg1,
                                        lArg2, lArg3, lArg4);
        }
    }while(lRet != DONE);   

    //This is where you would call the functions to read everything
    return 0;
}
