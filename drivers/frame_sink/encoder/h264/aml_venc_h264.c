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
#include <linux/types.h>
// #include <linux/amlogic/media/registers/register.h>
#include "../../../frame_provider/decoder/utils/vdec.h"

#include "aml_venc_h264.h"


#define HCODEC_IRQ_MBOX_CLR HCODEC_ASSIST_MBOX2_CLR_REG
#define HCODEC_IRQ_MBOX_MASK HCODEC_ASSIST_MBOX2_MASK


#define ADV_MV_LARGE_16x8 1
#define ADV_MV_LARGE_8x16 1
#define ADV_MV_LARGE_16x16 1

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


/* for temp */
#define HCODEC_MFDIN_REGC_MBLP		(HCODEC_MFDIN_REGB_AMPC + 0x1)
#define HCODEC_MFDIN_REG0D			(HCODEC_MFDIN_REGB_AMPC + 0x2)
#define HCODEC_MFDIN_REG0E			(HCODEC_MFDIN_REGB_AMPC + 0x3)
#define HCODEC_MFDIN_REG0F			(HCODEC_MFDIN_REGB_AMPC + 0x4)
#define HCODEC_MFDIN_REG10			(HCODEC_MFDIN_REGB_AMPC + 0x5)
#define HCODEC_MFDIN_REG11			(HCODEC_MFDIN_REGB_AMPC + 0x6)
#define HCODEC_MFDIN_REG12			(HCODEC_MFDIN_REGB_AMPC + 0x7)
#define HCODEC_MFDIN_REG13			(HCODEC_MFDIN_REGB_AMPC + 0x8)
#define HCODEC_MFDIN_REG14			(HCODEC_MFDIN_REGB_AMPC + 0x9)
#define HCODEC_MFDIN_REG15			(HCODEC_MFDIN_REGB_AMPC + 0xa)
#define HCODEC_MFDIN_REG16			(HCODEC_MFDIN_REGB_AMPC + 0xb)


/* V3 motion vector SAD */
static u32 v3_mv_sad[64] = {
	/* For step0 */
	0x00000004,
	0x00010008,
	0x00020010,
	0x00030018,
	0x00040020,
	0x00050028,
	0x00060038,
	0x00070048,
	0x00080058,
	0x00090068,
	0x000a0080,
	0x000b0098,
	0x000c00b0,
	0x000d00c8,
	0x000e00e8,
	0x000f0110,
	/* For step1 */
	0x00100002,
	0x00110004,
	0x00120008,
	0x0013000c,
	0x00140010,
	0x00150014,
	0x0016001c,
	0x00170024,
	0x0018002c,
	0x00190034,
	0x001a0044,
	0x001b0054,
	0x001c0064,
	0x001d0074,
	0x001e0094,
	0x001f00b4,
	/* For step2 */
	0x00200006,
	0x0021000c,
	0x0022000c,
	0x00230018,
	0x00240018,
	0x00250018,
	0x00260018,
	0x00270030,
	0x00280030,
	0x00290030,
	0x002a0030,
	0x002b0030,
	0x002c0030,
	0x002d0030,
	0x002e0030,
	0x002f0050,
	/* For step2 4x4-8x8 */
	0x00300001,
	0x00310002,
	0x00320002,
	0x00330004,
	0x00340004,
	0x00350004,
	0x00360004,
	0x00370006,
	0x00380006,
	0x00390006,
	0x003a0006,
	0x003b0006,
	0x003c0006,
	0x003d0006,
	0x003e0006,
	0x003f0006
};

const struct amlvenc_h264_me_params amlvenc_h264_me_defaults = {
	.me_mv_merge_ctl =
		(0x1 << 31)  |  /* [31] me_merge_mv_en_16 */
		(0x1 << 30)  |  /* [30] me_merge_small_mv_en_16 */
		(0x1 << 29)  |  /* [29] me_merge_flex_en_16 */
		(0x1 << 28)  |  /* [28] me_merge_sad_en_16 */
		(0x1 << 27)  |  /* [27] me_merge_mv_en_8 */
		(0x1 << 26)  |  /* [26] me_merge_small_mv_en_8 */
		(0x1 << 25)  |  /* [25] me_merge_flex_en_8 */
		(0x1 << 24)  |  /* [24] me_merge_sad_en_8 */
		/* [23:18] me_merge_mv_diff_16 - MV diff <= n pixel can be merged */
		(0x12 << 18) |
		/* [17:12] me_merge_mv_diff_8 - MV diff <= n pixel can be merged */
		(0x2b << 12) |
		/* [11:0] me_merge_min_sad - SAD >= 0x180 can be merged with other MV */
		(0x80 << 0),
		/* ( 0x4 << 18)  |
		* // [23:18] me_merge_mv_diff_16 - MV diff <= n pixel can be merged
		*/
		/* ( 0x3f << 12)  |
		* // [17:12] me_merge_mv_diff_8 - MV diff <= n pixel can be merged
		*/
		/* ( 0xc0 << 0),
		* // [11:0] me_merge_min_sad - SAD >= 0x180 can be merged with other MV
		*/

	.me_mv_weight_01 = (0x40 << 24) | (0x30 << 16) | (0x20 << 8) | 0x30,
	.me_mv_weight_23 = (0x40 << 8) | 0x30,
	.me_sad_range_inc = 0x03030303,
	.me_step0_close_mv = 0x003ffc21,
	.me_f_skip_sad = 0,
	.me_f_skip_weight = 0,
	.me_sad_enough_01 = 0,/* 0x00018010, */
	.me_sad_enough_23 = 0,/* 0x00000020, */
};

/* c & y tnr defaults */
const struct amlvenc_h264_tnr_params amlvenc_h264_tnr_defaults = {
	.mc_en = 1,
	.txt_mode = 0,
	.mot_sad_margin = 1,
	.mot_cortxt_rate = 1,
	.mot_distxt_ofst = 5,
	.mot_distxt_rate = 4,
	.mot_dismot_ofst = 4,
	.mot_frcsad_lock = 8,
	.mot2alp_frc_gain = 10,
	.mot2alp_nrm_gain = 216,
	.mot2alp_dis_gain = 128,
	.mot2alp_dis_ofst = 32,
	.alpha_min = 32,
	.alpha_max = 63,
	.deghost_os = 0,
};

/* c & y snr defaults */
const struct amlvenc_h264_snr_params amlvenc_h264_snr_defaults = {
	.err_norm = 1,
	.gau_bld_core = 1,
	.gau_bld_ofst = -1,
	.gau_bld_rate = 48,
	.gau_alp0_min = 0,
	.gau_alp0_max = 63,
	.beta2alp_rate = 16,
	.beta_min = 0,
	.beta_max = 63,
};

static void amlvenc_h264_write_quant_table(uint32_t table_addr, const uint32_t *quant_tbl)
{
    int i;

    WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
               (table_addr << 23) | /* quant_table_addr */
               (1 << 22)); /* quant_table_addr_update */

    for (i = 0; i < QTABLE_SIZE; i++) {
        WRITE_HREG(HCODEC_QUANT_TABLE_DATA, quant_tbl[i]);
    }
}

void amlvenc_h264_init_qtable(const struct amlvenc_h264_qtable_params *p)
{
    amlvenc_h264_write_quant_table(0, p->quant_tbl_i4);
    amlvenc_h264_write_quant_table(8, p->quant_tbl_i16);
    amlvenc_h264_write_quant_table(16, p->quant_tbl_me);
}

