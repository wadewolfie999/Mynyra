foreach(mode IN ITEMS paper live)
    execute_process(
        COMMAND "${TRADEBOT_EXECUTABLE}" --mode "${mode}" --resume data/results/snapshot.json
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

execute_process(
    COMMAND "${TRADEBOT_EXECUTABLE}" --mode backtest --resume ../outside.json
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
if(result EQUAL 0)
    message(FATAL_ERROR "uncontrolled resume path unexpectedly succeeded")
endif()
if(NOT error MATCHES "--resume path must be data/results/snapshot.json")
    message(FATAL_ERROR "uncontrolled resume path did not fail at path boundary: ${error}")
endif()
if(error MATCHES "Unable to open CSV" OR output MATCHES "LiveDataAdapter|BrokerGateway")
    message(FATAL_ERROR "uncontrolled resume path crossed startup side-effect boundary")
endif()
