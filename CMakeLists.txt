PROJECT(cppcbor)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

FIND_PACKAGE(Boost COMPONENTS unit_test_framework REQUIRED)

SET( CMAKE_CXX_FLAGS "-Wextra -Wall")

SET (HEADERS
    src/value.h
    src/cbor.h
    src/cborwriter.h
    src/cborreader.h
    src/cborprivate.h
)

SET (SOURCES
    src/value.cpp
    src/cborwriter.cpp
    src/cborreader.cpp
    tests/main.cpp
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

ADD_EXECUTABLE(test ${SOURCES} ${HEADERS})

TARGET_LINK_LIBRARIES(test
    ${Boost_LIBRARIES}
)
