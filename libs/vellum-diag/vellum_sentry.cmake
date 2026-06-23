# Links crash reporting (vellum_diag) into an app target and sets Sentry compile definitions.
#
# Usage (after add_subdirectory(libs/vellum-diag)):
#   vellum_target_sentry(my_app "my-app@${PROJECT_VERSION}")
function(vellum_target_sentry target release)
  if(NOT TARGET vellum_diag)
    message(FATAL_ERROR "vellum_target_sentry: vellum_diag must be defined first")
  endif()

  target_link_libraries(${target} PRIVATE vellum_diag)
  target_compile_definitions(${target} PRIVATE
    "VELLUM_SENTRY_RELEASE=\"${release}\"")
  if(DEFINED VELLUM_SENTRY_DSN AND NOT VELLUM_SENTRY_DSN STREQUAL "")
    target_compile_definitions(${target} PRIVATE
      "VELLUM_SENTRY_DSN=\"${VELLUM_SENTRY_DSN}\"")
  endif()
  if(DEFINED VELLUM_SENTRY_ENVIRONMENT AND NOT VELLUM_SENTRY_ENVIRONMENT STREQUAL "")
    target_compile_definitions(${target} PRIVATE
      "VELLUM_SENTRY_ENVIRONMENT=\"${VELLUM_SENTRY_ENVIRONMENT}\"")
  endif()
endfunction()
