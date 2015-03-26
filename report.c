#include "action.h"
#include "acttab.h"
#include "configlist.h"
#include "error.h"
#include "parse.h"
#include "set.h"
#include "lemon.h"
#include "table.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
#ifdef __cplusplus
extern "C" {
#endif
extern int access(const char *path, int mode);
#ifdef __cplusplus
}
#endif
#else
#include <unistd.h>
#endif

#define LINESIZE 1000
#define NO_OFFSET (-2147483647)

// TODO: move
extern int showPrecedenceConflict;
extern char *user_templatename;

/* Generate a filename with the given suffix.  Space to hold the
 * name comes from malloc() and must be freed by the calling
 * function.
 */
static char *
file_makename(struct lemon *lemp, const char *suffix) {
  char *name;
  char *cp;

  name = (char *)malloc(strlen(lemp->filename) + strlen(suffix) + 5);
  if (name == 0) {
    fprintf(stderr, "Can't allocate space for a filename.\n");
    exit(1);
  }
  strcpy(name, lemp->filename);
  cp = strrchr(name, '.');
  if (cp)
    *cp = 0;
  strcat(name, suffix);
  return name;
}

/* Open a file with a name based on the name of the input file,
 * but with a different (specified) suffix, and return a pointer
 * to the stream
 */
static FILE *
file_open(struct lemon *lemp, const char *suffix, const char *mode) {
  FILE *fp;

  if (lemp->outname)
    free(lemp->outname);
  lemp->outname = file_makename(lemp, suffix);
  fp = fopen(lemp->outname, mode);
  if (fp == 0 && *mode == 'w') {
    fprintf(stderr, "Can't open file \"%s\".\n", lemp->outname);
    lemp->errorcnt++;
    return 0;
  }
  return fp;
}

/* Duplicate the input file without comments and without actions
 * on rules
 */
void
Reprint(struct lemon *lemp) {
  struct rule *rp;
  struct symbol *sp;
  int i, j, maxlen, len, ncolumns, skip;
  printf("// Reprint of input file \"%s\".\n// Symbols:\n", lemp->filename);
  maxlen = 10;
  for (i = 0; i < lemp->nsymbol; i++) {
    sp = lemp->symbols[i];
    len = strlen(sp->name);
    if (len > maxlen)
      maxlen = len;
  }
  ncolumns = 76 / (maxlen + 5);
  if (ncolumns < 1)
    ncolumns = 1;
  skip = (lemp->nsymbol + ncolumns - 1) / ncolumns;
  for (i = 0; i < skip; i++) {
    printf("//");
    for (j = i; j < lemp->nsymbol; j += skip) {
      sp = lemp->symbols[j];
      assert(sp->index == j);
      printf(" %3d %-*.*s", j, maxlen, maxlen, sp->name);
    }
    printf("\n");
  }
  for (rp = lemp->rule; rp; rp = rp->next) {
    printf("%s", rp->lhs->name);
    printf(" ::=");
    for (i = 0; i < rp->nrhs; i++) {
      sp = rp->rhs[i];
      if (sp->type == MULTITERMINAL) {
        printf(" %s", sp->subsym[0]->name);
        for (j = 1; j < sp->nsubsym; j++) {
          printf("|%s", sp->subsym[j]->name);
        }
      } else {
        printf(" %s", sp->name);
      }
    }
    printf(".");
    if (rp->precsym)
      printf(" [%s]", rp->precsym->name);
    printf("\n");
  }
}

void
ConfigPrint(FILE *fp, struct config *cfp) {
  struct rule *rp;
  struct symbol *sp;
  int i, j;
  rp = cfp->rp;
  fprintf(fp, "%s ::=", rp->lhs->name);
  for (i = 0; i <= rp->nrhs; i++) {
    if (i == cfp->dot)
      fprintf(fp, " *");
    if (i == rp->nrhs)
      break;
    sp = rp->rhs[i];
    if (sp->type == MULTITERMINAL) {
      fprintf(fp, " %s", sp->subsym[0]->name);
      for (j = 1; j < sp->nsubsym; j++) {
        fprintf(fp, "|%s", sp->subsym[j]->name);
      }
    } else {
      fprintf(fp, " %s", sp->name);
    }
  }
}

/* Print an action to the given file descriptor.  Return FALSE if
 * nothing was actually printed.
 */
int
PrintAction(struct action *ap, FILE *fp, int indent) {
  int result = 1;
  switch (ap->type) {
  case SHIFT:
    fprintf(fp, "%*s shift  %d", indent, ap->sp->name, ap->x.stp->statenum);
    break;
  case REDUCE:
    fprintf(fp, "%*s reduce %d", indent, ap->sp->name, ap->x.rp->index);
    break;
  case ACCEPT:
    fprintf(fp, "%*s accept", indent, ap->sp->name);
    break;
  case ERROR:
    fprintf(fp, "%*s error", indent, ap->sp->name);
    break;
  case SRCONFLICT:
  case RRCONFLICT:
    fprintf(fp, "%*s reduce %-3d ** Parsing conflict **", indent, ap->sp->name, ap->x.rp->index);
    break;
  case SSCONFLICT:
    fprintf(fp, "%*s shift  %-3d ** Parsing conflict **", indent, ap->sp->name, ap->x.stp->statenum);
    break;
  case SH_RESOLVED:
    if (showPrecedenceConflict) {
      fprintf(fp, "%*s shift  %-3d -- dropped by precedence", indent, ap->sp->name, ap->x.stp->statenum);
    } else {
      result = 0;
    }
    break;
  case RD_RESOLVED:
    if (showPrecedenceConflict) {
      fprintf(fp, "%*s reduce %-3d -- dropped by precedence", indent, ap->sp->name, ap->x.rp->index);
    } else {
      result = 0;
    }
    break;
  case NOT_USED:
    result = 0;
    break;
  }
  return result;
}

