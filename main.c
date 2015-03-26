/*
** This file contains all sources (including headers) to the LEMON
** LALR(1) parser generator.  The sources have been combined into a
** single file to make it easy to include LEMON in the source tree
** and Makefile of another program.
**
** The author of this program disclaims copyright.
*/

#include "build.h"
#include "option.h"
#include "parse.h"
#include "report.h"
#include "set.h"
#include "lemon.h"
#include "table.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define MemoryCheck(X)          \
  if ((X) == 0) {               \
    extern void memory_error(); \
    memory_error();             \
  \
}

/*
** Main program file for the LEMON parser generator.
*/

/* forward declaration */
static void handle_D_option(char *);
static void handle_T_option(char *);

int nDefine = 0;     /* Number of -D options on the command line */
char **azDefine = 0; /* Name of the -D macros */
int showPrecedenceConflict = 0;
char *user_templatename = NULL;

/* The main program.  Parse the command line and do it... */
int
main(int argc, char **argv) {
  static int version = 0;
  static int rpflag = 0;
  static int basisflag = 0;
  static int compress = 0;
  static int quiet = 0;
  static int statistics = 0;
  static int mhflag = 0;
  static int nolinenosflag = 0;
  static int noResort = 0;
  static struct s_options options[] = {
      {OPT_FLAG, "b", (char *)&basisflag, "Print only the basis in report."},
      {OPT_FLAG, "c", (char *)&compress, "Don't compress the action table."},
      {OPT_FSTR, "D", (char *)handle_D_option, "Define an %ifdef macro."},
      {OPT_FSTR, "f", 0, "Ignored.  (Placeholder for -f compiler options.)"},
      {OPT_FLAG, "g", (char *)&rpflag, "Print grammar without actions."},
      {OPT_FSTR, "I", 0, "Ignored.  (Placeholder for '-I' compiler options.)"},
      {OPT_FLAG, "m", (char *)&mhflag, "Output a makeheaders compatible file."},
      {OPT_FLAG, "l", (char *)&nolinenosflag, "Do not print #line statements."},
      {OPT_FSTR, "O", 0, "Ignored.  (Placeholder for '-O' compiler options.)"},
      {OPT_FLAG, "p", (char *)&showPrecedenceConflict, "Show conflicts resolved by precedence rules"},
      {OPT_FLAG, "q", (char *)&quiet, "(Quiet) Don't print the report file."},
      {OPT_FLAG, "r", (char *)&noResort, "Do not sort or renumber states"},
      {OPT_FLAG, "s", (char *)&statistics, "Print parser stats to standard output."},
      {OPT_FLAG, "x", (char *)&version, "Print the version number."},
      {OPT_FSTR, "T", (char *)handle_T_option, "Specify a template file."},
      {OPT_FSTR, "W", 0, "Ignored.  (Placeholder for '-W' compiler options.)"},
      {OPT_FLAG, 0, 0, 0}};
  int i;
  int exitcode;
  struct lemon lem;

  OptInit(argv, options, stderr);
  if (version) {
    printf("Lemon version 1.0\n");
    exit(0);
  }
  if (OptNArgs() != 1) {
    fprintf(stderr, "Exactly one filename argument is required.\n");
    exit(1);
  }
  memset(&lem, 0, sizeof(lem));
  lem.errorcnt = 0;

  /* Initialize the machine */
  Strsafe_init();
  Symbol_init();
  State_init();
  lem.argv0 = argv[0];
  lem.filename = OptArg(0);
  lem.basisflag = basisflag;
  lem.nolinenosflag = nolinenosflag;
  Symbol_new("$");
  lem.errsym = Symbol_new("error");
  lem.errsym->useCnt = 0;

  /* Parse the input file */
  Parse(&lem);
  if (lem.errorcnt)
    exit(lem.errorcnt);
  if (lem.nrule == 0) {
    fprintf(stderr, "Empty grammar.\n");
    exit(1);
  }

  /* Count and index the symbols of the grammar */
  Symbol_new("{default}");
  lem.nsymbol = Symbol_count();
  lem.symbols = Symbol_arrayof();
  for (i = 0; i < lem.nsymbol; i++)
    lem.symbols[i]->index = i;
  qsort(lem.symbols, (size_t)lem.nsymbol, sizeof(struct symbol *), Symbolcmpp);
  for (i = 0; i < lem.nsymbol; i++)
    lem.symbols[i]->index = i;
  while (lem.symbols[i - 1]->type == MULTITERMINAL) {
    i--;
  }
  assert(strcmp(lem.symbols[i - 1]->name, "{default}") == 0);
  lem.nsymbol = i - 1;
  for (i = 1; isupper(lem.symbols[i]->name[0]); i++)
    ;
  lem.nterminal = i;

  /* Generate a reprint of the grammar, if requested on the command line */
  if (rpflag) {
    Reprint(&lem);
  } else {
    /* Initialize the size for all follow and first sets */
    SetSize(lem.nterminal + 1);

    /* Find the precedence for every production rule (that has one) */
    FindRulePrecedences(&lem);

    /* Compute the lambda-nonterminals and the first-sets for every
    ** nonterminal */
    FindFirstSets(&lem);

    /* Compute all LR(0) states.  Also record follow-set propagation
    ** links so that the follow-set can be computed later */
    lem.nstate = 0;
    FindStates(&lem);
    lem.sorted = State_arrayof();

    /* Tie up loose ends on the propagation links */
    FindLinks(&lem);

    /* Compute the follow set of every reducible configuration */
    FindFollowSets(&lem);

    /* Compute the action tables */
    FindActions(&lem);

    /* Compress the action tables */
    if (compress == 0)
      CompressTables(&lem);

    /* Reorder and renumber the states so that states with fewer choices
    ** occur at the end.  This is an optimization that helps make the
    ** generated parser tables smaller. */
    if (noResort == 0)
      ResortStates(&lem);

    /* Generate a report of the parser generated.  (the "y.output" file) */
    if (!quiet)
      ReportOutput(&lem);

    /* Generate the source code for the parser */
    ReportTable(&lem, mhflag);

    /* Produce a header file for use by the scanner.  (This step is
    ** omitted if the "-m" option is used because makeheaders will
    ** generate the file for us.) */
    if (!mhflag)
      ReportHeader(&lem);
  }
  if (statistics) {
    printf("Parser statistics: %d terminals, %d nonterminals, %d rules\n", lem.nterminal, lem.nsymbol - lem.nterminal,
           lem.nrule);
    printf("                   %d states, %d parser table entries, %d conflicts\n", lem.nstate, lem.tablesize,
           lem.nconflict);
  }
  if (lem.nconflict > 0) {
    fprintf(stderr, "%d parsing conflicts.\n", lem.nconflict);
  }

  /* return 0 on success, 1 on failure. */
  exitcode = ((lem.errorcnt > 0) || (lem.nconflict > 0)) ? 1 : 0;
  return (exitcode);
}

/* This routine is called with the argument to each -D command-line option.
** Add the macro defined to the azDefine array.
*/
static void
handle_D_option(char *z) {
  char **paz;
  nDefine++;
  azDefine = (char **)realloc(azDefine, sizeof(azDefine[0]) * nDefine);
  if (azDefine == 0) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  paz = &azDefine[nDefine - 1];
  *paz = (char *)malloc(strlen(z) + 1);
  if (*paz == 0) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  strcpy(*paz, z);
  for (z = *paz; *z && *z != '='; z++) {
  }
  *z = 0;
}

static void
handle_T_option(char *z) {
  user_templatename = (char *)malloc(strlen(z) + 1);
  MemoryCheck(user_templatename);
  strcpy(user_templatename, z);
}
