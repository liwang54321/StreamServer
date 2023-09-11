#include "VideoEncoder.h"

#include <spdlog/spdlog.h>

#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))


VideoEncoder::VideoEncoder(const FrameInfo& frame_info, const StreamInfo& stream_info,
    int timeout, bool is_camera_dma)
    : frame_info_(frame_info)
    , stream_info_(stream_info)
    , is_running_(true)
    , is_camera_dma_(is_camera_dma)
{
}

VideoEncoder::~VideoEncoder(void)
{
    if (ctx_ != nullptr) {
        mpp_destroy(ctx_);
    }
}

static inline MppCodingType AdaptStreamType(std::string_view stream_type)
{
    if (stream_type.compare("H265") == 0) {
        return MPP_VIDEO_CodingHEVC;
    } else if (stream_type.compare("H264") == 0) {
        return MPP_VIDEO_CodingAVC;
    }

    spdlog::error("Unkown support type {}", stream_type);
    return MPP_VIDEO_CodingUnused;
}

static inline MppFrameFormat AdaptFrameType(std::string_view stream_type)
{
    if (stream_type.compare("BGR24") == 0) {
        return MPP_FMT_BGR888;
    }

    spdlog::error("Unkonw Frame Type {}", stream_type);
    return MPP_FMT_BUTT;
}

bool VideoEncoder::Init(const std::function<void(uint8_t*, uint32_t)>& package_callback)
{
    auto encode_format = AdaptStreamType(stream_info_.StreamType);
    auto ret = mpp_check_support_format(MPP_CTX_ENC, encode_format);
    if (ret == MPP_SUCCESS) {
        spdlog::info("Mpp Support {}", stream_info_.StreamType);
    } else {
        spdlog::error("Mpp Not Support {}", stream_info_.StreamType);
        return false;
    }

    ret = mpp_create(&ctx_, &api_);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Create mpp error {}", ret);
        return false;
    }

    ret = mpp_init(ctx_, MPP_CTX_ENC, encode_format);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Mpp Init error {}", ret);
        return false;
    }

    // SetEncCfgDef(&rc_cfg_, &prep_cfg_);
    // set timeout
    ret = api_->control(ctx_, MPP_SET_INPUT_TIMEOUT, &timeout);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Mpp Set input time out error {}", ret);
        return false;
    }

    ret = api_->control(ctx_, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Mpp Set output time out error {}", ret);
        return false;
    }

    prep_cfg_.change = MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_ROTATION | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg_.width = frame_info_.width;
    prep_cfg_.height = frame_info_.height;
    prep_cfg_.format = AdaptFrameType(frame_info_.format);
    if (MPP_FRAME_FMT_IS_RGB(prep_cfg_.format)) {
        prep_cfg_.hor_stride = MPP_ALIGN(frame_info_.width * 3, 16);
    } else if (MPP_FRAME_FMT_IS_YUV(prep_cfg_.format)) {
        prep_cfg_.hor_stride = MPP_ALIGN(frame_info_.width, 16);
    }
    prep_cfg_.ver_stride = MPP_ALIGN(frame_info_.height, 16);    

    spdlog::info("width {}, height {}, hor_stride {}, ver_stride {}, format {}",
        prep_cfg_.width, prep_cfg_.height, prep_cfg_.hor_stride, prep_cfg_.ver_stride, prep_cfg_.format);
    ret = api_->control(ctx_, MPP_ENC_SET_PREP_CFG, (MppParam)&prep_cfg_);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Mpp Set prep error {}", ret);
        return false;
    }

    rc_cfg_.change = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg_.rc_mode = MPP_ENC_RC_MODE_VBR;
    rc_cfg_.quality = MPP_ENC_RC_QUALITY_BEST;
    rc_cfg_.fps_in_flex = 0;
    rc_cfg_.fps_in_num = frame_info_.fps;
    rc_cfg_.fps_in_denorm = 1;
    rc_cfg_.fps_out_flex = 0;
    rc_cfg_.fps_out_num = frame_info_.fps;
    rc_cfg_.fps_out_denorm = 1;
    rc_cfg_.gop = stream_info_.gop;
    rc_cfg_.skip_cnt = 0;
    rc_cfg_.drop_mode = MPP_ENC_RC_DROP_FRM_DISABLED;
    rc_cfg_.drop_threshold = 20;
    rc_cfg_.drop_gap = 1;
    auto bps = frame_info_.width * frame_info_.height / 8 * frame_info_.fps;
    rc_cfg_.bps_target = bps;
    switch (rc_cfg_.rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP:
        break;
    case MPP_ENC_RC_MODE_CBR:
        rc_cfg_.bps_max = bps * 17 / 16;
        rc_cfg_.bps_min = bps * 15 / 16;
        break;
    case MPP_ENC_RC_MODE_VBR:
    case MPP_ENC_RC_MODE_AVBR:
        rc_cfg_.bps_max = bps * 17 / 16;
        rc_cfg_.bps_min = bps * 1 / 16;
        break;
    default:
        rc_cfg_.bps_max = bps * 17 / 16;
        rc_cfg_.bps_min = bps * 15 / 16;
        break;
    }

    switch (encode_format) {
    case MPP_VIDEO_CodingAVC:
    case MPP_VIDEO_CodingHEVC: {
        switch (rc_cfg_.rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            RK_S32 fix_qp = 0;
            rc_cfg_.qp_init = fix_qp;
            rc_cfg_.qp_max = fix_qp;
            rc_cfg_.qp_min = fix_qp;
            rc_cfg_.qp_max_i = fix_qp;
            rc_cfg_.qp_min_i = fix_qp;
            // rc_cfg_.qp_ip = fix_qp;
        } break;
        case MPP_ENC_RC_MODE_CBR:
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            rc_cfg_.qp_init = -1;
            rc_cfg_.qp_max = 51;
            rc_cfg_.qp_min = 10;
            rc_cfg_.qp_max_i = 51;
            rc_cfg_.qp_min_i = 10;
            // rc_cfg_.qp_ip = 2;
        } break;
        default: {
            spdlog::error("unsupport encoder rc mode {}", rc_cfg_.rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8:
        rc_cfg_.qp_init = 40;
        rc_cfg_.qp_max = 127;
        rc_cfg_.qp_min = 0;
        rc_cfg_.qp_max_i = 127;
        rc_cfg_.qp_min_i = 0;
        // rc_cfg_.qp_ip = 6;
        break;
    case MPP_VIDEO_CodingMJPEG: {
        /* jpeg use special codec config to control qtable */
        // mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        // mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        // mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    default: {
    } break;
    }

    ret = api_->control(ctx_, MPP_ENC_SET_RC_CFG, (MppParam)&rc_cfg_);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Mpp Set Rc error {}", ret);
        return false;
    }

    codec_cfg_.change = 0;
    codec_cfg_.coding = AdaptStreamType(stream_info_.StreamType);
    switch (codec_cfg_.coding) {
    case MPP_VIDEO_CodingAVC:
        codec_cfg_.h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE | MPP_ENC_H264_CFG_CHANGE_ENTROPY | MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
        codec_cfg_.h264.profile = 100;
        codec_cfg_.h264.level = 40;
        codec_cfg_.h264.entropy_coding_mode = 1;
        codec_cfg_.h264.cabac_init_idc = 0;
        codec_cfg_.h264.transform8x8_mode = 1;
        break;
    case MPP_VIDEO_CodingMJPEG:
        codec_cfg_.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
        codec_cfg_.jpeg.quant = 10;
        break;
    case MPP_VIDEO_CodingVP8:
    case MPP_VIDEO_CodingHEVC:
        break;
    default:
        spdlog::error("Not support {}", stream_info_.StreamType);
        return false;
    }
    ret = api_->control(ctx_, MPP_ENC_SET_CODEC_CFG, &codec_cfg_);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Mpp Set MPP_ENC_SET_CODEC_CFG error {}", ret);
        return false;
    }

    auto sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = api_->control(ctx_, MPP_ENC_SET_SEI_CFG, &sei_mode);
    if (ret) {
        spdlog::error("mpi control enc set sei cfg failed ret %d\n", ret);
        return false;
    }

    package_callback_ = package_callback;
    recv_thread_ = std::thread(&VideoEncoder::EncRecvThread, this);

    return true;
}

