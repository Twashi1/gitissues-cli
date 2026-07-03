#ifndef _GITISSUES_LOG_H_
#define _GITISSUES_LOG_H_

#include <gitissues/defines.h>
#include <gitissues/errs.h>
#include <stdarg.h>
#include <stdio.h>

static inline void _logMessage(char const *level, char const *file, int line,
                               char const *fmt, ...) {
  va_list args;

  printf("%s [%s:%d] ", level, file, line);

  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  putchar('\n');
}

#define GITISSUES_LOG_LEVEL(level, fmt, ...)                                   \
  do {                                                                         \
    _logMessage(level, __FILE__ + GITISSUES_SOURCE_DIR_LENGTH, __LINE__, fmt,  \
                ##__VA_ARGS__);                                                \
  } while (0)

#if defined(NDEBUG)
#define GITISSUES_LOG_DEBUG(str, ...) ((void)0)
#define GITISSUES_LOG_WARN(str, ...) ((void)0)
#define GITISSUES_LOG_ERROR(str, ...) ((void)0)
#else
#define GITISSUES_LOG_DEBUG(str, ...)                                          \
  GITISSUES_LOG_LEVEL("DEBUG", str, ##__VA_ARGS__)
#define GITISSUES_LOG_WARN(str, ...)                                           \
  GITISSUES_LOG_LEVEL("WARN", str, ##__VA_ARGS__)
#define GITISSUES_LOG_ERROR(str, ...)                                          \
  GITISSUES_LOG_LEVEL("ERROR", str, ##__VA_ARGS__)
#endif

#endif
