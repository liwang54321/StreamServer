#pragma once

#include <string>

#include <mk_media.h>

struct StreamSinkInfo {
    std::string app;
    std::string stream_id;
    std::string stream_type;
    uint8_t fps;
    uint32_t width;
    uint32_t height;
};

typedef int (*SendPackage_t)(mk_media, const void*, int, uint64_t, uint64_t);

class StreamSink {
public:
    StreamSink(const StreamSinkInfo& info);
    ~StreamSink(void);

    bool Init(void);
    bool SendPackage(uint8_t* data, uint32_t size, uint64_t dts_ms, uint64_t pts_ms);
    void Stop(void);

private:
    static void OnMkMediaClose(void* self);
    static int OnMkMediaPause(void* self, int pause);
    static void OnMkMediaSourceRegist(void* self, mk_media_source sender, int regist);
    static int OnMkMediaSeek(void* self, uint32_t stamp_ms);
    static int OnMkMediaSpeed(void* self, float speed);

private:
    mk_media media_ = nullptr;
    StreamSinkInfo info_;
    SendPackage_t send_ = nullptr;
};
