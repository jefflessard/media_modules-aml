/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 * Copyright (C) 2024 Jean-François Lessard
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef __AVC_ENCODER_HW_OPS_H__
#define __AVC_ENCODER_HW_OPS_H__


#define H264_ENC_CBR
#define MULTI_SLICE_MC


#define IE_PIPELINE_BLOCK_SHIFT 0
#define IE_PIPELINE_BLOCK_MASK  0x1f
#define ME_PIXEL_MODE_SHIFT 5
#define ME_PIXEL_MODE_MASK  0x3

#ifdef H264_ENC_CBR
#define CBR_TABLE_SIZE  0x800
#define CBR_SHORT_SHIFT 12 /* same as disable */
#define CBR_LONG_MB_NUM 2
#define START_TABLE_ID 8
#define CBR_LONG_THRESH 4
#endif

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

/* For GX */
#define INFO_DUMP_START_ADDR      HCODEC_HENC_SCRATCH_I

/* [31:0] NUM_ROWS_PER_SLICE_P */
/* [15:0] NUM_ROWS_PER_SLICE_I */
#define FIXED_SLICE_CFG             HCODEC_HENC_SCRATCH_L

/* for SVC */
#define H264_ENC_SVC_PIC_TYPE      HCODEC_HENC_SCRATCH_K


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

/* define for PIC  header */
#define ENC_SLC_REF 0x8410
#define ENC_SLC_NON_REF 0x8010

/* me weight offset should not very small, it used by v1 me module. */
/* the min real sad for me is 16 by hardware. */
#define ME_WEIGHT_OFFSET 0x520
#define I4MB_WEIGHT_OFFSET 0x655
#define I16MB_WEIGHT_OFFSET 0x560
#define QTABLE_SIZE 8

/* --------------------------------------------------- */
/* ENCODER_STATUS define */
/* --------------------------------------------------- */
enum amlvenc_hcodec_status {
    ENCODER_IDLE = 0,
    ENCODER_SEQUENCE = 1,
    ENCODER_PICTURE = 2,
    ENCODER_IDR = 3,
    ENCODER_NON_IDR = 4,
    ENCODER_MB_HEADER = 5,
    ENCODER_MB_DATA = 6,

    ENCODER_SEQUENCE_DONE = 7,
    ENCODER_PICTURE_DONE = 8,
    ENCODER_IDR_DONE = 9,
    ENCODER_NON_IDR_DONE = 10,
    ENCODER_MB_HEADER_DONE = 11,
    ENCODER_MB_DATA_DONE = 12,

    ENCODER_NON_IDR_INTRA = 13,
    ENCODER_NON_IDR_INTER = 14,

    ENCODER_ERROR = 0xff,
};


/********************************************
 * defines for H.264 mb_type
 *******************************************
 */
enum amlvenc_henc_mb_type {
    HENC_MB_Type_PBSKIP = 0x0,
    HENC_MB_Type_PSKIP = 0x0,
    HENC_MB_Type_BSKIP_DIRECT = 0x0,
    HENC_MB_Type_P16x16 = 0x1,
    HENC_MB_Type_P16x8 = 0x2,
    HENC_MB_Type_P8x16 = 0x3,
    HENC_MB_Type_SMB8x8 = 0x4,
    HENC_MB_Type_SMB8x4 = 0x5,
    HENC_MB_Type_SMB4x8 = 0x6,
    HENC_MB_Type_SMB4x4 = 0x7,
    HENC_MB_Type_P8x8 = 0x8,
    HENC_MB_Type_I4MB = 0x9,
    HENC_MB_Type_I16MB = 0xa,
    HENC_MB_Type_IBLOCK = 0xb,
    HENC_MB_Type_SI4MB = 0xc,
    HENC_MB_Type_I8MB = 0xd,
    HENC_MB_Type_IPCM = 0xe,
    HENC_MB_Type_AUTO = 0xf,

    HENC_MB_CBP_AUTO = 0xff,
    HENC_SKIP_RUN_AUTO = 0xffff,
};

/**
 * struct amlvenc_h264_qtable_params - Quantization table prameters of H.264 encoder
 * @quant_tbl_i4: Quantization table for I4 mode
 * @quant_tbl_i16: Quantization table for I16 mode
 * @quant_tbl_me: Quantization table for motion estimation
 */
struct amlvenc_h264_qtable_params {
    u32 *quant_tbl_i4;
    u32 *quant_tbl_i16;
    u32 *quant_tbl_me;
};

// Struct for amlvenc_h264_init_encoder parameters
struct amlvenc_h264_init_encoder_params {
    bool idr;
    u32 idr_pic_id;
    u32 init_qppicture;
    u32 frame_number;
    u32 log2_max_frame_num;
    u32 pic_order_cnt_lsb;
    u32 log2_max_pic_order_cnt_lsb;
};

