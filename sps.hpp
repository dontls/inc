#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned long DWORD;

inline UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit) {
  // 计算0bit的个数
  UINT nZeroNum = 0;
  while (nStartBit < nLen * 8) {
    if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
    {
      break;
    }
    nZeroNum++;
    nStartBit++;
  }
  nStartBit++;

  // 计算结果
  DWORD dwRet = 0;
  for (UINT i = 0; i < nZeroNum; i++) {
    dwRet <<= 1;
    if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
      dwRet += 1;
    }
    nStartBit++;
  }
  return (1 << nZeroNum) - 1 + dwRet;
}

inline int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit) {
  int UeVal = Ue(pBuff, nLen, nStartBit);
  double k = UeVal;
  int nValue = ceil(
      k /
      2); // ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
  if (UeVal % 2 == 0)
    nValue = -nValue;
  return nValue;
}

inline DWORD u(UINT BitCount, BYTE *buf, UINT &nStartBit) {
  DWORD dwRet = 0;
  for (UINT i = 0; i < BitCount; i++) {
    dwRet <<= 1;
    if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
      dwRet += 1;
    }
    nStartBit++;
  }
  return dwRet;
}

/**
 * H264的NAL起始码防竞争机制
 *
 * @param buf SPS数据内容
 *
 * @无返回值
 */
