
FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)

ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_SOURCE_DIR}/src/koala_yacc.c
         ${PROJECT_SOURCE_DIR}/include/koala_yacc.h
  DEPENDS ${PROJECT_SOURCE_DIR}/src/yacc/koala.y
  COMMAND ${BISON_EXECUTABLE}
          -dvt
          --defines=${PROJECT_SOURCE_DIR}/include/koala_yacc.h
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

SET (KOALA_COMMON_SRC
  log.c vector.c hashfunc.c hashtable.c buffer.c atomstring.c mem.c
  typedesc.c cache.c stringbuf.c properties.c atomtable.c image.c options.c
  stringex.c pkgnode.c opcode.c
  )
ADD_LIBRARY(common STATIC ${KOALA_COMMON_SRC})
set_target_properties(common PROPERTIES VERSION 0.8.5 SOVERSION 0)

SET (KOALAC_SRC
  koala_lex.c koala_yacc.c symbol.c ast.c parser.c codegen.c koalac.c
  parser_ident.c parser_operator.c parser_array.c parser_closure.c parser_map.c
  )
add_executable(klc ${KOALAC_SRC})
target_link_libraries(klc common)
set_target_properties(klc PROPERTIES OUTPUT_NAME koalac)

SET (KOALA_OBJECT_SRC
  object.c eval.c state.c package.c
  objects/codeobject.c objects/tupleobject.c 
  objects/stringobject.c objects/intobject.c 
  packages/lang.c packages/io.c
  )
ADD_LIBRARY(koala STATIC ${KOALA_OBJECT_SRC})
target_link_libraries(koala common task)
set_target_properties(koala PROPERTIES VERSION 0.8.5 SOVERSION 0)

add_executable(kl koala.c)
target_link_libraries(kl koala)
set_target_properties(kl PROPERTIES OUTPUT_NAME koala)

install (TARGETS klc DESTINATION bin)
install (TARGETS kl DESTINATION bin)