/**
 * struct amlvenc_h264_me_params - Motion estimation parameters
 * @me_mv_merge_ctl: Motion vector merge control
 * @me_step0_close_mv: Step 0 close motion vector
 * @me_sad_enough_01: SAD enough value for steps 0 and 1
 * @me_sad_enough_23: SAD enough value for steps 2 and 3
 * @me_f_skip_sad: SAD value for frame skip
 * @me_f_skip_weight: Weight for frame skip
 * @me_mv_weight_01: Motion vector weight for steps 0 and 1
 * @me_mv_weight_23: Motion vector weight for steps 2 and 3
 * @me_sad_range_inc: SAD range increment
 */
struct amlvenc_h264_me_params {
    u32 me_mv_merge_ctl;
    u32 me_step0_close_mv;
    u32 me_sad_enough_01;
    u32 me_sad_enough_23;
    u32 me_f_skip_sad;
    u32 me_f_skip_weight;
    u32 me_mv_weight_01;
    u32 me_mv_weight_23;
    u32 me_sad_range_inc;
};

struct amlvenc_h264_tnr_params {
    u32 mc_en;
    u32 txt_mode;
    u32 mot_sad_margin;
    u32 mot_cortxt_rate;
    u32 mot_distxt_ofst;
    u32 mot_distxt_rate;
    u32 mot_dismot_ofst;
    u32 mot_frcsad_lock;
    u32 mot2alp_frc_gain;
    u32 mot2alp_nrm_gain;
    u32 mot2alp_dis_gain;
    u32 mot2alp_dis_ofst;
    u32 alpha_min;
    u32 alpha_max;
    u32 deghost_os;
};

/**
 * struct amlvenc_h264_snr_params - Intra prediction and deblocking parameters
 * @err_norm:
 * @gau_bld_core:
 * @gau_bld_ofst:
 * @gau_bld_rate: SNR gaussian blur rate
 * @gau_alp0_min: Minimum SNR gaussian alpha0 value
 * @gau_alp0_max: Maximum SNR gaussian alpha0 value
 * @beta2alp_rate: Beta to alpha rate for deblocking
 * @beta_min: Minimum beta value for deblocking
 * @beta_max: Maximum beta value for deblocking
 */
struct amlvenc_h264_snr_params {
    u32 err_norm;
    u32 gau_bld_core;
    s32 gau_bld_ofst;
    u32 gau_bld_rate;
    u32 gau_alp0_min;
    u32 gau_alp0_max;
    u32 beta2alp_rate;
    u32 beta_min;
    u32 beta_max;
};

/**
 * struct amlvenc_h264_init_params - Parameters for initializing H.264 encoder
 * @idr: IDR frame flag
 * @quant: Quantization parameter
 * @qp_mode: Quantization parameter mode
 * @encoder_width: Width of the encoded frame
 * @encoder_height: Height of the encoded frame
 * @i4_weight: I4 mode weight
 * @i16_weight: I16 mode weight
 * @me_weight: Motion estimation weight
 * @cbr_ddr_start_addr: Start address for CBR DDR
 * @cbr_start_tbl_id: Start table ID for CBR
 * @cbr_short_shift: Short shift for CBR
 * @cbr_long_mb_num: Number of long macroblocks for CBR
 * @cbr_long_th: Long threshold for CBR
 * @cbr_block_w: CBR block width
 * @cbr_block_h: CBR block height
 * @dump_ddr_start_addr: Start address for dump DDR
 * @qtable_params: Quantization tables
 * @me_params: Motion estimation parameters
 * @intra_deblock_params: Intra prediction and deblocking parameters
 */
struct amlvenc_h264_init_params {
    bool idr;
    u32 quant;
    u32 qp_mode;
    u32 encoder_width;
    u32 encoder_height;
    u32 i4_weight;
    u32 i16_weight;
    u32 me_weight;
    u32 cbr_ddr_start_addr;
    u32 cbr_start_tbl_id;
    u32 cbr_short_shift;
    u32 cbr_long_mb_num;
    u32 cbr_long_th;
    u32 cbr_block_w;
    u32 cbr_block_h;
    u32 dump_ddr_start_addr;
    struct amlvenc_h264_qtable_params *qtable;
    struct amlvenc_h264_me_params *me;
};

// Struct for amlvenc_h264_configure_mdfin parameters
struct amlvenc_h264_mdfin_params {
    u32 input;
    u8 iformat;
    u8 oformat;
    u32 picsize_x;
    u32 picsize_y;
    u8 r2y_en;
    u8 nr_mode;
    u8 ifmt_extra;
    struct amlvenc_h264_snr_params *y_snr;
    struct amlvenc_h264_snr_params *c_snr;
    struct amlvenc_h264_tnr_params *y_tnr;
    struct amlvenc_h264_tnr_params *c_tnr;
};

