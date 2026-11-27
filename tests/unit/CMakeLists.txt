set(UNIT_TEST_ENV "QT_QPA_PLATFORM=offscreen")
if(WIN32)
    get_filename_component(_qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
    file(TO_NATIVE_PATH "${_qt_bin_dir}" _qt_bin_dir_native)
    string(REPLACE ";" "\\;" _escaped_path "$ENV{PATH}")
    set(UNIT_TEST_ENV
        "QT_QPA_PLATFORM=offscreen"
        "PATH=${_qt_bin_dir_native}\\;${_escaped_path}"
    )
endif()

function(add_deskmon_unit_test target_name source_file)
    qt_add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE DeskmonCore Qt6::Test)
    add_test(NAME ${target_name} COMMAND ${target_name})
    set_tests_properties(${target_name} PROPERTIES ENVIRONMENT "${UNIT_TEST_ENV}")
endfunction()

add_deskmon_unit_test(test_database_and_repos test_database_and_repos.cpp)
add_deskmon_unit_test(test_task_management test_task_management.cpp)
add_deskmon_unit_test(test_services_idle_auth test_services_idle_auth.cpp)
add_deskmon_unit_test(test_updater test_updater.cpp)
