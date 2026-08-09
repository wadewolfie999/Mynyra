#include "CTraderGate7Runtime.hpp"

#include <string_view>

int main(int argc, char** argv)
{
    const bool preflight = argc == 2
        && std::string_view(argv[1]) == "--preflight";
    if (argc > 2 || (argc == 2 && !preflight)) return 2;
    return tradebot::ctrader::runCTraderGate7Proof(preflight);
}
