if (NOT DEFINED TRADEBOT_EXECUTABLE)
    message(FATAL_ERROR "TRADEBOT_EXECUTABLE is required")
endif()

execute_process(
    COMMAND "${TRADEBOT_EXECUTABLE}" --mode live
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if (result EQUAL 0)
    message(FATAL_ERROR "default build unexpectedly accepted --mode live")
endif()

if (NOT error MATCHES "LIVE runtime is contained")
    message(FATAL_ERROR
        "LIVE startup did not fail at the WP-0 containment boundary: ${error}")
endif()

if (output MATCHES "Auth loaded|Connecting to|network bridge established")
    message(FATAL_ERROR "LIVE startup crossed into credential/network setup")
endif()