void amlvenc_h264_init_me(struct amlvenc_h264_me_params *p)
{
	p->me_mv_merge_ctl =
        (0x1 << 31) | /* [31] me_merge_mv_en_16 */
        (0x1 << 30) | /* [30] me_merge_small_mv_en_16 */
        (0x1 << 29) | /* [29] me_merge_flex_en_16 */
        (0x1 << 28) | /* [28] me_merge_sad_en_16 */
        (0x1 << 27) | /* [27] me_merge_mv_en_8 */
        (0x1 << 26) | /* [26] me_merge_small_mv_en_8 */
        (0x1 << 25) | /* [25] me_merge_flex_en_8 */
        (0x1 << 24) | /* [24] me_merge_sad_en_8 */
        (0x12 << 18) |
        /* [23:18] me_merge_mv_diff_16 - MV diff
         *	<= n pixel can be merged
         */
        (0x2b << 12) |
        /* [17:12] me_merge_mv_diff_8 - MV diff
         *	<= n pixel can be merged
         */
        (0x80 << 0);
    /* [11:0] me_merge_min_sad - SAD
     *	>= 0x180 can be merged with other MV
     */

	p->me_mv_weight_01 = (ME_MV_STEP_WEIGHT_1 << 24) |
                      (ME_MV_PRE_WEIGHT_1 << 16) |
                      (ME_MV_STEP_WEIGHT_0 << 8) |
                      (ME_MV_PRE_WEIGHT_0 << 0);

	p->me_mv_weight_23 = (ME_MV_STEP_WEIGHT_3 << 24) |
                      (ME_MV_PRE_WEIGHT_3 << 16) |
                      (ME_MV_STEP_WEIGHT_2 << 8) |
                      (ME_MV_PRE_WEIGHT_2 << 0);

	p->me_sad_range_inc = (ME_SAD_RANGE_3 << 24) |
                       (ME_SAD_RANGE_2 << 16) |
                       (ME_SAD_RANGE_1 << 8) |
                       (ME_SAD_RANGE_0 << 0);

	p->me_step0_close_mv = (0x100 << 10) |
                        /* me_step0_big_sad -- two MV sad
                         *  diff bigger will use use 1
                         */
                        (2 << 5) | /* me_step0_close_mv_y */
                        (2 << 0);  /* me_step0_close_mv_x */

	p->me_f_skip_sad = (0x00 << 24) |            /* force_skip_sad_3 */
                    (STEP_2_SKIP_SAD << 16) | /* force_skip_sad_2 */
                    (STEP_1_SKIP_SAD << 8) |  /* force_skip_sad_1 */
                    (STEP_0_SKIP_SAD << 0);   /* force_skip_sad_0 */

	p->me_f_skip_weight = (0x00 << 24) | /* force_skip_weight_3 */
                       /* force_skip_weight_2 */
                       (STEP_2_SKIP_WEIGHT << 16) |
                       /* force_skip_weight_1 */
                       (STEP_1_SKIP_WEIGHT << 8) |
                       /* force_skip_weight_0 */
                       (STEP_0_SKIP_WEIGHT << 0);

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB)
    {
		p->me_f_skip_sad = 0;
		p->me_f_skip_weight = 0;
		p->me_mv_weight_01 = 0;
		p->me_mv_weight_23 = 0;
    }

	p->me_sad_enough_01 = (ME_SAD_ENOUGH_1_DATA << 12) |
                       /* me_sad_enough_1 */
                       (ME_SAD_ENOUGH_0_DATA << 0) |
                       /* me_sad_enough_0 */
                       (0 << 12) | /* me_sad_enough_1 */
                       (0 << 0);   /* me_sad_enough_0 */

	p->me_sad_enough_23 = (ADV_MV_8x8_ENOUGH_DATA << 12) |
                       /* adv_mv_8x8_enough */
                       (ME_SAD_ENOUGH_2_DATA << 0) |
                       /* me_sad_enough_2 */
                       (0 << 12) | /* me_sad_enough_3 */
                       (0 << 0);   /* me_sad_enough_2 */
}

void amlvenc_h264_init_output_stream_buffer(u32 bitstreamStart, u32 bitstreamEnd)
{
    WRITE_HREG(HCODEC_VLC_VB_MEM_CTL,
               ((1 << 31) | (0x3f << 24) |
                (0x20 << 16) | (2 << 0)));
    WRITE_HREG(HCODEC_VLC_VB_START_PTR, bitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_WR_PTR, bitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_SW_RD_PTR, bitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_END_PTR, bitstreamEnd);
    WRITE_HREG(HCODEC_VLC_VB_CONTROL, 1);
    WRITE_HREG(HCODEC_VLC_VB_CONTROL,
               ((0 << 14) | (7 << 3) |
                (1 << 1) | (0 << 0)));
}

void amlvenc_h264_init_input_dct_buffer(u32 dct_buff_start_addr, u32 dct_buff_end_addr)
{
    WRITE_HREG(HCODEC_QDCT_MB_START_PTR, dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_END_PTR, dct_buff_end_addr);
    WRITE_HREG(HCODEC_QDCT_MB_WR_PTR, dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_RD_PTR, dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_BUFF, 0);
}

void amlvenc_h264_init_me_parameters(const struct amlvenc_h264_me_params *p)
{
    WRITE_HREG(HCODEC_ME_MV_MERGE_CTL, p->me_mv_merge_ctl);
    WRITE_HREG(HCODEC_ME_STEP0_CLOSE_MV, p->me_step0_close_mv);
    WRITE_HREG(HCODEC_ME_SAD_ENOUGH_01, p->me_sad_enough_01);
    WRITE_HREG(HCODEC_ME_SAD_ENOUGH_23, p->me_sad_enough_23);
    WRITE_HREG(HCODEC_ME_F_SKIP_SAD, p->me_f_skip_sad);
    WRITE_HREG(HCODEC_ME_F_SKIP_WEIGHT, p->me_f_skip_weight);
    WRITE_HREG(HCODEC_ME_MV_WEIGHT_01, p->me_mv_weight_01);
    WRITE_HREG(HCODEC_ME_MV_WEIGHT_23, p->me_mv_weight_23);
    WRITE_HREG(HCODEC_ME_SAD_RANGE_INC, p->me_sad_range_inc);
}

void amlvenc_h264_init_input_reference_buffer(s32 canvas)
{
    WRITE_HREG(HCODEC_ANC0_CANVAS_ADDR, canvas);
    WRITE_HREG(HCODEC_VLC_HCMD_CONFIG, 0);
}

void amlvenc_h264_init_firmware_assist_buffer(u32 assit_buffer_offset)
{
    WRITE_HREG(MEM_OFFSET_REG, assit_buffer_offset);
}

void amlvenc_h264_init_dblk_buffer(s32 canvas)
{
    WRITE_HREG(HCODEC_REC_CANVAS_ADDR, canvas);
    WRITE_HREG(HCODEC_DBKR_CANVAS_ADDR, canvas);
    WRITE_HREG(HCODEC_DBKW_CANVAS_ADDR, canvas);
}

void amlvenc_h264_init_encoder(const struct amlvenc_h264_init_encoder_params *p) {
	WRITE_HREG(HCODEC_VLC_TOTAL_BYTES, 0);
	WRITE_HREG(HCODEC_VLC_CONFIG, 0x07);
	WRITE_HREG(HCODEC_VLC_INT_CONTROL, 0);

	WRITE_HREG(HCODEC_ASSIST_AMR1_INT0, 0x15);
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT1, 0x8);
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT3, 0x14);

	WRITE_HREG(IDR_PIC_ID, p->idr_pic_id);
	WRITE_HREG(FRAME_NUMBER, p->idr ? 0 : p->frame_number);
	WRITE_HREG(PIC_ORDER_CNT_LSB, p->idr ? 0 : p->pic_order_cnt_lsb);

	WRITE_HREG(LOG2_MAX_PIC_ORDER_CNT_LSB, p->log2_max_pic_order_cnt_lsb);
	WRITE_HREG(LOG2_MAX_FRAME_NUM, p->log2_max_frame_num);
	WRITE_HREG(ANC0_BUFFER_ID, 0);
	WRITE_HREG(QPPICTURE, p->init_qppicture);
}

void amlvenc_h264_configure_ie_me(enum amlvenc_henc_mb_type ie_me_mb_type) {
	u32 ie_cur_ref_sel = 0;
	u32 ie_pipeline_block = 12;
	/* currently disable half and sub pixel */
	u32 ie_me_mode =
		(ie_pipeline_block & IE_PIPELINE_BLOCK_MASK) <<
	      IE_PIPELINE_BLOCK_SHIFT;

	WRITE_HREG(IE_ME_MODE, ie_me_mode);
	WRITE_HREG(IE_REF_SEL, ie_cur_ref_sel);
	WRITE_HREG(IE_ME_MB_TYPE, ie_me_mb_type);
}

