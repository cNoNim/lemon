#include "error.h"
#include "parse.h"
#include "lemon.h"
#include "table.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** Input file parser for the LEMON parser generator.
*/

/* The state of the parser */
enum e_state {
  INITIALIZE,
  WAITING_FOR_DECL_OR_RULE,
  WAITING_FOR_DECL_KEYWORD,
  WAITING_FOR_DECL_ARG,
  WAITING_FOR_PRECEDENCE_SYMBOL,
  WAITING_FOR_ARROW,
  IN_RHS,
  LHS_ALIAS_1,
  LHS_ALIAS_2,
  LHS_ALIAS_3,
  RHS_ALIAS_1,
  RHS_ALIAS_2,
  PRECEDENCE_MARK_1,
  PRECEDENCE_MARK_2,
  RESYNC_AFTER_RULE_ERROR,
  RESYNC_AFTER_DECL_ERROR,
  WAITING_FOR_DESTRUCTOR_SYMBOL,
  WAITING_FOR_DATATYPE_SYMBOL,
  WAITING_FOR_FALLBACK_ID,
  WAITING_FOR_WILDCARD_ID,
  WAITING_FOR_CLASS_ID,
  WAITING_FOR_CLASS_TOKEN
};
struct pstate {
  char *filename;             /* Name of the input file */
  int tokenlineno;            /* Linenumber at which current token starts */
  int errorcnt;               /* Number of errors so far */
  char *tokenstart;           /* Text of current token */
  struct lemon *gp;           /* Global state vector */
  enum e_state state;         /* The state of the parser */
  struct symbol *fallback;    /* The fallback token */
  struct symbol *tkclass;     /* Token class symbol */
  struct symbol *lhs;         /* Left-hand side of current rule */
  const char *lhsalias;       /* Alias for the LHS */
  int nrhs;                   /* Number of right-hand side symbols seen */
  struct symbol *rhs[MAXRHS]; /* RHS symbols */
  const char *alias[MAXRHS];  /* Aliases for each RHS symbol (or NULL) */
  struct rule *prevrule;      /* Previous rule parsed */
  const char *declkeyword;    /* Keyword of a declaration */
  char **declargslot;         /* Where the declaration argument should be put */
  int insertLineMacro;        /* Add #line before declaration insert */
  int *decllinenoslot;        /* Where to write declaration line number */
  enum e_assoc declassoc;     /* Assign this association to decl arguments */
  int preccounter;            /* Assign this precedence to decl arguments */
  struct rule *firstrule;     /* Pointer to first rule in the grammar */
  struct rule *lastrule;      /* Pointer to the most recently parsed rule */
};

// TODO: move
extern int nDefine;
extern char **azDefine;

/* forward declarations */
static void parseonetoken(struct pstate *);
static void preprocess_input(char *);

