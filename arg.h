#ifndef _LEMON_ARG_H_
#define _LEMON_ARG_H_

extern char * argv0;

#define USED(x) ((void)(x))

#define ARGBEGIN  for ((argv0||(argv0 = *argv)), argv++, argc--;   \
                       argv[0] && argv[0][1] && argv[0][0] == '-'; \
                       argc--, argv++) {                           \
                    char _argch = '\0', ** _argv; int brk;         \
                    if (argv[0][1] == '-' && argv[0][2] == '\0') { \
                      argv++;                                      \
                      argc--;                                      \
                      break;                                       \
                    }                                              \
                    for (brk = 0, argv[0]++, _argv = argv;         \
                         argv[0][0] && !brk;                       \
                         argv[0]++) {                              \
                      if (_argv != argv) break;                    \
                      _argch = argv[0][0];                         \
                      switch (_argch)
#define ARGEND      } USED(_argch);                                \
                  } USED(argv); USED(argc);
#define ARGCH()   _argch
#define EARGF(x)  ((argv[0][1] == '\0' && argv[1] == NULL) ?        \
                    ((x), abort(), (char *)0) :                    \
                    (brk = 1, argv[0][1] != '\0' ?                 \
                     (&argv[0][0]) :                               \
                     (argc--, argv++, argv[0])))
#define ARGF()    ((argv[0][1] == '\0' && argv[1] == NULL) ?        \
                    (char *)0 :                                    \
                    (brk = 1, argv[0][1] != '\0' ?                 \
                     (&argv[0][0]) :                               \
                     (argc--, argv++, argv[0])))


#endif //_LEMON_ARG_H_
