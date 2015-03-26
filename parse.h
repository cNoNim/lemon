#ifndef _LEMON_PARSE_H_
#define _LEMON_PARSE_H_

/*
 * Input file parser for the LEMON parser generator.
 */

#define MAXRHS 1000

struct lemon;

void Parse(struct lemon *);

#endif //_LEMON_PARSE_H_
