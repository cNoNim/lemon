#ifndef _LEMON_REPORT_H_
#define _LEMON_REPORT_H_

/*
 * Procedures for generating reports and tables in the LEMON parser generator.
 */

void Reprint(struct lemon *);
void ReportOutput(struct lemon *);
void ReportTable(struct lemon *, int);
void ReportHeader(struct lemon *);
void CompressTables(struct lemon *);
void ResortStates(struct lemon *);

#endif //_LEMON_REPORT_H_
