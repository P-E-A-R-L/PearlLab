# Set default file path
if(NOT DEFINED YAML_FILE)
    set(YAML_FILE "${CMAKE_SOURCE_DIR}/config.yaml")
endif()

# Run Python YAML parser
execute_process(
        COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/parse_yaml.py ${YAML_FILE}
        OUTPUT_VARIABLE YAML_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

# Split output lines and set as CMake variables
string(REPLACE "\n" ";" YAML_LINES "${YAML_OUTPUT}")

foreach(line IN LISTS YAML_LINES)
    string(REGEX MATCH "^([A-Za-z0-9_.-]+)=(.*)$" matched "${line}")
    if(matched)
        string(REGEX REPLACE "^([A-Za-z0-9_.-]+)=(.*)$" "\\1" key "${line}")
        string(REGEX REPLACE "^([A-Za-z0-9_.-]+)=(.*)$" "\\2" value "${line}")
        set(${key} "${value}" PARENT_SCOPE)
        message(STATUS "[YAML] ${key}=${value}")
    endif()
endforeach()
