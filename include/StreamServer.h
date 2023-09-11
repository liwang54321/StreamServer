#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "mk_media.h"

#include "StreamSink.h"

struct StreamServerInfo {
    uint16_t rtsp_port;
    uint16_t rtmp_port;
    uint16_t http_port;
};

class StreamServer {
public:
    StreamServer(const StreamServerInfo& info);
    ~StreamServer(void);

    bool Init(void);

    std::unique_ptr<StreamSink> CreateSink(const StreamSinkInfo& sink_info);

    void Stop(void);

private:
    StreamServerInfo info_;
};