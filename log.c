
#include "log.h"

#if defined(LOG_INFO)
int loglevel = LOG_LEVEL_INFO;
#elif defined(LOG_WARN)
int loglevel = LOG_LEVEL_WARN;
#elif defined(LOG_ERROR)
int loglevel = LOG_LEVEL_ERROR;
#else
int loglevel = LOG_LEVEL_INFO;
#endif
