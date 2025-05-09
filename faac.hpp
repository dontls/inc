#pragma once

#include "faac.h"
#include <string.h>

namespace libfaac {

const int KAmpBufSize = 2048;

enum {
  STREAM_RAW = 0,
  STREAM_ADTS = 1,
};

class Encoder {
private:
  faacEncHandle faac_ = nullptr;
  unsigned long maxOutput_ = 0;
  unsigned long inputSamples_ = 0;
  int nSpecialLen_ = 0;
  unsigned char *pOutBytes_ = nullptr;
  char cSpecialData_[8] = {}; // 音频特殊信息
  char *pAmpData_ = nullptr;
  int nAmpOff_ = 0;

private:
  // 8000 1 16 STREAM_RAW
  bool init(int nSample, int nChn, int nBits, int nOfmt) {
    faac_ = faacEncOpen(nSample, nChn, &inputSamples_, &maxOutput_);
    // 计算最大输入字节
    faacEncConfigurationPtr cfg = faacEncGetCurrentConfiguration(faac_);
    cfg->inputFormat = FAAC_INPUT_16BIT;
    cfg->outputFormat = nOfmt; // 0:RAW——STREAM	1:ADTS-STREAM
    cfg->useTns = true;
    cfg->useLfe = false;
    cfg->aacObjectType = LOW;
    cfg->shortctl = SHORTCTL_NORMAL;
    cfg->quantqual = 100;
    cfg->bandWidth = 0;
    cfg->bitRate = 44100;
    cfg->mpegVersion = MPEG2;
    cfg->version = MPEG2;
    faacEncSetConfiguration(faac_, cfg);

    unsigned char *pBuffer = 0;
    unsigned long ulLength = 0;
    faacEncGetDecoderSpecificInfo(faac_, &pBuffer, &ulLength);

    ::memcpy(cSpecialData_, pBuffer, ulLength);
    nSpecialLen_ = (int)ulLength;

    // 音频帧的播放时间 = 一个AAC帧对应的采样样本的个数 / 采样频率(单位为s)
    // 一帧 1024个 sample。采样率 Samplerate 44100KHz，每秒44100个sample, 所以
    // 根据公式   音频帧的播放时间 = 一个AAC帧对应的采样样本的个数 / 采样频率
    // 当前AAC一帧的播放时间是 = 1024 * 1000000 / 44100 = 22320ns(单位为ns)
    // duration = 1024 * 1000 / nSample;
    pOutBytes_ = new unsigned char[maxOutput_];
    // inputSamples_ = KAmpBufSize / (nBits / 8);
    return true;
  }

  void uninit() {
    if (faac_) {
      faacEncClose(faac_);
    }
    if (pOutBytes_) {
      delete[] pOutBytes_;
      pOutBytes_ = NULL;
    }
    if (pAmpData_) {
      delete[] pAmpData_;
      pAmpData_ = NULL;
    }
  }

public:
  Encoder(int nSample, int nChn, int nBits, int nOfmt) {
    pAmpData_ = new char[KAmpBufSize * 2];
    this->init(nSample, nChn, nBits, nOfmt);
  }
  ~Encoder() { this->uninit(); }

  // 编码 返回编码后数据长度
  int Encode(char *pcmData, int pcmLen) {
    if (NULL == faac_) {
      return 0;
    }
    memcpy(pAmpData_ + nAmpOff_, pcmData, pcmLen);
    nAmpOff_ += pcmLen;
    if (nAmpOff_ < KAmpBufSize) {
      return 0;
    }
    int accLen = 0;
    while (accLen < 1) {
      accLen = faacEncEncode(faac_, (int *)pAmpData_, inputSamples_, pOutBytes_,
                             maxOutput_);
    }
    nAmpOff_ -= KAmpBufSize; // 剩余未编码AAC的PCM数据
    if (nAmpOff_ > 0) {
      ::memmove(pAmpData_, pAmpData_ + KAmpBufSize, nAmpOff_);
    }
    return accLen;
  }

  char *SpecialData() { return cSpecialData_; }

  char *Data() { return (char *)pOutBytes_; }
};
} // namespace libfaac
