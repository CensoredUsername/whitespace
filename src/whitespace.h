/* A whitespace compiler */
#ifndef WHITESPACE_H
#define WHITESPACE_H

#include "wstypes.h"
#include "wsparser.h"
#include "wsserialize.h"
#include "wscompiler.h"
#include "wsmachine.h"

/* ok, so how does this work.
 * wstypes.h contains the defintions of all non-ws-runtime data types used by the program,
 * wsparser.h contains the code necessary to parse the whitespace code into a data structure
 * wscompile.h compiles this structure by replacing any labels by instruction indexes in the data structure
 * wsserialize.h can convert these data structures into a string format for serialization purposes
 * wsmachine.h contains a full implementation of the intepreter executing these commands
 */ 

int main(int argc, char **argv);

#endif