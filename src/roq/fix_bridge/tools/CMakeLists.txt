set(TARGET_NAME ${PROJECT_NAME}-tools)

set(SOURCES crypto.cpp mapping.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)

add_dependencies(${TARGET_NAME} ${PROJECT_NAME}-flags-autogen-headers)
