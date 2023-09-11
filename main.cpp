#include <cstring>
#include <fstream>

#include <StreamServer.h>
#include <VideoCapture.h>
#include <VideoEncoder.h>

#include <spdlog/spdlog.h>

int main(int argc, char** argv)
{
    auto capture = std::make_shared<VideoCapture>("/dev/video0", 5, 2);

    if (!capture->Init()) {
        return -1;
    }
    CaptureVideoInfo cap_info;
    if (!capture->GetVideoInfo(cap_info)) {
        return -1;
    }
    FrameInfo frame_info;
    frame_info.format = std::move(cap_info.pixelformat);
    frame_info.fps = 60;
    frame_info.height = cap_info.height;
    frame_info.width = cap_info.width;

    StreamInfo stream_info;
    stream_info.gop = 120;
    stream_info.StreamType = "H265";

    VideoEncoder encoder(frame_info, stream_info, 10, true);
    StreamServerInfo server_info;
    server_info.http_port = 10000;
    server_info.rtmp_port = 10001;
    server_info.rtsp_port = 10002;

    StreamServer server(server_info);
    if (!server.Init()) {
        return -1;
    }
    StreamSinkInfo sink_info;
    sink_info.app = "live";
    sink_info.stream_id = "1";
    sink_info.height = cap_info.height;
    sink_info.width = cap_info.width;
    sink_info.stream_type = stream_info.StreamType;

    auto sink = server.CreateSink(sink_info);
    if (!sink->Init()) {
        return -1;
    }

    if (!encoder.Init([&](uint8_t* data, uint32_t size) {
            sink->SendPackage(data, size, 0, 0);
            // spdlog::info("Get Package {} {}", fmt::ptr(data), size);
        })) {
        return -1;
    }

    if (!capture->Setup([&](std::vector<PlaneData>& data, int dma_fd) {
            // spdlog::info("Get Package {} {}", data[0].start, data[0].size);
            encoder.PutFrame((uint8_t*)data[0].start, data[0].size, dma_fd);
        })) {
        while (true) {
            capture->Reset();
        }
        return -1;
    }

    return 0;
}