inline void de_emulation_prevention(BYTE *buf, unsigned int *buf_size) {
  unsigned int i = 0, j = 0;
  BYTE *tmp_ptr = NULL;
  unsigned int tmp_buf_size = 0;
  int val = 0;

  tmp_ptr = buf;
  tmp_buf_size = *buf_size;
  for (i = 0; i < (tmp_buf_size - 2); i++) {
    // check for 0x000003
    val =
        (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
    if (val == 0) {
      // kick out 0x03
      for (j = i + 2; j < tmp_buf_size - 1; j++)
        tmp_ptr[j] = tmp_ptr[j + 1];

      // and so we should devrease bufsize
      (*buf_size)--;
    }
  }

  return;
}

/**
 * 解码SPS,获取视频图像宽、高信息
 *
 * @param buf SPS数据内容
 * @param nLen SPS数据的长度
 * @param width 图像宽度
 * @param height 图像高度
 * @成功则返回1 , 失败则返回0
 */

namespace hevc {

enum {
  NALU_TYPE_CODED_SLICE_TRAIL_N = 0, // 0
  NALU_TYPE_CODED_SLICE_TRAIL_R,     // 1

  NALU_TYPE_CODED_SLICE_TSA_N, // 2
  NALU_TYPE_CODED_SLICE_TLA,   // 3   // Current name in the spec: TSA_R

  NALU_TYPE_CODED_SLICE_STSA_N, // 4
  NALU_TYPE_CODED_SLICE_STSA_R, // 5

  NALU_TYPE_CODED_SLICE_RADL_N, // 6
  NALU_TYPE_CODED_SLICE_DLP,    // 7 // Current name in the spec: RADL_R

  NALU_TYPE_CODED_SLICE_RASL_N, // 8
  NALU_TYPE_CODED_SLICE_TFD,    // 9 // Current name in the spec: RASL_R

  NALU_TYPE_RESERVED_10,
  NALU_TYPE_RESERVED_11,
  NALU_TYPE_RESERVED_12,
  NALU_TYPE_RESERVED_13,
  NALU_TYPE_RESERVED_14,
  NALU_TYPE_RESERVED_15,          // Current name in the spec: BLA_W_LP
  NALU_TYPE_CODED_SLICE_BLA,      // 16   // Current name in the spec: BLA_W_LP
  NALU_TYPE_CODED_SLICE_BLANT,    // 17   // Current name in the spec: BLA_W_DLP
  NALU_TYPE_CODED_SLICE_BLA_N_LP, // 18
  NALU_TYPE_CODED_SLICE_IDR,      // 19  // Current name in the spec: IDR_W_DLP
  NALU_TYPE_CODED_SLICE_IDR_N_LP, // 20
  NALU_TYPE_CODED_SLICE_CRA,      // 21
  NALU_TYPE_RESERVED_22,
  NALU_TYPE_RESERVED_23,

  NALU_TYPE_RESERVED_24,
  NALU_TYPE_RESERVED_25,
  NALU_TYPE_RESERVED_26,
  NALU_TYPE_RESERVED_27,
  NALU_TYPE_RESERVED_28,
  NALU_TYPE_RESERVED_29,
  NALU_TYPE_RESERVED_30,
  NALU_TYPE_RESERVED_31,

  NALU_TYPE_VPS,                   // 32
  NALU_TYPE_SPS,                   // 33
  NALU_TYPE_PPS,                   // 34
  NALU_TYPE_ACCESS_UNIT_DELIMITER, // 35
  NALU_TYPE_EOS,                   // 36
  NALU_TYPE_EOB,                   // 37
  NALU_TYPE_FILLER_DATA,           // 38
  NALU_TYPE_SEI,                   // 39 Prefix SEI
  NALU_TYPE_SEI_SUFFIX,            // 40 Suffix SEI
  NALU_TYPE_RESERVED_41,
  NALU_TYPE_RESERVED_42,
  NALU_TYPE_RESERVED_43,
  NALU_TYPE_RESERVED_44,
  NALU_TYPE_RESERVED_45,
  NALU_TYPE_RESERVED_46,
  NALU_TYPE_RESERVED_47,
  NALU_TYPE_UNSPECIFIED_48,
  NALU_TYPE_UNSPECIFIED_49,
  NALU_TYPE_UNSPECIFIED_50,
  NALU_TYPE_UNSPECIFIED_51,
  NALU_TYPE_UNSPECIFIED_52,
  NALU_TYPE_UNSPECIFIED_53,
  NALU_TYPE_UNSPECIFIED_54,
  NALU_TYPE_UNSPECIFIED_55,
  NALU_TYPE_UNSPECIFIED_56,
  NALU_TYPE_UNSPECIFIED_57,
  NALU_TYPE_UNSPECIFIED_58,
  NALU_TYPE_UNSPECIFIED_59,
  NALU_TYPE_UNSPECIFIED_60,
  NALU_TYPE_UNSPECIFIED_61,
  NALU_TYPE_UNSPECIFIED_62,
  NALU_TYPE_UNSPECIFIED_63,
  NALU_TYPE_INVALID,
};
inline int decode_sps(BYTE *buf, unsigned int nLen, uint16_t &width,
                      uint16_t &height, int &fps) {
  if (nLen < 20) {
    return false;
  }
  UINT StartBit = 0;
  fps = 0;
  de_emulation_prevention(buf, &nLen);
  u(4, buf, StartBit); // sps_video_parameter_set_id
  int sps_max_sub_layers_minus1 = u(3, buf, StartBit);
  if (sps_max_sub_layers_minus1 > 6) {
    return false;
  }
  u(1, buf, StartBit);
  {
    u(3, buf, StartBit);
    int profile = u(5, buf, StartBit);
    u(32, buf, StartBit);
    u(48, buf, StartBit);
    int level = u(8, buf, StartBit); // general_level_idc
    uint8_t sub_layer_profile_present_flag[6] = {0};
    uint8_t sub_layer_level_present_flag[6] = {0};
    for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
      sub_layer_profile_present_flag[i] = u(1, buf, StartBit);
      sub_layer_level_present_flag[i] = u(1, buf, StartBit);
    }
    if (sps_max_sub_layers_minus1 > 0) {
      for (int i = sps_max_sub_layers_minus1; i < 8; i++) {
        uint8_t reserved_zero_2bits = u(2, buf, StartBit);
      }
    }
    for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
      if (sub_layer_profile_present_flag[i]) {
        u(8, buf, StartBit);
        u(32, buf, StartBit);
        u(48, buf, StartBit);
      }
      if (sub_layer_level_present_flag[i]) {
        u(8, buf, StartBit); // sub_layer_level_idc[i]
      }
    }
  }
  uint32_t sps_seq_parameter_set_id = Ue(buf, nLen, StartBit);
  if (sps_seq_parameter_set_id > 15) {
    return false;
  }
  uint32_t chroma_format_idc = Ue(buf, nLen, StartBit);
  if (sps_seq_parameter_set_id > 3) {
    return false;
  }
  if (chroma_format_idc == 3) {
    u(1, buf, StartBit);
  }
  width = Ue(buf, nLen, StartBit);  // pic_width_in_luma_samples
  height = Ue(buf, nLen, StartBit); // pic_height_in_luma_samples
  if (u(1, buf, StartBit)) {
    Ue(buf, nLen, StartBit);
    Ue(buf, nLen, StartBit);
    Ue(buf, nLen, StartBit);
    Ue(buf, nLen, StartBit);
  }
  uint32_t bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
  uint32_t bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
  if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
    return false;
  } //...
  fps = 25;
  return true;
}
} // namespace hevc

