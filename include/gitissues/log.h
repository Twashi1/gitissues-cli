#ifndef _GITISSUES_LOG_H_
#define _GITISSUES_LOG_H_

#include <gitissues/defines.h>
#include <stdarg.h>
#include <stdio.h>

enum LogLevel { LOG_LEVEL_DEBUG, LOG_LEVEL_WARN, LOG_LEVEL_ERROR };

struct LogStats {
  int numErrors;
  int numWarnings;
  int numDebugs;
};

extern struct LogStats _logStats;

// TODO: hacky way to be able to detect if an error occurred
static inline int getNumErrors(void) { return _logStats.numErrors; }

static inline void _logMessage(enum LogLevel level, char const *file, int line,
                               char const *fmt, ...) {
  va_list args;

  char const *levelStr = NULL;

  switch (level) {
  case LOG_LEVEL_DEBUG:
    _logStats.numDebugs++;
    levelStr = "DEBUG";
    break;
  case LOG_LEVEL_WARN:
    _logStats.numWarnings++;
    levelStr = "WARN";
    break;
  case LOG_LEVEL_ERROR:
    _logStats.numErrors++;
    levelStr = "ERROR";
    break;
  }

  printf("%s [%s:%d] ", levelStr, file, line);

  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  putchar('\n');
}

#define GITISSUES_LOG_LEVEL(level, fmt, ...)                                   \
  do {                                                                         \
    _logMessage(level, (char *)__FILE__ + GITISSUES_SOURCE_DIR_LENGTH,         \
                __LINE__, fmt, ##__VA_ARGS__);                                 \
  } while (0)

#if defined(NDEBUG)
#define GITISSUES_LOG_DEBUG(str, ...) ((void)0)
#define GITISSUES_LOG_WARN(str, ...) ((void)0)
#define GITISSUES_LOG_ERROR(str, ...) ((void)0)
#else
#define GITISSUES_LOG_DEBUG(str, ...)                                          \
  GITISSUES_LOG_LEVEL(LOG_LEVEL_DEBUG, str, ##__VA_ARGS__)
#define GITISSUES_LOG_WARN(str, ...)                                           \
  GITISSUES_LOG_LEVEL(LOG_LEVEL_WARN, str, ##__VA_ARGS__)
#define GITISSUES_LOG_ERROR(str, ...)                                          \
  GITISSUES_LOG_LEVEL(LOG_LEVEL_ERROR, str, ##__VA_ARGS__)
#endif

#endif
