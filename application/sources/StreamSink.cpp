#include "StreamSink.h"

#include <spdlog/spdlog.h>

void StreamSink::OnMkMediaClose(void* self)
{
    auto server = (StreamSink*)self;
    spdlog::info("Media Server {} {} Close", server->info_.app, server->info_.stream_id);
}

int StreamSink::OnMkMediaPause(void* self, int pause)
{
    auto server = (StreamSink*)self;
    spdlog::info("Media Server {}", pause == 1 ? "Pause" : "unpause");
    return 0;
}

void StreamSink::OnMkMediaSourceRegist(void* self, mk_media_source sender, int regist)
{
    auto server = (StreamSink*)self;

    spdlog::info("Media Server {} {} {} Regist, count {}, total count {}",
        mk_media_source_get_app(sender),
        mk_media_source_get_stream(sender),
        regist == 1 ? "regist" : "unregist",
        mk_media_source_get_reader_count(sender),
        mk_media_source_get_total_reader_count(sender));
    return;
}

int StreamSink::OnMkMediaSeek(void* self, uint32_t stamp_ms)
{
    spdlog::info("Media Server Seek {}ms", stamp_ms);
    // igonre , 1 is ack
    return 0;
}

int StreamSink::OnMkMediaSpeed(void* self, float speed)
{
    auto server = (StreamSink*)self;
    spdlog::info("Media Server On Speed {}", speed);
    return 0;
}

StreamSink::StreamSink(const StreamSinkInfo& info)
    : info_(info)
{
}

StreamSink::~StreamSink(void)
{
    Stop();
}

bool StreamSink::Init(void)
{
    media_ = mk_media_create("__defaultVhost__", info_.app.c_str(), info_.stream_id.c_str(), 0, 0, 0);
    if (media_ == nullptr) {
        spdlog::error("Create Media Server error");
        return false;
    }

    mk_media_set_on_close(media_, StreamSink::OnMkMediaClose, this);
    mk_media_set_on_pause(media_, StreamSink::OnMkMediaPause, this);
    mk_media_set_on_regist(media_, StreamSink::OnMkMediaSourceRegist, this);
    mk_media_set_on_seek(media_, StreamSink::OnMkMediaSeek, this);
    mk_media_set_on_speed(media_, StreamSink::OnMkMediaSpeed, this);
    auto codec_type = info_.stream_type.compare("H265") == 0 ? 1 : 0;
    if (mk_media_init_video(media_, codec_type, info_.width, info_.height, info_.fps, 0) != 1) {
        spdlog::error("Media init error");
        return false;
    }

    if (info_.stream_type.compare("H264") == 0) {
        send_ = mk_media_input_h264;
    } else if (info_.stream_type.compare("H265") == 0) {
        send_ = mk_media_input_h265;
    }

    mk_media_init_complete(media_);
    return true;
}

bool StreamSink::SendPackage(uint8_t* data, uint32_t size, uint64_t dts_ms, uint64_t pts_ms)
{
    if (send_(media_, data, size, dts_ms, pts_ms) == 0) {
        spdlog::error("Send Package error, data = {} size = {}, dts = {}, pts = {}",
            fmt::ptr(data), size, dts_ms, pts_ms);
        return false;
    }
    return true;
}

void StreamSink::Stop(void)
{
    if (media_ != nullptr) {
        mk_media_release(media_);
        media_ = nullptr;
    }
}