void amlvenc_h264_configure_slice(u32 fixed_slice_cfg, u32 rows_per_slice, u32 encoder_height)
{
#ifdef MULTI_SLICE_MC
    if (fixed_slice_cfg)
        WRITE_HREG(FIXED_SLICE_CFG, fixed_slice_cfg);
    else if (rows_per_slice !=
             (encoder_height + 15) >> 4)
    {
        u32 mb_per_slice = (encoder_height + 15) >> 4;
        mb_per_slice = mb_per_slice * rows_per_slice;
        WRITE_HREG(FIXED_SLICE_CFG, mb_per_slice);
    }
    else
        WRITE_HREG(FIXED_SLICE_CFG, 0);
#else
	WRITE_HREG(FIXED_SLICE_CFG, 0);
#endif
}

void amlvenc_h264_configure_mdfin(struct amlvenc_h264_mdfin_params *p) {
	u8 dsample_en; /* Downsample Enable */
	u8 interp_en;  /* Interpolation Enable */
	u8 y_size;     /* 0:16 Pixels for y direction pickup; 1:8 pixels */
	u8 r2y_mode;   /* RGB2YUV Mode, range(0~3) */
	/* mfdin_reg3_canv[25:24];
	 *  // bytes per pixel in x direction for index0, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx0_bppx;
	/* mfdin_reg3_canv[27:26];
	 *  // bytes per pixel in x direction for index1-2, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx1_bppx;
	/* mfdin_reg3_canv[29:28];
	 *  // bytes per pixel in y direction for index0, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx0_bppy;
	/* mfdin_reg3_canv[31:30];
	 *  // bytes per pixel in y direction for index1-2, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx1_bppy;
	u8 ifmt444, ifmt422, ifmt420, linear_bytes4p;
	u8 nr_enable;
	u8 cfg_y_snr_en;
	u8 cfg_y_tnr_en;
	u8 cfg_c_snr_en;
	u8 cfg_c_tnr_en;
	u32 linear_bytesperline;
	s32 reg_offset;
	bool linear_enable = false;
	bool format_err = false;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL) {
		if ((p->iformat == 7) && (p->ifmt_extra > 2))
			format_err = true;
	} else if (p->iformat == 7)
		format_err = true;

	if (format_err) {
		pr_err(
            "mfdin format err, iformat:%d, ifmt_extra:%d\n",
			p->iformat, p->ifmt_extra);
		return;
	}
	if (p->iformat != 7)
		p->ifmt_extra = 0;

	ifmt444 = ((p->iformat == 1) || (p->iformat == 5) || (p->iformat == 8) ||
		(p->iformat == 9) || (p->iformat == 12)) ? 1 : 0;
	if (p->iformat == 7 && p->ifmt_extra == 1)
		ifmt444 = 1;
	ifmt422 = ((p->iformat == 0) || (p->iformat == 10)) ? 1 : 0;
	if (p->iformat == 7 && p->ifmt_extra != 1)
		ifmt422 = 1;
	ifmt420 = ((p->iformat == 2) || (p->iformat == 3) || (p->iformat == 4) ||
		(p->iformat == 11)) ? 1 : 0;
	dsample_en = ((ifmt444 && (p->oformat != 2)) ||
		(ifmt422 && (p->oformat == 0))) ? 1 : 0;
	interp_en = ((ifmt422 && (p->oformat == 2)) ||
		(ifmt420 && (p->oformat != 0))) ? 1 : 0;
	y_size = (p->oformat != 0) ? 1 : 0;
	if (p->iformat == 12)
		y_size = 0;
	r2y_mode = (p->r2y_en == 1) ? 1 : 0; /* Fixed to 1 (TODO) */
	canv_idx0_bppx = (p->iformat == 1) ? 3 : (p->iformat == 0) ? 2 : 1;
	canv_idx1_bppx = (p->iformat == 4) ? 0 : 1;
	canv_idx0_bppy = 1;
	canv_idx1_bppy = (p->iformat == 5) ? 1 : 0;

	if ((p->iformat == 8) || (p->iformat == 9) || (p->iformat == 12))
		linear_bytes4p = 3;
	else if (p->iformat == 10)
		linear_bytes4p = 2;
	else if (p->iformat == 11)
		linear_bytes4p = 1;
	else
		linear_bytes4p = 0;
	if (p->iformat == 12)
		linear_bytesperline = p->picsize_x * 4;
	else
		linear_bytesperline = p->picsize_x * linear_bytes4p;

	if (p->iformat < 8)
		linear_enable = false;
	else
		linear_enable = true;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXBB) {
		reg_offset = -8;
		/* nr_mode: 0:Disabled 1:SNR Only 2:TNR Only 3:3DNR */
		nr_enable = (p->nr_mode) ? 1 : 0;
		cfg_y_snr_en = ((p->nr_mode == 1) || (p->nr_mode == 3)) ? 1 : 0;
		cfg_y_tnr_en = ((p->nr_mode == 2) || (p->nr_mode == 3)) ? 1 : 0;
		cfg_c_snr_en = cfg_y_snr_en;
		/* cfg_c_tnr_en = cfg_y_tnr_en; */
		cfg_c_tnr_en = 0;

		/* NR For Y */
		WRITE_HREG((HCODEC_MFDIN_REG0D + reg_offset),
			((cfg_y_snr_en << 0) |
			(p->y_snr->err_norm << 1) |
			(p->y_snr->gau_bld_core << 2) |
			(((p->y_snr->gau_bld_ofst) & 0xff) << 6) |
			(p->y_snr->gau_bld_rate << 14) |
			(p->y_snr->gau_alp0_min << 20) |
			(p->y_snr->gau_alp0_max << 26)));
		WRITE_HREG((HCODEC_MFDIN_REG0E + reg_offset),
			((cfg_y_tnr_en << 0) |
			(p->y_tnr->mc_en << 1) |
			(p->y_tnr->txt_mode << 2) |
			(p->y_tnr->mot_sad_margin << 3) |
			(p->y_tnr->alpha_min << 7) |
			(p->y_tnr->alpha_max << 13) |
			(p->y_tnr->deghost_os << 19)));
		WRITE_HREG((HCODEC_MFDIN_REG0F + reg_offset),
			((p->y_tnr->mot_cortxt_rate << 0) |
			(p->y_tnr->mot_distxt_ofst << 8) |
			(p->y_tnr->mot_distxt_rate << 4) |
			(p->y_tnr->mot_dismot_ofst << 16) |
			(p->y_tnr->mot_frcsad_lock << 24)));
		WRITE_HREG((HCODEC_MFDIN_REG10 + reg_offset),
			((p->y_tnr->mot2alp_frc_gain << 0) |
			(p->y_tnr->mot2alp_nrm_gain << 8) |
			(p->y_tnr->mot2alp_dis_gain << 16) |
			(p->y_tnr->mot2alp_dis_ofst << 24)));
		WRITE_HREG((HCODEC_MFDIN_REG11 + reg_offset),
			((p->y_snr->beta2alp_rate << 0) |
			(p->y_snr->beta_min << 8) |
			(p->y_snr->beta_max << 14)));

		/* NR For C */
		WRITE_HREG((HCODEC_MFDIN_REG12 + reg_offset),
			((cfg_y_snr_en << 0) |
			(p->c_snr->err_norm << 1) |
			(p->c_snr->gau_bld_core << 2) |
			(((p->c_snr->gau_bld_ofst) & 0xff) << 6) |
			(p->c_snr->gau_bld_rate << 14) |
			(p->c_snr->gau_alp0_min << 20) |
			(p->c_snr->gau_alp0_max << 26)));

		WRITE_HREG((HCODEC_MFDIN_REG13 + reg_offset),
			((cfg_c_tnr_en << 0) |
			(p->c_tnr->mc_en << 1) |
			(p->c_tnr->txt_mode << 2) |
			(p->c_tnr->mot_sad_margin << 3) |
			(p->c_tnr->alpha_min << 7) |
			(p->c_tnr->alpha_max << 13) |
			(p->c_tnr->deghost_os << 19)));
		WRITE_HREG((HCODEC_MFDIN_REG14 + reg_offset),
			((p->c_tnr->mot_cortxt_rate << 0) |
			(p->c_tnr->mot_distxt_ofst << 8) |
			(p->c_tnr->mot_distxt_rate << 4) |
			(p->c_tnr->mot_dismot_ofst << 16) |
			(p->c_tnr->mot_frcsad_lock << 24)));
		WRITE_HREG((HCODEC_MFDIN_REG15 + reg_offset),
			((p->c_tnr->mot2alp_frc_gain << 0) |
			(p->c_tnr->mot2alp_nrm_gain << 8) |
			(p->c_tnr->mot2alp_dis_gain << 16) |
			(p->c_tnr->mot2alp_dis_ofst << 24)));

		WRITE_HREG((HCODEC_MFDIN_REG16 + reg_offset),
			((p->c_snr->beta2alp_rate << 0) |
			(p->c_snr->beta_min << 8) |
			(p->c_snr->beta_max << 14)));

		WRITE_HREG((HCODEC_MFDIN_REG1_CTRL + reg_offset),
			(p->iformat << 0) | (p->oformat << 4) |
			(dsample_en << 6) | (y_size << 8) |
			(interp_en << 9) | (p->r2y_en << 12) |
			(r2y_mode << 13) | (p->ifmt_extra << 16) |
			(nr_enable << 19));
		if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
			WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
				(p->picsize_x << 16) | (p->picsize_y << 0));
		} else {
			WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
				(p->picsize_x << 14) | (p->picsize_y << 0));
		}
	} else {
		reg_offset = 0;
		WRITE_HREG((HCODEC_MFDIN_REG1_CTRL + reg_offset),
			(p->iformat << 0) | (p->oformat << 4) |
			(dsample_en << 6) | (y_size << 8) |
			(interp_en << 9) | (p->r2y_en << 12) |
			(r2y_mode << 13));

		WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
			(p->picsize_x << 12) | (p->picsize_y << 0));
	}

	if (linear_enable == false) {
		WRITE_HREG((HCODEC_MFDIN_REG3_CANV + reg_offset),
			(p->input & 0xffffff) |
			(canv_idx1_bppy << 30) |
			(canv_idx0_bppy << 28) |
			(canv_idx1_bppx << 26) |
			(canv_idx0_bppx << 24));
		WRITE_HREG((HCODEC_MFDIN_REG4_LNR0 + reg_offset),
			(0 << 16) | (0 << 0));
		WRITE_HREG((HCODEC_MFDIN_REG5_LNR1 + reg_offset), 0);
	} else {
		WRITE_HREG((HCODEC_MFDIN_REG3_CANV + reg_offset),
			(canv_idx1_bppy << 30) |
			(canv_idx0_bppy << 28) |
			(canv_idx1_bppx << 26) |
			(canv_idx0_bppx << 24));
		WRITE_HREG((HCODEC_MFDIN_REG4_LNR0 + reg_offset),
			(linear_bytes4p << 16) | (linear_bytesperline << 0));
		WRITE_HREG((HCODEC_MFDIN_REG5_LNR1 + reg_offset), p->input);
	}

	if (p->iformat == 12)
		WRITE_HREG((HCODEC_MFDIN_REG9_ENDN + reg_offset),
			(2 << 0) | (1 << 3) | (0 << 6) |
			(3 << 9) | (6 << 12) | (5 << 15) |
			(4 << 18) | (7 << 21));
	else
		WRITE_HREG((HCODEC_MFDIN_REG9_ENDN + reg_offset),
			(7 << 0) | (6 << 3) | (5 << 6) |
			(4 << 9) | (3 << 12) | (2 << 15) |
			(1 << 18) | (0 << 21));
}

