set(TARGET_NAME ${PROJECT_NAME})

add_subdirectory(flags)

set(SOURCES
    application.cpp
    config.cpp
    controller.cpp
    fix_log.cpp
    session.cpp
    session_manager.cpp
    settings.cpp
    shared.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_subdirectory(tools)

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_NAME}-flags
          ${PROJECT_NAME}-tools
          roq-api::roq-api
          roq-cache::roq-cache
          roq-client::roq-client
          roq-client::roq-client-flags
          roq-core::roq-core
          roq-fix::roq-fix
          roq-flags::roq-flags
          roq-io::roq-io
          roq-codec::roq-codec
          roq-logging::roq-logging
          roq-logging::roq-logging-flags
          roq-market::roq-market
          fmt::fmt
          tomlplusplus::tomlplusplus
          unordered_dense::unordered_dense)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

target_compile_definitions(
  ${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${TARGET_NAME}" ROQ_HOST="${ROQ_HOST}" ROQ_BUILD_VERSION="${GIT_REPO_VERSION}"
                         ROQ_GIT_DESCRIBE_HASH="${GIT_DESCRIBE_HASH}" ROQ_BUILD_NUMBER="${ROQ_BUILD_NUMBER}" ROQ_BUILD_TYPE="${ROQ_BUILD_TYPE}")

install(TARGETS ${TARGET_NAME})

if(BUILD_DOCS)

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}
    FILES_MATCHING
    PATTERN "*.rst*"
    PATTERN "CMakeFiles" EXCLUDE)

endif()
