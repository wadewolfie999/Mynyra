#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace tradebot::ctrader {

// Stateful decoder for cTrader's four-byte big-endian length prefix. The
// buffered bytes survive receive deadlines so a partial header or payload is
// never reinterpreted as the start of the next frame.
class CTraderFrameDecoder final {
public:
    enum class Result : std::uint8_t {
        NeedMore,
        FrameReady,
        Malformed,
        ResourceExhausted
    };

    explicit CTraderFrameDecoder(std::size_t maximumFrameBytes) noexcept
        : m_maximumFrameBytes(maximumFrameBytes)
        , m_maximumBufferedBytes(
              maximumFrameBytes <= (std::numeric_limits<std::size_t>::max() - 8) / 2
                  ? maximumFrameBytes * 2 + 8
                  : maximumFrameBytes)
    {}

    ~CTraderFrameDecoder() { clear(); }

    CTraderFrameDecoder(const CTraderFrameDecoder&) = delete;
    CTraderFrameDecoder& operator=(const CTraderFrameDecoder&) = delete;

    Result append(std::string_view bytes) noexcept
    {
        if (bytes.empty()) return Result::NeedMore;
        if (bytes.size() > m_maximumBufferedBytes
            || m_buffer.size() > m_maximumBufferedBytes - bytes.size()) {
            clear();
            return Result::Malformed;
        }
        try {
            m_buffer.append(bytes.data(), bytes.size());
            return Result::NeedMore;
        } catch (...) {
            clear();
            return Result::ResourceExhausted;
        }
    }

    Result next(std::string& frame) noexcept
    {
        wipe(frame);
        if (m_buffer.size() < 4) return Result::NeedMore;
        const auto* header = reinterpret_cast<const unsigned char*>(
            m_buffer.data());
        const std::uint32_t length =
              (static_cast<std::uint32_t>(header[0]) << 24)
            | (static_cast<std::uint32_t>(header[1]) << 16)
            | (static_cast<std::uint32_t>(header[2]) << 8)
            | static_cast<std::uint32_t>(header[3]);
        if (length == 0 || length > m_maximumFrameBytes) {
            clear();
            return Result::Malformed;
        }
        const std::size_t total = 4 + static_cast<std::size_t>(length);
        if (m_buffer.size() < total) return Result::NeedMore;
        try {
            frame.assign(m_buffer.data() + 4, length);
            std::fill_n(m_buffer.begin(), total, '\0');
            m_buffer.erase(0, total);
            return Result::FrameReady;
        } catch (...) {
            wipe(frame);
            clear();
            return Result::ResourceExhausted;
        }
    }

    std::size_t bufferedBytes() const noexcept { return m_buffer.size(); }

    void clear() noexcept
    {
        wipe(m_buffer);
    }

private:
    static void wipe(std::string& value) noexcept
    {
        volatile char* bytes = value.empty() ? nullptr : value.data();
        for (std::size_t i = 0; i < value.size(); ++i) bytes[i] = '\0';
        value.clear();
    }

    std::size_t m_maximumFrameBytes{0};
    std::size_t m_maximumBufferedBytes{0};
    std::string m_buffer;
};

} // namespace tradebot::ctrader