void amlvenc_h264_init(const struct amlvenc_h264_init_params *p) {
    u32 data32;
    u32 pic_width, pic_height;
    u32 pic_mb_nr;
    u32 pic_mbx, pic_mby;
    u32 i_pic_qp, p_pic_qp;
    u32 i_pic_qp_c, p_pic_qp_c;
    u32 pic_width_in_mb;
    u32 slice_qp;

    pic_width  = p->encoder_width;
    pic_height = p->encoder_height;
    pic_mb_nr  = 0;
    pic_mbx    = 0;
    pic_mby    = 0;
    i_pic_qp   = p->quant;
    p_pic_qp   = p->quant;

    pic_width_in_mb = (pic_width + 15) / 16;
    WRITE_HREG(HCODEC_HDEC_MC_OMEM_AUTO,
        (1 << 31) | /* use_omem_mb_xy */
        ((pic_width_in_mb - 1) << 16)); /* omem_max_mb_x */

    WRITE_HREG(HCODEC_VLC_ADV_CONFIG,
        /* early_mix_mc_hcmd -- will enable in P Picture */
        (0 << 10) |
        (1 << 9) | /* update_top_left_mix */
        (1 << 8) | /* p_top_left_mix */
        /* mv_cal_mixed_type -- will enable in P Picture */
        (0 << 7) |
        /* mc_hcmd_mixed_type -- will enable in P Picture */
        (0 << 6) |
        (1 << 5) | /* use_separate_int_control */
        (1 << 4) | /* hcmd_intra_use_q_info */
        (1 << 3) | /* hcmd_left_use_prev_info */
        (1 << 2) | /* hcmd_use_q_info */
        (1 << 1) | /* use_q_delta_quant */
        /* detect_I16_from_I4 use qdct detected mb_type */
        (0 << 0));

	WRITE_HREG(HCODEC_QDCT_ADV_CONFIG,
		(1 << 29) | /* mb_info_latch_no_I16_pred_mode */
		(1 << 28) | /* ie_dma_mbxy_use_i_pred */
		(1 << 27) | /* ie_dma_read_write_use_ip_idx */
		(1 << 26) | /* ie_start_use_top_dma_count */
		(1 << 25) | /* i_pred_top_dma_rd_mbbot */
		(1 << 24) | /* i_pred_top_dma_wr_disable */
		/* i_pred_mix -- will enable in P Picture */
		(0 << 23) |
		(1 << 22) | /* me_ab_rd_when_intra_in_p */
		(1 << 21) | /* force_mb_skip_run_when_intra */
		/* mc_out_mixed_type -- will enable in P Picture */
		(0 << 20) |
		(1 << 19) | /* ie_start_when_quant_not_full */
		(1 << 18) | /* mb_info_state_mix */
		/* mb_type_use_mix_result -- will enable in P Picture */
		(0 << 17) |
		/* me_cb_ie_read_enable -- will enable in P Picture */
		(0 << 16) |
		/* ie_cur_data_from_me -- will enable in P Picture */
		(0 << 15) |
		(1 << 14) | /* rem_per_use_table */
		(0 << 13) | /* q_latch_int_enable */
		(1 << 12) | /* q_use_table */
		(0 << 11) | /* q_start_wait */
		(1 << 10) | /* LUMA_16_LEFT_use_cur */
		(1 << 9) | /* DC_16_LEFT_SUM_use_cur */
		(1 << 8) | /* c_ref_ie_sel_cur */
		(0 << 7) | /* c_ipred_perfect_mode */
		(1 << 6) | /* ref_ie_ul_sel */
		(1 << 5) | /* mb_type_use_ie_result */
		(1 << 4) | /* detect_I16_from_I4 */
		(1 << 3) | /* ie_not_wait_ref_busy */
		(1 << 2) | /* ie_I16_enable */
		(3 << 0)); /* ie_done_sel  // fastest when waiting */

	if (p->i4_weight && p->i16_weight && p->me_weight) {
		WRITE_HREG(HCODEC_IE_WEIGHT,
			(p->i16_weight << 16) |
			(p->i4_weight << 0));
		WRITE_HREG(HCODEC_ME_WEIGHT,
			(p->me_weight << 0));
		WRITE_HREG(HCODEC_SAD_CONTROL_0,
			/* ie_sad_offset_I16 */
			(p->i16_weight << 16) |
			/* ie_sad_offset_I4 */
			(p->i4_weight << 0));
		WRITE_HREG(HCODEC_SAD_CONTROL_1,
			/* ie_sad_shift_I16 */
			(IE_SAD_SHIFT_I16 << 24) |
			/* ie_sad_shift_I4 */
			(IE_SAD_SHIFT_I4 << 20) |
			/* me_sad_shift_INTER */
			(ME_SAD_SHIFT_INTER << 16) |
			/* me_sad_offset_INTER */
			(p->me_weight << 0));
	} else {
		WRITE_HREG(HCODEC_IE_WEIGHT,
			(I16MB_WEIGHT_OFFSET << 16) |
			(I4MB_WEIGHT_OFFSET << 0));
		WRITE_HREG(HCODEC_ME_WEIGHT,
			(ME_WEIGHT_OFFSET << 0));
		WRITE_HREG(HCODEC_SAD_CONTROL_0,
			/* ie_sad_offset_I16 */
			(I16MB_WEIGHT_OFFSET << 16) |
			/* ie_sad_offset_I4 */
			(I4MB_WEIGHT_OFFSET << 0));
		WRITE_HREG(HCODEC_SAD_CONTROL_1,
			/* ie_sad_shift_I16 */
			(IE_SAD_SHIFT_I16 << 24) |
			/* ie_sad_shift_I4 */
			(IE_SAD_SHIFT_I4 << 20) |
			/* me_sad_shift_INTER */
			(ME_SAD_SHIFT_INTER << 16) |
			/* me_sad_offset_INTER */
			(ME_WEIGHT_OFFSET << 0));
	}

	WRITE_HREG(HCODEC_ADV_MV_CTL0,
		(ADV_MV_LARGE_16x8 << 31) |
		(ADV_MV_LARGE_8x16 << 30) |
		(ADV_MV_8x8_WEIGHT << 16) |   /* adv_mv_8x8_weight */
		/* adv_mv_4x4x4_weight should be set bigger */
		(ADV_MV_4x4x4_WEIGHT << 0));
	WRITE_HREG(HCODEC_ADV_MV_CTL1,
		/* adv_mv_16x16_weight */
		(ADV_MV_16x16_WEIGHT << 16) |
		(ADV_MV_LARGE_16x16 << 15) |
		(ADV_MV_16_8_WEIGHT << 0));  /* adv_mv_16_8_weight */

	amlvenc_h264_init_qtable(p->qtable);

	if (p->idr) {
		i_pic_qp =
			p->qtable->quant_tbl_i4[0] & 0xff;
		i_pic_qp +=
			p->qtable->quant_tbl_i16[0] & 0xff;
		i_pic_qp /= 2;
		p_pic_qp = i_pic_qp;
	} else {
		i_pic_qp =
			p->qtable->quant_tbl_i4[0] & 0xff;
		i_pic_qp +=
			p->qtable->quant_tbl_i16[0] & 0xff;
		p_pic_qp = p->qtable->quant_tbl_me[0] & 0xff;
		slice_qp = (i_pic_qp + p_pic_qp) / 3;
		i_pic_qp = slice_qp;
		p_pic_qp = i_pic_qp;
	}
#ifdef H264_ENC_CBR
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB) {
		data32 = READ_HREG(HCODEC_SAD_CONTROL_1);
		data32 = data32 & 0xffff; /* remove sad shift */
		WRITE_HREG(HCODEC_SAD_CONTROL_1, data32);
		WRITE_HREG(H264_ENC_CBR_TABLE_ADDR, p->cbr_ddr_start_addr);
		WRITE_HREG(H264_ENC_CBR_MB_SIZE_ADDR, p->cbr_ddr_start_addr + CBR_TABLE_SIZE);
		WRITE_HREG(H264_ENC_CBR_CTL,
			(p->cbr_start_tbl_id << 28) |
			(p->cbr_short_shift << 24) |
			(p->cbr_long_mb_num << 16) |
			(p->cbr_long_th << 0));
		WRITE_HREG(H264_ENC_CBR_REGION_SIZE,
			(p->cbr_block_w << 16) |
			(p->cbr_block_h << 0));
	}
