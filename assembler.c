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

char *registers[8] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7"
};

char *pseudoOps[3] = {
    ".fill",
    ".orig",
    ".end"
};

char *binaryRegisters[8] = {
    "000",
    "001",
    "010",
    "011",
    "100",
    "101",
    "110",
    "111"
};

char * BinaryRegister(char *arg) {
	for (int i = 0; i < 8; i++) {
		if (strcmp(arg, registers[i]) == 0)
			return binaryRegisters[i];
    }
    exit(4);
}

#define MAX_LABEL_LEN 20
#define MAX_SYMBOLS 255
typedef struct {
    char* labelName;  
    int address;
} TableEntry;
TableEntry symbolTable[MAX_SYMBOLS];
int tableSize = 0;

int isTrap(char * label) {
    if(strcmp(label, "getc") == 0) {
        printf("invalid label, Trap");
        exit(4);
    }
    if(strcmp(label, "in") == 0) {
        printf("invalid label, trap");
        exit(4);
    }
    if(strcmp(label, "out") == 0) {
        printf("invalid label, trap");
        exit(4);
    }
    if(strcmp(label, "puts") == 0) {
        printf("invalid label, trap");
        exit(4);
    }
    return 0;
}       

int isRegister(char* label) {
    for(int i = 0; i < 8; i++) {
        if(strcmp(label, registers[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int isalphanumeric(char* label) {
    for (int i = 0; i < (strlen(label)); i++) {
        //Test edge case at size 20
        if (isalnum(label[i]) == 0) {//Zero means failed
            exit(4);
        }
    }

    return 1;
}

void makeEntry(char* label, int address) {
    if(tableSize > MAX_SYMBOLS) {
        printf("symbol table full");
        exit(4);
    }
    if(isalphanumeric(label) && label[0] != 'x') {
        isTrap(label);
        if(isRegister(label)) {
            printf("invalid label, register");   
            exit(4);
        }
        if(!isalpha(label[0])) {
            printf("invalid label, first char non alpha");
            exit(4);       
        }
    }
//    printf("valid");
    for(int i = 0; i < tableSize; i++) {
	    if(strcmp(label, symbolTable[i].labelName) == 0) 
	        exit(4);
	}
    symbolTable[tableSize].labelName = (char*) malloc(21);
    strcpy(symbolTable[tableSize].labelName, label);
    symbolTable[tableSize].address = address;
    tableSize++;
}

int symbolTableSearch(char* label) {
    for(int i = 0; i < tableSize; i++) {
        if(strcmp(label, symbolTable[i].labelName) == 0) {
            return symbolTable[i].address;
        }
    }
    exit(1);
    
}

int isOpcode(char *check) {
    for(int i = 0; i < 26; i++) {
        if(strcmp(check, opcodes[i]) == 0) {
            return 1;    
        }
    }
    return 0;   
}

char* toHex(char* instr) {
    int value = (int) strtol(instr, NULL, 2);
    char* hexString = (char*) malloc(14);
    strcat(hexString, "0x");
    
    char buffer[4];
    
    if(sprintf(buffer, "%x", value) == 3) {
        strcat(hexString, "0");
    }
    for(int i = 0; i < 4; i++) {
        if(isalpha(buffer[i]) != 0) {
            buffer[i] = toupper(buffer[i]);
        }   
    }
    strcat(hexString, buffer);
    return hexString;
}

int toBinary(int n) {
    return (n == 0 || n == 1 ? n :((n % 2) + 10 * toBinary(n/2)));
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

void posImmediate(int num, int size, char* immediate, char* str) {
    num = toBinary(num);
    int n = num;
    int count = sprintf(str, "%d", num);
    int zeros = size - count;
    for(zeros; zeros > 0; zeros--) {
        strcat(immediate, "0");
    }
    strcat(immediate, str);
}

void negImmediate(int num, int size, char* immediate, char* str) {
    num *= -1;
    num = toBinary(num);
    int count = sprintf(str, "%d", num);
    int ones = size - count;
    for(ones; ones > 0; ones--) {
        strcat(immediate, "1");
    }
    for(int i = 0; i < strlen(str); i++) {
        if(str[i] == '1') {
            str[i] == '0';
        } else {
            str[i] == '1';
        }
    }
    for(int i = strlen(str); i >= 0; i--) {
        if(str[i] == '1') {
            str[i] == '0';
        } else {
            str[i] == '1';
            break;
        }
    }
    strcat(immediate, str);
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
    int flag = 0;
    int PC = 0;
    int start = 0;
    do {
        lRet = readAndParse(infile, lLine, &lLabel, &lOpcode, &lArg1,
                                &lArg2, &lArg3, &lArg4);
        if(lRet != DONE && lRet != EMPTY_LINE) {
            printf("%s, %s, %s, %s, %s, %s. \n", lLabel, lOpcode, lArg1,
                                        lArg2, lArg3, lArg4);
            if(!flag) {
                if(strcmp(lOpcode, ".orig")== 0) {
                    PC = toNum(lArg1);
                    start = PC;
                    if(PC%2 || (PC < 0x3000 || PC > 0x300C) || strcmp(lLabel, "") != 0) {
                        printf("bad .orig");
                        exit(3);
                    }
                } else {
                    printf("Found something before .orig/ no .orig");
                    exit(4);
                }
                flag = 1;     
            }
            
            if(strcmp(lLabel, "") != 0) {
                makeEntry(lLabel, PC);
            }
            if(strcmp(lOpcode, ".orig") == 0) {

            } else {
                PC+=0x0002;
            }
        }
    }while(lRet != DONE);

    infile = fopen(argv[1], "r");
    PC = start;
    char instr[17] = {0};
    do {
        //printf("%d", PC);
        lRet = readAndParse(infile, lLine, &lLabel, &lOpcode, &lArg1,
                                &lArg2, &lArg3, &lArg4);
        if(lRet != DONE && lRet != EMPTY_LINE) {
            if(lLabel != '\0' && isOpcode(lOpcode) == 0 && lOpcode[0] != '.') 
                exit(2);
            if(strcmp(lOpcode, "add") == 0) {                           //____________________ADD INSRUCTION____________________
                strcat(instr, "0001");
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {//Check registers validity
                   // printf("searching registers");
					strcat(instr, BinaryRegister(lArg1));
					strcat(instr, BinaryRegister(lArg2));
				} else {
		    		exit(4);
				}
				if(isRegister(lArg3) != 1) {
					strcat(instr, "1");
					int num = toNum(lArg3);
					if(num > 15 || num < -16) {
						exit(3);
					} else {
                        char immediate[6] = {0};
                        if(num >= 0) {
                            //printf("calc immediate");
                            char str[6] = {0};
                            posImmediate(num, 5, immediate, str);       
                            strcat(instr, immediate);
                            printf(instr);
                            printf("\n");
                        } else {
                            char str[6] = {0};
                            negImmediate(num, 5, immediate, str);    
                            strcat(instr, immediate);
                            printf(instr);
                            printf("\n");
                        }
                    }
				} else {
                    strcat(instr, "000");
                    strcat(instr, BinaryRegister(lArg3));
                    printf(instr);
                    printf("\n");
                }
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
			} else if(strcmp(lOpcode, "and") == 0) {                    //___________________AND INSTRUCTION____________________
                strcat(instr, "0101");
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                    strcat(instr, BinaryRegister(lArg2));
                } else {
                    exit(4);
                }
                if(isRegister(lArg3) != 1) {
                    strcat(instr, "1");
                    int num = toNum(lArg3);
                    if(num > 15 || num < -16) {
                        exit(3);
                    } else {
                        char immediate[6] = {0};
                        if(num >= 0) {
                            char str[6] = {0};
                            posImmediate(num, 5, immediate, str);
                            strcat(instr, immediate);
                            printf(instr);
                            printf("\n");
                        } else {
                            char str[6] = {0};
                            negImmediate(num, 5, immediate, str);
                            strcat(instr, immediate);
                            printf(instr);
                            printf("\n");
                        }
                    }
                } else {
                    strcat(instr, "000");
                    strcat(instr, BinaryRegister(lArg3));
                    printf(instr);
                    printf("\n");
                }
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
			} else if (    strcmp(lOpcode, "br") == 0|| strcmp(lOpcode, "brn") ==0   ||
                strcmp(lOpcode, "brp") == 0 ||
                strcmp(lOpcode, "brz") == 0 ||
             strcmp(lOpcode, "brzp") == 0 ||
                strcmp(lOpcode, "brn") == 0 ||
             strcmp(lOpcode, "brnp") == 0 ||
             strcmp(lOpcode, "brnzp") == 0) {
                char * bits =(char*) (malloc(4));
                strcat(instr, "0000");
    if (strcmp(lOpcode, "br") == 0) {
        bits = "111";
        strcat(instr,bits);
    }

    else if (strcmp(lOpcode, "brn") == 0) {
        bits="100";
        strcat(instr,bits);
    }

    else if (strcmp(lOpcode, "brp") == 0) {
        bits="001";
        strcat(instr,bits);
    }

    else if (strcmp(lOpcode, "brz") == 0) {
        bits="010";
        strcat(instr,bits);
     }
    else if (strcmp(lOpcode, "brzp") == 0) {
        bits="011";
        strcat(instr,bits);}

    else if (strcmp(lOpcode, "brnp") == 0) {
    bits="101";
    strcat(instr,bits);
    }
    else if (strcmp(lOpcode, "brn") == 0) {
    bits="100";
    strcat(instr,bits);        
    }  

    else if (strcmp(lOpcode, "brnzp") == 0) {
    bits="111";
    strcat(instr,bits);    
    }

    int label = symbolTableSearch(lArg1);
    int num = (label -(PC +2))/2;    
            if (num > 255 || num < -256) {
                exit(3);
            } else {
                char immediate[10] = {0};
                if (num >= 0) {
                    char str[10] = {0};
                    posImmediate(num, 9, immediate, str);
                    strcat(instr, immediate);
                    //printf(immediate);
                    printf(instr);
                    printf("\n");
                    fputs(toHex(instr), outfile);
                    fputs("\n",outfile);

                } else {
                    char str[10] = {0};
                    negImmediate(num, 9, immediate, str);
                    strcat(instr, immediate);
                    printf(instr);
                    printf("\n");
                    fputs(toHex(instr), outfile);
                    fputs("\n", outfile);
                }
            }
        } else if(strcmp(lOpcode, "jmp") == 0) {
                strcat(instr, "1100000");
                if(isRegister(lArg1) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                } else {
                    exit(4);
                }
                strcat(instr, "000000");
                printf(instr);
                printf("\n");
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "jsr") == 0) {
                strcat(instr, "01001");
                int label = symbolTableSearch(lArg1);
                int offset = (label -(PC + 2))/2;
                if(offset > 511 || offset < -512) {
                    exit(3);
                }
                char immediate[12] = {0};
                if(offset >= 0) {
                    char str[12] = {0};
                    posImmediate(offset, 11, immediate, str);
                    strcat(instr, immediate);
                    printf(instr);
                    printf("\n");
                    fputs(toHex(instr), outfile);
                    fputs("\n", outfile);
                } else {
                    char str[12] = {0};
                    negImmediate(offset, 11, immediate, str);
                    strcat(instr, immediate);
                    printf(instr);
                    printf("\n");
                    fputs(toHex(instr), outfile);
                    fputs("\n", outfile);
                }
            } else if(strcmp(lOpcode, "jsrr") == 0) {
                strcat(instr, "0100000");
                if(isRegister(lArg1) == 0) {
                    strcat(instr, BinaryRegister(lArg1));
                } else {
                    exit(4);
                }
                strcat(instr, "000000");
                printf(instr);
                printf("\n");
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "ldb") == 0 || strcmp(lOpcode, "ldw") == 0) {
                    if(strcmp(lOpcode, "ldb") == 0) {                
                        strcat(instr, "0010");
                    } else if(strcmp(lOpcode, "ldw") == 0) {
                        strcat(instr, "0110");
                    }
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                    strcat(instr, BinaryRegister(lArg2));
                } else {
                    exit(4);
                }
                if(isRegister(lArg3) != 1) {
                    int num = toNum(lArg3);
                    if(num > 31 || num < -32) {
                        exit(3);
                    }
                    char immediate[7] = {0};
                    if(num >= 0) {
                        char str[7] = {0};
                        posImmediate(num, 6, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                    } else {
                        char str[7] = {0};
                        negImmediate(num, 6, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                    }
                    fputs(toHex(instr), outfile);
                    fputs("\n", outfile);
                } else {
                    exit(4);
                }
            } else if(strcmp(lOpcode, "lea") == 0) {
                strcat(instr, "1110");
                if(isRegister(lArg1) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                } else {
                    exit(4);
                }
                int label = symbolTableSearch(lArg2);
                int offset = (label - (PC + 2))/2;

                if(offset > 255 || offset < -256) {
                    exit(3);
                } else {
                    char immediate[10] = {0};
                    if(offset >= 0) {
                        char str[10] = {0};
                        posImmediate(offset, 9, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                    } else {
                        char str[10] = {0};
                        negImmediate(offset, 9, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                    }
                }
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "not") == 0) {
                strcat(instr, "1001");
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                    strcat(instr, BinaryRegister(lArg2));
                } else {
                    exit(4);
                }
                strcat(instr, "111111");
                printf(instr);
                printf("\n");
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "ret") == 0) {
                strcat(instr, "1100000111000000");
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "rti") == 0) {
                strcat(instr, "1000000000000000");
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);  
            } else if(strcmp(lOpcode, "trap") == 0 || strcmp(lOpcode, "halt") == 0) {
                strcat(instr, "1111000000100101");
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "nop") == 0) {
                fputs("0x0000", outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, "stb") == 0 || strcmp(lOpcode, "stw") == 0) {
                if(strcmp(lOpcode, "stb") == 0) {
                    strcat(instr, "0011");
                } else if(strcmp(lOpcode, "stw") == 0) {
                    strcat(instr, "0111");
                }
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                    strcat(instr, BinaryRegister(lArg2));
                } else {
                    exit(4);
                }
				if(isRegister(lArg3) != 1) {
                    int num = toNum(lArg3);
                    if(num > 31 || num < -32) {
                        exit(3);
                    }
                    char immediate[7] = {0};
                    if(num >= 0) {
                        char str[7] = {0};
                        posImmediate(num, 6, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                    } else {
                        char str[7] = {0};
                        negImmediate(num, 6, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                    }
                    fputs(toHex(instr), outfile);
                    fputs("\n", outfile);
                } else {
                    exit(4);
                }
            } else if(strcmp(lOpcode, "lshf") == 0 || strcmp(lOpcode, "rshfl") == 0 || strcmp(lOpcode, "rshfa") == 0) {
                strcat(instr, "1101");
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                    strcat(instr, BinaryRegister(lArg2));
                }
                if(strcmp(lOpcode, "lshf") == 0) {
                    strcat(instr, "00");
                } else if(strcmp(lOpcode, "rshfl") == 0) {
                    strcat(instr, "01");
                } else if(strcmp(lOpcode, "rshfa") == 0) {
                    strcat(instr, "11");
                }
                if(isRegister(lArg3) != 1) {
                    int amount = toNum(lArg3);
                    if(amount > 8 || amount < -7) {
                        exit(3);
                    } 
                    char immediate[5] = {0};
                    if(amount >= 0) {
                        char str[5] = {0};
                        posImmediate(amount, 4, immediate, str);
                        strcat(instr, immediate);
                        printf(instr);
                        printf("\n");
                        fputs(toHex(instr), outfile);
                        fputs("\n", outfile);
                    } else {
                        char str[5] = {0};
                        negImmediate(amount, 4, immediate, str);
                        strcat(instr, immediate);
                        fputs(toHex(instr), outfile);
                        fputs("\n", outfile);
                    }
                }
            } else if(strcmp(lOpcode, "xor") == 0) {
                strcat(instr, "1001");
                if(isRegister(lArg1) == 1 && isRegister(lArg2) == 1) {
                    strcat(instr, BinaryRegister(lArg1));
                    strcat(instr, BinaryRegister(lArg2));
                } else {
                    exit(4);
                }
                if(isRegister(lArg3) != 1) {
                    strcat(instr, "1");
                    int num = toNum(lArg3);
                    if(num > 15 || num < -16) {
                        exit(3);
                    } else {
                        char immediate[6] = {0};
                        if(num >= 0) {
                            char str[6] = {0};
                            posImmediate(num, 5, immediate, str);
                            strcat(instr, immediate);
                        } else {
                            char str[6] = {0};
                            negImmediate(num , 5, immediate, str);
                            strcat(instr, immediate);
                        }
                    }    
                } else {
                    strcat(instr, "000");
                    strcat(instr, BinaryRegister(lArg3));
                }
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, ".fill") == 0) {
                char str[17] = {0};
                if(isRegister(lArg1) != 1) {
                    int num = toNum(lArg1);
                    if(num > 32767 || num < -32768) {
                        exit(3);
                    } else {
                        if(num >= 0) {
                            posImmediate(num, 16, instr, str);
                        } else {
                            negImmediate(num, 16, instr, str);
                        }
                    }
                } else {
                    if(strcmp(lArg1, "") == 0) {
                        exit(4);   
                    }   
                }
                fputs(toHex(instr), outfile);
                fputs("\n", outfile);
            } else if(strcmp(lOpcode, ".end") == 0)
                exit(0);
            if(strcmp(lOpcode, ".orig") == 0) {
                sprintf(instr, "%x", PC);
                fputs("0x", outfile);
                fputs(instr, outfile);
                fputs("\n", outfile); 
            } else {
                 PC +=0x0002;
            }
        }
        memset(instr, 0, sizeof(instr));
    }while(lRet != DONE);
    return 0;
}
