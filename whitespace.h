/* A whitespace compiler */
#ifndef WHITESPACE_H
#define WHITESPACE_H

#define DEBUG 0

#define SPACE ' '
#define TAB '\t'
#define BREAK '\n'

#include "wstypes.h"
#include "wsparser.h"
#include "wsserialize.h"
#include "wscompiler.h"
#include "wsmachine.h"

/* ok, so how does this work.
 * wstypes.h and wsint.h contain definitions of the ws_string and ws_int types used to ease data handling.
 * wsparser.h parses whitespace code into a simple data structure
 * wsserialize.h can convert these data structures to a serialized format for inspection purposes
 * wscompile.h processes the parsed data structure, so wsmachine.h can run it 
 */ 

int main(int argc, char **argv);

#endif