#endif
	WRITE_HREG(HCODEC_QDCT_VLC_QUANT_CTL_0,
		(0 << 19) | /* vlc_delta_quant_1 */
		(i_pic_qp << 13) | /* vlc_quant_1 */
		(0 << 6) | /* vlc_delta_quant_0 */
		(i_pic_qp << 0)); /* vlc_quant_0 */
	WRITE_HREG(HCODEC_QDCT_VLC_QUANT_CTL_1,
		(14 << 6) | /* vlc_max_delta_q_neg */
		(13 << 0)); /* vlc_max_delta_q_pos */
	WRITE_HREG(HCODEC_VLC_PIC_SIZE,
		pic_width | (pic_height << 16));
	WRITE_HREG(HCODEC_VLC_PIC_POSITION,
		(pic_mb_nr << 16) |
		(pic_mby << 8) |
		(pic_mbx << 0));

	/* synopsys parallel_case full_case */
	switch (i_pic_qp) {
	case 0:
		i_pic_qp_c = 0;
		break;
	case 1:
		i_pic_qp_c = 1;
		break;
	case 2:
		i_pic_qp_c = 2;
		break;
	case 3:
		i_pic_qp_c = 3;
		break;
	case 4:
		i_pic_qp_c = 4;
		break;
	case 5:
		i_pic_qp_c = 5;
		break;
	case 6:
		i_pic_qp_c = 6;
		break;
	case 7:
		i_pic_qp_c = 7;
		break;
	case 8:
		i_pic_qp_c = 8;
		break;
	case 9:
		i_pic_qp_c = 9;
		break;
	case 10:
		i_pic_qp_c = 10;
		break;
	case 11:
		i_pic_qp_c = 11;
		break;
	case 12:
		i_pic_qp_c = 12;
		break;
	case 13:
		i_pic_qp_c = 13;
		break;
	case 14:
		i_pic_qp_c = 14;
		break;
	case 15:
		i_pic_qp_c = 15;
		break;
	case 16:
		i_pic_qp_c = 16;
		break;
	case 17:
		i_pic_qp_c = 17;
		break;
	case 18:
		i_pic_qp_c = 18;
		break;
	case 19:
		i_pic_qp_c = 19;
		break;
	case 20:
		i_pic_qp_c = 20;
		break;
	case 21:
		i_pic_qp_c = 21;
		break;
	case 22:
		i_pic_qp_c = 22;
		break;
	case 23:
		i_pic_qp_c = 23;
		break;
	case 24:
		i_pic_qp_c = 24;
		break;
	case 25:
		i_pic_qp_c = 25;
		break;
	case 26:
		i_pic_qp_c = 26;
		break;
	case 27:
		i_pic_qp_c = 27;
		break;
	case 28:
		i_pic_qp_c = 28;
		break;
	case 29:
		i_pic_qp_c = 29;
		break;
	case 30:
		i_pic_qp_c = 29;
		break;
	case 31:
		i_pic_qp_c = 30;
		break;
	case 32:
		i_pic_qp_c = 31;
		break;
	case 33:
		i_pic_qp_c = 32;
		break;
	case 34:
		i_pic_qp_c = 32;
		break;
	case 35:
		i_pic_qp_c = 33;
		break;
	case 36:
		i_pic_qp_c = 34;
		break;
	case 37:
		i_pic_qp_c = 34;
		break;
	case 38:
		i_pic_qp_c = 35;
		break;
	case 39:
		i_pic_qp_c = 35;
		break;
	case 40:
		i_pic_qp_c = 36;
		break;
	case 41:
		i_pic_qp_c = 36;
		break;
	case 42:
		i_pic_qp_c = 37;
		break;
	case 43:
		i_pic_qp_c = 37;
		break;
	case 44:
		i_pic_qp_c = 37;
		break;
	case 45:
		i_pic_qp_c = 38;
		break;
	case 46:
		i_pic_qp_c = 38;
		break;
	case 47:
		i_pic_qp_c = 38;
		break;
	case 48:
		i_pic_qp_c = 39;
		break;
	case 49:
		i_pic_qp_c = 39;
		break;
	case 50:
		i_pic_qp_c = 39;
		break;
	default:
		i_pic_qp_c = 39;
		break;
	}

	/* synopsys parallel_case full_case */
	switch (p_pic_qp) {
	case 0:
		p_pic_qp_c = 0;
		break;
	case 1:
		p_pic_qp_c = 1;
		break;
	case 2:
		p_pic_qp_c = 2;
		break;
	case 3:
		p_pic_qp_c = 3;
		break;
	case 4:
		p_pic_qp_c = 4;
		break;
	case 5:
		p_pic_qp_c = 5;
		break;
	case 6:
		p_pic_qp_c = 6;
		break;
	case 7:
		p_pic_qp_c = 7;
		break;
	case 8:
		p_pic_qp_c = 8;
		break;
	case 9:
		p_pic_qp_c = 9;
		break;
	case 10:
		p_pic_qp_c = 10;
		break;
	case 11:
		p_pic_qp_c = 11;
		break;
	case 12:
		p_pic_qp_c = 12;
		break;
	case 13:
		p_pic_qp_c = 13;
		break;
	case 14:
		p_pic_qp_c = 14;
		break;
	case 15:
		p_pic_qp_c = 15;
		break;
	case 16:
		p_pic_qp_c = 16;
		break;
	case 17:
		p_pic_qp_c = 17;
		break;
	case 18:
		p_pic_qp_c = 18;
		break;
	case 19:
		p_pic_qp_c = 19;
		break;
	case 20:
		p_pic_qp_c = 20;
		break;
	case 21:
		p_pic_qp_c = 21;
		break;
	case 22:
		p_pic_qp_c = 22;
		break;
	case 23:
		p_pic_qp_c = 23;
		break;
	case 24:
		p_pic_qp_c = 24;
		break;
	case 25:
		p_pic_qp_c = 25;
		break;
	case 26:
		p_pic_qp_c = 26;
		break;
	case 27:
		p_pic_qp_c = 27;
		break;
	case 28:
		p_pic_qp_c = 28;
		break;
	case 29:
		p_pic_qp_c = 29;
		break;
	case 30:
		p_pic_qp_c = 29;
		break;
	case 31:
		p_pic_qp_c = 30;
		break;
	case 32:
		p_pic_qp_c = 31;
		break;
	case 33:
		p_pic_qp_c = 32;
		break;
	case 34:
		p_pic_qp_c = 32;
		break;
	case 35:
		p_pic_qp_c = 33;
		break;
	case 36:
		p_pic_qp_c = 34;
		break;
	case 37:
		p_pic_qp_c = 34;
		break;
	case 38:
		p_pic_qp_c = 35;
		break;
	case 39:
		p_pic_qp_c = 35;
		break;
	case 40:
		p_pic_qp_c = 36;
		break;
	case 41:
		p_pic_qp_c = 36;
		break;
	case 42:
		p_pic_qp_c = 37;
		break;
	case 43:
		p_pic_qp_c = 37;
		break;
	case 44:
		p_pic_qp_c = 37;
		break;
	case 45:
		p_pic_qp_c = 38;
		break;
	case 46:
		p_pic_qp_c = 38;
		break;
	case 47:
		p_pic_qp_c = 38;
		break;
	case 48:
		p_pic_qp_c = 39;
		break;
	case 49:
		p_pic_qp_c = 39;
		break;
	case 50:
		p_pic_qp_c = 39;
		break;
	default:
		p_pic_qp_c = 39;
		break;
	}
	WRITE_HREG(HCODEC_QDCT_Q_QUANT_I,
		(i_pic_qp_c << 22) |
		(i_pic_qp << 16) |
		((i_pic_qp_c % 6) << 12) |
		((i_pic_qp_c / 6) << 8) |
		((i_pic_qp % 6) << 4) |
		((i_pic_qp / 6) << 0));

	WRITE_HREG(HCODEC_QDCT_Q_QUANT_P,
		(p_pic_qp_c << 22) |
		(p_pic_qp << 16) |
		((p_pic_qp_c % 6) << 12) |
		((p_pic_qp_c / 6) << 8) |
		((p_pic_qp % 6) << 4) |
		((p_pic_qp / 6) << 0));

