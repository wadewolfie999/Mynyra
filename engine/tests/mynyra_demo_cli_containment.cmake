if(NOT DEFINED TRADEBOT_EXECUTABLE)
    message(FATAL_ERROR "TRADEBOT_EXECUTABLE is required")
endif()

execute_process(
    COMMAND "${TRADEBOT_EXECUTABLE}" --mode demo --provider ctrader
            --symbol XAUUSD --timeframe M1 unexpected.csv
    RESULT_VARIABLE csv_result
    OUTPUT_VARIABLE csv_stdout
    ERROR_VARIABLE csv_stderr)
if(csv_result EQUAL 0)
    message(FATAL_ERROR "DEMO accepted a positional CSV input")
endif()
if(NOT csv_stderr MATCHES "DEMO rejects")
    message(FATAL_ERROR "DEMO CSV rejection was not explicit: ${csv_stderr}")
endif()

execute_process(
    COMMAND "${TRADEBOT_EXECUTABLE}" --mode demo --provider ctrader
            --symbol XAUUSD --timeframe M5
    RESULT_VARIABLE timeframe_result
    OUTPUT_VARIABLE timeframe_stdout
    ERROR_VARIABLE timeframe_stderr)
if(timeframe_result EQUAL 0)
    message(FATAL_ERROR "DEMO accepted an unsupported timeframe")
endif()
if(NOT timeframe_stderr MATCHES "fixed to")
    message(FATAL_ERROR "DEMO timeframe rejection was not explicit: ${timeframe_stderr}")
endif()

execute_process(
    COMMAND "${TRADEBOT_EXECUTABLE}" --mode backtest --commission-demo-order
    RESULT_VARIABLE mode_result
    OUTPUT_VARIABLE mode_stdout
    ERROR_VARIABLE mode_stderr)
if(mode_result EQUAL 0)
    message(FATAL_ERROR "non-DEMO mode accepted a commissioning flag")
endif()
if(NOT mode_stderr MATCHES "require --mode demo")
    message(FATAL_ERROR "commissioning mode rejection was not explicit: ${mode_stderr}")
endif()

foreach(override_flag IN ITEMS --endpoint --account --volume)
    execute_process(
        COMMAND "${TRADEBOT_EXECUTABLE}" --mode demo --provider ctrader
                --symbol XAUUSD --timeframe M1 "${override_flag}" unsafe
        RESULT_VARIABLE override_result
        OUTPUT_VARIABLE override_stdout
        ERROR_VARIABLE override_stderr)
    if(override_result EQUAL 0)
        message(FATAL_ERROR "DEMO accepted forbidden override ${override_flag}")
    endif()
    if(NOT override_stderr MATCHES "unsupported option")
        message(FATAL_ERROR
            "DEMO override rejection was not explicit for ${override_flag}: ${override_stderr}")
    endif()
endforeach()
