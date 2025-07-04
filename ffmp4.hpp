#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

// "${HOME}/vcpkg/installed/x64-linux/lib/libavformat.a",
// "${HOME}/vcpkg/installed/x64-linux/lib/libavcodec.a",
// "${HOME}/vcpkg/installed/x64-linux/lib/libswresample.a",
// "${HOME}/vcpkg/installed/x64-linux/lib/libavutil.a"

#include "faac.hpp"
#include "sps.hpp"

namespace libffmpeg {

class Mp4 {
private:
  AVFormatContext *pOfmtCtx_ = nullptr;
  uint32_t nVSIdx_ = 0, nASIdx_ = 0;
  int64_t firstFrameTs_ = 0;
  int64_t ldts_ = 0;

  libfaac::Encoder *faac_ = nullptr;

public:
  Mp4(const char *filename) { this->init(filename); }
  ~Mp4() { this->Close(); }

  void Close() {
    if (pOfmtCtx_) {
      if (firstFrameTs_ > 0) {
        av_write_trailer(pOfmtCtx_);
      }
      avio_closep(&pOfmtCtx_->pb);
      avformat_free_context(pOfmtCtx_);
      pOfmtCtx_ = nullptr;
    }
    if (faac_) {
      delete faac_;
    }
  }
  // ftype: 1 I, 2 p, 3 pcm
  void WriteFrame(int64_t ts, uint8_t ftype, char *data, size_t len) {
    char *frame = data;
    if (ftype < 0x03) {
      nalu::Units units{};
      if ((frame = nalu::Split(frame, len, units)) == nullptr) {
        return;
      }
      if (firstFrameTs_ == 0 && ftype == 0x01 &&
          newAVStream(units, data, frame - data - 4)) {
        firstFrameTs_ = ts;
      }
    }
    if (firstFrameTs_ == 0) {
      return;
    }
    // 关键帧
    if (ftype == 3) {
      frame = faac_->Encode(frame, len, len);
    }
    if (len == 0) {
      return;
    }
    int64_t dts = ts - firstFrameTs_;
    if (ldts_ > 0 && dts == ldts_) {
      dts += 20;
    }
    ldts_ = dts;
    AVPacket *pkt = av_packet_alloc();
    if (ftype == 0x01) {
      pkt->flags |= AV_PKT_FLAG_KEY;
    }
    pkt->stream_index = ftype == 0x03 ? nASIdx_ : nVSIdx_;
    pkt->data = (uint8_t *)frame;
    pkt->size = (int)len;
    // 当前相对于第一包数据的毫秒数
    pkt->dts = dts;
    pkt->pts = dts;
    // pOfmtCtx_里面的time_base 和
    //_pIfmtCtx里面的time_base不一样，一定要做下面的转换，否则，就会出现视频时长不正确，帧率不一样的错误
    av_packet_rescale_ts(pkt, av_make_q(1, 1000),
                         pOfmtCtx_->streams[pkt->stream_index]->time_base);
    int ret = av_interleaved_write_frame(pOfmtCtx_, pkt);
    avio_flush(pOfmtCtx_->pb);
    av_packet_free(&pkt);
  }

  void ReadFrame() {}

private:
  bool init(const char *filename) {
    int code = avformat_alloc_output_context2(&pOfmtCtx_, NULL, NULL, filename);
    if (code != 0 || NULL == pOfmtCtx_) {
      return false;
    }
    pOfmtCtx_->debug |= FF_FDEBUG_TS;

    code = avio_open(&(pOfmtCtx_->pb), filename, AVIO_FLAG_WRITE);
    return 0 == code;
  }

  bool newAVStream(nalu::Units &units, char *extra, int extraLen) {
    {
      AVStream *pAvStream = avformat_new_stream(pOfmtCtx_, NULL);
      if (pAvStream == NULL) {
        return false;
      }
      auto ns = nalu::Sort(units);
      auto avCode = ns.size() == 2 ? AV_CODEC_ID_H264 : AV_CODEC_ID_HEVC;
      UINT _nWidth = 0, _nHeight = 0;
      int _nfps = 0;
      ns[0].decode_sps(_nWidth, _nHeight, _nfps);
      nVSIdx_ = pOfmtCtx_->nb_streams - 1;
      pAvStream->codecpar->width = _nWidth;
      pAvStream->codecpar->height = _nHeight;
      pAvStream->codecpar->format = AV_PIX_FMT_YUV420P;
      pAvStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
      pAvStream->codecpar->codec_id = avCode;
      pAvStream->avg_frame_rate = av_make_q(_nfps, 1);
      pAvStream->r_frame_rate = av_make_q(_nfps, 1);

      // 此处需要按照该规则来创建对应的缓冲区,否则调用avformat_free_context会报错
      // 也可以采用自己的缓冲区,然后再avformat_free_context之前把这两个变量置空
      // 需要把Sps/Pps/Vps写入扩展信息中
      pAvStream->codecpar->extradata =
          (uint8_t *)av_malloc(extraLen + AV_INPUT_BUFFER_PADDING_SIZE);
      ::memcpy(pAvStream->codecpar->extradata, extra, extraLen);
      pAvStream->codecpar->extradata_size = extraLen;
    }
    //  创建音频输入格式
    {
      AVStream *pAvStream = avformat_new_stream(pOfmtCtx_, NULL);
      if (pAvStream == NULL) {
        return false;
      }
      nASIdx_ = pOfmtCtx_->nb_streams - 1;
      pAvStream->codecpar->ch_layout.nb_channels = 1;
      pAvStream->codecpar->sample_rate = 8000;
      pAvStream->codecpar->format = AV_SAMPLE_FMT_S16;
      pAvStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
      pAvStream->codecpar->codec_id = AV_CODEC_ID_AAC;
      pAvStream->codecpar->frame_size = 1024;

      pAvStream->codecpar->bit_rate = 16000;
      faac_ = new libfaac::Encoder(8000, 1, 16, libfaac::STREAM_RAW);
      int specialLen = 0;
      unsigned char *pSpecialData = faac_->SpecialData(&specialLen);
      pAvStream->codecpar->extradata =
          (uint8_t *)av_malloc(specialLen + AV_INPUT_BUFFER_PADDING_SIZE);
      ::memcpy(pAvStream->codecpar->extradata, pSpecialData, specialLen);
      pAvStream->codecpar->extradata_size = specialLen;
    }
    // 写入流信息
    return avformat_write_header(pOfmtCtx_, NULL) == 0;
  }
};
} // namespace libffmpeg