/* In spite of its name, this function is really a scanner.  It read
** in the entire input file (all at once) then tokenizes it.  Each
** token is passed to the function "parseonetoken" which builds all
** the appropriate data structures in the global state vector "gp".
*/
void
Parse(struct lemon *gp) {
  struct pstate ps;
  FILE *fp;
  char *filebuf;
  long filesize;
  int lineno;
  int c;
  char *cp, *nextcp;
  int startline = 0;

  memset(&ps, '\0', sizeof(ps));
  ps.gp = gp;
  ps.filename = gp->filename;
  ps.errorcnt = 0;
  ps.state = INITIALIZE;

  /* Begin by reading the input file */
  fp = fopen(ps.filename, "rb");
  if (fp == 0) {
    ErrorMsg(ps.filename, 0, "Can't open this file for reading.");
    gp->errorcnt++;
    return;
  }
  fseek(fp, 0, 2);
  filesize = ftell(fp);
  rewind(fp);
  filebuf = (char *)malloc((size_t)(filesize + 1));
  if (filesize > 100000000 || filebuf == 0) {
    ErrorMsg(ps.filename, 0, "Input file too large.");
    gp->errorcnt++;
    fclose(fp);
    return;
  }
  if (fread(filebuf, 1, (size_t)filesize, fp) != filesize) {
    ErrorMsg(ps.filename, 0, "Can't read in all %d bytes of this file.", filesize);
    free(filebuf);
    gp->errorcnt++;
    fclose(fp);
    return;
  }
  fclose(fp);
  filebuf[filesize] = 0;

  /* Make an initial pass through the file to handle %ifdef and %ifndef */
  preprocess_input(filebuf);

  /* Now scan the text of the input file */
  lineno = 1;
  for (cp = filebuf; (c = *cp) != 0;) {
    if (c == '\n')
      lineno++; /* Keep track of the line number */
    if (isspace(c)) {
      cp++;
      continue;
    }                               /* Skip all white space */
    if (c == '/' && cp[1] == '/') { /* Skip C++ style comments */
      cp += 2;
      while ((c = *cp) != 0 && c != '\n')
        cp++;
      continue;
    }
    if (c == '/' && cp[1] == '*') { /* Skip C style comments */
      cp += 2;
      while ((c = *cp) != 0 && (c != '/' || cp[-1] != '*')) {
        if (c == '\n')
          lineno++;
        cp++;
      }
      if (c)
        cp++;
      continue;
    }
    ps.tokenstart = cp;      /* Mark the beginning of the token */
    ps.tokenlineno = lineno; /* Linenumber on which token begins */
    if (c == '\"') {         /* String literals */
      cp++;
      while ((c = *cp) != 0 && c != '\"') {
        if (c == '\n')
          lineno++;
        cp++;
      }
      if (c == 0) {
        ErrorMsg(ps.filename, startline, "String starting on this line is not terminated before the end of the file.");
        ps.errorcnt++;
        nextcp = cp;
      } else {
        nextcp = cp + 1;
      }
    } else if (c == '{') { /* A block of C code */
      int level;
      cp++;
      for (level = 1; (c = *cp) != 0 && (level > 1 || c != '}'); cp++) {
        if (c == '\n')
          lineno++;
        else if (c == '{')
          level++;
        else if (c == '}')
          level--;
        else if (c == '/' && cp[1] == '*') { /* Skip comments */
          int prevc;
          cp = &cp[2];
          prevc = 0;
          while ((c = *cp) != 0 && (c != '/' || prevc != '*')) {
            if (c == '\n')
              lineno++;
            prevc = c;
            cp++;
          }
        } else if (c == '/' && cp[1] == '/') { /* Skip C++ style comments too */
          cp = &cp[2];
          while ((c = *cp) != 0 && c != '\n')
            cp++;
          if (c)
            lineno++;
        } else if (c == '\'' || c == '\"') { /* String a character literals */
          int startchar, prevc;
          startchar = c;
          prevc = 0;
          for (cp++; (c = *cp) != 0 && (c != startchar || prevc == '\\'); cp++) {
            if (c == '\n')
              lineno++;
            if (prevc == '\\')
              prevc = 0;
            else
              prevc = c;
          }
        }
      }
      if (c == 0) {
        ErrorMsg(ps.filename, ps.tokenlineno,
                 "C code starting on this line is not terminated before the end of the file.");
        ps.errorcnt++;
        nextcp = cp;
      } else {
        nextcp = cp + 1;
      }
    } else if (isalnum(c)) { /* Identifiers */
      while ((c = *cp) != 0 && (isalnum(c) || c == '_'))
        cp++;
      nextcp = cp;
    } else if (c == ':' && cp[1] == ':' && cp[2] == '=') { /* The operator "::=" */
      cp += 3;
      nextcp = cp;
    } else if ((c == '/' || c == '|') && isalpha(cp[1])) {
      cp += 2;
      while ((c = *cp) != 0 && (isalnum(c) || c == '_'))
        cp++;
      nextcp = cp;
    } else { /* All other (one character) operators */
      cp++;
      nextcp = cp;
    }
    c = *cp;
    *cp = 0;            /* Null terminate the token */
    parseonetoken(&ps); /* Parse the token */
    *cp = (char)c;      /* Restore the buffer */
    cp = nextcp;
  }
  free(filebuf); /* Release the buffer after parsing */
  gp->rule = ps.firstrule;
  gp->errorcnt = ps.errorcnt;
}