#ifdef ENABLE_IGNORE_FUNCTION
	WRITE_HREG(HCODEC_IGNORE_CONFIG,
		(1 << 31) | /* ignore_lac_coeff_en */
		(1 << 26) | /* ignore_lac_coeff_else (<1) */
		(1 << 21) | /* ignore_lac_coeff_2 (<1) */
		(2 << 16) | /* ignore_lac_coeff_1 (<2) */
		(1 << 15) | /* ignore_cac_coeff_en */
		(1 << 10) | /* ignore_cac_coeff_else (<1) */
		(1 << 5)  | /* ignore_cac_coeff_2 (<1) */
		(3 << 0));  /* ignore_cac_coeff_1 (<2) */

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB)
		WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
			(1 << 31) | /* ignore_t_lac_coeff_en */
			(1 << 26) | /* ignore_t_lac_coeff_else (<1) */
			(2 << 21) | /* ignore_t_lac_coeff_2 (<2) */
			(6 << 16) | /* ignore_t_lac_coeff_1 (<6) */
			(1<<15) | /* ignore_cdc_coeff_en */
			(0<<14) | /* ignore_t_lac_coeff_else_le_3 */
			(1<<13) | /* ignore_t_lac_coeff_else_le_4 */
			(1<<12) | /* ignore_cdc_only_when_empty_cac_inter */
			(1<<11) | /* ignore_cdc_only_when_one_empty_inter */
			/* ignore_cdc_range_max_inter 0-0, 1-1, 2-2, 3-3 */
			(2<<9) |
			/* ignore_cdc_abs_max_inter 0-1, 1-2, 2-3, 3-4 */
			(0<<7) |
			/* ignore_cdc_only_when_empty_cac_intra */
			(1<<5) |
			/* ignore_cdc_only_when_one_empty_intra */
			(1<<4) |
			/* ignore_cdc_range_max_intra 0-0, 1-1, 2-2, 3-3 */
			(1<<2) |
			/* ignore_cdc_abs_max_intra 0-1, 1-2, 2-3, 3-4 */
			(0<<0));
	else
		WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
			(1 << 31) | /* ignore_t_lac_coeff_en */
			(1 << 26) | /* ignore_t_lac_coeff_else (<1) */
			(1 << 21) | /* ignore_t_lac_coeff_2 (<1) */
			(5 << 16) | /* ignore_t_lac_coeff_1 (<5) */
			(0 << 0));
#else
	WRITE_HREG(HCODEC_IGNORE_CONFIG, 0);
	WRITE_HREG(HCODEC_IGNORE_CONFIG_2, 0);
