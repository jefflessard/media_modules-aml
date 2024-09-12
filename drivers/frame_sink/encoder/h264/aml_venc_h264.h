/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef __AVC_ENCODER_HW_OPS_H__
#define __AVC_ENCODER_HW_OPS_H__


#define H264_ENC_CBR
#define MULTI_SLICE_MC


#define HCODEC_IRQ_MBOX_CLR HCODEC_ASSIST_MBOX2_CLR_REG
#define HCODEC_IRQ_MBOX_MASK HCODEC_ASSIST_MBOX2_MASK


/********************************************
 *  AV Scratch Register Re-Define
 ****************************************** *
 */
#define ENCODER_STATUS            HCODEC_HENC_SCRATCH_0
#define MEM_OFFSET_REG            HCODEC_HENC_SCRATCH_1
#define DEBUG_REG                 HCODEC_HENC_SCRATCH_2
#define IDR_PIC_ID                HCODEC_HENC_SCRATCH_5
#define FRAME_NUMBER              HCODEC_HENC_SCRATCH_6
#define PIC_ORDER_CNT_LSB         HCODEC_HENC_SCRATCH_7
#define LOG2_MAX_PIC_ORDER_CNT_LSB  HCODEC_HENC_SCRATCH_8
#define LOG2_MAX_FRAME_NUM          HCODEC_HENC_SCRATCH_9
#define ANC0_BUFFER_ID              HCODEC_HENC_SCRATCH_A
#define QPPICTURE                   HCODEC_HENC_SCRATCH_B

#define IE_ME_MB_TYPE               HCODEC_HENC_SCRATCH_D

/* bit 0-4, IE_PIPELINE_BLOCK
 * bit 5    me half pixel in m8
 *		disable i4x4 in gxbb
 * bit 6    me step2 sub pixel in m8
 *		disable i16x16 in gxbb
 */
#define IE_ME_MODE                  HCODEC_HENC_SCRATCH_E
#define IE_REF_SEL                  HCODEC_HENC_SCRATCH_F

#define IE_PIPELINE_BLOCK_SHIFT 0
#define IE_PIPELINE_BLOCK_MASK  0x1f
#define ME_PIXEL_MODE_SHIFT 5
#define ME_PIXEL_MODE_MASK  0x3

/* [31:0] NUM_ROWS_PER_SLICE_P */
/* [15:0] NUM_ROWS_PER_SLICE_I */
#define FIXED_SLICE_CFG             HCODEC_HENC_SCRATCH_L

/* For GX */
#define INFO_DUMP_START_ADDR      HCODEC_HENC_SCRATCH_I

/* For CBR */
#define H264_ENC_CBR_TABLE_ADDR   HCODEC_HENC_SCRATCH_3
#define H264_ENC_CBR_MB_SIZE_ADDR      HCODEC_HENC_SCRATCH_4
/* Bytes(Float) * 256 */
#define H264_ENC_CBR_CTL          HCODEC_HENC_SCRATCH_G
/* [31:28] : init qp table idx */
/* [27:24] : short_term adjust shift */
/* [23:16] : Long_term MB_Number between adjust, */
/* [15:0] Long_term adjust threshold(Bytes) */
#define H264_ENC_CBR_TARGET_SIZE  HCODEC_HENC_SCRATCH_H
/* Bytes(Float) * 256 */
#define H264_ENC_CBR_PREV_BYTES   HCODEC_HENC_SCRATCH_J
#define H264_ENC_CBR_REGION_SIZE   HCODEC_HENC_SCRATCH_J

/* for SVC */
#define H264_ENC_SVC_PIC_TYPE      HCODEC_HENC_SCRATCH_K

/* define for PIC  header */
#define ENC_SLC_REF 0x8410
#define ENC_SLC_NON_REF 0x8010

/* --------------------------------------------------- */
/* ENCODER_STATUS define */
/* --------------------------------------------------- */
#define ENCODER_IDLE              0
#define ENCODER_SEQUENCE          1
#define ENCODER_PICTURE           2
#define ENCODER_IDR               3
#define ENCODER_NON_IDR           4
#define ENCODER_MB_HEADER         5
#define ENCODER_MB_DATA           6

