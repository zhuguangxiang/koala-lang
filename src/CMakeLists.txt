
FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)

ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_SOURCE_DIR}/src/koala_yacc.c
  DEPENDS ${PROJECT_SOURCE_DIR}/src/yacc/koala.y
  COMMAND ${BISON_EXECUTABLE}
          --defines=${PROJECT_SOURCE_DIR}/include/koala_tokens.h
          --output=${PROJECT_SOURCE_DIR}/src/koala_yacc.c 
          ${PROJECT_SOURCE_DIR}/src/yacc/koala.y
  )

ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_SOURCE_DIR}/src/koala_lex.c
  DEPENDS ${PROJECT_SOURCE_DIR}/src/yacc/koala.l
  COMMAND ${FLEX_EXECUTABLE}
          --header-file=${PROJECT_SOURCE_DIR}/include/koala_lex.h
          --outfile=${PROJECT_SOURCE_DIR}/src/koala_lex.c
          ${PROJECT_SOURCE_DIR}/src/yacc/koala.l
  )

SET (LIBKOALAC_SRC
      koala_lex.c koala_yacc.c symbol.c ast.c parser.c codegen.c checker.c
    )

SET_SOURCE_FILES_PROPERTIES(koala_lex.c GENERATED)
SET_SOURCE_FILES_PROPERTIES(koala_yacc.c GENERATED)

ADD_LIBRARY(koalac SHARED ${LIBKOALAC_SRC})

set_target_properties(koalac PROPERTIES VERSION 0.5 SOVERSION 0)

SET (LIBKOALA_SRC
      log.c vector.c hashfunc.c hashtable.c buffer.c atomstring.c mem.c
      cache.c properties.c 
      typedesc.c object.c stringobject.c tupleobject.c codeobject.c 
      intobject.c package.c state.c atomtable.c opcode.c image.c
    )

ADD_LIBRARY(koala SHARED ${LIBKOALA_SRC})

set_target_properties(koala PROPERTIES VERSION 0.5 SOVERSION 0)
