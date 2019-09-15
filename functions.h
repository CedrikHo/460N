//
// Created by cedri on 9/9/2019.
//
#include <stdio.h>    /* standard input/output library */
#include <stdlib.h>    /* Standard C Library */
#include <string.h>    /* String operations library */
#include <ctype.h>    /* Library for useful character operations */
#include <limits.h>    /* Library for definitions of common variable type characteristics */

#include <strings.h>
#ifndef LAB1EE460N_FUNCTIONS_H
#define LAB1EE460N_FUNCTIONS_H

int insertIntoSymbolTable(char label[], int address);
int isValidlabel(char label[]);
int isalphanumeric(char label[]);
int searchLabelsForRestricted(char *label);
char *strlwr(char *str);


#endif //LAB1EE460N_FUNCTIONS_H
