# Links crash reporting (vellum_diag) into an app target and sets Sentry compile definitions.
#
# Usage (after add_subdirectory(libs/vellum-diag)):
#   vellum_target_sentry(vellum "vellum@${PROJECT_VERSION}" DSN VELLUM_SENTRY_DSN)
#   vellum_target_sentry(vellum-lsp "vellum-lsp@${PROJECT_VERSION}" DSN VELLUM_LSP_SENTRY_DSN)
function(vellum_target_sentry target release)
  if(NOT TARGET vellum_diag)
    message(FATAL_ERROR "vellum_target_sentry: vellum_diag must be defined first")
  endif()

  cmake_parse_arguments(SENTRY "" "DSN" "" ${ARGN})

  set(_dsn "")
  if(SENTRY_DSN AND DEFINED ${SENTRY_DSN} AND NOT "${${SENTRY_DSN}}" STREQUAL "")
    set(_dsn "${${SENTRY_DSN}}")
  elseif(NOT SENTRY_DSN AND DEFINED VELLUM_SENTRY_DSN AND NOT VELLUM_SENTRY_DSN STREQUAL "")
    set(_dsn "${VELLUM_SENTRY_DSN}")
  endif()

  target_link_libraries(${target} PRIVATE vellum_diag)
  target_compile_definitions(${target} PRIVATE
    "VELLUM_SENTRY_RELEASE=\"${release}\"")
  if(NOT _dsn STREQUAL "")
    target_compile_definitions(${target} PRIVATE
      "VELLUM_SENTRY_DSN=\"${_dsn}\"")
  endif()
  if(DEFINED VELLUM_SENTRY_ENVIRONMENT AND NOT VELLUM_SENTRY_ENVIRONMENT STREQUAL "")
    target_compile_definitions(${target} PRIVATE
      "VELLUM_SENTRY_ENVIRONMENT=\"${VELLUM_SENTRY_ENVIRONMENT}\"")
  endif()
endfunction()