extern const struct amlvenc_h264_me_params amlvenc_h264_me_defaults;
extern const struct amlvenc_h264_snr_params amlvenc_h264_snr_defaults;
extern const struct amlvenc_h264_tnr_params amlvenc_h264_tnr_defaults;

/**
 * amlvenc_h264_init_qtable - Initialize quantization tables for H.264 encoding
 * @p: Pointer to the quantization table parameters
 */
void amlvenc_h264_init_qtable(const struct amlvenc_h264_qtable_params *p);

/**
 * amlvenc_h264_init_me - Initialize motion estimation parameters for H.264 encoding
 */
/**
 * amlvenc_h264_init_me - Initialize motion estimation parameters for H.264 encoding
 * @p: Pointer to the motion estimation parameters
 */
void amlvenc_h264_init_me(struct amlvenc_h264_me_params *p);

/**
 * amlvenc_h264_init_output_stream_buffer - Initialize output stream buffer for H.264 encoding
 * @p: Pointer to the output buffer parameters
 */
void amlvenc_h264_init_output_stream_buffer(u32 bitstreamStart, u32 bitstreamEnd);

/**
 * amlvenc_h264_init_input_dct_buffer - Initialize input DCT buffer for H.264 encoding
 * @p: Pointer to the input DCT buffer parameters
 */
void amlvenc_h264_init_input_dct_buffer(u32 dct_buff_start_addr, u32 dct_buff_end_addr);

/**
 * amlvenc_h264_init_input_reference_buffer - Initialize input reference buffer
 * @canvas: Canvas number for the reference buffer
 */
void amlvenc_h264_init_input_reference_buffer(int canvas);

/**
 * amlvenc_h264_init_firmware_assist_buffer - Initialize firmware assist buffer
 * @assist_buffer_offset: Offset for the assist buffer
 */
void amlvenc_h264_init_firmware_assist_buffer(u32 assist_buffer_offset);

/**
 * amlvenc_h264_init_dblk_buffer - Initialize deblocking buffer
 * @canvas: Canvas number for the deblocking buffer
 */
void amlvenc_h264_init_dblk_buffer(int canvas);

/**
 * amlvenc_h264_init_encoder - Initialize H.264 encoder parameters
 * @p: Pointer to the encoder initialization parameters
 */
void amlvenc_h264_init_encoder(const struct amlvenc_h264_init_encoder_params *p);

/**
 * amlvenc_h264_configure_ie_me - Configure IE and ME parameters for H.264 encoding
 * @ie_me_mb_type:
 */
void amlvenc_h264_configure_ie_me(enum amlvenc_henc_mb_type ie_me_mb_type);

/**
 * amlvenc_h264_configure_slice - Configure fixed slice parameters for H.264 encoding
 * @p: Pointer to the fixed slice configuration parameters
 */
void amlvenc_h264_configure_slice(u32 fixed_slice_cfg, u32 rows_per_slice, u32 encoder_height);

/**
 * amlvenc_h264_configure_mdfin - Configure MDFIN parameters for H.264 encoding
 * @p: Pointer to the MDFIN configuration parameters
 */
void amlvenc_h264_configure_mdfin(struct amlvenc_h264_mdfin_params *p);

/**
 * amlvenc_h264_init - Initialize H.264 encoder
 * @p: Pointer to the initialization parameters
 *
 * This function initializes the H.264 encoder with the provided parameters.
 * It sets up various encoder settings, including picture dimensions, quantization
 * parameters, and configures hardware registers for encoding.
 */
void amlvenc_h264_init(const struct amlvenc_h264_init_params *p);

/**
 * amlvenc_h264_init_me_parameters - Initialize motion estimation parameters
 * @p: Pointer to the motion estimation parameters
 *
 * This function writes the motion estimation parameters to the hardware registers.
 */
void amlvenc_h264_init_me_parameters(const struct amlvenc_h264_me_params *p);

void amlvenc_dos_sw_reset(u32 bits);

void amlvenc_hcodec_start(void);
void amlvenc_hcodec_stop(void);
void amlvenc_hcodec_assist_enable(void);
void amlvenc_hcodec_dma_load_firmware(dma_addr_t dma_handle, size_t size);
bool amlvenc_hcodec_dma_completed(void);
enum amlvenc_hcodec_status amlvenc_hcodec_status(void);
void amlvenc_hcodec_clear_status(void);
void amlvenc_hcodec_set_status(enum amlvenc_hcodec_status status);
void amlvenc_hcodec_canvas_config(u32 index, ulong addr, u32 width, u32 height, u32 wrap, u32 blkmode);


#endif /* __AVC_ENCODER_HW_OPS_H__ */