#endif

	WRITE_HREG(HCODEC_QDCT_MB_CONTROL,
		(1 << 9) | /* mb_info_soft_reset */
		(1 << 0)); /* mb read buffer soft reset */

	WRITE_HREG(HCODEC_QDCT_MB_CONTROL,
		(1 << 28) | /* ignore_t_p8x8 */
		(0 << 27) | /* zero_mc_out_null_non_skipped_mb */
		(0 << 26) | /* no_mc_out_null_non_skipped_mb */
		(0 << 25) | /* mc_out_even_skipped_mb */
		(0 << 24) | /* mc_out_wait_cbp_ready */
		(0 << 23) | /* mc_out_wait_mb_type_ready */
		(1 << 29) | /* ie_start_int_enable */
		(1 << 19) | /* i_pred_enable */
		(1 << 20) | /* ie_sub_enable */
		(1 << 18) | /* iq_enable */
		(1 << 17) | /* idct_enable */
		(1 << 14) | /* mb_pause_enable */
		(1 << 13) | /* q_enable */
		(1 << 12) | /* dct_enable */
		(1 << 10) | /* mb_info_en */
		(0 << 3) | /* endian */
		(0 << 1) | /* mb_read_en */
		(0 << 0)); /* soft reset */

	WRITE_HREG(HCODEC_SAD_CONTROL,
		(0 << 3) | /* ie_result_buff_enable */
		(1 << 2) | /* ie_result_buff_soft_reset */
		(0 << 1) | /* sad_enable */
		(1 << 0)); /* sad soft reset */
	WRITE_HREG(HCODEC_IE_RESULT_BUFFER, 0);

	WRITE_HREG(HCODEC_SAD_CONTROL,
		(1 << 3) | /* ie_result_buff_enable */
		(0 << 2) | /* ie_result_buff_soft_reset */
		(1 << 1) | /* sad_enable */
		(0 << 0)); /* sad soft reset */

	WRITE_HREG(HCODEC_IE_CONTROL,
		(1 << 30) | /* active_ul_block */
		(0 << 1) | /* ie_enable */
		(1 << 0)); /* ie soft reset */

	WRITE_HREG(HCODEC_IE_CONTROL,
		(1 << 30) | /* active_ul_block */
		(0 << 1) | /* ie_enable */
		(0 << 0)); /* ie soft reset */

	WRITE_HREG(HCODEC_ME_SKIP_LINE,
		(8 << 24) | /* step_3_skip_line */
		(8 << 18) | /* step_2_skip_line */
		(2 << 12) | /* step_1_skip_line */
		(0 << 6) | /* step_0_skip_line */
		(0 << 0));

	WRITE_HREG(HCODEC_ME_MV_MERGE_CTL, p->me->me_mv_merge_ctl);
	WRITE_HREG(HCODEC_ME_STEP0_CLOSE_MV, p->me->me_step0_close_mv);
	WRITE_HREG(HCODEC_ME_SAD_ENOUGH_01, p->me->me_sad_enough_01);
	WRITE_HREG(HCODEC_ME_SAD_ENOUGH_23, p->me->me_sad_enough_23);
	WRITE_HREG(HCODEC_ME_F_SKIP_SAD, p->me->me_f_skip_sad);
	WRITE_HREG(HCODEC_ME_F_SKIP_WEIGHT, p->me->me_f_skip_weight);
	WRITE_HREG(HCODEC_ME_MV_WEIGHT_01, p->me->me_mv_weight_01);
	WRITE_HREG(HCODEC_ME_MV_WEIGHT_23, p->me->me_mv_weight_23);
	WRITE_HREG(HCODEC_ME_SAD_RANGE_INC, p->me->me_sad_range_inc);

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL) {
		WRITE_HREG(HCODEC_V5_SIMPLE_MB_CTL, 0);
		WRITE_HREG(HCODEC_V5_SIMPLE_MB_CTL,
			(v5_use_small_diff_cnt << 7) |
			(v5_simple_mb_inter_all_en << 6) |
			(v5_simple_mb_inter_8x8_en << 5) |
			(v5_simple_mb_inter_16_8_en << 4) |
			(v5_simple_mb_inter_16x16_en << 3) |
			(v5_simple_mb_intra_en << 2) |
			(v5_simple_mb_C_en << 1) |
			(v5_simple_mb_Y_en << 0));
		WRITE_HREG(HCODEC_V5_MB_DIFF_SUM, 0);
		WRITE_HREG(HCODEC_V5_SMALL_DIFF_CNT,
			(v5_small_diff_C<<16) |
			(v5_small_diff_Y<<0));
		if (p->qp_mode == 1) {
			WRITE_HREG(HCODEC_V5_SIMPLE_MB_DQUANT, 0);
		} else {
			WRITE_HREG(HCODEC_V5_SIMPLE_MB_DQUANT, v5_simple_dq_setting);
		}
		WRITE_HREG(HCODEC_V5_SIMPLE_MB_ME_WEIGHT, v5_simple_me_weight_setting);
		/* txlx can remove it */
		WRITE_HREG(HCODEC_QDCT_CONFIG, 1 << 0);
	}

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXL) {
		WRITE_HREG(HCODEC_V4_FORCE_SKIP_CFG,
			(i_pic_qp << 26) | /* v4_force_q_r_intra */
			(i_pic_qp << 20) | /* v4_force_q_r_inter */
			(0 << 19) | /* v4_force_q_y_enable */
			(5 << 16) | /* v4_force_qr_y */
			(6 << 12) | /* v4_force_qp_y */
			(0 << 0)); /* v4_force_skip_sad */

		/* V3 Force skip */
		WRITE_HREG(HCODEC_V3_SKIP_CONTROL,
			(1 << 31) | /* v3_skip_enable */
			(0 << 30) | /* v3_step_1_weight_enable */
			(1 << 28) | /* v3_mv_sad_weight_enable */
			(1 << 27) | /* v3_ipred_type_enable */
			(V3_FORCE_SKIP_SAD_1 << 12) |
			(V3_FORCE_SKIP_SAD_0 << 0));
		WRITE_HREG(HCODEC_V3_SKIP_WEIGHT,
			(V3_SKIP_WEIGHT_1 << 16) |
			(V3_SKIP_WEIGHT_0 << 0));
		WRITE_HREG(HCODEC_V3_L1_SKIP_MAX_SAD,
			(V3_LEVEL_1_F_SKIP_MAX_SAD << 16) |
			(V3_LEVEL_1_SKIP_MAX_SAD << 0));
		WRITE_HREG(HCODEC_V3_L2_SKIP_WEIGHT,
			(V3_FORCE_SKIP_SAD_2 << 16) |
			(V3_SKIP_WEIGHT_2 << 0));
		if (p->i4_weight && p->i16_weight && p->me_weight) {
			unsigned int off1, off2;

			off1 = V3_IE_F_ZERO_SAD_I4 - I4MB_WEIGHT_OFFSET;
			off2 = V3_IE_F_ZERO_SAD_I16
				- I16MB_WEIGHT_OFFSET;
			WRITE_HREG(HCODEC_V3_F_ZERO_CTL_0,
				((p->i16_weight + off2) << 16) |
				((p->i4_weight + off1) << 0));
			off1 = V3_ME_F_ZERO_SAD - ME_WEIGHT_OFFSET;
			WRITE_HREG(HCODEC_V3_F_ZERO_CTL_1,
				(0 << 25) |
				/* v3_no_ver_when_top_zero_en */
				(0 << 24) |
				/* v3_no_hor_when_left_zero_en */
				(3 << 16) |  /* type_hor break */
				((p->me_weight + off1) << 0));
		} else {
			WRITE_HREG(HCODEC_V3_F_ZERO_CTL_0,
				(V3_IE_F_ZERO_SAD_I16 << 16) |
				(V3_IE_F_ZERO_SAD_I4 << 0));
			WRITE_HREG(HCODEC_V3_F_ZERO_CTL_1,
				(0 << 25) |
				/* v3_no_ver_when_top_zero_en */
				(0 << 24) |
				/* v3_no_hor_when_left_zero_en */
				(3 << 16) |  /* type_hor break */
				(V3_ME_F_ZERO_SAD << 0));
		}
	} else if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB) {
		/* V3 Force skip */
		WRITE_HREG(HCODEC_V3_SKIP_CONTROL,
			(1 << 31) | /* v3_skip_enable */
			(0 << 30) | /* v3_step_1_weight_enable */
			(1 << 28) | /* v3_mv_sad_weight_enable */
			(1 << 27) | /* v3_ipred_type_enable */
			(0 << 12) | /* V3_FORCE_SKIP_SAD_1 */
			(0 << 0)); /* V3_FORCE_SKIP_SAD_0 */
		WRITE_HREG(HCODEC_V3_SKIP_WEIGHT,
			(V3_SKIP_WEIGHT_1 << 16) |
			(V3_SKIP_WEIGHT_0 << 0));
		WRITE_HREG(HCODEC_V3_L1_SKIP_MAX_SAD,
			(V3_LEVEL_1_F_SKIP_MAX_SAD << 16) |
			(V3_LEVEL_1_SKIP_MAX_SAD << 0));
		WRITE_HREG(HCODEC_V3_L2_SKIP_WEIGHT,
			(0 << 16) | /* V3_FORCE_SKIP_SAD_2 */
			(V3_SKIP_WEIGHT_2 << 0));
		WRITE_HREG(HCODEC_V3_F_ZERO_CTL_0,
			(0 << 16) | /* V3_IE_F_ZERO_SAD_I16 */
			(0 << 0)); /* V3_IE_F_ZERO_SAD_I4 */
		WRITE_HREG(HCODEC_V3_F_ZERO_CTL_1,
			(0 << 25) | /* v3_no_ver_when_top_zero_en */
			(0 << 24) | /* v3_no_hor_when_left_zero_en */
			(3 << 16) |  /* type_hor break */
			(0 << 0)); /* V3_ME_F_ZERO_SAD */
	}
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB) {
		int i;
		/* MV SAD Table */
		for (i = 0; i < 64; i++)
			WRITE_HREG(HCODEC_V3_MV_SAD_TABLE, v3_mv_sad[i]);

		/* IE PRED SAD Table*/
		WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_0,
			(C_ipred_weight_H << 24) |
			(C_ipred_weight_V << 16) |
			(I4_ipred_weight_else << 8) |
			(I4_ipred_weight_most << 0));
		WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_1,
			(I16_ipred_weight_DC << 24) |
			(I16_ipred_weight_H << 16) |
			(I16_ipred_weight_V << 8) |
			(C_ipred_weight_DC << 0));
		WRITE_HREG(HCODEC_V3_LEFT_SMALL_MAX_SAD,
			(v3_left_small_max_me_sad << 16) |
			(v3_left_small_max_ie_sad << 0));
	}
	WRITE_HREG(HCODEC_IE_DATA_FEED_BUFF_INFO, 0);
	WRITE_HREG(HCODEC_CURR_CANVAS_CTRL, 0);
	data32 = READ_HREG(HCODEC_VLC_CONFIG);
	data32 = data32 | (1 << 0); /* set pop_coeff_even_all_zero */
	WRITE_HREG(HCODEC_VLC_CONFIG, data32);

	WRITE_HREG(INFO_DUMP_START_ADDR, p->dump_ddr_start_addr);

	/* clear mailbox interrupt */
	WRITE_HREG(HCODEC_IRQ_MBOX_CLR, 1);

	/* enable mailbox interrupt */
	WRITE_HREG(HCODEC_IRQ_MBOX_MASK, 1);
}