#define ENCODER_SEQUENCE_DONE          7
#define ENCODER_PICTURE_DONE           8
#define ENCODER_IDR_DONE               9
#define ENCODER_NON_IDR_DONE           10
#define ENCODER_MB_HEADER_DONE         11
#define ENCODER_MB_DATA_DONE           12

#define ENCODER_NON_IDR_INTRA     13
#define ENCODER_NON_IDR_INTER     14

#define ENCODER_ERROR     0xff

/********************************************
 * defines for H.264 mb_type
 *******************************************
 */
#define HENC_MB_Type_PBSKIP                      0x0
#define HENC_MB_Type_PSKIP                       0x0
#define HENC_MB_Type_BSKIP_DIRECT                0x0
#define HENC_MB_Type_P16x16                      0x1
#define HENC_MB_Type_P16x8                       0x2
#define HENC_MB_Type_P8x16                       0x3
#define HENC_MB_Type_SMB8x8                      0x4
#define HENC_MB_Type_SMB8x4                      0x5
#define HENC_MB_Type_SMB4x8                      0x6
#define HENC_MB_Type_SMB4x4                      0x7
#define HENC_MB_Type_P8x8                        0x8
#define HENC_MB_Type_I4MB                        0x9
#define HENC_MB_Type_I16MB                       0xa
#define HENC_MB_Type_IBLOCK                      0xb
#define HENC_MB_Type_SI4MB                       0xc
#define HENC_MB_Type_I8MB                        0xd
#define HENC_MB_Type_IPCM                        0xe
#define HENC_MB_Type_AUTO                        0xf

#define HENC_MB_CBP_AUTO                         0xff
#define HENC_SKIP_RUN_AUTO                     0xffff

#define ADV_MV_LARGE_16x8 1
#define ADV_MV_LARGE_8x16 1
#define ADV_MV_LARGE_16x16 1

/* me weight offset should not very small, it used by v1 me module. */
/* the min real sad for me is 16 by hardware. */
#define ME_WEIGHT_OFFSET 0x520
#define I4MB_WEIGHT_OFFSET 0x655
#define I16MB_WEIGHT_OFFSET 0x560

#define ADV_MV_16x16_WEIGHT 0x080
#define ADV_MV_16_8_WEIGHT 0x0e0
#define ADV_MV_8x8_WEIGHT 0x240
#define ADV_MV_4x4x4_WEIGHT 0x3000

#define IE_SAD_SHIFT_I16 0x001
#define IE_SAD_SHIFT_I4 0x001
#define ME_SAD_SHIFT_INTER 0x001

#define STEP_2_SKIP_SAD 0
#define STEP_1_SKIP_SAD 0
#define STEP_0_SKIP_SAD 0
#define STEP_2_SKIP_WEIGHT 0
#define STEP_1_SKIP_WEIGHT 0
#define STEP_0_SKIP_WEIGHT 0

#define ME_SAD_RANGE_0 0x1 /* 0x0 */
#define ME_SAD_RANGE_1 0x0
#define ME_SAD_RANGE_2 0x0
#define ME_SAD_RANGE_3 0x0

/* use 0 for v3, 0x18 for v2 */
#define ME_MV_PRE_WEIGHT_0 0x18
/* use 0 for v3, 0x18 for v2 */
#define ME_MV_PRE_WEIGHT_1 0x18
#define ME_MV_PRE_WEIGHT_2 0x0
#define ME_MV_PRE_WEIGHT_3 0x0

/* use 0 for v3, 0x18 for v2 */
#define ME_MV_STEP_WEIGHT_0 0x18
/* use 0 for v3, 0x18 for v2 */
#define ME_MV_STEP_WEIGHT_1 0x18
#define ME_MV_STEP_WEIGHT_2 0x0
#define ME_MV_STEP_WEIGHT_3 0x0

#define ME_SAD_ENOUGH_0_DATA 0x00
#define ME_SAD_ENOUGH_1_DATA 0x04
#define ME_SAD_ENOUGH_2_DATA 0x11
#define ADV_MV_8x8_ENOUGH_DATA 0x20

