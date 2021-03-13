#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PROJECT_NAME "koala-lang")
SET(SKIP_TESTS 0)
SET(DEBUG_TYPE DEBUG)

PROJECT(${PROJECT_NAME})

IF(DEBUG_TYPE STREQUAL DEBUG)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -g -fvar-tracking")
ENDIF()

MESSAGE("-- dir:" ${CMAKE_CURRENT_BINARY_DIR}/src)

# LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/src) 
INCLUDE_DIRECTORIES(include)

# ADD_SUBDIRECTORY(src)

IF(NOT SKIP_TESTS)
    ENABLE_TESTING()
    ADD_SUBDIRECTORY(test)
ENDIF()
