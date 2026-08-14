set(generated_paths
    data/results/snapshot.json
    data/results/run.csv
    data/archive/state.json
    data/historical/replay.bin
    output/report.json
    handoff/session.md
    handoffs/session.md
    artifacts/evidence.json)
foreach(path IN LISTS generated_paths)
    execute_process(
        COMMAND git check-ignore -q -- "${path}"
        WORKING_DIRECTORY "${TRADEBOT_SOURCE_DIR}"
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "generated path is not ignored: ${path}")
    endif()
endforeach()

foreach(path IN ITEMS data/samples/replay.csv tests/fixtures/replay.bin)
    execute_process(
        COMMAND git check-ignore -q -- "${path}"
        WORKING_DIRECTORY "${TRADEBOT_SOURCE_DIR}"
        RESULT_VARIABLE result)
    if(result EQUAL 0)
        message(FATAL_ERROR "intentional fixture path is hidden: ${path}")
    endif()
endforeach()
