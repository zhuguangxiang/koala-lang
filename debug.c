
#include "debug.h"

#if defined(DBG_INFO)
int debug_level = DEBUG_LEVEL_INFO;
#elif defined(DBG_WARN)
int debug_level = DEBUG_LEVEL_WARN;
#elif defined(DBG_ERROR)
int debug_level = DEBUG_LEVEL_ERROR;
#else
int debug_level = DEBUG_LEVEL_INFO;
#endif