void amlvenc_dos_sw_reset(u32 bits)
{
	READ_VREG(DOS_SW_RESET1);
	READ_VREG(DOS_SW_RESET1);
	READ_VREG(DOS_SW_RESET1);
	WRITE_VREG(DOS_SW_RESET1, bits);
	WRITE_VREG(DOS_SW_RESET1, 0);
	READ_VREG(DOS_SW_RESET1);
	READ_VREG(DOS_SW_RESET1);
	READ_VREG(DOS_SW_RESET1);
}

void amlvenc_hcodec_start(void)
{
	WRITE_HREG(HCODEC_MPSR, 0x0001);
}

void amlvenc_hcodec_stop(void)
{
	WRITE_HREG(HCODEC_MPSR, 0);
	WRITE_HREG(HCODEC_CPSR, 0);
}

void amlvenc_hcodec_assist_enable(void)
{
	WRITE_HREG(HCODEC_ASSIST_MMC_CTRL1, 0x32);
}

void amlvenc_hcodec_dma_load_firmware(dma_addr_t dma_handle, size_t size)
{
    WRITE_HREG(HCODEC_IMEM_DMA_ADR, dma_handle);
    WRITE_HREG(HCODEC_IMEM_DMA_COUNT, size / 4);
    // WRITE_HREG(HCODEC_IMEM_DMA_CTRL, (0x8000 | (7 << 16)));
    WRITE_VREG(HCODEC_IMEM_DMA_CTRL, (0x8000 | (0xf << 16)));
}

bool amlvenc_hcodec_dma_completed(void)
{
	return !(READ_HREG(HCODEC_IMEM_DMA_CTRL) & 0x8000);
}

enum amlvenc_hcodec_status amlvenc_hcodec_status(void)
{
	WRITE_HREG(HCODEC_IRQ_MBOX_CLR, 1);
	return READ_HREG(ENCODER_STATUS);
}

void amlvenc_hcodec_clear_status(void)
{
	WRITE_HREG(ENCODER_STATUS, ENCODER_IDLE);
}

void amlvenc_hcodec_set_status(enum amlvenc_hcodec_status status){
	WRITE_HREG(ENCODER_STATUS, status);
}

void amlvenc_hcodec_canvas_config(u32 index, ulong addr, u32 width, u32 height, u32 wrap, u32 blkmode) {
	unsigned long datah_temp, datal_temp;
#if 1
	ulong start_addr = addr >> 3;
	u32 cav_width = (((width + 31)>>5)<<2);
	u32 cav_height = height;
	u32 x_wrap_en = 0;
	u32 y_wrap_en = 0;
	u32 blk_mode = 0;//blkmode;
	u32 cav_endian = 0;

	datal_temp = (start_addr & 0x1fffffff) |
				((cav_width & 0x7 ) << 29 );

	datah_temp = ((cav_width  >> 3) & 0x1ff) |
				((cav_height & 0x1fff) <<9 ) |
				((x_wrap_en & 1) << 22 ) |
				((y_wrap_en & 1) << 23) |
				((blk_mode & 0x3) << 24) |
				( cav_endian << 26);

#else
	u32 endian = 0;
	u32 addr_bits_l = ((((addr + 7) >> 3) & CANVAS_ADDR_LMASK) << CAV_WADDR_LBIT);
	u32 width_l     = ((((width    + 7) >> 3) & CANVAS_WIDTH_LMASK) << CAV_WIDTH_LBIT);
	u32 width_h     = ((((width    + 7) >> 3) >> CANVAS_WIDTH_LWID) << CAV_WIDTH_HBIT);
	u32 height_h    = (height & CANVAS_HEIGHT_MASK) << CAV_HEIGHT_HBIT;
	u32 blkmod_h    = (blkmode & CANVAS_BLKMODE_MASK) << CAV_BLKMODE_HBIT;
	u32 switch_bits_ctl = (endian & 0xf) << CAV_ENDIAN_HBIT;
	u32 wrap_h      = (0 << 23);
	datal_temp = addr_bits_l | width_l;
	datah_temp = width_h | height_h | wrap_h | blkmod_h | switch_bits_ctl;
#endif
	/*
	if (core == VDEC_1) {
		WRITE_VREG(MDEC_CAV_CFG0, 0);	//[0]canv_mode, by default is non-canv-mode
		WRITE_VREG(MDEC_CAV_LUT_DATAL, datal_temp);
		WRITE_VREG(MDEC_CAV_LUT_DATAH, datah_temp);
		WRITE_VREG(MDEC_CAV_LUT_ADDR,  index);
	} else if (core == VDEC_HCODEC) */ {
		WRITE_HREG(HCODEC_MDEC_CAV_CFG0, 0);	//[0]canv_mode, by default is non-canv-mode
		WRITE_HREG(HCODEC_MDEC_CAV_LUT_DATAL, datal_temp);
		WRITE_HREG(HCODEC_MDEC_CAV_LUT_DATAH, datah_temp);
		WRITE_HREG(HCODEC_MDEC_CAV_LUT_ADDR,  index);
	}

	/*
	cav_lut_info_store(index, addr, width, height, wrap, blkmode, 0);

	if (vdec_get_debug() & 0x40000000) {
		enc_pr(LOG_INFO, "(%s %2d) addr: %lx, width: %d, height: %d, blkm: %d, endian: %d\n",
			__func__, index, addr, width, height, blkmode, 0);
		enc_pr(LOG_INFO, "data(h,l): 0x%8lx, 0x%8lx\n", datah_temp, datal_temp);
	}
	*/
}