namespace avc {

enum {
  NALU_TYPE_SLICE = 1,
  NALU_TYPE_DPA = 2,
  NALU_TYPE_DPB = 3,
  NALU_TYPE_DPC = 4,
  NALU_TYPE_IDR = 5,
  NALU_TYPE_SEI = 6,
  NALU_TYPE_SPS = 7,
  NALU_TYPE_PPS = 8,
  NALU_TYPE_AUD = 9,
  NALU_TYPE_EOSEQ = 10,
  NALU_TYPE_EOSTREAM = 11,
  NALU_TYPE_FILL = 12,
};

inline int decode_sps(BYTE *buf, unsigned int nLen, uint16_t &width,
                      uint16_t &height, int &fps) {
  UINT StartBit = 0;
  fps = 0;
  de_emulation_prevention(buf, &nLen);

  int forbidden_zero_bit = u(1, buf, StartBit);
  int nal_ref_idc = u(2, buf, StartBit);
  int NALU_TYPE_type = u(5, buf, StartBit);
  if (NALU_TYPE_type == 7) {
    int profile_idc = u(8, buf, StartBit);
    int constraint_set0_flag = u(1, buf, StartBit); //(buf[1] & 0x80)>>7;
    int constraint_set1_flag = u(1, buf, StartBit); //(buf[1] & 0x40)>>6;
    int constraint_set2_flag = u(1, buf, StartBit); //(buf[1] & 0x20)>>5;
    int constraint_set3_flag = u(1, buf, StartBit); //(buf[1] & 0x10)>>4;
    int reserved_zero_4bits = u(4, buf, StartBit);
    int level_idc = u(8, buf, StartBit);

    int seq_parameter_set_id = Ue(buf, nLen, StartBit);

    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
        profile_idc == 144) {
      int chroma_format_idc = Ue(buf, nLen, StartBit);
      if (chroma_format_idc == 3)
        int residual_colour_transform_flag = u(1, buf, StartBit);
      int bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
      int bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
      int qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
      int seq_scaling_matrix_present_flag = u(1, buf, StartBit);

      int seq_scaling_list_present_flag[8];
      if (seq_scaling_matrix_present_flag) {
        for (int i = 0; i < 8; i++) {
          seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
        }
      }
    }
    int log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
    int pic_order_cnt_type = Ue(buf, nLen, StartBit);
    if (pic_order_cnt_type == 0)
      int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
    else if (pic_order_cnt_type == 1) {
      int delta_pic_order_always_zero_flag = u(1, buf, StartBit);
      int offset_for_non_ref_pic = Se(buf, nLen, StartBit);
      int offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
      int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

      int *offset_for_ref_frame =
          new int[num_ref_frames_in_pic_order_cnt_cycle];
      for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
      delete[] offset_for_ref_frame;
    }
    int num_ref_frames = Ue(buf, nLen, StartBit);
    int gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
    int pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
    int pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

    width = (pic_width_in_mbs_minus1 + 1) * 16;
    height = (pic_height_in_map_units_minus1 + 1) * 16;

    int frame_mbs_only_flag = u(1, buf, StartBit);
    if (!frame_mbs_only_flag)
      int mb_adaptive_frame_field_flag = u(1, buf, StartBit);

    int direct_8x8_inference_flag = u(1, buf, StartBit);
    int frame_cropping_flag = u(1, buf, StartBit);
    if (frame_cropping_flag) {
      int frame_crop_left_offset = Ue(buf, nLen, StartBit);
      int frame_crop_right_offset = Ue(buf, nLen, StartBit);
      int frame_crop_top_offset = Ue(buf, nLen, StartBit);
      int frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
    }
    int vui_parameter_present_flag = u(1, buf, StartBit);
    if (vui_parameter_present_flag) {
      int aspect_ratio_info_present_flag = u(1, buf, StartBit);
      if (aspect_ratio_info_present_flag) {
        int aspect_ratio_idc = u(8, buf, StartBit);
        if (aspect_ratio_idc == 255) {
          int sar_width = u(16, buf, StartBit);
          int sar_height = u(16, buf, StartBit);
        }
      }
      int overscan_info_present_flag = u(1, buf, StartBit);
      if (overscan_info_present_flag)
        int overscan_appropriate_flagu = u(1, buf, StartBit);
      int video_signal_type_present_flag = u(1, buf, StartBit);
      if (video_signal_type_present_flag) {
        int video_format = u(3, buf, StartBit);
        int video_full_range_flag = u(1, buf, StartBit);
        int colour_description_present_flag = u(1, buf, StartBit);
        if (colour_description_present_flag) {
          int colour_primaries = u(8, buf, StartBit);
          int transfer_characteristics = u(8, buf, StartBit);
          int matrix_coefficients = u(8, buf, StartBit);
        }
      }
      int chroma_loc_info_present_flag = u(1, buf, StartBit);
      if (chroma_loc_info_present_flag) {
        int chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
        int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
      }
      int timing_info_present_flag = u(1, buf, StartBit);
      if (timing_info_present_flag) {
        int num_units_in_tick = u(32, buf, StartBit);
        int time_scale = u(32, buf, StartBit);
        fps = time_scale / (2 * num_units_in_tick);
      }
    }
    if (fps == 0) {
      fps = 25;
    }
    return true;
  } else
    return false;
}
} // namespace avc

// 返回nalu结束位置, 包含nalu头

struct nalu {
  int type;
  int size;
  char *data;
};

// 返回下个nalu开始的位置，length剩余数据长度
inline char *naluparse(char *data, size_t &length, std::vector<nalu> &nalus) {
  if (length <= 0) {
    return nullptr;
  }
  char *ptr = data;
  size_t i = 7;
  for (; i < length; i++) {
    if (ptr[i] == 0x01 && ptr[i - 1] == 0x00 && ptr[i - 2] == 0x00 &&
        ptr[i - 3] == 0x00) {
      break;
    }
  }
  // 最后一个
  if (i >= length) {
    return ptr;
  }
  nalu u;
  u.data = ptr;
  u.type = ptr[4];
  u.size = i - 3;
  nalus.push_back(u);
  length -= u.size;
  return naluparse(ptr + u.size, length, nalus);
} // namespace nalu
