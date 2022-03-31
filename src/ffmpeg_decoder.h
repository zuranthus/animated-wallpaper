#pragma once

#include <filesystem>
#include <memory>
#include <chrono>
#include "expected.h"

extern "C" {
    #include <libavformat/avformat.h>
}

using namespace std::chrono_literals;

namespace ffmpeg_decoder {

class VideoDecoder;
template <typename T>
using raii_ptr = std::unique_ptr<T, std::function<void(T*)>>;

class StreamDecoderContext {
    raii_ptr<AVFormatContext> avformat_{};
    raii_ptr<AVCodecContext> avcodec_{};
    int avstream_index_{};
public:
    StreamDecoderContext(raii_ptr<AVFormatContext> avformat, raii_ptr<AVCodecContext> avcodec, int avstream_id)
        : avformat_(std::move(avformat)), avcodec_(std::move(avcodec)), avstream_index_(avstream_id) {}
    StreamDecoderContext(StreamDecoderContext&&) = default;
    StreamDecoderContext& operator=(StreamDecoderContext&&) = default;

    auto avformat() { return avformat_.get(); }
    auto avcodec() { return avcodec_.get(); }
    auto avstream() { return avformat_->streams[avstream_index_]; }
};

class FileLoader {
    auto MakeError(std::string_view message) const;
    std::filesystem::path path;
public:
    FileLoader(const std::filesystem::path& path) : path(path) {}
    auto VideoStreamDecoder() -> expected<VideoDecoder>;
};

class VideoDecoder {
    StreamDecoderContext context;
    raii_ptr<AVPacket> avpacket;
    raii_ptr<AVFrame> avframe;
    bool finished {false};

    auto NextAVPacket() -> expected<std::unique_ptr<AVPacket, void(*)(AVPacket*)>>;
public:
    using chrono_ms = std::chrono::milliseconds;

    struct Frame {
        raii_ptr<AVFrame> avframe{};
        chrono_ms start_time{-1ms};
        chrono_ms end_time{-1ms};
        auto width() { return avframe->width; }
        auto height() { return avframe->height; }
    };
    explicit VideoDecoder(StreamDecoderContext&& context);

    auto width() { return context.avcodec()->width; }
    auto height() { return context.avcodec()->height; }
    auto frames() { return context.avstream()->nb_frames; }

    bool HasFrames() const { return !finished; }
    auto NextFrame() -> expected<Frame>;
    auto Reset() -> expected<void>;
    auto CalculateDuration() -> expected<chrono_ms>;
};

}
