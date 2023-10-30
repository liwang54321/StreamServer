#include "StreamServer.h"

#include <spdlog/spdlog.h>

StreamServer::StreamServer(const StreamServerInfo& info)
    : info_(info)
{
}

StreamServer::~StreamServer(void)
{
    mk_stop_all_server();
}

bool StreamServer::Init(void)
{
    mk_config config;
    config.ini = NULL;
    config.ini_is_path = 1;
    config.log_level = 0;
    config.log_mask = LOG_CONSOLE;
    config.log_file_path = NULL;
    config.log_file_days = 0;
    config.ssl = NULL;
    config.ssl_is_path = 1;
    config.ssl_pwd = NULL;
    config.thread_num = 0;

    mk_env_init(&config);
    mk_http_server_start(info_.http_port, 0);
    mk_rtsp_server_start(info_.rtsp_port, 0);
    mk_rtmp_server_start(info_.rtmp_port, 0);

    return true;
}

std::unique_ptr<StreamSink> StreamServer::CreateSink(const StreamSinkInfo& sink_info)
{
    return std::make_unique<StreamSink>(sink_info);
}

void StreamServer::Stop(void)
{
    mk_stop_all_server();
    return;
}
