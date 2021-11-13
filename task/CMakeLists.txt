#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(TASK_SRCS
    task.c timer.c event.c)

add_library(task STATIC ${TASK_SRCS})
target_link_libraries(task util pthread)
set_target_properties(task PROPERTIES
    OUTPUT_NAME koala-task)

add_library(task_so SHARED ${TASK_SRCS})
target_link_libraries(task_so util_so pthread)
set_target_properties(task_so PROPERTIES
    VERSION ${KOALA_VERSION}
    SOVERSION ${KOALA_VERSION_MAJOR}
    OUTPUT_NAME koala-task)

install(TARGETS task DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS task_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/task/
    DESTINATION koala-${KOALA_VERSION_MAIN}/include/task
    FILES_MATCHING
    PATTERN "*.h")
