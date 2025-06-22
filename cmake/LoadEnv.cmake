# cmake/LoadEnv.cmake
# this loads all the variables from a .env file into the CMake scope.

# Allow passing a custom path or default to ".env" in project root
if(NOT DEFINED ENV_FILE)
    set(ENV_FILE "${CMAKE_SOURCE_DIR}/.env")
endif()

if(EXISTS "${ENV_FILE}")
    file(READ "${ENV_FILE}" ENV_CONTENTS)
    string(REPLACE "\r\n" "\n" ENV_CONTENTS "${ENV_CONTENTS}")
    string(REPLACE "\n" ";" ENV_LINES "${ENV_CONTENTS}")

    foreach(line IN LISTS ENV_LINES)
        string(STRIP "${line}" line)
        if(line MATCHES "^#")    # Skip comments
            continue()
        endif()

        string(REGEX MATCH "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" matched "${line}")
        if(matched)
            string(REGEX REPLACE "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" "\\1" key "${line}")
            string(REGEX REPLACE "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" "\\2" value "${line}")

            set(${key} "${value}")

            message(STATUS "[ENV] Loaded ${key}=${value}")
        endif()
    endforeach()
else()
    message(WARNING "No ENV file found at ${ENV_FILE}")
endif()
