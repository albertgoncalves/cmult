#ifndef __EXIT_H__
#define __EXIT_H__

#include <stdio.h>
#include <stdlib.h>

#define BOLD_PURPLE    "\033[1;35m"
#define BOLD           "\033[1m"
#define BOLD_UNDERLINE "\033[1;4m"
#define CLOSE          "\033[0m"

#define EXIT_IF(condition)                                  \
    if (condition) {                                        \
        fprintf(stderr,                                     \
                "<%s%s%s>.%s%s%s() @%sline %d%s: %sExit%s", \
                BOLD,                                       \
                __FILE__,                                   \
                CLOSE,                                      \
                BOLD,                                       \
                __func__,                                   \
                CLOSE,                                      \
                BOLD_UNDERLINE,                             \
                __LINE__,                                   \
                CLOSE,                                      \
                BOLD_PURPLE,                                \
                CLOSE);                                     \
        exit(EXIT_FAILURE);                                 \
    }

#endif
