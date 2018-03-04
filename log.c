
#include "log.h"

int loglevel =
#if defined(LOG_INFO)
LOG_LEVEL_INFO
#elif defined(LOG_WARN)
LOG_LEVEL_WARN
#elif defined(LOG_ERROR)
LOG_LEVEL_ERROR
#else
LOG_LEVEL_INFO
#endif
;
