#include "VideoCapture.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <magic_enum.hpp>
#include <spdlog/spdlog.h>

#include <thread>

static inline std::string AdaptFrameType(uint32_t frame_type)
{
    switch (frame_type) {
    case V4L2_PIX_FMT_BGR24:
        return "BGR24";
    default:
        spdlog::error("Unkonw frame type {}", frame_type);
        return "Error";
    }
}

static inline uint32_t AdaptFrameType(std::string_view frame_type)
{
    if (frame_type.compare("BGR24") == 0) {
        return V4L2_PIX_FMT_BGR24;
    } else if (frame_type.compare("NV12") == 0) {
        return V4L2_PIX_FMT_NV12;
    } else {
        spdlog::error("Can not find this Format {}", frame_type);
        return -1;
    }
}

static inline int VideoIoctl(int fd, int req, void* arg)
{
    struct timespec poll_time;
    int ret;
    uint8_t cnt = 0;
    constexpr uint8_t max_cnt = 10;
    while ((ret = ioctl(fd, req, arg))) {
        if (ret == -1 && (EINTR != errno && EAGAIN != errno)) {
            break;
        }
        if (cnt++ > max_cnt)
            break;
        // 10 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return ret;
}

VideoCapture::VideoCapture(std::string_view video_path, int buf_count, uint32_t timeout)
    : video_path_(video_path)
    , buf_count_(buf_count)
    , is_running_(true)
    , timeout_(timeout)
{
}

bool VideoCapture::CheckCaptureAbility(void)
{
    struct v4l2_capability arg_capability;
    std::memset((void*)&arg_capability, 0, sizeof(arg_capability));
    auto ret = VideoIoctl(fd_, VIDIOC_QUERYCAP, &arg_capability);
    if (ret < 0) {
        spdlog::error("Get video capability error");
        return false;
    }

    spdlog::info("Video Info: {}, {}, 0x{:08x}",
        (char*)arg_capability.driver, (char*)arg_capability.bus_info,
        arg_capability.capabilities);

    ability_.is_capture = arg_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE;
    ability_.is_capture_mplane = arg_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE;
    ability_.is_stream = arg_capability.capabilities & V4L2_CAP_STREAMING;
    ability_.is_readwrite = arg_capability.capabilities & V4L2_CAP_READWRITE;

    if (ability_.is_capture_mplane) {
        spdlog::info("Get Video Mplane");
        buf_type_ = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    } else if (ability_.is_capture) {
        spdlog::info("Get Video Oplane");
        buf_type_ = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    } else {
        spdlog::error("Video Not Have Bility Video MPlane");
        return false;
    }

    return true;
}

bool VideoCapture::CheckVideoFormat(void)
{
    struct v4l2_format arg_format;
    std::memset(&arg_format, 0, sizeof(arg_format));
    arg_format.type = buf_type_;
    auto ret = VideoIoctl(fd_, VIDIOC_G_FMT, &arg_format);
    if (ret < 0) {
        spdlog::error("Get Video format error {}", strerror(errno));
        return false;
    }

    if (buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        video_info_.height = arg_format.fmt.pix_mp.height;
        video_info_.width = arg_format.fmt.pix_mp.width;
        video_info_.pixelformat = AdaptFrameType(arg_format.fmt.pix_mp.pixelformat);
        video_info_.num_planes = arg_format.fmt.pix_mp.num_planes;
        auto colorspace_int = arg_format.fmt.pix_mp.colorspace;
        auto colorspace = magic_enum::enum_cast<v4l2_colorspace>(arg_format.fmt.pix_mp.colorspace);
        if (colorspace.has_value()) {
            video_info_.colorspace = magic_enum::enum_name(colorspace.value());
        } else {
            spdlog::info("colorspace {}", colorspace_int);
            video_info_.colorspace = "None";
        }
    } else {
        video_info_.height = arg_format.fmt.pix.height;
        video_info_.width = arg_format.fmt.pix.width;
        video_info_.pixelformat = AdaptFrameType(arg_format.fmt.pix.pixelformat);
        video_info_.num_planes = 1;
        video_info_.sizeimage.push_back(arg_format.fmt.pix.sizeimage);
        auto colorspace_int = arg_format.fmt.pix_mp.colorspace;
        auto colorspace = magic_enum::enum_cast<v4l2_colorspace>(arg_format.fmt.pix.colorspace);
        if (colorspace.has_value()) {
            video_info_.colorspace = magic_enum::enum_name(colorspace.value());
        } else {
            spdlog::error("colorspace {}", colorspace_int);
            video_info_.colorspace = "None";
        }
    }

    for (uint32_t i = 0; i < video_info_.num_planes; i++) {
        video_info_.sizeimage.push_back(arg_format.fmt.pix_mp.plane_fmt[i].sizeimage);
    }

    spdlog::info("Video height: {}", video_info_.height);
    spdlog::info("Video width: {}", video_info_.width);
    spdlog::info("Video pixelformat: {}", video_info_.pixelformat);
    spdlog::info("Video num_planes: {}", video_info_.num_planes);
    spdlog::info("Video colorspace: {}", video_info_.colorspace);

    return true;
}

std::vector<std::string> VideoCapture::EnumVideoFormat(void)
{
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = buf_type_;
    std::vector<std::string> fmts;
    while (!VideoIoctl(fd_, VIDIOC_ENUM_FMT, &fmtdesc)) {
        auto fmt = fmt::format("{}{}{}{}", fmtdesc.pixelformat & 0xFF,
            (fmtdesc.pixelformat >> 8) & 0xFF, (fmtdesc.pixelformat >> 16) & 0xFF,
            (fmtdesc.pixelformat >> 24) & 0xFF);
        spdlog::info("fmt name: [{}] \tfmt pixelformat: {}", std::string((char*)fmtdesc.description), fmt);
        fmtdesc.index++;
        fmts.emplace_back(std::move(fmt));
    }
    return fmts;
}

bool VideoCapture::SetVideoFormat(CaptureVideoInfo& video_info)
{
    struct v4l2_format fmt;
    std::memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = buf_type_;
    if (buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        fmt.fmt.pix_mp.width = video_info.width;
        fmt.fmt.pix_mp.height = video_info.height;
        fmt.fmt.pix_mp.pixelformat = AdaptFrameType(video_info.pixelformat);
        fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    } else {
        fmt.fmt.pix.width = video_info.width;
        fmt.fmt.pix.height = video_info.height;
        fmt.fmt.pix.pixelformat = AdaptFrameType(video_info.pixelformat);
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
    }

    if (VideoIoctl(fd_, VIDIOC_S_FMT, &fmt) < 0) {
        spdlog::error("Video set format fail");
        return false;
    }
    return true;
}

uint32_t VideoCapture::ReqVideoBuffers(uint32_t buf_count)
{
    struct v4l2_requestbuffers req;
    std::memset(&req, 0, sizeof(req));
    req.count = buf_count;
    req.type = buf_type_;
    req.memory = V4L2_MEMORY_MMAP;
    if (VideoIoctl(fd_, VIDIOC_REQBUFS, &req) < 0) {
        spdlog::error("Reqbufs fail {}", strerror(errno));
        return 0;
    }
    spdlog::info("buffer req number: {}", req.count);
    buf_count_ = req.count;
    return req.count;
}

bool VideoCapture::QueryVideoBuffers(void)
{
    auto num_planes = video_info_.num_planes;
    struct v4l2_buffer buf;
    planes_data_.resize(buf_count_);
    planes_buffers_.resize(buf_count_);
    for (int i = 0; i < buf_count_; i++) {
        std::memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = buf_type_;
        buf.memory = V4L2_MEMORY_MMAP;
        if (buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            planes_buffers_[i] = (struct v4l2_plane*)calloc(num_planes, sizeof(struct v4l2_plane));
            buf.m.planes = planes_buffers_[i];
            buf.length = num_planes;
        }

        int ret = VideoIoctl(fd_, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            spdlog::error("Unable to query buffer. {}", strerror(errno));
            return false;
        }

        planes_data_[i].data.resize(num_planes);
        for (int j = 0; j < num_planes; j++) {
            if (buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                planes_data_[i].data[j].start = mmap(NULL /* start anywhere */,
                    (planes_buffers_[i] + j)->length,
                    PROT_READ | PROT_WRITE /* required */,
                    MAP_SHARED /* recommended */,
                    fd_,
                    (planes_buffers_[i] + j)->m.mem_offset);
            } else {
                planes_data_[i].data[j].start = (char*)mmap(NULL,
                    buf.length,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd_,
                    buf.m.offset);
            }

            if (MAP_FAILED == planes_data_[i].data[j].start) {
                spdlog::error("mmap failed");
                buf_count_ = i;
                return false;
            }
        }

        struct v4l2_exportbuffer expbuf = { 0 };
        expbuf.type = buf_type_;
        expbuf.index = buf.index;
        expbuf.flags = O_CLOEXEC;
        if (VideoIoctl(fd_, VIDIOC_EXPBUF, &expbuf) < 0) {
            spdlog::error("get dma buf failed");
            return false;
        } else {
            planes_data_[i].dma_fd = expbuf.fd;
        }
    }
    return true;
}

bool VideoCapture::QueueVideoBuffers(void)
{
    auto num_planes = video_info_.num_planes;
    struct v4l2_buffer buf;
    for (int i = 0; i < buf_count_; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = buf_type_;
        buf.memory = V4L2_MEMORY_MMAP;

        if (buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buf.length = num_planes;
            buf.m.planes = planes_buffers_[i];
        }

        if (VideoIoctl(fd_, VIDIOC_QBUF, &buf) < 0) {
            spdlog::error("VIDIOC_QBUF failed");
            return false;
        }
    }
    return true;
}

bool VideoCapture::Init(void)
{
    fd_ = ::open(video_path_.c_str(), O_RDWR);
    if (fd_ < 0) {
        spdlog::error("Can not open {}", video_path_);
        return false;
    }

    if (!CheckCaptureAbility()) {
        return false;
    }

    if (!CheckVideoFormat()) {
        return false;
    }

    auto video_format = EnumVideoFormat();
    if (video_format.size() == 0) {
        spdlog::error("Can not find video format");
        return false;
    }

    auto buf_count_real = ReqVideoBuffers(buf_count_);
    if(buf_count_real == 0) {
        return false;
    }
    
    if (!QueryVideoBuffers()) {
        return false;
    }
    if (!QueueVideoBuffers()) {
        return false;
    }

    return true;
}

bool VideoCapture::GetVideoInfo(CaptureVideoInfo& info)
{
    info = video_info_;
    return true;
}

bool VideoCapture::Setup(const std::function<void(PlaneData&, int dma_fd)>& callback)
{
    PlaneData ret_data;
    assert(buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE);
    callback_op_ = callback;
    if (VideoIoctl(fd_, VIDIOC_STREAMON, &buf_type_) < 0) {
        spdlog::error("VIDIOC_STREAMON failed");
        return false;
    }

    struct v4l2_buffer buf;
    fd_set fds;
    timeval* tv = nullptr;
    if (timeout_ != 0) {
        tv = (timeval*)malloc(sizeof(timeval));
        tv->tv_sec = timeout_;
        tv->tv_usec = 0;
    }
    while (is_running_) {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);
        tv->tv_sec = timeout_;
        auto r = select(fd_ + 1, &fds, NULL, NULL, tv);
        if (-1 == r) {
            if (EINTR == errno)
                continue;
            spdlog::error("select err {}", strerror(errno));
        }
        if (0 == r) {
            spdlog::error("select timeout");
            return false;
        }
        std::memset(&buf, 0, sizeof(buf));
        buf.type = buf_type_;
        buf.memory = V4L2_MEMORY_MMAP;
        if (VideoIoctl(fd_, VIDIOC_DQBUF, &buf) < 0) {
            spdlog::error("dqbuf fail {}", strerror(errno));
        }

        if (buf.index > buf_count_) {
            spdlog::error("VIDIOC_DQBUF error {}", buf.index);
            continue;
        }

        ret_data.size = buf.bytesused;
        ret_data.start = planes_data_[buf.index].data[0].start;

        callback_op_(ret_data, planes_data_[buf.index].dma_fd);

        if (VideoIoctl(fd_, VIDIOC_QBUF, &buf) < 0) {
            spdlog::error("failture VIDIOC_QBUF");
        }
    }
    return true;
}

