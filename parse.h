#ifndef _LEMON_PARSE_H_
#define _LEMON_PARSE_H_

#ifdef TEST
#define MAXRHS 5       /* Set low to exercise exception code */
#else
#define MAXRHS 1000
#endif

struct lemon;

void Parse(struct lemon *lemp);

#endif //_LEMON_PARSE_H_
