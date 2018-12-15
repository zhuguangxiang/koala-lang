
FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)

ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_SOURCE_DIR}/src/koala_yacc.c
         ${PROJECT_SOURCE_DIR}/include/koala_tokens.h
  DEPENDS ${PROJECT_SOURCE_DIR}/src/yacc/koala.y
  COMMAND ${BISON_EXECUTABLE}
          -dvt
          --defines=${PROJECT_SOURCE_DIR}/include/koala_tokens.h
          --output=${PROJECT_SOURCE_DIR}/src/koala_yacc.c
          ${PROJECT_SOURCE_DIR}/src/yacc/koala.y
  )

ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_SOURCE_DIR}/src/koala_lex.c
         ${PROJECT_SOURCE_DIR}/include/koala_lex.h
  DEPENDS ${PROJECT_SOURCE_DIR}/src/yacc/koala.l
  COMMAND ${FLEX_EXECUTABLE}
          --header-file=${PROJECT_SOURCE_DIR}/include/koala_lex.h
          --outfile=${PROJECT_SOURCE_DIR}/src/koala_lex.c
          ${PROJECT_SOURCE_DIR}/src/yacc/koala.l
  )

SET_SOURCE_FILES_PROPERTIES(koala_lex.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(koala_tokens.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(koala_lex.c GENERATED)
SET_SOURCE_FILES_PROPERTIES(koala_yacc.c GENERATED)

SET (LIBKOALA_SRC
      log.c vector.c hashfunc.c hashtable.c buffer.c atomstring.c mem.c
      cache.c properties.c
      typedesc.c object.c stringobject.c tupleobject.c codeobject.c
      intobject.c package.c state.c atomtable.c image.c options.c eval.c
    )

SET (NATIVE_SRC
      native/io.c
    )

ADD_LIBRARY(koala SHARED ${LIBKOALA_SRC} ${NATIVE_SRC})

set_target_properties(koala PROPERTIES VERSION 0.8.5 SOVERSION 0)

SET (KOALAC_SRC
      koalac.c koala_lex.c koala_yacc.c symbol.c ast.c parser.c codegen.c
      checker.c
    )

add_executable(klc ${KOALAC_SRC})
target_link_libraries(klc koala)
set_target_properties(klc PROPERTIES OUTPUT_NAME koalac)

add_executable(kl koala.c)
target_link_libraries(kl koala task)
set_target_properties(kl PROPERTIES OUTPUT_NAME koala)

install (TARGETS koala DESTINATION lib/koala)

install (TARGETS klc DESTINATION bin)
install (TARGETS kl DESTINATION bin)
