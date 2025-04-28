#pragma once

#include "faac.h"
#include <string.h>

namespace libav {

const int KAmpBufSize = 2048;

enum {
  AAC_STREAM_RAW = 0,
  AAC_STREAM_ADTS = 1,
};

class Faac {
private:
  faacEncHandle faac_;
  unsigned long maxOutputBytes_;
  unsigned long inputSamples_;
  int nSpecialLen_;
  unsigned char *pOutBytes_;
  char cSpecialData_[8]; // 音频特殊信息
  char *pAmpData_;
  int nAmpOff_;

private:
  // 8000 1 16 AAC_STREAM_RAW
  bool init(int nSample, int nChn, int nBits, int nOfmt) {
    faac_ = faacEncOpen(nSample, nChn, &inputSamples_, &maxOutputBytes_);
    // unsigned long maxInputBytes =_inputSamples * nBitsPerSample / 8;  //
    // 计算最大输入字节
    faacEncConfigurationPtr pfaacEncConf =
        faacEncGetCurrentConfiguration(faac_);
    pfaacEncConf->inputFormat = FAAC_INPUT_16BIT;
    pfaacEncConf->outputFormat = nOfmt; // 0:RAW——STREAM	1:ADTS-STREAM
    pfaacEncConf->useTns = true;
    pfaacEncConf->useLfe = false;
    pfaacEncConf->aacObjectType = LOW;
    pfaacEncConf->shortctl = SHORTCTL_NORMAL;
    pfaacEncConf->quantqual = 100;
    pfaacEncConf->bandWidth = 0;
    pfaacEncConf->bitRate = 44100;
    pfaacEncConf->mpegVersion = MPEG2;
    pfaacEncConf->version = MPEG2;
    faacEncSetConfiguration(faac_, pfaacEncConf);

    unsigned char *pConfBuffer = 0;
    unsigned long ulConfigLength = 0;
    faacEncGetDecoderSpecificInfo(faac_, &pConfBuffer, &ulConfigLength);

    ::memcpy(cSpecialData_, pConfBuffer, ulConfigLength);
    nSpecialLen_ = (int)ulConfigLength;

    // 音频帧的播放时间 = 一个AAC帧对应的采样样本的个数 / 采样频率(单位为s)
    // 一帧 1024个 sample。采样率 Samplerate 44100KHz，每秒44100个sample, 所以
    // 根据公式   音频帧的播放时间 = 一个AAC帧对应的采样样本的个数 / 采样频率
    // 当前AAC一帧的播放时间是 = 1024 * 1000000 / 44100 = 22320ns(单位为ns)
    // duration = 1024 * 1000 / nSample;
    pOutBytes_ = new unsigned char[maxOutputBytes_];
    inputSamples_ = KAmpBufSize / (nSample / 8);
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
  Faac(int nSample, int nChn, int nBits, int nOfmt) {
    faac_ = NULL;
    pOutBytes_ = NULL;
    nAmpOff_ = 0;
    pAmpData_ = new char[KAmpBufSize * 2];
    this->init(nSample, nChn, nBits, nOfmt);
  }
  ~Faac() { this->uninit(); }

  // 编码 返回编码后数据长度
  char *Encode(char *pcmData, int pcmLen, int &accLen) {
    if (NULL == faac_) {
      return 0;
    }
    memcpy(pAmpData_ + nAmpOff_, pcmData, pcmLen);
    nAmpOff_ += pcmLen;
    if (nAmpOff_ < KAmpBufSize) {
      return 0;
    }
    int nEncDataLen = faacEncEncode(faac_, (int *)pAmpData_, inputSamples_,
                                    pOutBytes_, maxOutputBytes_);
    // 需要更新AMP Buffer数据，并且删除原有的数据
    nAmpOff_ -= KAmpBufSize; // 剩余未编码AAC的PCM数据
    if (nAmpOff_ > 0) {
      ::memmove(pAmpData_, pAmpData_ + KAmpBufSize, nAmpOff_);
    }
    accLen = nEncDataLen;
    return (char *)pOutBytes_;
  }

  char *SpecialData(int *len) {
    *len = nSpecialLen_;
    return cSpecialData_;
  }
};
} // namespace libav
