#pragma once

#include <rockchip/rk_mpi.h>

#include <functional>
#include <string>
#include <thread>

struct FrameInfo {
    uint32_t height;
    uint32_t width;
    std::string format;
    uint8_t fps;
};

struct StreamInfo {
    std::string StreamType;
    uint32_t gop;
};

class VideoEncoder {
public:
    VideoEncoder(const FrameInfo& frame_info, const StreamInfo& stream_info, 
        int timeout = -1, bool is_camera_dma = false);
    ~VideoEncoder(void);

    bool Init(const std::function<void(uint8_t*, uint32_t)>& package_callback);

    bool PutFrame(uint8_t* data, uint32_t size, int dma_fd = -1);

private:
    static void EncRecvThread(VideoEncoder* self);

private:
    MppCtx ctx_ = nullptr;
    MppApi* api_ = nullptr;
    MppEncRcCfg rc_cfg_;
    MppEncPrepCfg prep_cfg_;
    MppEncCodecCfg codec_cfg_;

    int timeout = -1;
    bool is_camera_dma_ = false;
    FrameInfo frame_info_;
    StreamInfo stream_info_;

    bool is_running_ = false;
    std::thread recv_thread_;
    std::function<void(uint8_t*, uint32_t)> package_callback_;
};