/* V4_COLOR_BLOCK_FIX */
#define V3_FORCE_SKIP_SAD_0 0x10
/* 4 Blocks */
#define V3_FORCE_SKIP_SAD_1 0x60
/* 16 Blocks + V3_SKIP_WEIGHT_2 */
#define V3_FORCE_SKIP_SAD_2 0x250
/* almost disable it -- use t_lac_coeff_2 output to F_ZERO is better */
#define V3_ME_F_ZERO_SAD (ME_WEIGHT_OFFSET + 0x10)

#define V3_IE_F_ZERO_SAD_I16 (I16MB_WEIGHT_OFFSET + 0x10)
#define V3_IE_F_ZERO_SAD_I4 (I4MB_WEIGHT_OFFSET + 0x20)

#define V3_SKIP_WEIGHT_0 0x10
/* 4 Blocks  8 separate search sad can be very low */
#define V3_SKIP_WEIGHT_1 0x8 /* (4 * ME_MV_STEP_WEIGHT_1 + 0x100) */
#define V3_SKIP_WEIGHT_2 0x3

#define V3_LEVEL_1_F_SKIP_MAX_SAD 0x0
#define V3_LEVEL_1_SKIP_MAX_SAD 0x6

#define I4_ipred_weight_most   0x18
#define I4_ipred_weight_else   0x28

#define C_ipred_weight_V       0x04
#define C_ipred_weight_H       0x08
#define C_ipred_weight_DC      0x0c

#define I16_ipred_weight_V       0x04
#define I16_ipred_weight_H       0x08
#define I16_ipred_weight_DC      0x0c

/* 0x00 same as disable */
#define v3_left_small_max_ie_sad 0x00
#define v3_left_small_max_me_sad 0x40

#define v5_use_small_diff_cnt 0
#define v5_simple_mb_inter_all_en 1
#define v5_simple_mb_inter_8x8_en 1
#define v5_simple_mb_inter_16_8_en 1
#define v5_simple_mb_inter_16x16_en 1
#define v5_simple_mb_intra_en 1
#define v5_simple_mb_C_en 0
#define v5_simple_mb_Y_en 1
#define v5_small_diff_Y 0x10
#define v5_small_diff_C 0x18
/* shift 8-bits, 2, 1, 0, -1, -2, -3, -4 */
#define v5_simple_dq_setting 0x43210fed
#define v5_simple_me_weight_setting 0

#ifdef H264_ENC_CBR
#define CBR_TABLE_SIZE  0x800
#define CBR_SHORT_SHIFT 12 /* same as disable */
#define CBR_LONG_MB_NUM 2
#define START_TABLE_ID 8
#define CBR_LONG_THRESH 4
#endif


extern u32 me_mv_merge_ctl;
extern u32 me_mv_weight_01;
extern u32 me_mv_weight_23;
extern u32 me_sad_range_inc;
extern u32 me_step0_close_mv;
extern u32 me_f_skip_sad;
extern u32 me_f_skip_weight;
extern u32 me_sad_enough_01;
extern u32 me_sad_enough_23;


