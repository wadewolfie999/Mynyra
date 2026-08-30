if (NOT DEFINED REPLAY_EXECUTABLE OR NOT EXISTS "${REPLAY_EXECUTABLE}")
    message(FATAL_ERROR "REPLAY_EXECUTABLE is required")
endif()

file(REMOVE_RECURSE "${REPLAY_TEST_ROOT}")
file(MAKE_DIRECTORY "${REPLAY_TEST_ROOT}")

set(INPUT "${REPLAY_TEST_ROOT}/candles.csv")
set(CONFIG "${REPLAY_TEST_ROOT}/config.txt")
set(MANIFEST "${REPLAY_TEST_ROOT}/manifest.txt")
file(WRITE "${INPUT}" "1,XAUUSD,100,102,99,101,10\n2,XAUUSD,101,103,100,102,11\n")
file(WRITE "${CONFIG}" "runner_schema=1\nstrategy=offline_replay\n")
file(SHA256 "${REPLAY_EXECUTABLE}" ARTIFACT_SHA)
file(SHA256 "${INPUT}" INPUT_SHA)
file(SHA256 "${CONFIG}" CONFIG_SHA)

function(write_manifest PATH MAX_RECORDS)
    file(WRITE "${PATH}"
        "schema_version=1\n"
        "run_id=offline-cli-test\n"
        "artifact_sha256=${ARTIFACT_SHA}\n"
        "input_sha256=${INPUT_SHA}\n"
        "config_sha256=${CONFIG_SHA}\n"
        "mode=BACKTEST\n"
        "max_input_bytes=4096\n"
        "max_config_bytes=4096\n"
        "max_records=${MAX_RECORDS}\n"
        "max_runtime_milliseconds=10000\n"
        "provider_allowed=false\n"
        "orders_allowed=false\n")
endfunction()

write_manifest("${MANIFEST}" 8)
foreach(RUN_NAME IN ITEMS first second)
    execute_process(
        COMMAND "${REPLAY_EXECUTABLE}" --manifest "${MANIFEST}" --input "${INPUT}"
                --config "${CONFIG}" --evidence-dir "${REPLAY_TEST_ROOT}/${RUN_NAME}"
        RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE OUTPUT
        ERROR_VARIABLE ERROR)
    if (NOT RESULT EQUAL 0)
        message(FATAL_ERROR "valid offline replay failed: ${RESULT}: ${ERROR}")
    endif()
endforeach()
file(SHA256 "${REPLAY_TEST_ROOT}/first/result.json" FIRST_SHA)
file(SHA256 "${REPLAY_TEST_ROOT}/second/result.json" SECOND_SHA)
if (NOT FIRST_SHA STREQUAL SECOND_SHA)
    message(FATAL_ERROR "repeated offline evidence was not deterministic")
endif()

file(APPEND "${INPUT}" "3,XAUUSD,102,104,101,103,12\n")
execute_process(
    COMMAND "${REPLAY_EXECUTABLE}" --manifest "${MANIFEST}" --input "${INPUT}"
            --config "${CONFIG}" --evidence-dir "${REPLAY_TEST_ROOT}/tampered-input"
    RESULT_VARIABLE RESULT)
if (RESULT EQUAL 0 OR NOT EXISTS "${REPLAY_TEST_ROOT}/tampered-input/result.json")
    message(FATAL_ERROR "tampered input did not produce rejected evidence")
endif()
file(READ "${REPLAY_TEST_ROOT}/tampered-input/result.json" TAMPERED_EVIDENCE)
if (NOT TAMPERED_EVIDENCE MATCHES "INPUT_REJECTED")
    message(FATAL_ERROR "tampered input evidence is missing rejection status")
endif()

file(WRITE "${REPLAY_TEST_ROOT}/unknown-manifest.txt" "unknown=value\n")
execute_process(
    COMMAND "${REPLAY_EXECUTABLE}" --manifest "${REPLAY_TEST_ROOT}/unknown-manifest.txt"
            --input "${INPUT}" --config "${CONFIG}"
            --evidence-dir "${REPLAY_TEST_ROOT}/unknown"
    RESULT_VARIABLE RESULT)
if (RESULT EQUAL 0)
    message(FATAL_ERROR "unknown manifest key was accepted")
endif()

file(WRITE "${INPUT}" "1,XAUUSD,100,102,99,101,10\n2,XAUUSD,101,103,100,102,11\n")
file(SHA256 "${INPUT}" INPUT_SHA)
write_manifest("${REPLAY_TEST_ROOT}/resource-manifest.txt" 1)
execute_process(
    COMMAND "${REPLAY_EXECUTABLE}" --manifest "${REPLAY_TEST_ROOT}/resource-manifest.txt"
            --input "${INPUT}" --config "${CONFIG}"
            --evidence-dir "${REPLAY_TEST_ROOT}/resource"
    RESULT_VARIABLE RESULT)
if (RESULT EQUAL 0 OR NOT EXISTS "${REPLAY_TEST_ROOT}/resource/result.json")
    message(FATAL_ERROR "resource exhaustion did not produce failed evidence")
endif()
file(READ "${REPLAY_TEST_ROOT}/resource/result.json" RESOURCE_EVIDENCE)
if (NOT RESOURCE_EVIDENCE MATCHES "RESOURCE_EXHAUSTED")
    message(FATAL_ERROR "resource failure evidence is missing terminal status")
endif()

file(MAKE_DIRECTORY "${REPLAY_TEST_ROOT}/existing-output")
write_manifest("${MANIFEST}" 8)
execute_process(
    COMMAND "${REPLAY_EXECUTABLE}" --manifest "${MANIFEST}" --input "${INPUT}"
            --config "${CONFIG}" --evidence-dir "${REPLAY_TEST_ROOT}/existing-output"
    RESULT_VARIABLE RESULT)
if (RESULT EQUAL 0 OR EXISTS "${REPLAY_TEST_ROOT}/existing-output/result.json")
    message(FATAL_ERROR "existing output directory was accepted")
endif()