/* Generate the "y.output" log file */
void
ReportOutput(struct lemon *lemp) {
  int i;
  struct state *stp;
  struct config *cfp;
  struct action *ap;
  FILE *fp;

  fp = file_open(lemp, ".out", "wb");
  if (fp == 0)
    return;
  for (i = 0; i < lemp->nstate; i++) {
    stp = lemp->sorted[i];
    fprintf(fp, "State %d:\n", stp->statenum);
    if (lemp->basisflag)
      cfp = stp->bp;
    else
      cfp = stp->cfp;
    while (cfp) {
      char buf[20];
      if (cfp->dot == cfp->rp->nrhs) {
        sprintf(buf, "(%d)", cfp->rp->index);
        fprintf(fp, "    %5s ", buf);
      } else {
        fprintf(fp, "          ");
      }
      ConfigPrint(fp, cfp);
      fprintf(fp, "\n");
      if (lemp->basisflag)
        cfp = cfp->bp;
      else
        cfp = cfp->next;
    }
    fprintf(fp, "\n");
    for (ap = stp->ap; ap; ap = ap->next) {
      if (PrintAction(ap, fp, 30))
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "----------------------------------------------------\n");
  fprintf(fp, "Symbols:\n");
  for (i = 0; i < lemp->nsymbol; i++) {
    int j;
    struct symbol *sp;

    sp = lemp->symbols[i];
    fprintf(fp, "  %3d: %s", i, sp->name);
    if (sp->type == NONTERMINAL) {
      fprintf(fp, ":");
      if (sp->lambda) {
        fprintf(fp, " <lambda>");
      }
      for (j = 0; j < lemp->nterminal; j++) {
        if (sp->firstset && SetFind(sp->firstset, j)) {
          fprintf(fp, " %s", lemp->symbols[j]->name);
        }
      }
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
  return;
}

/* Search for the file "name" which is in the same directory as
 * the exacutable
 */
static char *
pathsearch(char *argv0, char *name, int modemask) {
  const char *pathlist;
  char *pathbufptr;
  char *pathbuf;
  char *path, *cp;
  char c;

#ifdef __WIN32__
  cp = strrchr(argv0, '\\');
#else
  cp = strrchr(argv0, '/');
#endif
  if (cp) {
    c = *cp;
    *cp = 0;
    path = (char *)malloc(strlen(argv0) + strlen(name) + 2);
    if (path)
      sprintf(path, "%s/%s", argv0, name);
    *cp = c;
  } else {
    pathlist = getenv("PATH");
    if (pathlist == 0)
      pathlist = ".:/bin:/usr/bin";
    pathbuf = (char *)malloc(strlen(pathlist) + 1);
    path = (char *)malloc(strlen(pathlist) + strlen(name) + 2);
    if ((pathbuf != 0) && (path != 0)) {
      pathbufptr = pathbuf;
      strcpy(pathbuf, pathlist);
      while (*pathbuf) {
        cp = strchr(pathbuf, ':');
        if (cp == 0)
          cp = &pathbuf[strlen(pathbuf)];
        c = *cp;
        *cp = 0;
        sprintf(path, "%s/%s", pathbuf, name);
        *cp = c;
        if (c == 0)
          pathbuf[0] = 0;
        else
          pathbuf = &cp[1];
        if (access(path, modemask) == 0)
          break;
      }
      free(pathbufptr);
    }
  }
  return path;
}

/* Given an action, compute the integer value for that action
 * which is to be put in the action table of the generated machine.
 * Return negative if no action should be generated.
 */
static int
compute_action(struct lemon *lemp, struct action *ap) {
  int act;
  switch (ap->type) {
  case SHIFT:
    act = ap->x.stp->statenum;
    break;
  case REDUCE:
    act = ap->x.rp->index + lemp->nstate;
    break;
  case ERROR:
    act = lemp->nstate + lemp->nrule;
    break;
  case ACCEPT:
    act = lemp->nstate + lemp->nrule + 1;
    break;
  default:
    act = -1;
    break;
  }
  return act;
}

/* The next cluster of routines are for reading the template file
 * and writing the results to the generated parser
 * The first function transfers data from "in" to "out" until
 * a line is seen which begins with "%%".  The line number is
 * tracked.
 *
 * if name!=0, then any word that begin with "Parse" is changed to
 * begin with *name instead.
 */
static void
tplt_xfer(char *name, FILE *in, FILE *out, int *lineno) {
  int i, iStart;
  char line[LINESIZE];
  while (fgets(line, LINESIZE, in) && (line[0] != '%' || line[1] != '%')) {
    (*lineno)++;
    iStart = 0;
    if (name) {
      for (i = 0; line[i]; i++) {
        if (line[i] == 'P' && strncmp(&line[i], "Parse", 5) == 0 && (i == 0 || !isalpha(line[i - 1]))) {
          if (i > iStart)
            fprintf(out, "%.*s", i - iStart, &line[iStart]);
          fprintf(out, "%s", name);
          i += 4;
          iStart = i + 1;
        }
      }
    }
    fprintf(out, "%s", &line[iStart]);
  }
}

/* The next function finds the template file and opens it, returning
 * a pointer to the opened file.
 */
static FILE *
tplt_open(struct lemon *lemp) {
  static char templatename[] = "lempar.c";
  char buf[1000];
  FILE *in;
  char *tpltname;
  char *cp;

  /* first, see if user specified a template filename on the command line. */
  if (user_templatename != 0) {
    if (access(user_templatename, 004) == -1) {
      fprintf(stderr, "Can't find the parser driver template file \"%s\".\n", user_templatename);
      lemp->errorcnt++;
      return 0;
    }
    in = fopen(user_templatename, "rb");
    if (in == 0) {
      fprintf(stderr, "Can't open the template file \"%s\".\n", user_templatename);
      lemp->errorcnt++;
      return 0;
    }
    return in;
  }

  cp = strrchr(lemp->filename, '.');
  if (cp) {
    sprintf(buf, "%.*s.lt", (int)(cp - lemp->filename), lemp->filename);
  } else {
    sprintf(buf, "%s.lt", lemp->filename);
  }
  if (access(buf, 004) == 0) {
    tpltname = buf;
  } else if (access(templatename, 004) == 0) {
    tpltname = templatename;
  } else {
    tpltname = pathsearch(lemp->argv0, templatename, 0);
  }
  if (tpltname == 0) {
    fprintf(stderr, "Can't find the parser driver template file \"%s\".\n", templatename);
    lemp->errorcnt++;
    return 0;
  }
  in = fopen(tpltname, "rb");
  if (in == 0) {
    fprintf(stderr, "Can't open the template file \"%s\".\n", templatename);
    lemp->errorcnt++;
    return 0;
  }
  return in;
}

/* Print a #line directive line to the output file. */
static void
tplt_linedir(FILE *out, int lineno, char *filename) {
  fprintf(out, "#line %d \"", lineno);
  while (*filename) {
    if (*filename == '\\')
      putc('\\', out);
    putc(*filename, out);
    filename++;
  }
  fprintf(out, "\"\n");
}

/* Print a string to the file and keep the linenumber up to date */
static void
tplt_print(FILE *out, struct lemon *lemp, char *str, int *lineno) {
  if (str == 0)
    return;
  while (*str) {
    putc(*str, out);
    if (*str == '\n')
      (*lineno)++;
    str++;
  }
  if (str[-1] != '\n') {
    putc('\n', out);
    (*lineno)++;
  }
  if (!lemp->nolinenosflag) {
    (*lineno)++;
    tplt_linedir(out, *lineno, lemp->outname);
  }
  return;
}

/* The following routine emits code for the destructor for the
 * symbol sp
 */
void
emit_destructor_code(FILE *out, struct symbol *sp, struct lemon *lemp, int *lineno) {
  char *cp = 0;

  if (sp->type == TERMINAL) {
    cp = lemp->tokendest;
    if (cp == 0)
      return;
    fprintf(out, "{\n");
    (*lineno)++;
  } else if (sp->destructor) {
    cp = sp->destructor;
    fprintf(out, "{\n");
    (*lineno)++;
    if (!lemp->nolinenosflag) {
      (*lineno)++;
      tplt_linedir(out, sp->destLineno, lemp->filename);
    }
  } else if (lemp->vardest) {
    cp = lemp->vardest;
    if (cp == 0)
      return;
    fprintf(out, "{\n");
    (*lineno)++;
  } else {
    assert(0); // Cannot happen
  }
  for (; *cp; cp++) {
    if (*cp == '$' && cp[1] == '$') {
      fprintf(out, "(yypminor->yy%d)", sp->dtnum);
      cp++;
      continue;
    }
    if (*cp == '\n')
      (*lineno)++;
    fputc(*cp, out);
  }
  fprintf(out, "\n");
  (*lineno)++;
  if (!lemp->nolinenosflag) {
    (*lineno)++;
    tplt_linedir(out, *lineno, lemp->outname);
  }
  fprintf(out, "}\n");
  (*lineno)++;
  return;
}

/* Return TRUE (non-zero) if the given symbol has a destructor. */
int
has_destructor(struct symbol *sp, struct lemon *lemp) {
  int ret;
  if (sp->type == TERMINAL) {
    ret = lemp->tokendest != 0;
  } else {
    ret = lemp->vardest != 0 || sp->destructor != 0;
  }
  return ret;
}

/* Append text to a dynamically allocated string.  If zText is 0 then
 * reset the string to be empty again.  Always return the complete text
 * of the string (which is overwritten with each call).
 *
 * n bytes of zText are stored.  If n==0 then all of zText up to the first
 * \000 terminator is stored.  zText can contain up to two instances of
 * %d.  The values of p1 and p2 are written into the first and second
 * %d.
 *
 * If n==-1, then the previous character is overwritten.
 */
static char *
append_str(const char *zText, int n, int p1, int p2) {
  static char empty[1] = {0};
  static char *z = 0;
  static size_t alloced = 0;
  static int used = 0;
  int c;
  char zInt[40];
  if (zText == 0) {
    used = 0;
    return z;
  }
  if (n <= 0) {
    if (n < 0) {
      used += n;
      assert(used >= 0);
    }
    n = (int)strlen(zText);
  }
  if ((int)(n + sizeof(zInt) * 2 + used) >= alloced) {
    alloced = n + sizeof(zInt) * 2 + used + 200;
    z = (char *)realloc(z, alloced);
  }
  if (z == 0)
    return empty;
  while (n-- > 0) {
    c = *(zText++);
    if (c == '%' && n > 0 && zText[0] == 'd') {
      sprintf(zInt, "%d", p1);
      p1 = p2;
      strcpy(&z[used], zInt);
      used += strlen(&z[used]);
      zText++;
      n--;
    } else {
      z[used++] = (char)c;
    }
  }
  z[used] = 0;
  return z;
}

/* zCode is a string that is the action associated with a rule.  Expand
 * the symbols in this string so that the refer to elements of the parser
 * stack.
 */
static void
translate_code(struct lemon *lemp, struct rule *rp) {
  char *cp, *xp;
  int i;
  char lhsused = 0;  // True if the LHS element has been used
  char used[MAXRHS]; // True for each RHS element which is used

  for (i = 0; i < rp->nrhs; i++)
    used[i] = 0;
  lhsused = 0;

  if (rp->code == 0) {
    static char newlinestr[2] = {'\n', '\0'};
    rp->code = newlinestr;
    rp->line = rp->ruleline;
  }

  append_str(0, 0, 0, 0);

  /* This const cast is wrong but harmless, if we're careful. */
  for (cp = (char *)rp->code; *cp; cp++) {
    if (isalpha(*cp) && (cp == rp->code || (!isalnum(cp[-1]) && cp[-1] != '_'))) {
      char saved;
      for (xp = &cp[1]; isalnum(*xp) || *xp == '_'; xp++)
        ;
      saved = *xp;
      *xp = 0;
      if (rp->lhsalias && strcmp(cp, rp->lhsalias) == 0) {
        append_str("yygotominor.yy%d", 0, rp->lhs->dtnum, 0);
        cp = xp;
        lhsused = 1;
      } else {
        for (i = 0; i < rp->nrhs; i++) {
          if (rp->rhsalias[i] && strcmp(cp, rp->rhsalias[i]) == 0) {
            if (cp != rp->code && cp[-1] == '@') {
              /* If the argument is of the form @X then substituted
               * the token number of X, not the value of X
               */
              append_str("yymsp[%d].major", -1, i - rp->nrhs + 1, 0);
            } else {
              struct symbol *sp = rp->rhs[i];
              int dtnum;
              if (sp->type == MULTITERMINAL) {
                dtnum = sp->subsym[0]->dtnum;
              } else {
                dtnum = sp->dtnum;
              }
              append_str("yymsp[%d].minor.yy%d", 0, i - rp->nrhs + 1, dtnum);
            }
            cp = xp;
            used[i] = 1;
            break;
          }
        }
      }
      *xp = saved;
    }
    append_str(cp, 1, 0, 0);
  }

  /* Check to make sure the LHS has been used */
  if (rp->lhsalias && !lhsused) {
    ErrorMsg(lemp->filename, rp->ruleline, "Label \"%s\" for \"%s(%s)\" is never used.", rp->lhsalias, rp->lhs->name,
             rp->lhsalias);
    lemp->errorcnt++;
  }

  /* Generate destructor code for RHS symbols which are not used in the
   * reduce code
   */
  for (i = 0; i < rp->nrhs; i++) {
    if (rp->rhsalias[i] && !used[i]) {
      ErrorMsg(lemp->filename, rp->ruleline, "Label %s for \"%s(%s)\" is never used.", rp->rhsalias[i],
               rp->rhs[i]->name, rp->rhsalias[i]);
      lemp->errorcnt++;
    } else if (rp->rhsalias[i] == 0) {
      if (has_destructor(rp->rhs[i], lemp)) {
        append_str("  yy_destructor(yypParser,%d,&yymsp[%d].minor);\n", 0, rp->rhs[i]->index, i - rp->nrhs + 1);
      } else {
        /* No destructor defined for this term */
      }
    }
  }
  if (rp->code) {
    cp = append_str(0, 0, 0, 0);
    rp->code = Strsafe(cp ? cp : "");
  }
}

/* Generate code which executes when the rule "rp" is reduced.  Write
 * the code to "out".  Make sure lineno stays up-to-date.
 */
static void
emit_code(FILE *out, struct rule *rp, struct lemon *lemp, int *lineno) {
  const char *cp;

  /* Generate code to do the reduce action */
  if (rp->code) {
    if (!lemp->nolinenosflag) {
      (*lineno)++;
      tplt_linedir(out, rp->line, lemp->filename);
    }
    fprintf(out, "{%s", rp->code);
    for (cp = rp->code; *cp; cp++) {
      if (*cp == '\n')
        (*lineno)++;
    }
    fprintf(out, "}\n");
    (*lineno)++;
    if (!lemp->nolinenosflag) {
      (*lineno)++;
      tplt_linedir(out, *lineno, lemp->outname);
    }
  }

  return;
}

/* Print the definition of the union used for the parser's data stack.
 * This union contains fields for every possible data type for tokens
 * and nonterminals.  In the process of computing and printing this
 * union, also set the ".dtnum" field of every terminal and nonterminal
 * symbol.
 */
void
print_stack_union(FILE *out,          // The output stream
                  struct lemon *lemp, // The main info structure for this parser
                  int *plineno,       // Pointer to the line number
                  int mhflag          // True if generating makeheaders output
                  ) {
  int lineno = *plineno; // The line number of the output
  char **types;          // A hash table of datatypes
  size_t arraysize;      // Size of the "types" array
  size_t maxdtlength;    // Maximum length of any ".datatype" field.
  char *stddt;           // Standardized name for a datatype
  int i, j;              // Loop counters
  unsigned hash;         // For hashing the name of a type
  const char *name;      // Name of the parser

  /* Allocate and initialize types[] and allocate stddt[] */
  arraysize = (size_t)(lemp->nsymbol * 2);
  types = (char **)calloc(arraysize, sizeof(char *));
  if (types == 0) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  for (i = 0; i < arraysize; i++)
    types[i] = 0;
  maxdtlength = 0;
  if (lemp->vartype) {
    maxdtlength = strlen(lemp->vartype);
  }
  for (i = 0; i < lemp->nsymbol; i++) {
    size_t len;
    struct symbol *sp = lemp->symbols[i];
    if (sp->datatype == 0)
      continue;
    len = strlen(sp->datatype);
    if (len > maxdtlength)
      maxdtlength = len;
  }
  stddt = (char *)malloc(maxdtlength * 2 + 1);
  if (stddt == 0) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  /* Build a hash table of datatypes. The ".dtnum" field of each symbol
   * is filled in with the hash index plus 1.  A ".dtnum" value of 0 is
   * used for terminal symbols.  If there is no %default_type defined then
   * 0 is also used as the .dtnum value for nonterminals which do not specify
   * a datatype using the %type directive.
   */
  for (i = 0; i < lemp->nsymbol; i++) {
    struct symbol *sp = lemp->symbols[i];
    char *cp;
    if (sp == lemp->errsym) {
      sp->dtnum = arraysize + 1;
      continue;
    }
    if (sp->type != NONTERMINAL || (sp->datatype == 0 && lemp->vartype == 0)) {
      sp->dtnum = 0;
      continue;
    }
    cp = sp->datatype;
    if (cp == 0)
      cp = lemp->vartype;
    j = 0;
    while (isspace(*cp))
      cp++;
    while (*cp)
      stddt[j++] = *cp++;
    while (j > 0 && isspace(stddt[j - 1]))
      j--;
    stddt[j] = 0;
    if (lemp->tokentype && strcmp(stddt, lemp->tokentype) == 0) {
      sp->dtnum = 0;
      continue;
    }
    hash = 0;
    for (j = 0; stddt[j]; j++) {
      hash = hash * 53 + stddt[j];
    }
    hash = (hash & 0x7fffffff) % arraysize;
    while (types[hash]) {
      if (strcmp(types[hash], stddt) == 0) {
        sp->dtnum = hash + 1;
        break;
      }
      hash++;
      if (hash >= (unsigned)arraysize)
        hash = 0;
    }
    if (types[hash] == 0) {
      sp->dtnum = hash + 1;
      types[hash] = (char *)malloc(strlen(stddt) + 1);
      if (types[hash] == 0) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
      }
      strcpy(types[hash], stddt);
    }
  }

  /* Print out the definition of YYTOKENTYPE and YYMINORTYPE */
  name = lemp->name ? lemp->name : "Parse";
  lineno = *plineno;
  if (mhflag) {
    fprintf(out, "#if INTERFACE\n");
    lineno++;
  }
  fprintf(out, "#define %sTOKENTYPE %s\n", name, lemp->tokentype ? lemp->tokentype : "void*");
  lineno++;
  if (mhflag) {
    fprintf(out, "#endif\n");
    lineno++;
  }
  fprintf(out, "typedef union {\n");
  lineno++;
  fprintf(out, "  int yyinit;\n");
  lineno++;
  fprintf(out, "  %sTOKENTYPE yy0;\n", name);
  lineno++;
  for (i = 0; i < arraysize; i++) {
    if (types[i] == 0)
      continue;
    fprintf(out, "  %s yy%d;\n", types[i], i + 1);
    lineno++;
    free(types[i]);
  }
  if (lemp->errsym->useCnt) {
    fprintf(out, "  int yy%d;\n", lemp->errsym->dtnum);
    lineno++;
  }
  free(stddt);
  free(types);
  fprintf(out, "} YYMINORTYPE;\n");
  lineno++;
  *plineno = lineno;
}

/* Return the name of a C datatype able to represent values between
 * lwr and upr, inclusive.
 */
static const char *
minimum_size_type(int lwr, int upr) {
  if (lwr >= 0) {
    if (upr <= 255) {
      return "unsigned char";
    } else if (upr < 65535) {
      return "unsigned short int";
    } else {
      return "unsigned int";
    }
  } else if (lwr >= -127 && upr <= 127) {
    return "signed char";
  } else if (lwr >= -32767 && upr < 32767) {
    return "short";
  } else {
    return "int";
  }
}

/* Each state contains a set of token transaction and a set of
 * nonterminal transactions.  Each of these sets makes an instance
 * of the following structure.  An array of these structures is used
 * to order the creation of entries in the yy_action[] table.
 */
struct axset {
  struct state *stp; // A pointer to a state
  int isTkn;         // True to use tokens.  False for non-terminals
  int nAction;       // Number of actions
  int iOrder;        // Original order of action sets
};

/* Compare to axset structures for sorting purposes */
static int
axset_compare(const void *a, const void *b) {
  struct axset *p1 = (struct axset *)a;
  struct axset *p2 = (struct axset *)b;
  int c;
  c = p2->nAction - p1->nAction;
  if (c == 0) {
    c = p2->iOrder - p1->iOrder;
  }
  assert(c != 0 || p1 == p2);
  return c;
}

/* Write text on "out" that describes the rule "rp". */
static void
writeRuleText(FILE *out, struct rule *rp) {
  int j;
  fprintf(out, "%s ::=", rp->lhs->name);
  for (j = 0; j < rp->nrhs; j++) {
    struct symbol *sp = rp->rhs[j];
    if (sp->type != MULTITERMINAL) {
      fprintf(out, " %s", sp->name);
    } else {
      int k;
      fprintf(out, " %s", sp->subsym[0]->name);
      for (k = 1; k < sp->nsubsym; k++) {
        fprintf(out, "|%s", sp->subsym[k]->name);
      }
    }
  }
}

/* Generate C source code for the parser */
void
ReportTable(struct lemon *lemp, int mhflag // Output in makeheaders format if true
            ) {
  FILE *out, *in;
  char line[LINESIZE];
  int lineno;
  struct state *stp;
  struct action *ap;
  struct rule *rp;
  struct acttab *pActtab;
  int i, j, n;
  const char *name;
  int mnTknOfst, mxTknOfst;
  int mnNtOfst, mxNtOfst;
  struct axset *ax;

  in = tplt_open(lemp);
  if (in == 0)
    return;
  out = file_open(lemp, ".c", "wb");
  if (out == 0) {
    fclose(in);
    return;
  }
  lineno = 1;
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate #defines for all tokens */
  if (mhflag) {
    const char *prefix;
    fprintf(out, "#if INTERFACE\n");
    lineno++;
    if (lemp->tokenprefix)
      prefix = lemp->tokenprefix;
    else
      prefix = "";
    for (i = 1; i < lemp->nterminal; i++) {
      fprintf(out, "#define %s%-30s %2d\n", prefix, lemp->symbols[i]->name, i);
      lineno++;
    }
    fprintf(out, "#endif\n");
    lineno++;
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate the defines */
  fprintf(out, "#define YYCODETYPE %s\n", minimum_size_type(0, lemp->nsymbol + 1));
  lineno++;
  fprintf(out, "#define YYNOCODE %d\n", lemp->nsymbol + 1);
  lineno++;
  fprintf(out, "#define YYACTIONTYPE %s\n", minimum_size_type(0, lemp->nstate + lemp->nrule + 5));
  lineno++;
  if (lemp->wildcard) {
    fprintf(out, "#define YYWILDCARD %d\n", lemp->wildcard->index);
    lineno++;
  }
  print_stack_union(out, lemp, &lineno, mhflag);
  fprintf(out, "#ifndef YYSTACKDEPTH\n");
  lineno++;
  if (lemp->stacksize) {
    fprintf(out, "#define YYSTACKDEPTH %s\n", lemp->stacksize);
    lineno++;
  } else {
    fprintf(out, "#define YYSTACKDEPTH 100\n");
    lineno++;
  }
  fprintf(out, "#endif\n");
  lineno++;
  if (mhflag) {
    fprintf(out, "#if INTERFACE\n");
    lineno++;
  }
  name = lemp->name ? lemp->name : "Parse";
  if (lemp->arg && lemp->arg[0]) {
    int i;
    i = strlen(lemp->arg);
    while (i >= 1 && isspace(lemp->arg[i - 1]))
      i--;
    while (i >= 1 && (isalnum(lemp->arg[i - 1]) || lemp->arg[i - 1] == '_'))
      i--;
    fprintf(out, "#define %sARG_SDECL %s;\n", name, lemp->arg);
    lineno++;
    fprintf(out, "#define %sARG_PDECL ,%s\n", name, lemp->arg);
    lineno++;
    fprintf(out, "#define %sARG_FETCH %s = yypParser->%s\n", name, lemp->arg, &lemp->arg[i]);
    lineno++;
    fprintf(out, "#define %sARG_STORE yypParser->%s = %s\n", name, &lemp->arg[i], &lemp->arg[i]);
    lineno++;
  } else {
    fprintf(out, "#define %sARG_SDECL\n", name);
    lineno++;
    fprintf(out, "#define %sARG_PDECL\n", name);
    lineno++;
    fprintf(out, "#define %sARG_FETCH\n", name);
    lineno++;
    fprintf(out, "#define %sARG_STORE\n", name);
    lineno++;
  }
  if (mhflag) {
    fprintf(out, "#endif\n");
    lineno++;
  }
  fprintf(out, "#define YYNSTATE %d\n", lemp->nstate);
  lineno++;
  fprintf(out, "#define YYNRULE %d\n", lemp->nrule);
  lineno++;
  if (lemp->errsym->useCnt) {
    fprintf(out, "#define YYERRORSYMBOL %d\n", lemp->errsym->index);
    lineno++;
    fprintf(out, "#define YYERRSYMDT yy%d\n", lemp->errsym->dtnum);
    lineno++;
  }
  if (lemp->has_fallback) {
    fprintf(out, "#define YYFALLBACK 1\n");
    lineno++;
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate the include code, if any */
  tplt_print(out, lemp, lemp->include, &lineno);
  if (mhflag) {
    char *nm = file_makename(lemp, ".h");
    fprintf(out, "#include \"%s\"\n", nm);
    lineno++;
    free(nm);
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate the action table and its associates:
   *
   *  yy_action[]        A single table containing all actions.
   *  yy_lookahead[]     A table containing the lookahead for each entry in
   *                     yy_action.  Used to detect hash collisions.
   *  yy_shift_ofst[]    For each state, the offset into yy_action for
   *                     shifting terminals.
   *  yy_reduce_ofst[]   For each state, the offset into yy_action for
   *                     shifting non-terminals after a reduce.
   *  yy_default[]       Default action for each state.
   */

  /* Compute the actions on all states and count them up */
  ax = (struct axset *)calloc(lemp->nstate * 2, sizeof(ax[0]));
  if (ax == 0) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  for (i = 0; i < lemp->nstate; i++) {
    stp = lemp->sorted[i];
    ax[i * 2].stp = stp;
    ax[i * 2].isTkn = 1;
    ax[i * 2].nAction = stp->nTknAct;
    ax[i * 2 + 1].stp = stp;
    ax[i * 2 + 1].isTkn = 0;
    ax[i * 2 + 1].nAction = stp->nNtAct;
  }
  mxTknOfst = mnTknOfst = 0;
  mxNtOfst = mnNtOfst = 0;

  /* Compute the action table.  In order to try to keep the size of the
   * action table to a minimum, the heuristic of placing the largest action
   * sets first is used.
   */
  for (i = 0; i < lemp->nstate * 2; i++)
    ax[i].iOrder = i;
  qsort(ax, lemp->nstate * 2, sizeof(ax[0]), axset_compare);
  pActtab = acttab_alloc();
  for (i = 0; i < lemp->nstate * 2 && ax[i].nAction > 0; i++) {
    stp = ax[i].stp;
    if (ax[i].isTkn) {
      for (ap = stp->ap; ap; ap = ap->next) {
        int action;
        if (ap->sp->index >= lemp->nterminal)
          continue;
        action = compute_action(lemp, ap);
        if (action < 0)
          continue;
        acttab_action(pActtab, ap->sp->index, action);
      }
      stp->iTknOfst = acttab_insert(pActtab);
      if (stp->iTknOfst < mnTknOfst)
        mnTknOfst = stp->iTknOfst;
      if (stp->iTknOfst > mxTknOfst)
        mxTknOfst = stp->iTknOfst;
    } else {
      for (ap = stp->ap; ap; ap = ap->next) {
        int action;
        if (ap->sp->index < lemp->nterminal)
          continue;
        if (ap->sp->index == lemp->nsymbol)
          continue;
        action = compute_action(lemp, ap);
        if (action < 0)
          continue;
        acttab_action(pActtab, ap->sp->index, action);
      }
      stp->iNtOfst = acttab_insert(pActtab);
      if (stp->iNtOfst < mnNtOfst)
        mnNtOfst = stp->iNtOfst;
      if (stp->iNtOfst > mxNtOfst)
        mxNtOfst = stp->iNtOfst;
    }
  }
  free(ax);

  /* Output the yy_action table */
  n = acttab_size(pActtab);
  fprintf(out, "#define YY_ACTTAB_COUNT (%d)\n", n);
  lineno++;
  fprintf(out, "static const YYACTIONTYPE yy_action[] = {\n");
  lineno++;
  for (i = j = 0; i < n; i++) {
    int action = acttab_yyaction(pActtab, i);
    if (action < 0)
      action = lemp->nstate + lemp->nrule + 2;
    if (j == 0)
      fprintf(out, " /* %5d */ ", i);
    fprintf(out, " %4d,", action);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      lineno++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  lineno++;

  /* Output the yy_lookahead table */
  fprintf(out, "static const YYCODETYPE yy_lookahead[] = {\n");
  lineno++;
  for (i = j = 0; i < n; i++) {
    int la = acttab_yylookahead(pActtab, i);
    if (la < 0)
      la = lemp->nsymbol;
    if (j == 0)
      fprintf(out, " /* %5d */ ", i);
    fprintf(out, " %4d,", la);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      lineno++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  lineno++;

  /* Output the yy_shift_ofst[] table */
  fprintf(out, "#define YY_SHIFT_USE_DFLT (%d)\n", mnTknOfst - 1);
  lineno++;
  n = lemp->nstate;
  while (n > 0 && lemp->sorted[n - 1]->iTknOfst == NO_OFFSET)
    n--;
  fprintf(out, "#define YY_SHIFT_COUNT (%d)\n", n - 1);
  lineno++;
  fprintf(out, "#define YY_SHIFT_MIN   (%d)\n", mnTknOfst);
  lineno++;
  fprintf(out, "#define YY_SHIFT_MAX   (%d)\n", mxTknOfst);
  lineno++;
  fprintf(out, "static const %s yy_shift_ofst[] = {\n", minimum_size_type(mnTknOfst - 1, mxTknOfst));
  lineno++;
  for (i = j = 0; i < n; i++) {
    int ofst;
    stp = lemp->sorted[i];
    ofst = stp->iTknOfst;
    if (ofst == NO_OFFSET)
      ofst = mnTknOfst - 1;
    if (j == 0)
      fprintf(out, " /* %5d */ ", i);
    fprintf(out, " %4d,", ofst);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      lineno++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  lineno++;

  /* Output the yy_reduce_ofst[] table */
  fprintf(out, "#define YY_REDUCE_USE_DFLT (%d)\n", mnNtOfst - 1);
  lineno++;
  n = lemp->nstate;
  while (n > 0 && lemp->sorted[n - 1]->iNtOfst == NO_OFFSET)
    n--;
  fprintf(out, "#define YY_REDUCE_COUNT (%d)\n", n - 1);
  lineno++;
  fprintf(out, "#define YY_REDUCE_MIN   (%d)\n", mnNtOfst);
  lineno++;
  fprintf(out, "#define YY_REDUCE_MAX   (%d)\n", mxNtOfst);
  lineno++;
  fprintf(out, "static const %s yy_reduce_ofst[] = {\n", minimum_size_type(mnNtOfst - 1, mxNtOfst));
  lineno++;
  for (i = j = 0; i < n; i++) {
    int ofst;
    stp = lemp->sorted[i];
    ofst = stp->iNtOfst;
    if (ofst == NO_OFFSET)
      ofst = mnNtOfst - 1;
    if (j == 0)
      fprintf(out, " /* %5d */ ", i);
    fprintf(out, " %4d,", ofst);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      lineno++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  lineno++;

  /* Output the default action table */
  fprintf(out, "static const YYACTIONTYPE yy_default[] = {\n");
  lineno++;
  n = lemp->nstate;
  for (i = j = 0; i < n; i++) {
    stp = lemp->sorted[i];
    if (j == 0)
      fprintf(out, " /* %5d */ ", i);
    fprintf(out, " %4d,", stp->iDflt);
    if (j == 9 || i == n - 1) {
      fprintf(out, "\n");
      lineno++;
      j = 0;
    } else {
      j++;
    }
  }
  fprintf(out, "};\n");
  lineno++;
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate the table of fallback tokens. */
  if (lemp->has_fallback) {
    int mx = lemp->nterminal - 1;
    while (mx > 0 && lemp->symbols[mx]->fallback == 0) {
      mx--;
    }
    for (i = 0; i <= mx; i++) {
      struct symbol *p = lemp->symbols[i];
      if (p->fallback == 0) {
        fprintf(out, "    0,  /* %10s => nothing */\n", p->name);
      } else {
        fprintf(out, "  %3d,  /* %10s => %s */\n", p->fallback->index, p->name, p->fallback->name);
      }
      lineno++;
    }
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate a table containing the symbolic name of every symbol */
  for (i = 0; i < lemp->nsymbol; i++) {
    sprintf(line, "\"%s\",", lemp->symbols[i]->name);
    fprintf(out, "  %-15s", line);
    if ((i & 3) == 3) {
      fprintf(out, "\n");
      lineno++;
    }
  }
  if ((i & 3) != 0) {
    fprintf(out, "\n");
    lineno++;
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate a table containing a text string that describes every
   * rule in the rule set of the grammar.  This information is used
   * when tracing REDUCE actions.
   */
  for (i = 0, rp = lemp->rule; rp; rp = rp->next, i++) {
    assert(rp->index == i);
    fprintf(out, " /* %3d */ \"", i);
    writeRuleText(out, rp);
    fprintf(out, "\",\n");
    lineno++;
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate code which executes every time a symbol is popped from
   * the stack while processing errors or while destroying the parser.
   * (In other words, generate the %destructor actions)
   */
  if (lemp->tokendest) {
    int once = 1;
    for (i = 0; i < lemp->nsymbol; i++) {
      struct symbol *sp = lemp->symbols[i];
      if (sp == 0 || sp->type != TERMINAL)
        continue;
      if (once) {
        fprintf(out, "      /* TERMINAL Destructor */\n");
        lineno++;
        once = 0;
      }
      fprintf(out, "    case %d: /* %s */\n", sp->index, sp->name);
      lineno++;
    }
    for (i = 0; i < lemp->nsymbol && lemp->symbols[i]->type != TERMINAL; i++)
      ;
    if (i < lemp->nsymbol) {
      emit_destructor_code(out, lemp->symbols[i], lemp, &lineno);
      fprintf(out, "      break;\n");
      lineno++;
    }
  }
  if (lemp->vardest) {
    struct symbol *dflt_sp = 0;
    int once = 1;
    for (i = 0; i < lemp->nsymbol; i++) {
      struct symbol *sp = lemp->symbols[i];
      if (sp == 0 || sp->type == TERMINAL || sp->index <= 0 || sp->destructor != 0)
        continue;
      if (once) {
        fprintf(out, "      /* Default NON-TERMINAL Destructor */\n");
        lineno++;
        once = 0;
      }
      fprintf(out, "    case %d: /* %s */\n", sp->index, sp->name);
      lineno++;
      dflt_sp = sp;
    }
    if (dflt_sp != 0) {
      emit_destructor_code(out, dflt_sp, lemp, &lineno);
    }
    fprintf(out, "      break;\n");
    lineno++;
  }
  for (i = 0; i < lemp->nsymbol; i++) {
    struct symbol *sp = lemp->symbols[i];
    if (sp == 0 || sp->type == TERMINAL || sp->destructor == 0)
      continue;
    fprintf(out, "    case %d: /* %s */\n", sp->index, sp->name);
    lineno++;

    /* Combine duplicate destructors into a single case */
    for (j = i + 1; j < lemp->nsymbol; j++) {
      struct symbol *sp2 = lemp->symbols[j];
      if (sp2 && sp2->type != TERMINAL && sp2->destructor && sp2->dtnum == sp->dtnum &&
          strcmp(sp->destructor, sp2->destructor) == 0) {
        fprintf(out, "    case %d: /* %s */\n", sp2->index, sp2->name);
        lineno++;
        sp2->destructor = 0;
      }
    }

    emit_destructor_code(out, lemp->symbols[i], lemp, &lineno);
    fprintf(out, "      break;\n");
    lineno++;
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate code which executes whenever the parser stack overflows */
  tplt_print(out, lemp, lemp->overflow, &lineno);
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate the table of rule information
   *
   * Note: This code depends on the fact that rules are number
   * sequentually beginning with 0.
   */
  for (rp = lemp->rule; rp; rp = rp->next) {
    fprintf(out, "  { %d, %d },\n", rp->lhs->index, rp->nrhs);
    lineno++;
  }
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate code which execution during each REDUCE action */
  for (rp = lemp->rule; rp; rp = rp->next) {
    translate_code(lemp, rp);
  }
  /* First output rules other than the default: rule */
  for (rp = lemp->rule; rp; rp = rp->next) {
    struct rule *rp2; // Other rules with the same action
    if (rp->code == 0)
      continue;
    if (rp->code[0] == '\n' && rp->code[1] == 0)
      continue; // Will be default:
    fprintf(out, "      case %d: /* ", rp->index);
    writeRuleText(out, rp);
    fprintf(out, " */\n");
    lineno++;
    for (rp2 = rp->next; rp2; rp2 = rp2->next) {
      if (rp2->code == rp->code) {
        fprintf(out, "      case %d: /* ", rp2->index);
        writeRuleText(out, rp2);
        fprintf(out, " */ yytestcase(yyruleno==%d);\n", rp2->index);
        lineno++;
        rp2->code = 0;
      }
    }
    emit_code(out, rp, lemp, &lineno);
    fprintf(out, "        break;\n");
    lineno++;
    rp->code = 0;
  }
  /* Finally, output the default: rule.  We choose as the default: all
   * empty actions.
   */
  fprintf(out, "      default:\n");
  lineno++;
  for (rp = lemp->rule; rp; rp = rp->next) {
    if (rp->code == 0)
      continue;
    assert(rp->code[0] == '\n' && rp->code[1] == 0);
    fprintf(out, "      /* (%d) ", rp->index);
    writeRuleText(out, rp);
    fprintf(out, " */ yytestcase(yyruleno==%d);\n", rp->index);
    lineno++;
  }
  fprintf(out, "        break;\n");
  lineno++;
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate code which executes if a parse fails */
  tplt_print(out, lemp, lemp->failure, &lineno);
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate code which executes when a syntax error occurs */
  tplt_print(out, lemp, lemp->error, &lineno);
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Generate code which executes when the parser accepts its input */
  tplt_print(out, lemp, lemp->accept, &lineno);
  tplt_xfer(lemp->name, in, out, &lineno);

  /* Append any addition code the user desires */
  tplt_print(out, lemp, lemp->extracode, &lineno);

  fclose(in);
  fclose(out);
  return;
}

/* Generate a header file for the parser */
void
ReportHeader(struct lemon *lemp) {
  FILE *out, *in;
  const char *prefix;
  char line[LINESIZE];
  char pattern[LINESIZE];
  int i;

  if (lemp->tokenprefix)
    prefix = lemp->tokenprefix;
  else
    prefix = "";
  in = file_open(lemp, ".h", "rb");
  if (in) {
    int nextChar;
    for (i = 1; i < lemp->nterminal && fgets(line, LINESIZE, in); i++) {
      sprintf(pattern, "#define %s%-30s %3d\n", prefix, lemp->symbols[i]->name, i);
      if (strcmp(line, pattern))
        break;
    }
    nextChar = fgetc(in);
    fclose(in);
    if (i == lemp->nterminal && nextChar == EOF) {
      // No change in the file.  Don't rewrite it.
      return;
    }
  }
  out = file_open(lemp, ".h", "wb");
  if (out) {
    for (i = 1; i < lemp->nterminal; i++) {
      fprintf(out, "#define %s%-30s %3d\n", prefix, lemp->symbols[i]->name, i);
    }
    fclose(out);
  }
  return;
}

/* Reduce the size of the action tables, if possible, by making use
 * of defaults.
 *
 * In this version, we take the most frequent REDUCE action and make
 * it the default.  Except, there is no default if the wildcard token
 * is a possible look-ahead.
 */
void
CompressTables(struct lemon *lemp) {
  struct state *stp;
  struct action *ap, *ap2;
  struct rule *rp, *rp2, *rbest;
  int nbest, n;
  int i;
  int usesWildcard;

  for (i = 0; i < lemp->nstate; i++) {
    stp = lemp->sorted[i];
    nbest = 0;
    rbest = 0;
    usesWildcard = 0;

    for (ap = stp->ap; ap; ap = ap->next) {
      if (ap->type == SHIFT && ap->sp == lemp->wildcard) {
        usesWildcard = 1;
      }
      if (ap->type != REDUCE)
        continue;
      rp = ap->x.rp;
      if (rp->lhsStart)
        continue;
      if (rp == rbest)
        continue;
      n = 1;
      for (ap2 = ap->next; ap2; ap2 = ap2->next) {
        if (ap2->type != REDUCE)
          continue;
        rp2 = ap2->x.rp;
        if (rp2 == rbest)
          continue;
        if (rp2 == rp)
          n++;
      }
      if (n > nbest) {
        nbest = n;
        rbest = rp;
      }
    }

    /* Do not make a default if the number of rules to default
     * is not at least 1 or if the wildcard token is a possible
     * lookahead.
     */
    if (nbest < 1 || usesWildcard)
      continue;

    /* Combine matching REDUCE actions into a single default */
    for (ap = stp->ap; ap; ap = ap->next) {
      if (ap->type == REDUCE && ap->x.rp == rbest)
        break;
    }
    assert(ap);
    ap->sp = Symbol_new("{default}");
    for (ap = ap->next; ap; ap = ap->next) {
      if (ap->type == REDUCE && ap->x.rp == rbest)
        ap->type = NOT_USED;
    }
    stp->ap = Action_sort(stp->ap);
  }
}

/* Compare two states for sorting purposes.  The smaller state is the
 * one with the most non-terminal actions.  If they have the same number
 * of non-terminal actions, then the smaller is the one with the most
 * token actions.
 */
static int
stateResortCompare(const void *a, const void *b) {
  const struct state *pA = *(const struct state **)a;
  const struct state *pB = *(const struct state **)b;
  int n;

  n = pB->nNtAct - pA->nNtAct;
  if (n == 0) {
    n = pB->nTknAct - pA->nTknAct;
    if (n == 0) {
      n = pB->statenum - pA->statenum;
    }
  }
  assert(n != 0);
  return n;
}

/* Renumber and resort states so that states with fewer choices
 * occur at the end.  Except, keep state 0 as the first state.
 */
void
ResortStates(struct lemon *lemp) {
  int i;
  struct state *stp;
  struct action *ap;

  for (i = 0; i < lemp->nstate; i++) {
    stp = lemp->sorted[i];
    stp->nTknAct = stp->nNtAct = 0;
    stp->iDflt = lemp->nstate + lemp->nrule;
    stp->iTknOfst = NO_OFFSET;
    stp->iNtOfst = NO_OFFSET;
    for (ap = stp->ap; ap; ap = ap->next) {
      if (compute_action(lemp, ap) >= 0) {
        if (ap->sp->index < lemp->nterminal) {
          stp->nTknAct++;
        } else if (ap->sp->index < lemp->nsymbol) {
          stp->nNtAct++;
        } else {
          stp->iDflt = compute_action(lemp, ap);
        }
      }
    }
  }
  qsort(&lemp->sorted[1], (size_t)(lemp->nstate - 1), sizeof(lemp->sorted[0]), stateResortCompare);
  for (i = 0; i < lemp->nstate; i++) {
    lemp->sorted[i]->statenum = i;
  }
}
