cmake_minimum_required(VERSION 3.14)

project(TradeBot-AIIO-Core
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)
find_library(DL_LIB dl)

# Gate 6 proof-only provider bindings. The exact compiler/runtime and pinned
# schema hashes are part of the accepted Gate 1 provenance contract.
option(TRADEBOT_ENABLE_CTRADER_GATE6
    "Build the authorized local-only cTrader Gate 6 proof target" OFF)
if (TRADEBOT_ENABLE_CTRADER_GATE6)
    if (NOT APPLE)
        message(FATAL_ERROR
            "Gate 6 runtime requires the authorized macOS Keychain environment")
    endif()
    enable_language(OBJCXX)
    find_package(Protobuf 35.1.0 EXACT CONFIG REQUIRED)
    find_package(CURL 8.7.1 REQUIRED)
    find_package(OpenSSL 3.6.3 REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit)
    find_library(SECURITY_FRAMEWORK Security)
    if (NOT APPKIT_FRAMEWORK OR NOT SECURITY_FRAMEWORK)
        message(FATAL_ERROR "Gate 6 requires AppKit and Security frameworks")
    endif()
    if (NOT Protobuf_VERSION VERSION_EQUAL "35.1.0")
        message(FATAL_ERROR
            "Gate 6 requires exact Protobuf package 35.1.0; found ${Protobuf_VERSION}")
    endif()

    set(CTRADER_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ctrader/openapi-proto")
    set(CTRADER_PROTO_FILES
        "${CTRADER_PROTO_DIR}/OpenApiCommonMessages.proto"
        "${CTRADER_PROTO_DIR}/OpenApiCommonModelMessages.proto"
        "${CTRADER_PROTO_DIR}/OpenApiMessages.proto"
        "${CTRADER_PROTO_DIR}/OpenApiModelMessages.proto"
    )
    set(CTRADER_PROTO_EXPECTED_HASHES
        "9816cd24b340dcc4eb28548eb4dd16735995d2a61889337591e5c4d8021652a2"
        "b95d7df670a7e890a53ec08f676198ace7bb0a074a4b07ff0b493c4be00a0dea"
        "a84df9b528e69a494e197d48e21d6291b3d76db31396663a8188721eef9fcf35"
        "56338dcac45a149227678b7c23637d64f4e2b607f6649af82394ce5c0957fedf"
    )
    list(LENGTH CTRADER_PROTO_FILES CTRADER_PROTO_COUNT)
    math(EXPR CTRADER_PROTO_LAST "${CTRADER_PROTO_COUNT} - 1")
    foreach(CTRADER_PROTO_INDEX RANGE ${CTRADER_PROTO_LAST})
        list(GET CTRADER_PROTO_FILES ${CTRADER_PROTO_INDEX} CTRADER_PROTO_FILE)
        list(GET CTRADER_PROTO_EXPECTED_HASHES ${CTRADER_PROTO_INDEX}
             CTRADER_PROTO_EXPECTED_HASH)
        file(SHA256 "${CTRADER_PROTO_FILE}" CTRADER_PROTO_ACTUAL_HASH)
        if (NOT CTRADER_PROTO_ACTUAL_HASH STREQUAL CTRADER_PROTO_EXPECTED_HASH)
            message(FATAL_ERROR
                "Pinned cTrader schema hash mismatch: ${CTRADER_PROTO_FILE}")
        endif()
    endforeach()

    protobuf_generate(
        LANGUAGE cpp
        OUT_VAR CTRADER_PROTO_GENERATED
        IMPORT_DIRS "${CTRADER_PROTO_DIR}"
        PROTOS ${CTRADER_PROTO_FILES}
    )
    add_library(ctrader_openapi_proto STATIC
        ${CTRADER_PROTO_GENERATED}
    )
    target_include_directories(ctrader_openapi_proto PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
    )
    target_link_libraries(ctrader_openapi_proto PUBLIC protobuf::libprotobuf)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(ctrader_openapi_proto PRIVATE
            -Wno-nullability-extension)
    endif()

    add_library(ctrader_gate6_core STATIC src/CTraderGate6Proof.cpp)
    target_include_directories(ctrader_gate6_core PUBLIC include)

    add_library(ctrader_gate6_runtime STATIC
        src/CTraderGate6Runtime.mm
        src/CTraderOAuthCorrelation.cpp
    )
    target_include_directories(ctrader_gate6_runtime PUBLIC include)
    target_link_libraries(ctrader_gate6_runtime PUBLIC
        ctrader_gate6_core
        ctrader_openapi_proto
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        ${APPKIT_FRAMEWORK}
        ${SECURITY_FRAMEWORK}
    )

    add_executable(ctrader_gate6_proof src/ctrader_gate6_main.cpp)
    target_link_libraries(ctrader_gate6_proof PRIVATE ctrader_gate6_runtime)
endif()

set(CORE_SOURCES
    src/CsvReader.cpp
    src/AsyncNetworkClient.cpp
    src/AuthManager.cpp
    src/LiveDataAdapter.cpp
    src/BrokerGateway.cpp
    src/EventLoop.cpp
    src/RegimeDetector.cpp
    src/RiskEngine.cpp
    src/SmaCrossStrategy.cpp
    src/MeanReversionStrategy.cpp
    src/PortfolioAllocator.cpp
    src/PortfolioManager.cpp
    src/ExecutionEngine.cpp
    src/AnalyticsEngine.cpp
    src/MetricsAggregator.cpp
    src/LocalMetricsExporter.cpp
    src/StateSerializer.cpp
    src/LocalDataReplayAdapter.cpp
    src/L2OrderBook.cpp
    src/TriggerOrderManager.cpp
    src/OrderLifecycleStore.cpp
    src/DeterministicBrokerAdapter.cpp
    src/CTraderOAuthCorrelation.cpp
)

add_library(tradebot_core_lib ${CORE_SOURCES})
target_include_directories(tradebot_core_lib PUBLIC include)
target_link_libraries(tradebot_core_lib PUBLIC Threads::Threads)
if (DL_LIB)
    target_link_libraries(tradebot_core_lib PUBLIC ${DL_LIB})
endif()

add_executable(tradebot_core src/main.cpp)
target_link_libraries(tradebot_core PRIVATE tradebot_core_lib)

add_executable(throughput_bench src/benchmarks/throughput_bench.cpp)
target_link_libraries(throughput_bench PRIVATE tradebot_core_lib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(throughput_bench PRIVATE -O3 -DNDEBUG -march=native)
elseif (MSVC)
    target_compile_options(throughput_bench PRIVATE /O2 /DNDEBUG)
endif()

add_executable(phase18_burnin src/benchmarks/phase18_burnin.cpp)
target_link_libraries(phase18_burnin PRIVATE tradebot_core_lib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(phase18_burnin PRIVATE -O3 -DNDEBUG -march=native)
elseif (MSVC)
    target_compile_options(phase18_burnin PRIVATE /O2 /DNDEBUG)
endif()

add_executable(apply_bbo_microbench src/benchmarks/apply_bbo_microbench.cpp)
target_link_libraries(apply_bbo_microbench PRIVATE tradebot_core_lib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(apply_bbo_microbench PRIVATE -O3 -DNDEBUG -march=native)
elseif (MSVC)
    target_compile_options(apply_bbo_microbench PRIVATE /O2 /DNDEBUG)
endif()

include(CTest)
if (BUILD_TESTING)
    add_executable(phase13_tests tests/phase13_tests.cpp)
    target_link_libraries(phase13_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase13_tests COMMAND phase13_tests)

    add_executable(phase15_tests tests/phase15_tests.cpp)
    target_link_libraries(phase15_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase15_tests COMMAND phase15_tests)

    add_executable(phase16_tests tests/phase16_tests.cpp)
    target_link_libraries(phase16_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase16_tests COMMAND phase16_tests)

    add_executable(phase17_tests tests/phase17_tests.cpp)
    target_link_libraries(phase17_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase17_tests COMMAND phase17_tests)

    add_executable(phase18_tests tests/phase18_tests.cpp)
    target_link_libraries(phase18_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase18_tests COMMAND phase18_tests)

    add_executable(phase22_tests tests/phase22_tests.cpp)
    target_link_libraries(phase22_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase22_tests COMMAND phase22_tests)

    add_executable(ctrader_gate5_1_tests tests/ctrader_gate5_1_tests.cpp)
    target_link_libraries(ctrader_gate5_1_tests PRIVATE tradebot_core_lib)
    add_test(NAME ctrader_gate5_1_tests COMMAND ctrader_gate5_1_tests)

    if (TRADEBOT_ENABLE_CTRADER_GATE6)
        add_executable(ctrader_gate6_tests tests/ctrader_gate6_tests.cpp)
        target_link_libraries(ctrader_gate6_tests PRIVATE
            ctrader_gate6_runtime
        )
        add_test(NAME ctrader_gate6_tests COMMAND ctrader_gate6_tests)
    endif()
endif()
