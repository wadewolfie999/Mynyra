#include "providers/ctrader/CTraderFrameDecoder.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using tradebot::ctrader::CTraderFrameDecoder;

void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::string framed(std::string_view payload)
{
    const auto size = static_cast<std::uint32_t>(payload.size());
    std::string result;
    result.push_back(static_cast<char>((size >> 24) & 0xff));
    result.push_back(static_cast<char>((size >> 16) & 0xff));
    result.push_back(static_cast<char>((size >> 8) & 0xff));
    result.push_back(static_cast<char>(size & 0xff));
    result.append(payload);
    return result;
}

void testPartialHeaderAndPayloadSurviveDeadlines()
{
    CTraderFrameDecoder decoder(64);
    const std::string bytes = framed("partial-provider-frame");
    std::string frame;
    require(decoder.append(std::string_view(bytes).substr(0, 2))
                == CTraderFrameDecoder::Result::NeedMore,
            "partial header append failed");
    require(decoder.next(frame) == CTraderFrameDecoder::Result::NeedMore
                && decoder.bufferedBytes() == 2,
            "partial header was discarded");
    require(decoder.append(std::string_view(bytes).substr(2, 5))
                == CTraderFrameDecoder::Result::NeedMore,
            "partial payload append failed");
    require(decoder.next(frame) == CTraderFrameDecoder::Result::NeedMore
                && decoder.bufferedBytes() == 7,
            "partial payload was discarded");
    require(decoder.append(std::string_view(bytes).substr(7))
                == CTraderFrameDecoder::Result::NeedMore,
            "final payload append failed");
    require(decoder.next(frame) == CTraderFrameDecoder::Result::FrameReady
                && frame == "partial-provider-frame"
                && decoder.bufferedBytes() == 0,
            "reassembled frame did not match");
}

void testMultipleFramesAndMalformedLengths()
{
    CTraderFrameDecoder decoder(16);
    const std::string bytes = framed("one") + framed("two");
    std::string frame;
    require(decoder.append(bytes) == CTraderFrameDecoder::Result::NeedMore,
            "multiple-frame append failed");
    require(decoder.next(frame) == CTraderFrameDecoder::Result::FrameReady
                && frame == "one",
            "first coalesced frame failed");
    require(decoder.next(frame) == CTraderFrameDecoder::Result::FrameReady
                && frame == "two",
            "second coalesced frame failed");

    const std::string zeroLength(4, '\0');
    require(decoder.append(zeroLength) == CTraderFrameDecoder::Result::NeedMore
                && decoder.next(frame) == CTraderFrameDecoder::Result::Malformed
                && decoder.bufferedBytes() == 0,
            "zero-length frame was accepted");
    require(decoder.append(framed("recovered"))
                == CTraderFrameDecoder::Result::NeedMore
                && decoder.next(frame) == CTraderFrameDecoder::Result::FrameReady
                && frame == "recovered",
            "decoder did not recover from a terminal malformed frame");

    CTraderFrameDecoder oversized(4);
    const std::string tooLarge = framed("12345");
    require(oversized.append(tooLarge)
                == CTraderFrameDecoder::Result::NeedMore
                && oversized.next(frame)
                    == CTraderFrameDecoder::Result::Malformed
                && oversized.bufferedBytes() == 0,
            "oversized frame was accepted");

    CTraderFrameDecoder bounded(4);
    const std::string partialHeader("\0\0", 2);
    require(bounded.append(partialHeader) == CTraderFrameDecoder::Result::NeedMore,
            "bounded decoder did not accept a partial header");
    const std::string excessive(17, 'x');
    require(bounded.append(excessive) == CTraderFrameDecoder::Result::Malformed
                && bounded.bufferedBytes() == 0,
            "buffer overflow did not fail closed and clear buffered bytes");
}

} // namespace

int main()
{
    testPartialHeaderAndPayloadSurviveDeadlines();
    testMultipleFramesAndMalformedLengths();
    std::cout << "ctrader_demo_frame_decoder_tests: PASS\n";
    return 0;
}
