#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct PlaneData {
    void* start;
    uint32_t size;
};

struct BufferData {
    int dma_fd;
    std::vector<PlaneData> data;
};

struct CaptureVideoInfo {
    uint32_t height;
    uint32_t width;
    std::string pixelformat;
    uint8_t num_planes;
    std::vector<uint32_t> sizeimage;
    std::string colorspace;
};

struct CaputreAbility {
    bool is_capture_mplane;
    bool is_capture;
    // if is_stream methon is mmap
    bool is_stream;
    // if not is_stream but readwriteable methon  is read
    bool is_readwrite;
};

class VideoCapture {
public:
    VideoCapture() = delete;
    VideoCapture(std::string_view video_path, int buf_size = 5, uint32_t timeout = 10);

    bool Init(void);

    ~VideoCapture(void);
    bool GetVideoInfo(CaptureVideoInfo& info);
    bool SetVideoFormat(CaptureVideoInfo& video_info);

    bool Setup(const std::function<void(PlaneData&, int dma_fd)>& callback);
    bool Setup(const std::function<void(std::vector<PlaneData>&, int dma_fd)>& callback);

    void Deinit(void);
    bool Reset(void);

private:
    bool CheckCaptureAbility(void);
    bool CheckVideoFormat(void);
    std::vector<std::string> EnumVideoFormat(void);

    uint32_t ReqVideoBuffers(uint32_t buf_count);
    bool QueryVideoBuffers(void);
    bool QueueVideoBuffers(void);

private:
    int fd_ = -1;
    const std::string video_path_;
    uint32_t timeout_;
    int buf_count_;

    CaputreAbility ability_;

    bool is_running_ = false;

    uint32_t buf_type_;

    CaptureVideoInfo video_info_;

    // for each buf, and each plane
    std::vector<BufferData> planes_data_;

    // for mplane
    std::vector<struct v4l2_plane*> planes_buffers_;

    std::function<void(PlaneData&, int dma_fd)> callback_op_;
    std::function<void(std::vector<PlaneData>&, int dma_fd)> callback_mp_;

};