/* Parse a single token */
static void
parseonetoken(struct pstate *psp) {
  const char *x;
  x = Strsafe(psp->tokenstart); /* Save the token permanently */
  switch (psp->state) {
  case INITIALIZE:
    psp->prevrule = 0;
    psp->preccounter = 0;
    psp->firstrule = psp->lastrule = 0;
    psp->gp->nrule = 0;
  /* Fall thru to next case */
  case WAITING_FOR_DECL_OR_RULE:
    if (x[0] == '%') {
      psp->state = WAITING_FOR_DECL_KEYWORD;
    } else if (islower(x[0])) {
      psp->lhs = Symbol_new(x);
      psp->nrhs = 0;
      psp->lhsalias = 0;
      psp->state = WAITING_FOR_ARROW;
    } else if (x[0] == '{') {
      if (psp->prevrule == 0) {
        ErrorMsg(psp->filename, psp->tokenlineno,
                 "There is no prior rule upon which to attach the code \
                            fragment which begins on this line.");
        psp->errorcnt++;
      } else if (psp->prevrule->code != 0) {
        ErrorMsg(psp->filename, psp->tokenlineno,
                 "Code fragment beginning on this line is not the first \
                            to follow the previous rule.");
        psp->errorcnt++;
      } else {
        psp->prevrule->line = psp->tokenlineno;
        psp->prevrule->code = &x[1];
      }
    } else if (x[0] == '[') {
      psp->state = PRECEDENCE_MARK_1;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Token \"%s\" should be either \"%%\" or a nonterminal name.", x);
      psp->errorcnt++;
    }
    break;
  case PRECEDENCE_MARK_1:
    if (!isupper(x[0])) {
      ErrorMsg(psp->filename, psp->tokenlineno, "The precedence symbol must be a terminal.");
      psp->errorcnt++;
    } else if (psp->prevrule == 0) {
      ErrorMsg(psp->filename, psp->tokenlineno, "There is no prior rule to assign precedence \"[%s]\".", x);
      psp->errorcnt++;
    } else if (psp->prevrule->precsym != 0) {
      ErrorMsg(psp->filename, psp->tokenlineno,
               "Precedence mark on this line is not the first \
                        to follow the previous rule.");
      psp->errorcnt++;
    } else {
      psp->prevrule->precsym = Symbol_new(x);
    }
    psp->state = PRECEDENCE_MARK_2;
    break;
  case PRECEDENCE_MARK_2:
    if (x[0] != ']') {
      ErrorMsg(psp->filename, psp->tokenlineno, "Missing \"]\" on precedence mark.");
      psp->errorcnt++;
    }
    psp->state = WAITING_FOR_DECL_OR_RULE;
    break;
  case WAITING_FOR_ARROW:
    if (x[0] == ':' && x[1] == ':' && x[2] == '=') {
      psp->state = IN_RHS;
    } else if (x[0] == '(') {
      psp->state = LHS_ALIAS_1;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Expected to see a \":\" following the LHS symbol \"%s\".",
               psp->lhs->name);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case LHS_ALIAS_1:
    if (isalpha(x[0])) {
      psp->lhsalias = x;
      psp->state = LHS_ALIAS_2;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "\"%s\" is not a valid alias for the LHS \"%s\"\n", x, psp->lhs->name);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case LHS_ALIAS_2:
    if (x[0] == ')') {
      psp->state = LHS_ALIAS_3;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Missing \")\" following LHS alias name \"%s\".", psp->lhsalias);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case LHS_ALIAS_3:
    if (x[0] == ':' && x[1] == ':' && x[2] == '=') {
      psp->state = IN_RHS;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Missing \"->\" following: \"%s(%s)\".", psp->lhs->name, psp->lhsalias);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case IN_RHS:
    if (x[0] == '.') {
      struct rule *rp;
      rp = (struct rule *)calloc(sizeof(struct rule) + sizeof(struct symbol *) * psp->nrhs + sizeof(char *) * psp->nrhs,
                                 1);
      if (rp == 0) {
        ErrorMsg(psp->filename, psp->tokenlineno, "Can't allocate enough memory for this rule.");
        psp->errorcnt++;
        psp->prevrule = 0;
      } else {
        int i;
        rp->ruleline = psp->tokenlineno;
        rp->rhs = (struct symbol **)&rp[1];
        rp->rhsalias = (const char **)&(rp->rhs[psp->nrhs]);
        for (i = 0; i < psp->nrhs; i++) {
          rp->rhs[i] = psp->rhs[i];
          rp->rhsalias[i] = psp->alias[i];
        }
        rp->lhs = psp->lhs;
        rp->lhsalias = psp->lhsalias;
        rp->nrhs = psp->nrhs;
        rp->code = 0;
        rp->precsym = 0;
        rp->index = psp->gp->nrule++;
        rp->nextlhs = rp->lhs->rule;
        rp->lhs->rule = rp;
        rp->next = 0;
        if (psp->firstrule == 0) {
          psp->firstrule = psp->lastrule = rp;
        } else {
          psp->lastrule->next = rp;
          psp->lastrule = rp;
        }
        psp->prevrule = rp;
      }
      psp->state = WAITING_FOR_DECL_OR_RULE;
    } else if (isalpha(x[0])) {
      if (psp->nrhs >= MAXRHS) {
        ErrorMsg(psp->filename, psp->tokenlineno, "Too many symbols on RHS of rule beginning at \"%s\".", x);
        psp->errorcnt++;
        psp->state = RESYNC_AFTER_RULE_ERROR;
      } else {
        psp->rhs[psp->nrhs] = Symbol_new(x);
        psp->alias[psp->nrhs] = 0;
        psp->nrhs++;
      }
    } else if ((x[0] == '|' || x[0] == '/') && psp->nrhs > 0) {
      struct symbol *msp = psp->rhs[psp->nrhs - 1];
      if (msp->type != MULTITERMINAL) {
        struct symbol *origsp = msp;
        msp = (struct symbol *)calloc(1, sizeof(*msp));
        memset(msp, 0, sizeof(*msp));
        msp->type = MULTITERMINAL;
        msp->nsubsym = 1;
        msp->subsym = (struct symbol **)calloc(1, sizeof(struct symbol *));
        msp->subsym[0] = origsp;
        msp->name = origsp->name;
        psp->rhs[psp->nrhs - 1] = msp;
      }
      msp->nsubsym++;
      msp->subsym = (struct symbol **)realloc(msp->subsym, sizeof(struct symbol *) * msp->nsubsym);
      msp->subsym[msp->nsubsym - 1] = Symbol_new(&x[1]);
      if (islower(x[1]) || islower(msp->subsym[0]->name[0])) {
        ErrorMsg(psp->filename, psp->tokenlineno, "Cannot form a compound containing a non-terminal");
        psp->errorcnt++;
      }
    } else if (x[0] == '(' && psp->nrhs > 0) {
      psp->state = RHS_ALIAS_1;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Illegal character on RHS of rule: \"%s\".", x);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case RHS_ALIAS_1:
    if (isalpha(x[0])) {
      psp->alias[psp->nrhs - 1] = x;
      psp->state = RHS_ALIAS_2;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "\"%s\" is not a valid alias for the RHS symbol \"%s\"\n", x,
               psp->rhs[psp->nrhs - 1]->name);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case RHS_ALIAS_2:
    if (x[0] == ')') {
      psp->state = IN_RHS;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Missing \")\" following LHS alias name \"%s\".", psp->lhsalias);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_RULE_ERROR;
    }
    break;
  case WAITING_FOR_DECL_KEYWORD:
    if (isalpha(x[0])) {
      psp->declkeyword = x;
      psp->declargslot = 0;
      psp->decllinenoslot = 0;
      psp->insertLineMacro = 1;
      psp->state = WAITING_FOR_DECL_ARG;
      if (strcmp(x, "name") == 0) {
        psp->declargslot = &(psp->gp->name);
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "include") == 0) {
        psp->declargslot = &(psp->gp->include);
      } else if (strcmp(x, "code") == 0) {
        psp->declargslot = &(psp->gp->extracode);
      } else if (strcmp(x, "token_destructor") == 0) {
        psp->declargslot = &psp->gp->tokendest;
      } else if (strcmp(x, "default_destructor") == 0) {
        psp->declargslot = &psp->gp->vardest;
      } else if (strcmp(x, "token_prefix") == 0) {
        psp->declargslot = &psp->gp->tokenprefix;
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "syntax_error") == 0) {
        psp->declargslot = &(psp->gp->error);
      } else if (strcmp(x, "parse_accept") == 0) {
        psp->declargslot = &(psp->gp->accept);
      } else if (strcmp(x, "parse_failure") == 0) {
        psp->declargslot = &(psp->gp->failure);
      } else if (strcmp(x, "stack_overflow") == 0) {
        psp->declargslot = &(psp->gp->overflow);
      } else if (strcmp(x, "extra_argument") == 0) {
        psp->declargslot = &(psp->gp->arg);
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "token_type") == 0) {
        psp->declargslot = &(psp->gp->tokentype);
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "default_type") == 0) {
        psp->declargslot = &(psp->gp->vartype);
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "stack_size") == 0) {
        psp->declargslot = &(psp->gp->stacksize);
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "start_symbol") == 0) {
        psp->declargslot = &(psp->gp->start);
        psp->insertLineMacro = 0;
      } else if (strcmp(x, "left") == 0) {
        psp->preccounter++;
        psp->declassoc = LEFT;
        psp->state = WAITING_FOR_PRECEDENCE_SYMBOL;
      } else if (strcmp(x, "right") == 0) {
        psp->preccounter++;
        psp->declassoc = RIGHT;
        psp->state = WAITING_FOR_PRECEDENCE_SYMBOL;
      } else if (strcmp(x, "nonassoc") == 0) {
        psp->preccounter++;
        psp->declassoc = NONE;
        psp->state = WAITING_FOR_PRECEDENCE_SYMBOL;
      } else if (strcmp(x, "destructor") == 0) {
        psp->state = WAITING_FOR_DESTRUCTOR_SYMBOL;
      } else if (strcmp(x, "type") == 0) {
        psp->state = WAITING_FOR_DATATYPE_SYMBOL;
      } else if (strcmp(x, "fallback") == 0) {
        psp->fallback = 0;
        psp->state = WAITING_FOR_FALLBACK_ID;
      } else if (strcmp(x, "wildcard") == 0) {
        psp->state = WAITING_FOR_WILDCARD_ID;
      } else if (strcmp(x, "token_class") == 0) {
        psp->state = WAITING_FOR_CLASS_ID;
      } else {
        ErrorMsg(psp->filename, psp->tokenlineno, "Unknown declaration keyword: \"%%%s\".", x);
        psp->errorcnt++;
        psp->state = RESYNC_AFTER_DECL_ERROR;
      }
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Illegal declaration keyword: \"%s\".", x);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    }
    break;
  case WAITING_FOR_DESTRUCTOR_SYMBOL:
    if (!isalpha(x[0])) {
      ErrorMsg(psp->filename, psp->tokenlineno, "Symbol name missing after %%destructor keyword");
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    } else {
      struct symbol *sp = Symbol_new(x);
      psp->declargslot = &sp->destructor;
      psp->decllinenoslot = &sp->destLineno;
      psp->insertLineMacro = 1;
      psp->state = WAITING_FOR_DECL_ARG;
    }
    break;
  case WAITING_FOR_DATATYPE_SYMBOL:
    if (!isalpha(x[0])) {
      ErrorMsg(psp->filename, psp->tokenlineno, "Symbol name missing after %%type keyword");
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    } else {
      struct symbol *sp = Symbol_find(x);
      if ((sp) && (sp->datatype)) {
        ErrorMsg(psp->filename, psp->tokenlineno, "Symbol %%type \"%s\" already defined", x);
        psp->errorcnt++;
        psp->state = RESYNC_AFTER_DECL_ERROR;
      } else {
        if (!sp) {
          sp = Symbol_new(x);
        }
        psp->declargslot = &sp->datatype;
        psp->insertLineMacro = 0;
        psp->state = WAITING_FOR_DECL_ARG;
      }
    }
    break;
  case WAITING_FOR_PRECEDENCE_SYMBOL:
    if (x[0] == '.') {
      psp->state = WAITING_FOR_DECL_OR_RULE;
    } else if (isupper(x[0])) {
      struct symbol *sp;
      sp = Symbol_new(x);
      if (sp->prec >= 0) {
        ErrorMsg(psp->filename, psp->tokenlineno, "Symbol \"%s\" has already be given a precedence.", x);
        psp->errorcnt++;
      } else {
        sp->prec = psp->preccounter;
        sp->assoc = psp->declassoc;
      }
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Can't assign a precedence to \"%s\".", x);
      psp->errorcnt++;
    }
    break;
  case WAITING_FOR_DECL_ARG:
    if (x[0] == '{' || x[0] == '\"' || isalnum(x[0])) {
      const char *zOld, *zNew;
      char *zBuf, *z;
      size_t n, nOld, nNew, nBack, nLine = 0;
      int addLineMacro;
      char zLine[50];
      zNew = x;
      if (zNew[0] == '"' || zNew[0] == '{')
        zNew++;
      nNew = strlen(zNew);
      if (*psp->declargslot) {
        zOld = *psp->declargslot;
      } else {
        zOld = "";
      }
      nOld = strlen(zOld);
      n = nOld + nNew + 20;
      addLineMacro =
          !psp->gp->nolinenosflag && psp->insertLineMacro && (psp->decllinenoslot == 0 || psp->decllinenoslot[0] != 0);
      if (addLineMacro) {
        for (z = psp->filename, nBack = 0; *z; z++) {
          if (*z == '\\')
            nBack++;
        }
        sprintf(zLine, "#line %d ", psp->tokenlineno);
        nLine = strlen(zLine);
        n += nLine + strlen(psp->filename) + nBack;
      }
      *psp->declargslot = (char *)realloc(*psp->declargslot, n);
      zBuf = *psp->declargslot + nOld;
      if (addLineMacro) {
        if (nOld && zBuf[-1] != '\n') {
          *(zBuf++) = '\n';
        }
        memcpy(zBuf, zLine, nLine);
        zBuf += nLine;
        *(zBuf++) = '"';
        for (z = psp->filename; *z; z++) {
          if (*z == '\\') {
            *(zBuf++) = '\\';
          }
          *(zBuf++) = *z;
        }
        *(zBuf++) = '"';
        *(zBuf++) = '\n';
      }
      if (psp->decllinenoslot && psp->decllinenoslot[0] == 0) {
        psp->decllinenoslot[0] = psp->tokenlineno;
      }
      memcpy(zBuf, zNew, nNew);
      zBuf += nNew;
      *zBuf = 0;
      psp->state = WAITING_FOR_DECL_OR_RULE;
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "Illegal argument to %%%s: %s", psp->declkeyword, x);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    }
    break;
  case WAITING_FOR_FALLBACK_ID:
    if (x[0] == '.') {
      psp->state = WAITING_FOR_DECL_OR_RULE;
    } else if (!isupper(x[0])) {
      ErrorMsg(psp->filename, psp->tokenlineno, "%%fallback argument \"%s\" should be a token", x);
      psp->errorcnt++;
    } else {
      struct symbol *sp = Symbol_new(x);
      if (psp->fallback == 0) {
        psp->fallback = sp;
      } else if (sp->fallback) {
        ErrorMsg(psp->filename, psp->tokenlineno, "More than one fallback assigned to token %s", x);
        psp->errorcnt++;
      } else {
        sp->fallback = psp->fallback;
        psp->gp->has_fallback = 1;
      }
    }
    break;
  case WAITING_FOR_WILDCARD_ID:
    if (x[0] == '.') {
      psp->state = WAITING_FOR_DECL_OR_RULE;
    } else if (!isupper(x[0])) {
      ErrorMsg(psp->filename, psp->tokenlineno, "%%wildcard argument \"%s\" should be a token", x);
      psp->errorcnt++;
    } else {
      struct symbol *sp = Symbol_new(x);
      if (psp->gp->wildcard == 0) {
        psp->gp->wildcard = sp;
      } else {
        ErrorMsg(psp->filename, psp->tokenlineno, "Extra wildcard to token: %s", x);
        psp->errorcnt++;
      }
    }
    break;
  case WAITING_FOR_CLASS_ID:
    if (!islower(x[0])) {
      ErrorMsg(psp->filename, psp->tokenlineno, "%%token_class must be followed by an identifier: ", x);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    } else if (Symbol_find(x)) {
      ErrorMsg(psp->filename, psp->tokenlineno, "Symbol \"%s\" already used", x);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    } else {
      psp->tkclass = Symbol_new(x);
      psp->tkclass->type = MULTITERMINAL;
      psp->state = WAITING_FOR_CLASS_TOKEN;
    }
    break;
  case WAITING_FOR_CLASS_TOKEN:
    if (x[0] == '.') {
      psp->state = WAITING_FOR_DECL_OR_RULE;
    } else if (isupper(x[0]) || ((x[0] == '|' || x[0] == '/') && isupper(x[1]))) {
      struct symbol *msp = psp->tkclass;
      msp->nsubsym++;
      msp->subsym = (struct symbol **)realloc(msp->subsym, sizeof(struct symbol *) * msp->nsubsym);
      if (!isupper(x[0]))
        x++;
      msp->subsym[msp->nsubsym - 1] = Symbol_new(x);
    } else {
      ErrorMsg(psp->filename, psp->tokenlineno, "%%token_class argument \"%s\" should be a token", x);
      psp->errorcnt++;
      psp->state = RESYNC_AFTER_DECL_ERROR;
    }
    break;
  case RESYNC_AFTER_RULE_ERROR:
  /*      if( x[0]=='.' ) psp->state = WAITING_FOR_DECL_OR_RULE;
  **      break; */
  case RESYNC_AFTER_DECL_ERROR:
    if (x[0] == '.')
      psp->state = WAITING_FOR_DECL_OR_RULE;
    if (x[0] == '%')
      psp->state = WAITING_FOR_DECL_KEYWORD;
    break;
  }
}