bool VideoCapture::Setup(const std::function<void(std::vector<PlaneData>&, int dma_fd)>& callback)
{
    assert(buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    callback_mp_ = callback;
    if (VideoIoctl(fd_, VIDIOC_STREAMON, &buf_type_) < 0) {
        spdlog::error("VIDIOC_STREAMON failed");
        return false;
    }

    std::vector<PlaneData> date_ret(video_info_.num_planes);
    struct v4l2_plane* tmp_plane = (struct v4l2_plane*)calloc(video_info_.num_planes, sizeof(*tmp_plane));

    fd_set fds;
    timeval* tv = nullptr;
    if (timeout_ != 0) {
        tv = (timeval*)malloc(sizeof(timeval));
        tv->tv_sec = timeout_;
        tv->tv_usec = 0;
    }
    struct v4l2_buffer buf;
    while (is_running_) {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);
        tv->tv_sec = timeout_;
        auto r = select(fd_ + 1, &fds, NULL, NULL, tv);
        if (-1 == r) {
            if (EINTR == errno)
                continue;
            spdlog::error("select err {}", strerror(errno));
        }
        if (0 == r) {
            spdlog::error("select timeout");
            break;
        }
        std::memset(&buf, 0, sizeof(buf));
        buf.type = buf_type_;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = tmp_plane;
        buf.length = video_info_.num_planes;

        if (VideoIoctl(fd_, VIDIOC_DQBUF, &buf) < 0) {
            spdlog::error("dqbuf fail {}", strerror(errno));
        }

        if (buf.index > buf_count_) {
            spdlog::error("VIDIOC_DQBUF error {}", buf.index);
            continue;
        }

        for (int j = 0; j < video_info_.num_planes; j++) {
            date_ret[j].start = planes_data_[buf.index].data[j].start;
            date_ret[j].size = (tmp_plane + j)->bytesused;
        }

        callback_mp_(date_ret, planes_data_[buf.index].dma_fd);

        if (VideoIoctl(fd_, VIDIOC_QBUF, &buf) < 0) {
            spdlog::error("failture VIDIOC_QBUF");
        }
    }

    if (VideoIoctl(fd_, VIDIOC_STREAMOFF, &buf_type_) < 0) {
        spdlog::error("VIDIOC_STREAMOFF fail");
        return false;
    }

    free(tmp_plane);

    return false;
}

void VideoCapture::Deinit(void)
{
    is_running_ = false;
    for (int i = 0; i < buf_count_; i++) {
        for (int j = 0; j < video_info_.num_planes; j++)
            if (MAP_FAILED != planes_data_[i].data[j].start) {
                if (-1 == munmap(planes_data_[i].data[j].start, (planes_buffers_[i] + j)->length)) {
                    planes_data_[i].data[j].start = MAP_FAILED;
                    spdlog::error("munmap error {}", strerror(errno));
                }
            }
    }
    for (auto& item : planes_buffers_) {
        if (item != nullptr) {
            free(item);
            item = nullptr;
        }
    }

    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
    return;
}

bool VideoCapture::Reset(void)
{
    Deinit();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
   
    if (!Init()) {
        spdlog::error("Reset VideoCapture failed");
        return false;
    }
    is_running_ = true;
    if (buf_type_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        return Setup(callback_mp_);
    } else {
        return Setup(callback_op_);
    }
}

VideoCapture::~VideoCapture(void)
{
    Deinit();
}