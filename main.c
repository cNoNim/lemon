#include "arg.h"
#include "error.h"

#include "build.h"
#include "parse.h"
#include "report.h"
#include "set.h"
#include "lemon.h"
#include "table.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* forward declaration */
static void handle_D_option(char *);
static void handle_T_option(char *);
static void usage();

char * argv0;

int nDefine = 0;     // Number of -D options on the command line
char **azDefine = 0; // Name of the -D macros
bool showPrecedenceConflict = false;
char *user_templatename = NULL;

/* The main program.  Parse the command line and do it... */
int
main(int argc, char **argv) {
  static bool rpflag = false;
  static bool basisflag = false;
  static bool compress = false;
  static bool quiet = false;
  static bool statistics = false;
  static bool nolinenosflag = false;
  static bool noResort = false;
  // TODO: remove this flag
  static bool mhflag = true;

  int i;
  int exitcode;
  struct lemon lem;

  ARGBEGIN {
    case 'D':
      handle_D_option(ARGF());
      break;
    case 'T':
      handle_T_option(ARGF());
      break;
    case 'b':
      basisflag = true;
      break;
    case 'c':
      compress = true;
      break;
    case 'g':
      rpflag = true;
      break;
    case 'l':
      nolinenosflag = true;
      break;
    case 'p':
      showPrecedenceConflict = true;
      break;
    case 'q':
      quiet = true;
      break;
    case 'r':
      noResort = true;
      break;
    case 's':
      statistics = true;
      break;
    case 'V':
      lprintf(LINFO, "Lemon version 1.0");
      exit(EXIT_SUCCESS);
    default:
    case 'h':
    case '?':
      usage();
  } ARGEND;
  if (argc != 1) {
    lprintf(LFATAL, "Exactly one filename argument is required.");
  }
  memset(&lem, 0, sizeof(lem));
  lem.errorcnt = 0;

  /* Initialize the machine */
  Strsafe_init();
  Symbol_init();
  State_init();
  lem.argv0 = argv0;
  lem.filename = argv[0];
  lem.basisflag = basisflag;
  lem.nolinenosflag = nolinenosflag;
  Symbol_new("$");
  lem.errsym = Symbol_new("error");
  lem.errsym->useCnt = 0;

  /* Parse the input file */
  Parse(&lem);
  if (lem.errorcnt)
    exit(EXIT_FAILURE);
  if (lem.nrule == 0) {
    lprintf(LFATAL, "Empty grammar.");
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
     * nonterminal
     */
    FindFirstSets(&lem);

    /* Compute all LR(0) states.  Also record follow-set propagation
     * links so that the follow-set can be computed later
     */
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
     * occur at the end.  This is an optimization that helps make the
     * generated parser tables smaller.
     */
    if (noResort == 0)
      ResortStates(&lem);

    /* Generate a report of the parser generated.  (the "y.output" file) */
    if (!quiet)
      ReportOutput(&lem);

    /* Generate the source code for the parser */
    ReportTable(&lem, mhflag);

    /* Produce a header file for use by the scanner.  (This step is
     * omitted if the "-m" option is used because makeheaders will
     * generate the file for us.)
     */
    if (!mhflag)
      ReportHeader(&lem);
  }
  if (statistics) {
    lprintf(LINFO,
            "Parser statistics: %d terminals, %d nonterminals, %d rules\n"
            "\t%d states, %d parser table entries, %d conflicts\n",
            lem.nterminal, lem.nsymbol - lem.nterminal, lem.nrule, lem.nstate,
            lem.tablesize, lem.nconflict);
  }
  if (lem.nconflict > 0) {
    lprintf(LERROR, "%d parsing conflicts.\n", lem.nconflict);
  }

  /* return 0 on success, 1 on failure. */
  exitcode = ((lem.errorcnt > 0) || (lem.nconflict > 0)) ? 1 : 0;
  return (exitcode);
}

/* This routine is called with the argument to each -D command-line option.
 * Add the macro defined to the azDefine array.
 */
static void
handle_D_option(char *z) {
  char **paz;
  nDefine++;
  azDefine = (char **)realloc(azDefine, sizeof(azDefine[0]) * nDefine);
  MemoryCheck(azDefine);
  paz = &azDefine[nDefine - 1];
  *paz = (char *)malloc(strlen(z) + 1);
  MemoryCheck(*paz);
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


static void
usage() {
  fprintf(stderr,
          "usage: %s -h\n"
          "usage: %s -V\n"
          "usage: %s [-bcglpqrs] [-D define] [-T template] grammar\n"
          "\t-b\tPrint only the basis in report.\n"
          "\t-c\tDon't compress the action table.\n"
          "\t-g\tPrint grammar without actions.\n"
          "\t-l\tDo not print #line statements.\n"
          "\t-p\tShow conflicts resolved by precedence rules\n"
          "\t-q\t(Quiet) Don't print the report file.\n"
          "\t-r\tDo not sort or renumber states\n"
          "\t-s\tPrint parser stats to standard output.\n"
          "\t-T\tSpecify a template file.\n"
          "\t-D\tDefine an %%ifdef macro.\n"
          "\t-h\tPrint usage infirmation.\n"
          "\t-V\tPrint the version number.\n", argv0, argv0, argv0);
  exit(EXIT_SUCCESS);
}