void VideoEncoder::EncRecvThread(VideoEncoder* self)
{
    MppPacket packet = NULL;
    while (self->is_running_) {
        auto ret = self->api_->encode_get_packet(self->ctx_, &packet);
        if (ret || NULL == packet) {
            spdlog::error("Get Package error, {}", ret);
            continue;
        }
        auto ptr = mpp_packet_get_pos(packet);
        auto len = mpp_packet_get_length(packet);
        auto pkt_eos = mpp_packet_get_eos(packet);

        self->package_callback_((uint8_t*)ptr, len);

        ret = mpp_packet_deinit(&packet);
        assert(ret == MPP_SUCCESS);
    }
}

bool VideoEncoder::PutFrame(uint8_t* data, uint32_t size, int dma_fd)
{
    MppFrame frame = nullptr;
    auto ret = mpp_frame_init(&frame);
    if (ret) {
        spdlog::error("mpp_frame_init failed {}", ret);
        return false;
    }
    mpp_frame_set_width(frame, frame_info_.width);
    mpp_frame_set_height(frame, frame_info_.height);

    mpp_frame_set_hor_stride(frame, prep_cfg_.hor_stride);
    mpp_frame_set_ver_stride(frame, prep_cfg_.ver_stride);

    mpp_frame_set_fmt(frame, prep_cfg_.format);
    mpp_frame_set_eos(frame, 0);

    MppBuffer buffer;

    MppBufferInfo info;
    memset(&info, 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_EXT_DMA;
    info.fd = dma_fd;
    info.size = size & 0x07ffffff;
    info.index = (size & 0xf8000000) >> 27;
    mpp_buffer_import(&buffer, &info);

    mpp_frame_set_buffer(frame, buffer);
    ret = api_->encode_put_frame(ctx_, frame);
    if (ret != MPP_SUCCESS) {
        spdlog::error("Encode frame error {}", ret);
        return false;
    }

    return true;
}
