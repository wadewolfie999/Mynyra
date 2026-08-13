foreach(mode IN ITEMS paper live)
    execute_process(
        COMMAND "${TRADEBOT_EXECUTABLE}" --mode "${mode}" --resume missing.json
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)
    if(result EQUAL 0)
        message(FATAL_ERROR "${mode} resume unexpectedly succeeded")
    endif()
    if(NOT error MATCHES "--resume is BACKTEST-only")
        message(FATAL_ERROR "${mode} resume did not fail at persistence boundary: ${error}")
    endif()
    if(error MATCHES "Unable to open CSV" OR output MATCHES "LiveDataAdapter|BrokerGateway")
        message(FATAL_ERROR "${mode} resume crossed startup side-effect boundary")
    endif()
endforeach()
