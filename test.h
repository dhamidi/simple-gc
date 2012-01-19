#ifndef TEST_H
#define TEST_H

#include <stdio.h>

static int ntests;
static int nfailed;
#define ok(expr,msg)                                    \
     do {                                               \
          ntests++;                                     \
          if (expr)                                     \
               printf("ok %d - %s\n",ntests,msg);       \
          else {                                        \
               printf("not ok %d - %s\n",ntests,msg);   \
               nfailed++;                               \
          }                                             \
     } while(0);

#define finish() \
     do {                                       \
          if (nfailed != 0)                     \
               printf("Failed %d/%d tests\n",   \
                      nfailed,ntests);          \
     } while(0);

#endif
