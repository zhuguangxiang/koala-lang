SET (LIBKOALA_SRC
      log.c vector.c hashfunc.c hashtable.c buffer.c atomstring.c mem.c
      typedesc.c object.c stringobject.c tupleobject.c codeobject.c 
      intobject.c
    )

ADD_LIBRARY(koala SHARED ${LIBKOALA_SRC})

set_target_properties(koala PROPERTIES VERSION 0.5 SOVERSION 0)