/* Run the preprocessor over the input file text.  The global variables
** azDefine[0] through azDefine[nDefine-1] contains the names of all defined
** macros.  This routine looks for "%ifdef" and "%ifndef" and "%endif" and
** comments them out.  Text in between is also commented out as appropriate.
*/
static void
preprocess_input(char *z) {
  int i, j, k;
  size_t n;
  int exclude = 0;
  int start = 0;
  int lineno = 1;
  int start_lineno = 1;
  for (i = 0; z[i]; i++) {
    if (z[i] == '\n')
      lineno++;
    if (z[i] != '%' || (i > 0 && z[i - 1] != '\n'))
      continue;
    if (strncmp(&z[i], "%endif", 6) == 0 && isspace(z[i + 6])) {
      if (exclude) {
        exclude--;
        if (exclude == 0) {
          for (j = start; j < i; j++)
            if (z[j] != '\n')
              z[j] = ' ';
        }
      }
      for (j = i; z[j] && z[j] != '\n'; j++)
        z[j] = ' ';
    } else if ((strncmp(&z[i], "%ifdef", 6) == 0 && isspace(z[i + 6])) ||
               (strncmp(&z[i], "%ifndef", 7) == 0 && isspace(z[i + 7]))) {
      if (exclude) {
        exclude++;
      } else {
        for (j = i + 7; isspace(z[j]); j++) {
        }
        for (n = 0; z[j + n] && !isspace(z[j + n]); n++) {
        }
        exclude = 1;
        for (k = 0; k < nDefine; k++) {
          if (strncmp(azDefine[k], &z[j], n) == 0 && strlen(azDefine[k]) == n) {
            exclude = 0;
            break;
          }
        }
        if (z[i + 3] == 'n')
          exclude = !exclude;
        if (exclude) {
          start = i;
          start_lineno = lineno;
        }
      }
      for (j = i; z[j] && z[j] != '\n'; j++)
        z[j] = ' ';
    }
  }
  if (exclude) {
    fprintf(stderr, "unterminated %%ifdef starting on line %d\n", start_lineno);
    exit(1);
  }
}
