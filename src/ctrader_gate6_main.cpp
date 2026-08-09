#include "CTraderGate6Runtime.hpp"

#include <string_view>

int main(int argc, char* argv[])
{
    if (argc == 1) {
        return tradebot::ctrader::runCTraderGate6Proof();
    }
    if (argc == 2 && std::string_view(argv[1]) == "--preflight-only") {
        return tradebot::ctrader::runCTraderGate6Proof(true);
    }
    return 2;
}