/* y tnr */
extern unsigned int y_tnr_mc_en;
extern unsigned int y_tnr_txt_mode;
extern unsigned int y_tnr_mot_sad_margin;
extern unsigned int y_tnr_mot_cortxt_rate;
extern unsigned int y_tnr_mot_distxt_ofst;
extern unsigned int y_tnr_mot_distxt_rate;
extern unsigned int y_tnr_mot_dismot_ofst;
extern unsigned int y_tnr_mot_frcsad_lock;
extern unsigned int y_tnr_mot2alp_frc_gain;
extern unsigned int y_tnr_mot2alp_nrm_gain;
extern unsigned int y_tnr_mot2alp_dis_gain;
extern unsigned int y_tnr_mot2alp_dis_ofst;
extern unsigned int y_tnr_alpha_min;
extern unsigned int y_tnr_alpha_max;
extern unsigned int y_tnr_deghost_os;
/* c tnr */
extern unsigned int c_tnr_mc_en;
extern unsigned int c_tnr_txt_mode;
extern unsigned int c_tnr_mot_sad_margin;
extern unsigned int c_tnr_mot_cortxt_rate;
extern unsigned int c_tnr_mot_distxt_ofst;
extern unsigned int c_tnr_mot_distxt_rate;
extern unsigned int c_tnr_mot_dismot_ofst;
extern unsigned int c_tnr_mot_frcsad_lock;
extern unsigned int c_tnr_mot2alp_frc_gain;
extern unsigned int c_tnr_mot2alp_nrm_gain;
extern unsigned int c_tnr_mot2alp_dis_gain;
extern unsigned int c_tnr_mot2alp_dis_ofst;
extern unsigned int c_tnr_alpha_min;
extern unsigned int c_tnr_alpha_max;
extern unsigned int c_tnr_deghost_os;
/* y snr */
extern unsigned int y_snr_err_norm;
extern unsigned int y_snr_gau_bld_core;
extern int y_snr_gau_bld_ofst;
extern unsigned int y_snr_gau_bld_rate;
extern unsigned int y_snr_gau_alp0_min;
extern unsigned int y_snr_gau_alp0_max;
extern unsigned int y_bld_beta2alp_rate;
extern unsigned int y_bld_beta_min;
extern unsigned int y_bld_beta_max;
/* c snr */
extern unsigned int c_snr_err_norm;
extern unsigned int c_snr_gau_bld_core;
extern int c_snr_gau_bld_ofst;
extern unsigned int c_snr_gau_bld_rate;
extern unsigned int c_snr_gau_alp0_min;
extern unsigned int c_snr_gau_alp0_max;
extern unsigned int c_bld_beta2alp_rate;
extern unsigned int c_bld_beta_min;
extern unsigned int c_bld_beta_max;

void amlvenc_h264_init_qtable(u32 *quant_tbl_i4, u32 *quant_tbl_i16, u32 *quant_tbl_me);

void amlvenc_h264_init_me(void);

void amlvenc_h264_init_output_stream_buffer(u32 bitstreamStart, u32 bitstreamEnd);

void amlvenc_h264_init_input_dct_buffer(u32 dct_buff_start_addr, u32 dct_buff_end_addr);

void amlvenc_h264_init_input_reference_buffer(s32 canvas);

void amlvenc_h264_init_firmware_assist_buffer(u32 assit_buffer_offset);

void amlvenc_h264_init_dblk_buffer(s32 canvas);

void amlvenc_h264_init_encoder(
	bool idr, u32 idr_pic_id, u32 init_qppicture,
	u32 frame_number, u32 log2_max_frame_num,
	u32 pic_order_cnt_lsb, u32 log2_max_pic_order_cnt_lsb
);

void amlvenc_h264_configure_ie_me_parameters(
	u32 fixed_slice_cfg, u32 rows_per_slice, u32 ie_me_mb_type, u32 encoder_height
);

void amlvenc_h264_configure_mdfin(
	u32 input, u8 iformat,
	u8 oformat, u32 picsize_x, u32 picsize_y,
	u8 r2y_en, u8 nr, u8 ifmt_extra
);

void amlvenc_h264_init(
	bool idr, u32 quant, u32 qp_mode,
	u32 encoder_width, u32 encoder_height,
	u32 i4_weight, u32 i16_weight, u32 me_weight,
	u32 *quant_tbl_i4, u32 *quant_tbl_i16, u32 *quant_tbl_me,
	u32 cbr_ddr_start_addr, u32 cbr_start_tbl_id,
	u32 cbr_short_shift, u32 cbr_long_mb_num, u32 cbr_long_th,
	u32 cbr_block_w, u32 cbr_block_h,
	u32 *v3_mv_sad, u32 dump_ddr_start_addr
);

void amlvenc_dos_sw_reset(u32 bits);

void amlvenc_hcodec_start(void);

void amlvenc_hcodec_stop(void);








#endif /* __AVC_ENCODER_HW_OPS_H__ */
