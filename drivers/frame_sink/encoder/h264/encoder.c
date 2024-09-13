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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/compat.h>
//#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
// #include <linux/amlogic/media/frame_sync/ptsserv.h>
// #include <linux/amlogic/media/utils/amstream.h>
// #include <linux/amlogic/media/canvas/canvas.h>
// #include <linux/amlogic/media/canvas/canvas_mgr.h>
// #include <linux/amlogic/media/codec_mm/codec_mm.h>
#include "../../../frame_provider/decoder/utils/vdec_canvas_utils.h"
//#include <linux/amlogic/media/utils/vdec_reg.h>
//#include "../../../frame_provider/decoder/utils/vdec.h"
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
// #include <linux/amlogic/media/utils/amports_config.h>
#include "encoder.h"
#include "../../../frame_provider/decoder/utils/amvdec.h"
#include "../../../common/chips/decoder_cpu_ver_info.h"
//#include "../../../frame_provider/decoder/utils/vdec.h"
#include "../../../frame_provider/decoder/utils/vdec_power_ctrl.h"

//#include <linux/amlogic/media/utils/vdec_reg.h>
//#include <linux/amlogic/power_ctrl.h>
#include <dt-bindings/power/sc2-pd.h>
#include <dt-bindings/power/t3-pd.h>
#include <linux/amlogic/power_domain.h>
//#include <linux/amlogic/power_ctrl.h>

// #include <linux/amlogic/media/utils/amlog.h>
// #include "../../../stream_input/amports/amports_priv.h"
#include "../../../frame_provider/decoder/utils/firmware.h"
// #include <linux/amlogic/media/registers/register.h>
#include <linux/of_reserved_mem.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,1)
#include <uapi/linux/sched/types.h>
#include <linux/sched/signal.h>
#endif

#define MORE_MODULE_PARAM

#include "aml_venc_h264.h"

#ifdef CONFIG_AM_JPEG_ENCODER
#include "jpegenc.h"
#endif

#define MHz (1000000)

#define CHECK_RET(_ret) if (ret) {enc_pr(LOG_ERROR, \
		"%s:%d:function call failed with result: %d\n",\
		__FUNCTION__, __LINE__, _ret);}

#define ENCODE_NAME "encoder"
#define AMVENC_CANVAS_INDEX 0xE4
#define AMVENC_CANVAS_MAX_INDEX 0xEF

#define MIN_SIZE amvenc_buffspec[0].min_buffsize
#define DUMP_INFO_BYTES_PER_MB 80

#define ADJUSTED_QP_FLAG 64

static s32 avc_device_major;
static struct device *amvenc_avc_dev;
#define DRIVER_NAME "amvenc_avc"
#define CLASS_NAME "amvenc_avc"
#define DEVICE_NAME "amvenc_avc"

#define ENC_ALIGN_64(x) (((x + 63) >> 6) << 6)
#define ENC_ALIGN_32(x) (((x + 31) >> 5) << 5)
#define ENC_ALIGN_16(x) (((x + 15) >> 4) <<4)

static struct encode_manager_s encode_manager;

/* #define MORE_MODULE_PARAM */

#define ENC_CANVAS_OFFSET  0x64

#define UCODE_MODE_FULL 0

/* #define ENABLE_IGNORE_FUNCTION */

static enum amlvenc_henc_mb_type ie_me_mb_type;
static u32 ie_me_mode;
static u32 ie_pipeline_block = 3;
static u32 ie_cur_ref_sel;
/* static u32 avc_endian = 6; */
static u32 clock_level = 5;

static u32 encode_print_level = LOG_ERROR;
static u32 no_timeout;
static int nr_mode = -1;
static u32 qp_table_debug;
static u32 use_reset_control;
static u32 use_ge2d;
static u32 dump_input;
static unsigned int enc_canvas_offset;

#ifdef H264_ENC_SVC
static u32 svc_enable = 0; /* Enable sac feature or not */
static u32 svc_ref_conf = 0; /* Continuous no reference numbers */
#endif

struct hcodec_clks {
	struct clk *hcodec_aclk;
	//struct clk *hcodec_bclk;
	//struct clk *hcodec_cclk;
};

static struct hcodec_clks s_hcodec_clks;
struct reset_control *hcodec_rst;


static struct amlvenc_h264_me_params me = {
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

/* [31:0] NUM_ROWS_PER_SLICE_P */
/* [15:0] NUM_ROWS_PER_SLICE_I */
static u32 fixed_slice_cfg;

/* y tnr */
static struct amlvenc_h264_tnr_params y_tnr = {
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

/* c tnr */
static struct amlvenc_h264_tnr_params c_tnr = {
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
/* y snr */
static struct amlvenc_h264_snr_params y_snr = {
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

/* c snr */
static struct amlvenc_h264_snr_params c_snr = {
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

static unsigned int qp_mode;

static DEFINE_SPINLOCK(lock);

static struct BuffInfo_s amvenc_buffspec[] = {
	{
		.lev_id = 0,
		.max_width = 1920,
		.max_height = 1088,
		.min_buffsize = 0x1400000,
		.dct = {
			.buf_start = 0,
			.buf_size = 0x800000, /* 1920x1088x4 */
		},
		.dec0_y = {
			.buf_start = 0x800000,
			.buf_size = 0x300000,
		},
		.dec1_y = {
			.buf_start = 0xb00000,
			.buf_size = 0x300000,
		},
		.assit = {
			.buf_start = 0xe10000,
			.buf_size = 0xc0000,
		},
		.bitstream = {
			.buf_start = 0xf00000,
			.buf_size = 0x100000,
		},
		.scale_buff = {
			.buf_start = 0x1000000,
			.buf_size = 0x300000,
		},
		.dump_info = {
			.buf_start = 0x1300000,
			.buf_size = 0xa0000, /* (1920x1088/256)x80 */
		},
		.cbr_info = {
			.buf_start = 0x13b0000,
			.buf_size = 0x2000,
		}
	}
};

enum ucode_type_e {
	UCODE_GXL,
	UCODE_TXL,
	UCODE_G12A,
	UCODE_MAX
};

const char *ucode_name[] = {
	"gxl_h264_enc",
	"txl_h264_enc_cavlc",
	"ga_h264_enc_cabac",
};

static void dma_flush(u32 buf_start, u32 buf_size);
static void cache_flush(u32 buf_start, u32 buf_size);
static int enc_dma_buf_get_phys(struct enc_dma_cfg *cfg, unsigned long *addr);
static void enc_dma_buf_unmap(struct enc_dma_cfg *cfg);

//static struct canvas_status_s canvas_stat[CANVAS_MAX_SIZE];
//static struct canvas_status_s mdec_cav_stat[MDEC_CAV_LUT_MAX];
/*
static struct canvas_config_s *mdec_cav_pool = NULL;

static void cav_lut_info_store(u32 index, ulong addr, u32 width,
	u32 height, u32 wrap, u32 blkmode, u32 endian)
{
	struct canvas_config_s *pool = NULL;

	if (index < 0 || index >= MDEC_CAV_LUT_MAX) {
		enc_pr(LOG_ERROR, "%s, error index %d\n", __func__, index);
		return;
	}
	if (mdec_cav_pool == NULL)
		mdec_cav_pool = vzalloc(sizeof(struct canvas_config_s)
			* (MDEC_CAV_LUT_MAX + 1));

	if (mdec_cav_pool == NULL) {
		enc_pr(LOG_ERROR, "%s failed, mdec_cav_pool null\n", __func__);
		return;
	}
	pool = &mdec_cav_pool[index];
	pool->width = width;
	pool->height = height;
	pool->block_mode = blkmode;
	pool->endian = endian;
	pool->phy_addr = addr;
}

*/

static struct file *file_open(const char *path, int flags, int rights)
{
	//return filp_open(path, flags, rights);
	return NULL;
}

static int file_close(struct file *file)
{
	//return filp_close(file, NULL);
	return 0;
}

static int file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	//return kernel_write(file, data, size, &offset);
	return 0;
}

static void canvas_config_proxy(u32 index, ulong addr, u32 width, u32 height,
		   u32 wrap, u32 blkmode) {

	if (!is_support_vdec_canvas()) {
		canvas_config(index, addr, width, height, wrap, blkmode);
	} else {
		amlvenc_hcodec_canvas_config(index, addr, width, height, wrap, blkmode);
	}
}

static int is_oversize(int w, int h, int max)
{
	if (w <= 0 || h <= 0)
		return true;

	if (h != 0 && (w > max / h))
		return true;

	return false;
}

s32 hcodec_hw_reset(void)
{
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2 && use_reset_control) {
		reset_control_reset(hcodec_rst);
		enc_pr(LOG_DEBUG, "request hcodec reset from application.\n");
	}
	return 0;
}

s32 hcodec_clk_prepare(struct device *dev, struct hcodec_clks *clks)
{
	int ret;

	clks->hcodec_aclk = devm_clk_get(dev, "cts_hcodec_aclk");

	if (IS_ERR_OR_NULL(clks->hcodec_aclk)) {
		enc_pr(LOG_ERROR, "failed to get hcodec aclk\n");
		return -1;
	}

	ret = clk_set_rate(clks->hcodec_aclk, 667 * MHz);
	CHECK_RET(ret);

	ret = clk_prepare(clks->hcodec_aclk);
	CHECK_RET(ret);

	enc_pr(LOG_INFO, "hcodec_clk_a: %lu MHz\n", clk_get_rate(clks->hcodec_aclk) / 1000000);

	return 0;
}

void hcodec_clk_unprepare(struct device *dev, struct hcodec_clks *clks)
{
	clk_unprepare(clks->hcodec_aclk);
	devm_clk_put(dev, clks->hcodec_aclk);

	//clk_unprepare(clks->wave_bclk);
	//devm_clk_put(dev, clks->wave_bclk);

	//clk_unprepare(clks->wave_aclk);
	//devm_clk_put(dev, clks->wave_aclk);
}

s32 hcodec_clk_config(u32 enable)
{
	if (enable) {
		clk_enable(s_hcodec_clks.hcodec_aclk);
		//clk_enable(s_hcodec_clks.wave_bclk);
		//clk_enable(s_hcodec_clks.wave_cclk);
	} else {
		clk_disable(s_hcodec_clks.hcodec_aclk);
		//clk_disable(s_hcodec_clks.wave_bclk);
		//clk_disable(s_hcodec_clks.wave_aclk);
	}

	return 0;
}

static const char *select_ucode(u32 ucode_index)
{
	enum ucode_type_e ucode = UCODE_GXL;

	#if 0
	switch (ucode_index) {
	case UCODE_MODE_FULL:
		if (get_cpu_type() >= MESON_CPU_MAJOR_ID_G12A)
			ucode = UCODE_G12A;
		else if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL)
			ucode = UCODE_TXL;
		else /* (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXL) */
			ucode = UCODE_GXL;
		break;
		break;
	default:
		break;
	}
	#else
	if (ucode_index == UCODE_MODE_FULL)
		ucode = UCODE_G12A;
	else
		ucode = UCODE_GXL;
	#endif
	return (const char *)ucode_name[ucode];
}

static void avc_canvas_init(struct encode_wq_s *wq)
{
	u32 canvas_width, canvas_height;
	u32 start_addr = wq->mem.buf_start;
	canvas_width = ((wq->pic.encoder_width + 31) >> 5) << 5;
	canvas_height = ((wq->pic.encoder_height + 15) >> 4) << 4;

	canvas_config_proxy(enc_canvas_offset,
	      start_addr + wq->mem.bufspec.dec0_y.buf_start,
	      canvas_width, canvas_height,
	      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	canvas_config_proxy(1 + enc_canvas_offset,
	      start_addr + wq->mem.bufspec.dec0_uv.buf_start,
	      canvas_width, canvas_height / 2,
	      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	/*here the third plane use the same address as the second plane*/
	canvas_config_proxy(2 + enc_canvas_offset,
	      start_addr + wq->mem.bufspec.dec0_uv.buf_start,
	      canvas_width, canvas_height / 2,
	      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

	canvas_config_proxy(3 + enc_canvas_offset,
	      start_addr + wq->mem.bufspec.dec1_y.buf_start,
	      canvas_width, canvas_height,
	      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	canvas_config_proxy(4 + enc_canvas_offset,
	      start_addr + wq->mem.bufspec.dec1_uv.buf_start,
	      canvas_width, canvas_height / 2,
	      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	/*here the third plane use the same address as the second plane*/
	canvas_config_proxy(5 + enc_canvas_offset,
	      start_addr + wq->mem.bufspec.dec1_uv.buf_start,
	      canvas_width, canvas_height / 2,
	      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
}

static void avc_buffspec_init(struct encode_wq_s *wq)
{
	u32 canvas_width, canvas_height;
	u32 start_addr = wq->mem.buf_start;
	u32 mb_w = (wq->pic.encoder_width + 15) >> 4;
	u32 mb_h = (wq->pic.encoder_height + 15) >> 4;
	u32 mbs = mb_w * mb_h;

	canvas_width = ((wq->pic.encoder_width + 31) >> 5) << 5;
	canvas_height = ((wq->pic.encoder_height + 15) >> 4) << 4;

	wq->mem.dct_buff_start_addr = start_addr +
		wq->mem.bufspec.dct.buf_start;
	wq->mem.dct_buff_end_addr =
		wq->mem.dct_buff_start_addr +
		wq->mem.bufspec.dct.buf_size - 1;
	enc_pr(LOG_INFO, "dct_buff_start_addr is 0x%x, wq:%p.\n",
		wq->mem.dct_buff_start_addr, (void *)wq);

	wq->mem.bufspec.dec0_uv.buf_start =
		wq->mem.bufspec.dec0_y.buf_start +
		canvas_width * canvas_height;
	wq->mem.bufspec.dec0_uv.buf_size = canvas_width * canvas_height / 2;
	wq->mem.bufspec.dec1_uv.buf_start =
		wq->mem.bufspec.dec1_y.buf_start +
		canvas_width * canvas_height;
	wq->mem.bufspec.dec1_uv.buf_size = canvas_width * canvas_height / 2;
	wq->mem.assit_buffer_offset = start_addr +
		wq->mem.bufspec.assit.buf_start;
	enc_pr(LOG_INFO, "assit_buffer_offset is 0x%x, wq: %p.\n",
		wq->mem.assit_buffer_offset, (void *)wq);
	/*output stream buffer config*/
	wq->mem.BitstreamStart = start_addr +
		wq->mem.bufspec.bitstream.buf_start;
	wq->mem.BitstreamEnd =
		wq->mem.BitstreamStart +
		wq->mem.bufspec.bitstream.buf_size - 1;
	enc_pr(LOG_INFO, "BitstreamStart is 0x%x, wq: %p.\n",
		wq->mem.BitstreamStart, (void *)wq);

	wq->mem.scaler_buff_start_addr =
		wq->mem.buf_start + wq->mem.bufspec.scale_buff.buf_start;
	wq->mem.dump_info_ddr_start_addr =
		wq->mem.buf_start + wq->mem.bufspec.dump_info.buf_start;
	enc_pr(LOG_INFO,
		"CBR: dump_info_ddr_start_addr:%x.\n",
		wq->mem.dump_info_ddr_start_addr);
	enc_pr(LOG_INFO, "CBR: buf_start :%d.\n",
		wq->mem.buf_start);
	enc_pr(LOG_INFO, "CBR: dump_info.buf_start :%d.\n",
		wq->mem.bufspec.dump_info.buf_start);
	wq->mem.dump_info_ddr_size =
		DUMP_INFO_BYTES_PER_MB * mbs;
	wq->mem.dump_info_ddr_size =
		(wq->mem.dump_info_ddr_size + PAGE_SIZE - 1)
		& ~(PAGE_SIZE - 1);
	wq->mem.cbr_info_ddr_start_addr =
		wq->mem.buf_start + wq->mem.bufspec.cbr_info.buf_start;
	wq->mem.cbr_info_ddr_size =
		wq->mem.bufspec.cbr_info.buf_size;
	wq->mem.cbr_info_ddr_virt_addr =
		codec_mm_vmap(wq->mem.cbr_info_ddr_start_addr,
		            wq->mem.bufspec.cbr_info.buf_size);

	wq->mem.dblk_buf_canvas =
		((enc_canvas_offset + 2) << 16) |
		((enc_canvas_offset + 1) << 8) |
		(enc_canvas_offset);
	wq->mem.ref_buf_canvas =
		((enc_canvas_offset + 5) << 16) |
		((enc_canvas_offset + 4) << 8) |
		(enc_canvas_offset + 3);
}

#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
static int scale_frame(struct encode_wq_s *wq,
	struct encode_request_s *request,
	struct config_para_ex_s *ge2d_config,
	u32 src_addr, bool canvas)
{
	struct ge2d_context_s *context = encode_manager.context;
	int src_top, src_left, src_width, src_height;
	struct canvas_s cs0, cs1, cs2, cd;
	u32 src_canvas, dst_canvas;
	u32 src_canvas_w, dst_canvas_w;
	u32 src_h = request->src_h;
	u32 dst_w = ((wq->pic.encoder_width + 15) >> 4) << 4;
	u32 dst_h = ((wq->pic.encoder_height + 15) >> 4) << 4;
	int input_format = GE2D_FORMAT_M24_NV21;

	src_top = request->crop_top;
	src_left = request->crop_left;
	src_width = request->src_w - src_left - request->crop_right;
	src_height = request->src_h - src_top - request->crop_bottom;
	enc_pr(LOG_INFO, "request->fmt=%d, %d %d, canvas=%d\n", request->fmt, FMT_NV21, FMT_BGR888, canvas);

	if (canvas) {
		if ((request->fmt == FMT_NV21)
			|| (request->fmt == FMT_NV12)) {
			src_canvas = src_addr & 0xffff;
			input_format = GE2D_FORMAT_M24_NV21;
		} else if (request->fmt == FMT_BGR888) {
			src_canvas = src_addr & 0xffffff;
			input_format = GE2D_FORMAT_S24_RGB; //Opposite color after ge2d
		} else if (request->fmt == FMT_RGBA8888) {
			src_canvas = src_addr & 0xffffff;
			input_format = GE2D_FORMAT_S32_ABGR;
		} else {
			src_canvas = src_addr & 0xffffff;
			input_format = GE2D_FORMAT_M24_YUV420;
		}
	} else {
		if ((request->fmt == FMT_NV21)
			|| (request->fmt == FMT_NV12)) {
			src_canvas_w =
				((request->src_w + 31) >> 5) << 5;
			canvas_config(enc_canvas_offset + 9,
				src_addr,
				src_canvas_w, src_h,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			canvas_config(enc_canvas_offset + 10,
				src_addr + src_canvas_w * src_h,
				src_canvas_w, src_h / 2,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			src_canvas =
				((enc_canvas_offset + 10) << 8)
				| (enc_canvas_offset + 9);
			input_format = GE2D_FORMAT_M24_NV21;
		} else if (request->fmt == FMT_BGR888) {
			src_canvas_w =
				((request->src_w + 31) >> 5) << 5;

			canvas_config(enc_canvas_offset + 9,
				src_addr,
				src_canvas_w * 3, src_h,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			src_canvas = enc_canvas_offset + 9;
			input_format = GE2D_FORMAT_S24_RGB; //Opposite color after ge2d
		} else if (request->fmt == FMT_RGBA8888) {
			src_canvas_w =
				((request->src_w + 31) >> 5) << 5;
			canvas_config(
				enc_canvas_offset + 9,
				src_addr,
				src_canvas_w * 4,
				src_h,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			src_canvas = enc_canvas_offset + 9;
			input_format = GE2D_FORMAT_S32_ABGR; //Opposite color after ge2d
		} else {
			src_canvas_w =
				((request->src_w + 63) >> 6) << 6;
			canvas_config(enc_canvas_offset + 9,
				src_addr,
				src_canvas_w, src_h,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			canvas_config(enc_canvas_offset + 10,
				src_addr + src_canvas_w * src_h,
				src_canvas_w / 2, src_h / 2,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			canvas_config(enc_canvas_offset + 11,
				src_addr + src_canvas_w * src_h * 5 / 4,
				src_canvas_w / 2, src_h / 2,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			src_canvas =
				((enc_canvas_offset + 11) << 16) |
				((enc_canvas_offset + 10) << 8) |
				(enc_canvas_offset + 9);
			input_format = GE2D_FORMAT_M24_YUV420;
		}
	}

	dst_canvas_w =  ((dst_w + 31) >> 5) << 5;

	canvas_config(enc_canvas_offset + 6,
		wq->mem.scaler_buff_start_addr,
		dst_canvas_w, dst_h,
		CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

	canvas_config(enc_canvas_offset + 7,
		wq->mem.scaler_buff_start_addr + dst_canvas_w * dst_h,
		dst_canvas_w, dst_h / 2,
		CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

	dst_canvas = ((enc_canvas_offset + 7) << 8) |
		(enc_canvas_offset + 6);

	ge2d_config->alu_const_color = 0;
	ge2d_config->bitmask_en  = 0;
	ge2d_config->src1_gb_alpha = 0;
	ge2d_config->dst_xy_swap = 0;
	canvas_read(src_canvas & 0xff, &cs0);
	canvas_read((src_canvas >> 8) & 0xff, &cs1);
	canvas_read((src_canvas >> 16) & 0xff, &cs2);
	ge2d_config->src_planes[0].addr = cs0.addr;
	ge2d_config->src_planes[0].w = dst_w * 4;//cs0.width;
	ge2d_config->src_planes[0].h = dst_h;//cs0.height;
	ge2d_config->src_planes[1].addr = cs1.addr;
	ge2d_config->src_planes[1].w = cs1.width;
	ge2d_config->src_planes[1].h = cs1.height;
	ge2d_config->src_planes[2].addr = cs2.addr;
	ge2d_config->src_planes[2].w = cs2.width;
	ge2d_config->src_planes[2].h = cs2.height;

	canvas_read(dst_canvas & 0xff, &cd);

	ge2d_config->dst_planes[0].addr = cd.addr;
	ge2d_config->dst_planes[0].w = dst_w * 4;//cd.width;
	ge2d_config->dst_planes[0].h = dst_h;//cd.height;
	ge2d_config->src_key.key_enable = 0;
	ge2d_config->src_key.key_mask = 0;
	ge2d_config->src_key.key_mode = 0;
	ge2d_config->src_para.canvas_index = src_canvas;
	ge2d_config->src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->src_para.format = input_format | GE2D_LITTLE_ENDIAN;
	ge2d_config->src_para.fill_color_en = 0;
	ge2d_config->src_para.fill_mode = 0;
	ge2d_config->src_para.x_rev = 0;
	ge2d_config->src_para.y_rev = 0;
	ge2d_config->src_para.color = 0xffffffff;
	ge2d_config->src_para.top = 0;
	ge2d_config->src_para.left = 0;
	ge2d_config->src_para.width = dst_w;//request->src_w;
	ge2d_config->src_para.height = dst_h;//request->src_h;
	ge2d_config->src2_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->dst_para.canvas_index = dst_canvas;
	ge2d_config->dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->dst_para.format =
		GE2D_FORMAT_M24_NV21 | GE2D_LITTLE_ENDIAN;

	if ((wq->pic.encoder_width >= 1280 && wq->pic.encoder_height >= 720) ||
		(wq->pic.encoder_width >= 720 && wq->pic.encoder_height >= 1280)) {
		ge2d_config->dst_para.format |= wq->pic.color_space;
	}

	ge2d_config->dst_para.fill_color_en = 0;
	ge2d_config->dst_para.fill_mode = 0;
	ge2d_config->dst_para.x_rev = 0;
	ge2d_config->dst_para.y_rev = 0;
	ge2d_config->dst_para.color = 0;
	ge2d_config->dst_para.top = 0;
	ge2d_config->dst_para.left = 0;
	ge2d_config->dst_para.width = dst_w;
	ge2d_config->dst_para.height = dst_h;
	ge2d_config->dst_para.x_rev = 0;
	ge2d_config->dst_para.y_rev = 0;


	if (ge2d_context_config_ex(context, ge2d_config) < 0) {
		enc_pr(LOG_ERROR, "++ge2d configing error.\n");
		return -1;
	}
	stretchblt_noalpha(context, src_left, src_top, src_width, src_height,
		0, 0, wq->pic.encoder_width, wq->pic.encoder_height);
	return dst_canvas_w*dst_h * 3 / 2;
}
#endif

static s32 dump_raw_input(struct encode_wq_s *wq, struct encode_request_s *request) {
	u8 *data;
	struct canvas_s cs0, cs1;//, cs2
	u32 y_addr, uv_addr, canvas_w, picsize_y;
	u32 input = request->src;
	//u8 iformat = MAX_FRAME_FMT;
	struct file *filp;
	if (request->type == CANVAS_BUFF) {
		if ((request->fmt == FMT_NV21) || (request->fmt == FMT_NV12)) {
			input = input & 0xffff;
			canvas_read(input & 0xff, &cs0);
			canvas_read((input >> 8) & 0xff, &cs1);
			enc_pr(LOG_INFO, "dump raw input for canvas source\n");
			y_addr = cs0.addr;
			uv_addr = cs1.addr;

			canvas_w = ((wq->pic.encoder_width + 31) >> 5) << 5;
			picsize_y = wq->pic.encoder_height;

			data = (u8*)phys_to_virt(y_addr);
			filp = file_open("/data/encoder.yuv", O_APPEND | O_RDWR, 0644);
			if (filp) {
				file_write(filp, 0, data, canvas_w * picsize_y);
				file_close(filp);
			} else
				enc_pr(LOG_ERROR, "open encoder.yuv failed\n");

		}
	}
	return 0;
}


static void get_pitches(struct encode_wq_s *wq,struct encode_request_s *request,u32 *pitch_w,u32 *pitch_h)
{
	u32 w_pitch_tmp = 0;
	u32 h_pitch_tmp = 0;

	if (FMT_NV21 == request->fmt || FMT_NV12 == request->fmt) {
		if (request->framesize == ENC_ALIGN_64(wq->pic.encoder_width) * ENC_ALIGN_64(wq->pic.encoder_height) * 3 / 2) {
			w_pitch_tmp = ENC_ALIGN_64(wq->pic.encoder_width);
			h_pitch_tmp = ENC_ALIGN_64(wq->pic.encoder_height);
		}
		else if (request->framesize == ENC_ALIGN_64(wq->pic.encoder_width) * ENC_ALIGN_16(wq->pic.encoder_height) * 3 / 2) {
			w_pitch_tmp = ENC_ALIGN_64(wq->pic.encoder_width);
			h_pitch_tmp = ENC_ALIGN_16(wq->pic.encoder_height);
		}
		else if (request->framesize == ENC_ALIGN_32(wq->pic.encoder_width) * ENC_ALIGN_16(wq->pic.encoder_height) * 3 / 2) {
			w_pitch_tmp = ENC_ALIGN_32(wq->pic.encoder_width);
			h_pitch_tmp = ENC_ALIGN_16(wq->pic.encoder_height);
		}
		else{
			w_pitch_tmp = ENC_ALIGN_32(wq->pic.encoder_width);
			h_pitch_tmp = wq->pic.encoder_height;
		}
	}
	*pitch_w = w_pitch_tmp;
	*pitch_h = h_pitch_tmp;
	enc_pr(LOG_INFO,"pitch:%d,h_pitch:%d,width:%d,height:%d",w_pitch_tmp,h_pitch_tmp,wq->pic.encoder_width,wq->pic.encoder_height);
}


static s32 set_input_format(struct encode_wq_s *wq,
			    struct encode_request_s *request)
{
	s32 ret = 0;
	u8 iformat = MAX_FRAME_FMT, oformat = MAX_FRAME_FMT, r2y_en = 0;
	u32 picsize_x, picsize_y, src_addr;
	u32 canvas_w = 0;
	u32 input = request->src;
	u32 input_y = 0;
	u32 input_u = 0;
	u32 input_v = 0;
	u8 ifmt_extra = 0;
	u32 pitch = ((wq->pic.encoder_width + 31) >> 5) << 5;
	u32 h_pitch = ((wq->pic.encoder_height + 15) >> 4) << 4;

	if ((request->fmt == FMT_RGB565) || (request->fmt >= MAX_FRAME_FMT))
		return -1;

	if (dump_input)
		dump_raw_input(wq, request);

	picsize_x = ((wq->pic.encoder_width + 15) >> 4) << 4;
	if (request->scale_enable) {
		picsize_y = ((wq->pic.encoder_height + 15) >> 4) << 4;
	}
	else {
		picsize_y = wq->pic.encoder_height;
	}
	oformat = 0;

	if ((request->type == LOCAL_BUFF)
		|| (request->type == PHYSICAL_BUFF)
		|| (request->type == DMA_BUFF)) {
		if ((request->type == LOCAL_BUFF) &&
			(request->flush_flag & AMVENC_FLUSH_FLAG_INPUT))
			dma_flush(wq->mem.dct_buff_start_addr,
				request->framesize);
		if (request->type == LOCAL_BUFF) {
			input = wq->mem.dct_buff_start_addr;
			src_addr =
				wq->mem.dct_buff_start_addr;
			picsize_y = ((wq->pic.encoder_height + 15) >> 4) << 4;
		} else if (request->type == DMA_BUFF) {
			if (request->plane_num == 3) {
				input_y = (unsigned long)request->dma_cfg[0].paddr;
				input_u = (unsigned long)request->dma_cfg[1].paddr;
				input_v = (unsigned long)request->dma_cfg[2].paddr;
			} else if (request->plane_num == 2) {
				input_y = (unsigned long)request->dma_cfg[0].paddr;
				input_u = (unsigned long)request->dma_cfg[1].paddr;
				input_v = input_u;
			} else if (request->plane_num == 1) {
				input_y = (unsigned long)request->dma_cfg[0].paddr;
				if (request->fmt == FMT_NV21
					|| request->fmt == FMT_NV12) {
					get_pitches(wq,request,&pitch,&h_pitch);
					input_u = input_y + pitch * h_pitch;
					input_v = input_u;
				}
				if (request->fmt == FMT_YUV420) {
					input_u = input_y + picsize_x * picsize_y;
					input_v = input_u + picsize_x * picsize_y  / 4;
				}
			}
			src_addr = input_y;
			//picsize_y = wq->pic.encoder_height;
			enc_pr(LOG_INFO, "dma addr[0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx]\n",
				(unsigned long)request->dma_cfg[0].vaddr,
				(unsigned long)request->dma_cfg[0].paddr,
				(unsigned long)request->dma_cfg[1].vaddr,
				(unsigned long)request->dma_cfg[1].paddr,
				(unsigned long)request->dma_cfg[2].vaddr,
				(unsigned long)request->dma_cfg[2].paddr);
		} else {
			src_addr = input;
			picsize_y = wq->pic.encoder_height;
		}
		if (request->scale_enable) {
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
			struct config_para_ex_s ge2d_config;

			memset(&ge2d_config, 0,
				sizeof(struct config_para_ex_s));
			scale_frame(
				wq, request,
				&ge2d_config,
				src_addr,
				false);
			iformat = 2;
			r2y_en = 0;
			input = ((enc_canvas_offset + 7) << 8) |
				(enc_canvas_offset + 6);
			ret = 0;

			if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) || \
				(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5M) || \
				(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X)) {
				/*
				 * for t3, after scaling before goto MFDIN, need to config canvas with scaler buffer
				 * */
				enc_pr(LOG_INFO, "reconfig with scaler buffer\n");
				canvas_w = ((wq->pic.encoder_width + 31) >> 5) << 5;
				iformat = 2;

				canvas_config_proxy(enc_canvas_offset + 6,
					wq->mem.scaler_buff_start_addr,
					canvas_w, picsize_y,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 7,
					wq->mem.scaler_buff_start_addr + canvas_w * picsize_y,
					canvas_w, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);

				input = ((enc_canvas_offset + 7) << 8) |
					(enc_canvas_offset + 6);
			}

			goto MFDIN;
#else
			enc_pr(LOG_ERROR,
				"Warning: need enable ge2d for scale frame!\n");
			return -1;
#endif
		}
		if ((request->fmt <= FMT_YUV444_PLANE) ||
			(request->fmt >= FMT_YUV422_12BIT))
			r2y_en = 0;
		else
			r2y_en = 1;

		if (request->fmt >= FMT_YUV422_12BIT) {
			iformat = 7;
			ifmt_extra = request->fmt - FMT_YUV422_12BIT;
			if (request->fmt == FMT_YUV422_12BIT)
				canvas_w = picsize_x * 24 / 8;
			else if (request->fmt == FMT_YUV444_10BIT)
				canvas_w = picsize_x * 32 / 8;
			else
				canvas_w = (picsize_x * 20 + 7) / 8;
			canvas_w = ((canvas_w + 31) >> 5) << 5;
			canvas_config_proxy(enc_canvas_offset + 6,
				input,
				canvas_w, picsize_y,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			input = enc_canvas_offset + 6;
			input = input & 0xff;
		} else if (request->fmt == FMT_YUV422_SINGLE)
			iformat = 10;
		else if ((request->fmt == FMT_YUV444_SINGLE)
			|| (request->fmt == FMT_RGB888)) {
			iformat = 1;
			if (request->fmt == FMT_RGB888)
				r2y_en = 1;
			canvas_w =  picsize_x * 3;
			canvas_w = ((canvas_w + 31) >> 5) << 5;
			canvas_config_proxy(enc_canvas_offset + 6,
				input,
				canvas_w, picsize_y,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			input = enc_canvas_offset + 6;
		} else if ((request->fmt == FMT_NV21)
			|| (request->fmt == FMT_NV12)) {
			canvas_w = pitch;//((wq->pic.encoder_width + 31) >> 5) << 5;
			iformat = (request->fmt == FMT_NV21) ? 2 : 3;
			if (request->type == DMA_BUFF) {
				canvas_config_proxy(enc_canvas_offset + 6,
					input_y,
					canvas_w, h_pitch,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 7,
					input_u,
					canvas_w, h_pitch / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
			} else {
				canvas_config_proxy(enc_canvas_offset + 6,
					input,
					canvas_w, picsize_y,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 7,
					input + canvas_w * picsize_y,
					canvas_w, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
			}
			input = ((enc_canvas_offset + 7) << 8) |
				(enc_canvas_offset + 6);
		} else if (request->fmt == FMT_YUV420) {
			iformat = 4;
			canvas_w = ((wq->pic.encoder_width + 63) >> 6) << 6;
			if (request->type == DMA_BUFF) {
				canvas_config_proxy(enc_canvas_offset + 6,
					input_y,
					canvas_w, picsize_y,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 7,
					input_u,
					canvas_w / 2, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 8,
					input_v,
					canvas_w / 2, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
			} else {
				canvas_config_proxy(enc_canvas_offset + 6,
					input,
					canvas_w, picsize_y,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 7,
					input + canvas_w * picsize_y,
					canvas_w / 2, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 8,
					input + canvas_w * picsize_y * 5 / 4,
					canvas_w / 2, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);

			}
			input = ((enc_canvas_offset + 8) << 16) |
				((enc_canvas_offset + 7) << 8) |
				(enc_canvas_offset + 6);
		} else if ((request->fmt == FMT_YUV444_PLANE)
			|| (request->fmt == FMT_RGB888_PLANE)) {
			if (request->fmt == FMT_RGB888_PLANE)
				r2y_en = 1;
			iformat = 5;
			canvas_w = ((wq->pic.encoder_width + 31) >> 5) << 5;
			canvas_config_proxy(enc_canvas_offset + 6,
				input,
				canvas_w, picsize_y,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			canvas_config_proxy(enc_canvas_offset + 7,
				input + canvas_w * picsize_y,
				canvas_w, picsize_y,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			canvas_config_proxy(enc_canvas_offset + 8,
				input + canvas_w * picsize_y * 2,
				canvas_w, picsize_y,
				CANVAS_ADDR_NOWRAP,
				CANVAS_BLKMODE_LINEAR);
			input = ((enc_canvas_offset + 8) << 16) |
				((enc_canvas_offset + 7) << 8) |
				(enc_canvas_offset + 6);
		} else if (request->fmt == FMT_RGBA8888) {
			r2y_en = 1;
			iformat = 12;
		}
		ret = 0;
	} else if (request->type == CANVAS_BUFF) {

		r2y_en = 0;
		if (request->scale_enable) {
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
			struct config_para_ex_s ge2d_config;
			memset(&ge2d_config, 0,
				sizeof(struct config_para_ex_s));
			scale_frame(
				wq, request,
				&ge2d_config,
				input, true);
			iformat = 2;
			r2y_en = 0;
			input = ((enc_canvas_offset + 7) << 8) |
				(enc_canvas_offset + 6);
			ret = 0;

			/*
			if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) {
				enc_pr(LOG_INFO, "reconfig with scaler buffer\n");
				canvas_w = ((wq->pic.encoder_width + 31) >> 5) << 5;
				iformat = 2;

				canvas_config_proxy(enc_canvas_offset + 6,
					wq->mem.scaler_buff_start_addr,
					canvas_w, picsize_y,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				canvas_config_proxy(enc_canvas_offset + 7,
					wq->mem.scaler_buff_start_addr + canvas_w * picsize_y,
					canvas_w, picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);

				input = ((enc_canvas_offset + 7) << 8) |
					(enc_canvas_offset + 6);
			}
			*/

			goto MFDIN;
#else
			enc_pr(LOG_ERROR,
				"Warning: need enable ge2d for scale frame!\n");
			return -1;
#endif
		}
		//enc_pr(LOG_INFO, "request->type=%u\n", request->type);
		if (request->fmt == FMT_YUV422_SINGLE) {
			iformat = 0;
			input = input & 0xff;
		} else if (request->fmt == FMT_YUV444_SINGLE) {
			iformat = 1;
			input = input & 0xff;
		} else if ((request->fmt == FMT_NV21)
			|| (request->fmt == FMT_NV12)) {
			iformat = (request->fmt == FMT_NV21) ? 2 : 3;
			input = input & 0xffff;

			if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) || \
				(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5M) || \
				(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X)) {
				struct canvas_s cs0, cs1;//, cs2
				u32 y_addr, uv_addr, canvas_w, picsize_y;
				u8 iformat = MAX_FRAME_FMT;
				canvas_read(input & 0xff, &cs0);
				canvas_read((input >> 8) & 0xff, &cs1);
				//enc_pr(LOG_INFO, "t3 canvas source input reconfig\n");
				y_addr = cs0.addr;
				uv_addr = cs1.addr;

				canvas_w = ((wq->pic.encoder_width + 31) >> 5) << 5;
				picsize_y = wq->pic.encoder_height;
				iformat = (request->fmt == FMT_NV21) ? 2 : 3;

				canvas_config_proxy(
					enc_canvas_offset + 6,
					y_addr,
					canvas_w,
					picsize_y,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);

				canvas_config_proxy(
					enc_canvas_offset + 7,
					uv_addr,
					canvas_w,
					picsize_y / 2,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);

				input = ((enc_canvas_offset + 7) << 8) |
					(enc_canvas_offset + 6);
			}
		} else if (request->fmt == FMT_YUV420) {
			iformat = 4;
			input = input & 0xffffff;
		} else if ((request->fmt == FMT_YUV444_PLANE)
			|| (request->fmt == FMT_RGB888_PLANE)) {
			if (request->fmt == FMT_RGB888_PLANE)
				r2y_en = 1;
			iformat = 5;
			input = input & 0xffffff;
		} else if ((request->fmt == FMT_YUV422_12BIT)
			|| (request->fmt == FMT_YUV444_10BIT)
			|| (request->fmt == FMT_YUV422_10BIT)) {
			iformat = 7;
			ifmt_extra = request->fmt - FMT_YUV422_12BIT;
			input = input & 0xff;
		} else
			ret = -1;
	}
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
MFDIN:
#endif
	if (ret == 0) {
		struct amlvenc_h264_mdfin_params mdfin = {
			.input = input,
			.iformat = iformat,
			.oformat = oformat,
			.picsize_x = picsize_x,
			.picsize_y = picsize_y,
			.r2y_en = r2y_en,
			.nr_mode = nr_mode,
			.nr_mode = request->nr_mode,
			.ifmt_extra = ifmt_extra,
			.y_snr = &y_snr,
			.c_snr = &c_snr,
			.y_tnr = &y_tnr,
			.c_tnr = &c_tnr,
		};
		amlvenc_h264_configure_mdfin(&mdfin);
	}

	return ret;
}

#ifdef H264_ENC_CBR
static void ConvertTable2Risc(void *table, u32 len)
{
	u32 i, j;
	u16 temp;
	u16 *tbl = (u16 *)table;

	if ((len < 8) || (len % 8) || (!table)) {
		enc_pr(LOG_ERROR, "ConvertTable2Risc tbl %p, len %d error\n",
			table, len);
		return;
	}
	for (i = 0; i < len / 8; i++) {
		j = i << 2;
		temp = tbl[j];
		tbl[j] = tbl[j + 3];
		tbl[j + 3] = temp;

		temp = tbl[j + 1];
		tbl[j + 1] = tbl[j + 2];
		tbl[j + 2] = temp;
	}

}
#endif

static void avc_prot_init(struct encode_wq_s *wq,
	struct encode_request_s *request, u32 quant, bool IDR)
{
	u32 i4_weight, i16_weight, me_weight;

	if (request != NULL) {
		i4_weight = request->i4_weight;
		i16_weight = request->i16_weight;
		me_weight = request->me_weight;
		wq->me_weight = request->me_weight;
		wq->i4_weight = request->i4_weight;
		wq->i16_weight = request->i16_weight;
	} else {
		i4_weight = I4MB_WEIGHT_OFFSET;
		i16_weight = I16MB_WEIGHT_OFFSET;
		me_weight = ME_WEIGHT_OFFSET;
	}

	struct amlvenc_h264_qtable_params qtable = {
		.quant_tbl_i4 = wq->quant_tbl_i4,
		.quant_tbl_i16 = wq->quant_tbl_i16,
		.quant_tbl_me = wq->quant_tbl_me,
	};

	struct amlvenc_h264_configure_encoder_params p = {
		.idr = IDR,
		.quant = quant,
		.qp_mode = qp_mode,
		.encoder_width = wq->pic.encoder_width,
		.encoder_height = wq->pic.encoder_height,
		.i4_weight = i4_weight,
		.i16_weight = i16_weight,
		.me_weight = me_weight,
		.cbr_ddr_start_addr = wq->mem.cbr_info_ddr_start_addr,
		.cbr_start_tbl_id = wq->cbr_info.start_tbl_id,
		.cbr_short_shift = wq->cbr_info.short_shift,
		.cbr_long_mb_num = wq->cbr_info.long_mb_num,
		.cbr_long_th = wq->cbr_info.long_th,
		.cbr_block_w = wq->cbr_info.block_w,
		.cbr_block_h = wq->cbr_info.block_h,
		.dump_ddr_start_addr = wq->mem.dump_info_ddr_start_addr,
		.qtable = &qtable,
		.me = &me,
	};

	amlvenc_h264_configure_encoder(&p);
}

void amvenc_reset(void)
{
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2 &&
			use_reset_control) {
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		amlvenc_dos_sw_reset1(
			(1 << 2)  | (1 << 6)  |
			(1 << 7)  | (1 << 8)  |
			(1 << 14) | (1 << 16) |
			(1 << 17)
		);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}
}

void amvenc_start(void)
{
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2 &&
			use_reset_control) {
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		amlvenc_dos_sw_reset1(
			(1 << 12) | (1 << 11)
		);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}

	amlvenc_hcodec_start();
}

void amvenc_stop(void)
{
	ulong timeout = jiffies + HZ;

	amlvenc_hcodec_stop();

	while (!amlvenc_hcodec_dma_completed()) {
		if (time_after(jiffies, timeout))
			break;
	}

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2 &&
			use_reset_control) {
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		amlvenc_dos_sw_reset1(
			(1 << 12) | (1 << 11) |
			(1 << 2)  | (1 << 6)  |
			(1 << 7)  | (1 << 8)  |
			(1 << 14) | (1 << 16) |
			(1 << 17)
		);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}

}

static void __iomem *mc_addr;
static u32 mc_addr_map;
#define MC_SIZE (4096 * 8)
s32 amvenc_loadmc(const char *p, struct encode_wq_s *wq)
{
	ulong timeout;
	s32 ret = 0;
    if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
        char *buf = vmalloc(0x1000 * 16);
        int ret = -1;
        enc_pr(LOG_INFO, "load firmware for t3 avc encoder\n");
        if (get_firmware_data(VIDEO_ENC_H264, buf) < 0) {
            //amvdec_disable();
            enc_pr(LOG_ERROR, "get firmware for 264 enc fail!\n");
            vfree(buf);
            return -1;
        }

		amlvenc_hcodec_stop();

        ret = amvdec_loadmc_ex(VFORMAT_H264_ENC, p, buf);

        if (ret < 0) {
            //amvdec_disable();
            vfree(buf);
            enc_pr(LOG_ERROR, "amvenc: the %s fw loading failed, err: %x\n",
                tee_enabled() ? "TEE" : "local", ret);
            return -EBUSY;
        }
        vfree(buf);
        return 0;
    }

	/* use static mempry*/
	if (mc_addr == NULL) {
		mc_addr = kmalloc(MC_SIZE, GFP_KERNEL);
		if (!mc_addr) {
			enc_pr(LOG_ERROR, "avc loadmc iomap mc addr error.\n");
			return -ENOMEM;
		}
	}

	enc_pr(LOG_ALL, "avc encode ucode name is %s\n", p);
	ret = get_data_from_name(p, (u8 *)mc_addr);
	if (ret < 0) {
		enc_pr(LOG_ERROR,
			"avc microcode fail ret=%d, name: %s, wq:%p.\n",
			ret, p, (void *)wq);
	}

	mc_addr_map = dma_map_single(
		&encode_manager.this_pdev->dev,
		mc_addr, MC_SIZE, DMA_TO_DEVICE);

	/* mc_addr_map = wq->mem.assit_buffer_offset; */
	/* mc_addr = ioremap_wc(mc_addr_map, MC_SIZE); */
	/* memcpy(mc_addr, p, MC_SIZE); */
	enc_pr(LOG_ALL, "address 0 is 0x%x\n", *((u32 *)mc_addr));
	enc_pr(LOG_ALL, "address 1 is 0x%x\n", *((u32 *)mc_addr + 1));
	enc_pr(LOG_ALL, "address 2 is 0x%x\n", *((u32 *)mc_addr + 2));
	enc_pr(LOG_ALL, "address 3 is 0x%x\n", *((u32 *)mc_addr + 3));

	amlvenc_hcodec_stop();

	/* Read CBUS register for timing */
	timeout = READ_HREG(HCODEC_MPSR);
	timeout = READ_HREG(HCODEC_MPSR);

	timeout = jiffies + HZ;

    amlvenc_hcodec_dma_load_firmware(mc_addr_map, MC_SIZE);

    while (!amlvenc_hcodec_dma_completed()) {
		if (time_before(jiffies, timeout))
			schedule();
		else {
			enc_pr(LOG_ERROR, "hcodec load mc error\n");
			ret = -EBUSY;
			break;
		}
	}
	dma_unmap_single(
		&encode_manager.this_pdev->dev,
		mc_addr_map, MC_SIZE, DMA_TO_DEVICE);
	return ret;
}

const u32 fix_mc[] __aligned(8) = {
	0x0809c05a, 0x06696000, 0x0c780000, 0x00000000
};


/*
 * DOS top level register access fix.
 * When hcodec is running, a protocol register HCODEC_CCPU_INTR_MSK
 * is set to make hcodec access one CBUS out of DOS domain once
 * to work around a HW bug for 4k2k dual decoder implementation.
 * If hcodec is not running, then a ucode is loaded and executed
 * instead.
 */
/*void amvenc_dos_top_reg_fix(void)
{
	bool hcodec_on;
	ulong flags;

	spin_lock_irqsave(&lock, flags);

	hcodec_on = vdec_on(VDEC_HCODEC);

	if ((hcodec_on) && (READ_VREG(HCODEC_MPSR) & 1)) {
		WRITE_HREG(HCODEC_CCPU_INTR_MSK, 1);
		spin_unlock_irqrestore(&lock, flags);
		return;
	}

	if (!hcodec_on)
		vdec_poweron(VDEC_HCODEC);

	amhcodec_loadmc(fix_mc);

	amhcodec_start();

	udelay(1000);

	amhcodec_stop();

	if (!hcodec_on)
		vdec_poweroff(VDEC_HCODEC);

	spin_unlock_irqrestore(&lock, flags);
}

bool amvenc_avc_on(void)
{
	bool hcodec_on;
	ulong flags;

	spin_lock_irqsave(&lock, flags);

	hcodec_on = vdec_on(VDEC_HCODEC);
	hcodec_on &= (encode_manager.wq_count > 0);

	spin_unlock_irqrestore(&lock, flags);
	return hcodec_on;
}
*/

static s32 avc_poweron(u32 clock)
{
	ulong flags;
	u32 data32;

	data32 = 0;
	amports_switch_gate("vdec", 1);
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
		hcodec_clk_config(1);
		udelay(20);
		if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) || \
			(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5M) || \
			(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X)) {
			vdec_poweron(VDEC_HCODEC);
			enc_pr(LOG_INFO, "vdec_poweron VDEC_HCODEC\n");
		} else {
			pwr_ctrl_psci_smc(PDID_T3_DOS_HCODEC, PWR_ON);
			enc_pr(LOG_INFO, "pwr_ctrl_psci_smc PDID_T3_DOS_HCODEC on\n");
		}
		udelay(20);
        /*
        enc_pr(LOG_INFO, "hcodec powered on, hcodec clk rate:%ld, pwr_state:%d\n",
            clk_get_rate(s_hcodec_clks.hcodec_aclk),
            !pwr_ctrl_status_psci_smc(PDID_T3_DOS_HCODEC));
        */
	} else {
		spin_lock_irqsave(&lock, flags);
		WRITE_AOREG(AO_RTI_PWR_CNTL_REG0,
			(READ_AOREG(AO_RTI_PWR_CNTL_REG0) & (~0x18)));
		udelay(10);
		/* Powerup HCODEC */
		/* [1:0] HCODEC */
		WRITE_AOREG(AO_RTI_GEN_PWR_SLEEP0,
				READ_AOREG(AO_RTI_GEN_PWR_SLEEP0) &
				((get_cpu_type() == MESON_CPU_MAJOR_ID_SM1 ||
				 get_cpu_type() >= MESON_CPU_MAJOR_ID_TM2)
				? ~0x1 : ~0x3));
		udelay(10);
		spin_unlock_irqrestore(&lock, flags);
	}
	spin_lock_irqsave(&lock, flags);
	amlvenc_dos_sw_reset1(0xffffffff);
	amlvenc_dos_hcodec_enable(clock);

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
	} else  {
		/* Remove HCODEC ISO */
		WRITE_AOREG(AO_RTI_GEN_PWR_ISO0,
				READ_AOREG(AO_RTI_GEN_PWR_ISO0) &
				((get_cpu_type() == MESON_CPU_MAJOR_ID_SM1 ||
				  get_cpu_type() >= MESON_CPU_MAJOR_ID_TM2)
				? ~0x1 : ~0x30));
	}
	udelay(10);
	/* Disable auto-clock gate */
	WRITE_VREG(DOS_GEN_CTRL0,
		(READ_VREG(DOS_GEN_CTRL0) | 0x1));
	WRITE_VREG(DOS_GEN_CTRL0,
		(READ_VREG(DOS_GEN_CTRL0) & 0xFFFFFFFE));
	spin_unlock_irqrestore(&lock, flags);
	mdelay(10);

	return 0;
}

static s32 avc_poweroff(void)
{
	ulong flags;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
		hcodec_clk_config(0);
		udelay(20);

		if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) || \
			(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5M) || \
			(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X)) {
			vdec_poweroff(VDEC_HCODEC);
			enc_pr(LOG_INFO, "vdec_poweroff VDEC_HCODEC\n");
		} else {
			pwr_ctrl_psci_smc(PDID_T3_DOS_HCODEC, PWR_OFF);
			enc_pr(LOG_INFO, "pwr_ctrl_psci_smc PDID_T3_DOS_HCODEC off\n");
		}
		udelay(20);
	} else {
		/* enable HCODEC isolation */
		spin_lock_irqsave(&lock, flags);
		WRITE_AOREG(AO_RTI_GEN_PWR_ISO0,
				READ_AOREG(AO_RTI_GEN_PWR_ISO0) |
				((get_cpu_type() == MESON_CPU_MAJOR_ID_SM1 ||
				  get_cpu_type() >= MESON_CPU_MAJOR_ID_TM2)
				? 0x1 : 0x30));
		spin_unlock_irqrestore(&lock, flags);
	}
	spin_lock_irqsave(&lock, flags);

	amlvenc_dos_hcodec_disable();

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {

	} else {
		/* HCODEC power off */
		WRITE_AOREG(AO_RTI_GEN_PWR_SLEEP0,
				READ_AOREG(AO_RTI_GEN_PWR_SLEEP0) |
				((get_cpu_type() == MESON_CPU_MAJOR_ID_SM1 ||
				  get_cpu_type() >= MESON_CPU_MAJOR_ID_TM2)
				? 0x1 : 0x3));
	}

	spin_unlock_irqrestore(&lock, flags);

	/* release DOS clk81 clock gating */
	amports_switch_gate("vdec", 0);
	return 0;
}

static s32 reload_mc(struct encode_wq_s *wq)
{
	const char *p = select_ucode(encode_manager.ucode_index);

	amvenc_stop();

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2 && use_reset_control) {
		hcodec_hw_reset();
	} else {
		amlvenc_dos_sw_reset1(0xffffffff);
	}

	udelay(10);

	amlvenc_hcodec_assist_enable();
	enc_pr(LOG_INFO, "reload microcode\n");

	if (amvenc_loadmc(p, wq) < 0)
		return -EBUSY;
	return 0;
}

static void encode_isr_tasklet(ulong data)
{
	struct encode_manager_s *manager = (struct encode_manager_s *)data;

	enc_pr(LOG_INFO, "encoder is done %d\n", manager->encode_hw_status);
	if (((manager->encode_hw_status == ENCODER_IDR_DONE)
		|| (manager->encode_hw_status == ENCODER_NON_IDR_DONE)
		|| (manager->encode_hw_status == ENCODER_SEQUENCE_DONE)
		|| (manager->encode_hw_status == ENCODER_PICTURE_DONE))
		&& (manager->process_irq)) {
		wake_up_interruptible(&manager->event.hw_complete);
	}
}

/* irq function */
static irqreturn_t enc_isr(s32 irq_number, void *para)
{
	struct encode_manager_s *manager = (struct encode_manager_s *)para;

	enc_pr(LOG_INFO, "*****ENC_ISR*****\n");

	manager->encode_hw_status  = amlvenc_hcodec_encoder_status();
	if ((manager->encode_hw_status == ENCODER_IDR_DONE)
		|| (manager->encode_hw_status == ENCODER_NON_IDR_DONE)
		|| (manager->encode_hw_status == ENCODER_SEQUENCE_DONE)
		|| (manager->encode_hw_status == ENCODER_PICTURE_DONE)) {
		enc_pr(LOG_ALL, "encoder stage is %d\n",
			manager->encode_hw_status);
	}

	if (((manager->encode_hw_status == ENCODER_IDR_DONE)
		|| (manager->encode_hw_status == ENCODER_NON_IDR_DONE)
		|| (manager->encode_hw_status == ENCODER_SEQUENCE_DONE)
		|| (manager->encode_hw_status == ENCODER_PICTURE_DONE))
		&& (!manager->process_irq)) {
		manager->process_irq = true;
		if (manager->encode_hw_status != ENCODER_SEQUENCE_DONE)
			manager->need_reset = true;

		tasklet_schedule(&manager->encode_tasklet);
	}
	return IRQ_HANDLED;
}

static s32 convert_request(struct encode_wq_s *wq, u32 *cmd_info)
{
	int i = 0;
	u8 *ptr;
	u32 data_offset;
	u32 cmd = cmd_info[0];
	unsigned long paddr = 0;
	struct enc_dma_cfg *cfg = NULL;
	s32 ret = 0;
	struct platform_device *pdev;

	if (!wq)
		return -1;

	memset(&wq->request, 0, sizeof(struct encode_request_s));
	wq->request.me_weight = ME_WEIGHT_OFFSET;
	wq->request.i4_weight = I4MB_WEIGHT_OFFSET;
	wq->request.i16_weight = I16MB_WEIGHT_OFFSET;

	if (cmd == ENCODER_SEQUENCE) {
		wq->request.cmd = cmd;
		wq->request.ucode_mode = cmd_info[1];
		wq->request.quant = cmd_info[2];
		wq->request.flush_flag = cmd_info[3];
		//wq->request.timeout = cmd_info[4];
		wq->request.timeout = 5000; /* 5000 ms */
	} else if ((cmd == ENCODER_IDR) || (cmd == ENCODER_NON_IDR)) {
		wq->request.cmd = cmd;
		wq->request.ucode_mode = cmd_info[1];
		wq->request.type = cmd_info[2];
		wq->request.fmt = cmd_info[3];
		wq->request.src = cmd_info[4];
		wq->request.framesize = cmd_info[5];
		wq->request.quant = cmd_info[6];
		wq->request.flush_flag = cmd_info[7];
		wq->request.timeout = cmd_info[8];
		wq->request.crop_top = cmd_info[9];
		wq->request.crop_bottom = cmd_info[10];
		wq->request.crop_left = cmd_info[11];
		wq->request.crop_right = cmd_info[12];
		wq->request.src_w = cmd_info[13];
		wq->request.src_h = cmd_info[14];
		wq->request.scale_enable = cmd_info[15];

		enc_pr(LOG_INFO, "hwenc: wq->pic.encoder_width %d, framesize:%d",
		      wq->pic.encoder_width,wq->request.framesize);
		enc_pr(LOG_INFO, "wq->pic.encoder_height:%d, request fmt=%d\n",
		      wq->pic.encoder_height, wq->request.fmt);

		if (/*((wq->pic.encoder_width >= 1280 && wq->pic.encoder_height >= 720) ||
			(wq->pic.encoder_width >= 720 && wq->pic.encoder_height >= 1280))
			&& */wq->request.fmt == FMT_RGBA8888/* && wq->pic.color_space != GE2D_FORMAT_BT601*/) {
			wq->request.scale_enable = 1;
			wq->request.src_w = wq->pic.encoder_width;
			wq->request.src_h = wq->pic.encoder_height;
			enc_pr(LOG_INFO, "hwenc: force wq->request.scale_enable=%d\n", wq->request.scale_enable);
		}
		/*
		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3 && wq->request.type == CANVAS_BUFF) {
			wq->request.scale_enable = 1;
			wq->request.src_w = wq->pic.encoder_width;
			wq->request.src_h = wq->pic.encoder_height;
			enc_pr(LOG_DEBUG, "hwenc: t3 canvas source, force wq->request.scale_enable=%d\n", wq->request.scale_enable);
		}
		*/

		wq->request.nr_mode =
			(nr_mode > 0) ? nr_mode : cmd_info[16];
		if (cmd == ENCODER_IDR)
			wq->request.nr_mode = 0;

		data_offset = 17 +
			(sizeof(wq->quant_tbl_i4)
			+ sizeof(wq->quant_tbl_i16)
			+ sizeof(wq->quant_tbl_me)) / 4;

		if (wq->request.quant == ADJUSTED_QP_FLAG) {
			ptr = (u8 *) &cmd_info[17];
			memcpy(wq->quant_tbl_i4, ptr,
				sizeof(wq->quant_tbl_i4));
			ptr += sizeof(wq->quant_tbl_i4);
			memcpy(wq->quant_tbl_i16, ptr,
				sizeof(wq->quant_tbl_i16));
			ptr += sizeof(wq->quant_tbl_i16);
			memcpy(wq->quant_tbl_me, ptr,
				sizeof(wq->quant_tbl_me));
			wq->request.i4_weight -=
				cmd_info[data_offset++];
			wq->request.i16_weight -=
				cmd_info[data_offset++];
			wq->request.me_weight -=
				cmd_info[data_offset++];
			if (qp_table_debug) {
				u8 *qp_tb = (u8 *)(&wq->quant_tbl_i4[0]);

				for (i = 0; i < 32; i++) {
					enc_pr(LOG_INFO, "%d ", *qp_tb);
					qp_tb++;
				}
				enc_pr(LOG_INFO, "\n");

				qp_tb = (u8 *)(&wq->quant_tbl_i16[0]);
				for (i = 0; i < 32; i++) {
					enc_pr(LOG_INFO, "%d ", *qp_tb);
					qp_tb++;
				}
				enc_pr(LOG_INFO, "\n");

				qp_tb = (u8 *)(&wq->quant_tbl_me[0]);
				for (i = 0; i < 32; i++) {
					enc_pr(LOG_INFO, "%d ", *qp_tb);
					qp_tb++;
				}
				enc_pr(LOG_INFO, "\n");
			}
		} else {
			memset(wq->quant_tbl_me, wq->request.quant,
				sizeof(wq->quant_tbl_me));
			memset(wq->quant_tbl_i4, wq->request.quant,
				sizeof(wq->quant_tbl_i4));
			memset(wq->quant_tbl_i16, wq->request.quant,
				sizeof(wq->quant_tbl_i16));
			data_offset += 3;
		}
		//add qp range check
		enc_pr(LOG_INFO, "wq->request.quant %d \n", wq->request.quant);
		{
			u8 *qp_tb = (u8 *)(&wq->quant_tbl_i4[0]);
			for (i = 0; i < 32; i++) {
				if (*qp_tb > 51) {
					enc_pr(LOG_ERROR, "i4 %d ", *qp_tb);
					*qp_tb = 51;
				}
				qp_tb++;
			}

			qp_tb = (u8 *)(&wq->quant_tbl_i16[0]);
			for (i = 0; i < 32; i++) {
				if (*qp_tb > 51) {
					enc_pr(LOG_ERROR, "i16 %d ", *qp_tb);
					*qp_tb = 51;
				}
				qp_tb++;
			}

			qp_tb = (u8 *)(&wq->quant_tbl_me[0]);
			for (i = 0; i < 32; i++) {
				if (*qp_tb > 51) {
					enc_pr(LOG_ERROR, "me %d ", *qp_tb);
					*qp_tb = 51;
				}
				qp_tb++;
			}
		}
#ifdef H264_ENC_CBR
		wq->cbr_info.block_w = cmd_info[data_offset++];
		wq->cbr_info.block_h = cmd_info[data_offset++];
		wq->cbr_info.long_th = cmd_info[data_offset++];
		wq->cbr_info.start_tbl_id = cmd_info[data_offset++];
		wq->cbr_info.short_shift = CBR_SHORT_SHIFT;
		wq->cbr_info.long_mb_num = CBR_LONG_MB_NUM;
#endif
		data_offset = 17 +
				(sizeof(wq->quant_tbl_i4)
				+ sizeof(wq->quant_tbl_i16)
				+ sizeof(wq->quant_tbl_me)) / 4 + 7;

		if (wq->request.type == DMA_BUFF) {
			wq->request.plane_num = cmd_info[data_offset++];
			enc_pr(LOG_INFO, "wq->request.plane_num %d\n",
				wq->request.plane_num);
			if (wq->request.fmt == FMT_NV12 ||
				wq->request.fmt == FMT_NV21 ||
				wq->request.fmt == FMT_YUV420 ||
				wq->request.fmt == FMT_RGBA8888) {
				if (wq->request.plane_num > 3) {
					enc_pr(LOG_ERROR, "wq->request.plane_num is invalid %d.\n",
							wq->request.plane_num);
					return -1;
				}
				for (i = 0; i < wq->request.plane_num; i++) {
					cfg = &wq->request.dma_cfg[i];
					cfg->dir = DMA_TO_DEVICE;
					cfg->fd = cmd_info[data_offset++];
					pdev = encode_manager.this_pdev;
					cfg->dev = &(pdev->dev);

					ret = enc_dma_buf_get_phys(cfg, &paddr);
					if (ret < 0) {
						enc_pr(LOG_ERROR,
							"import fd %d failed\n",
							cfg->fd);
						cfg->paddr = NULL;
						cfg->vaddr = NULL;
						return -1;
					}
					cfg->paddr = (void *)paddr;
					enc_pr(LOG_INFO, "vaddr %p\n",
						cfg->vaddr);
				}
			} else {
				enc_pr(LOG_ERROR, "error fmt = %d\n",
					wq->request.fmt);
			}
		}

	} else {
		enc_pr(LOG_ERROR, "error cmd = %d, wq: %p.\n",
			cmd, (void *)wq);
		return -1;
	}
	wq->request.parent = wq;
	return 0;
}

void amvenc_avc_start_cmd(struct encode_wq_s *wq,
			  struct encode_request_s *request)
{
	u32 reload_flag = 0;

	if (request->ucode_mode != encode_manager.ucode_index) {
		encode_manager.ucode_index = request->ucode_mode;
		if (reload_mc(wq)) {
			enc_pr(LOG_ERROR,
				"reload mc fail, wq:%p\n", (void *)wq);
			return;
		}
		reload_flag = 1;
		encode_manager.need_reset = true;
	}

	wq->hw_status = 0;
	wq->output_size = 0;
	wq->ucode_index = encode_manager.ucode_index;

	ie_me_mode = (0 & ME_PIXEL_MODE_MASK) << ME_PIXEL_MODE_SHIFT;

	if (encode_manager.need_reset) {
		amvenc_stop();
		reload_flag = 1;
		encode_manager.need_reset = false;
		encode_manager.encode_hw_status = ENCODER_IDLE;
		amvenc_reset();
		avc_canvas_init(wq);

		struct amlvenc_h264_init_encoder_params init_params = {
			.idr = request->cmd == ENCODER_IDR,
			.idr_pic_id = wq->pic.idr_pic_id,
			.init_qppicture = wq->pic.init_qppicture,
			.frame_number = wq->pic.frame_number,
			.log2_max_frame_num = wq->pic.log2_max_frame_num,
			.pic_order_cnt_lsb = wq->pic.pic_order_cnt_lsb,
			.log2_max_pic_order_cnt_lsb = wq->pic.log2_max_pic_order_cnt_lsb,
		};

		amlvenc_h264_init_encoder(&init_params);
		amlvenc_h264_init_input_dct_buffer(wq->mem.dct_buff_start_addr, wq->mem.dct_buff_end_addr);
		amlvenc_h264_init_output_stream_buffer(wq->mem.BitstreamStart, wq->mem.BitstreamEnd);

		avc_prot_init(
			wq, request, request->quant,
			(request->cmd == ENCODER_IDR) ? true : false);

		amlvenc_h264_init_firmware_assist_buffer(wq->mem.assit_buffer_offset);

		enc_pr(LOG_INFO,
			"begin to new frame, request->cmd: %d, ucode mode: %d, wq:%p\n",
			request->cmd, request->ucode_mode, (void *)wq);
	}

	if ((request->cmd == ENCODER_IDR) ||
		(request->cmd == ENCODER_NON_IDR)) {
#ifdef H264_ENC_SVC
		/* encode non reference frame or not */
		if (request->cmd == ENCODER_IDR)
			wq->pic.non_ref_cnt = 0; //IDR reset counter

		if (wq->pic.enable_svc && wq->pic.non_ref_cnt) {
			enc_pr(LOG_INFO,
				"PIC is NON REF cmd %d cnt %d value %s\n",
				request->cmd, wq->pic.non_ref_cnt,
				"ENC_SLC_NON_REF");
			amlvenc_h264_configure_svc_pic(false);
		} else {
			enc_pr(LOG_INFO,
				"PIC is REF cmd %d cnt %d val %s\n",
				request->cmd, wq->pic.non_ref_cnt,
				"ENC_SLC_REF");
			amlvenc_h264_configure_svc_pic(true);			
		}
#else
		/* if FW defined but not defined SVC in driver here*/
		amlvenc_h264_configure_svc_pic(true);
#endif
		amlvenc_h264_init_dblk_buffer(wq->mem.dblk_buf_canvas);
		amlvenc_h264_init_input_reference_buffer(wq->mem.ref_buf_canvas);
	}
	if ((request->cmd == ENCODER_IDR) ||
		(request->cmd == ENCODER_NON_IDR))
		set_input_format(wq, request);

	if (request->cmd == ENCODER_IDR)
		ie_me_mb_type = HENC_MB_Type_I4MB;
	else if (request->cmd == ENCODER_NON_IDR)
		ie_me_mb_type =
			(HENC_SKIP_RUN_AUTO << 16) |
			(HENC_MB_Type_AUTO << 4) |
			(HENC_MB_Type_AUTO << 0);
	else
		ie_me_mb_type = 0;

	amlvenc_h264_configure_ie_me(ie_me_mb_type);
	amlvenc_h264_configure_fixed_slice(fixed_slice_cfg, wq->pic.rows_per_slice, wq->pic.encoder_height);

	encode_manager.encode_hw_status = request->cmd;
	wq->hw_status = request->cmd;
	amlvenc_hcodec_set_encoder_status(request->cmd);
	if ((request->cmd == ENCODER_IDR)
		|| (request->cmd == ENCODER_NON_IDR)
		|| (request->cmd == ENCODER_SEQUENCE)
		|| (request->cmd == ENCODER_PICTURE))
		encode_manager.process_irq = false;

	if (reload_flag)
		amvenc_start();
	enc_pr(LOG_ALL, "amvenc_avc_start cmd out, request:%p.\n", (void*)request);
}

static void dma_flush(u32 buf_start, u32 buf_size)
{
	if ((buf_start == 0) || (buf_size == 0))
		return;
	dma_sync_single_for_device(
		&encode_manager.this_pdev->dev, buf_start,
		buf_size, DMA_TO_DEVICE);
}

static void cache_flush(u32 buf_start, u32 buf_size)
{
	if ((buf_start == 0) || (buf_size == 0))
		return;
	dma_sync_single_for_cpu(
		&encode_manager.this_pdev->dev, buf_start,
		buf_size, DMA_FROM_DEVICE);
}

static u32 getbuffer(struct encode_wq_s *wq, u32 type)
{
	u32 ret = 0;

	switch (type) {
	case ENCODER_BUFFER_INPUT:
		ret = wq->mem.dct_buff_start_addr;
		break;
	case ENCODER_BUFFER_REF0:
		ret = wq->mem.dct_buff_start_addr +
			wq->mem.bufspec.dec0_y.buf_start;
		break;
	case ENCODER_BUFFER_REF1:
		ret = wq->mem.dct_buff_start_addr +
			wq->mem.bufspec.dec1_y.buf_start;
		break;
	case ENCODER_BUFFER_OUTPUT:
		ret = wq->mem.BitstreamStart;
		break;
	case ENCODER_BUFFER_DUMP:
		ret = wq->mem.dump_info_ddr_start_addr;
		break;
	case ENCODER_BUFFER_CBR:
		ret = wq->mem.cbr_info_ddr_start_addr;
		break;
	default:
		break;
	}
	return ret;
}

s32 amvenc_avc_start(struct encode_wq_s *wq, u32 clock)
{
	const char *p = select_ucode(encode_manager.ucode_index);

	avc_poweron(clock);

	avc_canvas_init(wq);

	amlvenc_hcodec_assist_enable();

	if (amvenc_loadmc(p, wq) < 0)
		return -EBUSY;
	encode_manager.need_reset = true;
	encode_manager.process_irq = false;
	encode_manager.encode_hw_status = ENCODER_IDLE;
	amvenc_reset();

	struct amlvenc_h264_init_encoder_params init_params = {
		.idr = true,
		.idr_pic_id = wq->pic.idr_pic_id,
		.init_qppicture = wq->pic.init_qppicture,
		.frame_number = wq->pic.frame_number,
		.log2_max_frame_num = wq->pic.log2_max_frame_num,
		.pic_order_cnt_lsb = wq->pic.pic_order_cnt_lsb,
		.log2_max_pic_order_cnt_lsb = wq->pic.log2_max_pic_order_cnt_lsb,
	};

	amlvenc_h264_init_encoder(&init_params);
	amlvenc_h264_init_input_dct_buffer(wq->mem.dct_buff_start_addr, wq->mem.dct_buff_end_addr);  /* dct buffer setting */
	amlvenc_h264_init_output_stream_buffer(wq->mem.BitstreamStart, wq->mem.BitstreamEnd);  /* output stream buffer */

	ie_me_mode = (0 & ME_PIXEL_MODE_MASK) << ME_PIXEL_MODE_SHIFT;
	avc_prot_init(wq, NULL, wq->pic.init_qppicture, true);
	if (request_irq(encode_manager.irq_num, enc_isr, IRQF_SHARED,
		"enc-irq", (void *)&encode_manager) == 0)
		encode_manager.irq_requested = true;
	else
		encode_manager.irq_requested = false;

	/* decoder buffer , need set before each frame start */
	amlvenc_h264_init_dblk_buffer(wq->mem.dblk_buf_canvas);
	/* reference  buffer , need set before each frame start */
	amlvenc_h264_init_input_reference_buffer(wq->mem.ref_buf_canvas);
	amlvenc_h264_init_firmware_assist_buffer(wq->mem.assit_buffer_offset); /* assitant buffer for microcode */

	amlvenc_h264_configure_ie_me(0);

	amlvenc_hcodec_clear_encoder_status();

	amlvenc_h264_configure_fixed_slice(fixed_slice_cfg, wq->pic.rows_per_slice, wq->pic.encoder_height);

	amvenc_start();

	return 0;
}

void amvenc_avc_stop(void)
{
	if ((encode_manager.irq_num >= 0) &&
		(encode_manager.irq_requested == true)) {
		encode_manager.irq_requested = false;
		free_irq(encode_manager.irq_num, &encode_manager);
	}
	amvenc_stop();
	avc_poweroff();
}

static s32 avc_init(struct encode_wq_s *wq)
{
	s32 r = 0;

	encode_manager.ucode_index = wq->ucode_index;
	r = amvenc_avc_start(wq, clock_level);

	enc_pr(LOG_DEBUG,
		"init avc encode. microcode %d, ret=%d, wq:%px\n",
		encode_manager.ucode_index, r, (void *)wq);
	return 0;
}

static s32 amvenc_avc_light_reset(struct encode_wq_s *wq, u32 value)
{
	s32 r = 0;

	amvenc_avc_stop();

	mdelay(value);

	//encode_manager.ucode_index = UCODE_MODE_FULL;
	encode_manager.ucode_index = wq->ucode_index;
	r = amvenc_avc_start(wq, clock_level);

	enc_pr(LOG_DEBUG,
		"amvenc_avc_light_reset finish, wq:%px, ret=%d\n",
		 (void *)wq, r);
	return r;
}

#ifdef CONFIG_CMA
static u32 checkCMA(void)
{
	u32 ret;

	if (encode_manager.cma_pool_size > 0) {
		ret = encode_manager.cma_pool_size;
		ret = ret / MIN_SIZE;
	} else
		ret = 0;
	return ret;
}
#endif

/* file operation */
static s32 amvenc_avc_open(struct inode *inode, struct file *file)
{
	s32 r = 0;
	struct encode_wq_s *wq = NULL;

	file->private_data = NULL;
	enc_pr(LOG_DEBUG, "avc open\n");

#ifdef CONFIG_AM_JPEG_ENCODER
	if (jpegenc_on() == true) {
		enc_pr(LOG_ERROR,
			"hcodec in use for JPEG Encode now.\n");
		return -EBUSY;
	}
#endif

#ifdef CONFIG_CMA
	if ((encode_manager.use_reserve == false) &&
	    (encode_manager.check_cma == false)) {
		encode_manager.max_instance = checkCMA();
		if (encode_manager.max_instance > 0) {
			enc_pr(LOG_DEBUG,
				"amvenc_avc  check CMA pool success, max instance: %d.\n",
				encode_manager.max_instance);
		} else {
			enc_pr(LOG_ERROR,
				"amvenc_avc CMA pool too small.\n");
		}
		encode_manager.check_cma = true;
	}
#endif

	wq = create_encode_work_queue();
	if (wq == NULL) {
		enc_pr(LOG_ERROR, "amvenc_avc create instance fail.\n");
		return -EBUSY;
	}

#ifdef CONFIG_CMA
	if (encode_manager.use_reserve == false) {
		wq->mem.buf_start = codec_mm_alloc_for_dma(ENCODE_NAME,
			MIN_SIZE  >> PAGE_SHIFT, 0,
			CODEC_MM_FLAGS_CPU);
		if (wq->mem.buf_start) {
			wq->mem.buf_size = MIN_SIZE;
			enc_pr(LOG_DEBUG,
				"allocating phys 0x%x, size %dk, wq:%p.\n",
				wq->mem.buf_start,
				wq->mem.buf_size >> 10, (void *)wq);
		} else {
			enc_pr(LOG_ERROR,
				"CMA failed to allocate dma buffer for %s, wq:%p.\n",
				encode_manager.this_pdev->name,
				(void *)wq);
			destroy_encode_work_queue(wq);
			return -ENOMEM;
		}
	}
#endif

	if (wq->mem.buf_start == 0 ||
		wq->mem.buf_size < MIN_SIZE) {
		enc_pr(LOG_ERROR,
			"alloc mem failed, start: 0x%x, size:0x%x, wq:%p.\n",
			wq->mem.buf_start,
			wq->mem.buf_size, (void *)wq);
		destroy_encode_work_queue(wq);
		return -ENOMEM;
	}

	memcpy(&wq->mem.bufspec, &amvenc_buffspec[0],
		sizeof(struct BuffInfo_s));

	enc_pr(LOG_DEBUG,
		"amvenc_avc  memory config success, buff start:0x%x, size is 0x%x, wq:%p.\n",
		wq->mem.buf_start, wq->mem.buf_size, (void *)wq);

	file->private_data = (void *) wq;
	return r;
}

static s32 amvenc_avc_release(struct inode *inode, struct file *file)
{
	struct encode_wq_s *wq = (struct encode_wq_s *)file->private_data;

	if (wq) {
		enc_pr(LOG_DEBUG, "avc release, wq:%p\n", (void *)wq);
		destroy_encode_work_queue(wq);
	}
	return 0;
}

static long amvenc_avc_ioctl(struct file *file, u32 cmd, ulong arg)
{
	long r = 0;
	u32 amrisc_cmd = 0;
	struct encode_wq_s *wq = (struct encode_wq_s *)file->private_data;
#define MAX_ADDR_INFO_SIZE 52
	u32 addr_info[MAX_ADDR_INFO_SIZE + 4];
	ulong argV;
	u32 buf_start;
	s32 canvas = -1;
	struct canvas_s dst;
	u32 cpuid;
	memset(&dst, 0, sizeof(struct canvas_s));
	switch (cmd) {
	case AMVENC_AVC_IOC_GET_ADDR:
		if ((wq->mem.ref_buf_canvas & 0xff) == (enc_canvas_offset))
			put_user(1, (u32 *)arg);
		else
			put_user(2, (u32 *)arg);
		break;
	case AMVENC_AVC_IOC_INPUT_UPDATE:
		break;
	case AMVENC_AVC_IOC_NEW_CMD:
		if (copy_from_user(addr_info, (void *)arg,
			MAX_ADDR_INFO_SIZE * sizeof(u32))) {
			enc_pr(LOG_ERROR,
				"avc get new cmd error, wq:%p.\n", (void *)wq);
			return -1;
		}
		r = convert_request(wq, addr_info);
		if (r == 0)
			r = encode_wq_add_request(wq);
		if (r) {
			enc_pr(LOG_ERROR,
				"avc add new request error, wq:%p.\n",
				(void *)wq);
		}
		break;
	case AMVENC_AVC_IOC_GET_STAGE:
		put_user(wq->hw_status, (u32 *)arg);
		break;
	case AMVENC_AVC_IOC_GET_OUTPUT_SIZE:
		addr_info[0] = wq->output_size;
		addr_info[1] = wq->me_weight;
		addr_info[2] = wq->i4_weight;
		addr_info[3] = wq->i16_weight;
		r = copy_to_user((u32 *)arg,
			addr_info, 4 * sizeof(u32));
		break;
	case AMVENC_AVC_IOC_CONFIG_INIT:
		if (copy_from_user(addr_info, (void *)arg,
			MAX_ADDR_INFO_SIZE * sizeof(u32))) {
			enc_pr(LOG_ERROR,
				"avc config init error, wq:%p.\n", (void *)wq);
			return -1;
		}
		//wq->ucode_index = UCODE_MODE_FULL;
		wq->ucode_index = addr_info[0];
#ifdef MULTI_SLICE_MC
		wq->pic.rows_per_slice = addr_info[1];
		enc_pr(LOG_DEBUG,
			"avc init -- rows_per_slice: %d, wq: %p.\n",
			wq->pic.rows_per_slice, (void *)wq);
#endif
		enc_pr(LOG_DEBUG,
			"avc init as mode %d, wq: %px.\n",
			wq->ucode_index, (void *)wq);

		if (is_oversize(addr_info[2],
			addr_info[3],
			wq->mem.bufspec.max_width * wq->mem.bufspec.max_height)) {
			enc_pr(LOG_ERROR,
				"avc config init- encode size %dx%d is larger than supported (%dx%d).  wq:%p.\n",
				addr_info[2], addr_info[3],
				wq->mem.bufspec.max_width,
				wq->mem.bufspec.max_height, (void *)wq);
			return -1;
		}

		wq->pic.encoder_width = addr_info[2];
		wq->pic.encoder_height = addr_info[3];
		enc_pr(LOG_INFO, "hwenc: AMVENC_AVC_IOC_CONFIG_INIT: w:%d, h:%d\n", wq->pic.encoder_width, wq->pic.encoder_height);

		wq->pic.color_space = addr_info[4];
		enc_pr(LOG_INFO, "hwenc: AMVENC_AVC_IOC_CONFIG_INIT, wq->pic.color_space=%#x\n", wq->pic.color_space);

		/*
		if (wq->pic.encoder_width *
			wq->pic.encoder_height >= 1280 * 720)
			clock_level = 6;
		else
			clock_level = 5;
		*/
		avc_buffspec_init(wq);
		complete(&encode_manager.event.request_in_com);
		addr_info[1] = wq->mem.bufspec.dct.buf_start;
		addr_info[2] = wq->mem.bufspec.dct.buf_size;
		addr_info[3] = wq->mem.bufspec.bitstream.buf_start;
		addr_info[4] = wq->mem.bufspec.bitstream.buf_size;
		addr_info[5] = wq->mem.bufspec.scale_buff.buf_start;
		addr_info[6] = wq->mem.bufspec.scale_buff.buf_size;
		addr_info[7] = wq->mem.bufspec.dump_info.buf_start;
		addr_info[8] = wq->mem.bufspec.dump_info.buf_size;
		addr_info[9] = wq->mem.bufspec.cbr_info.buf_start;
		addr_info[10] = wq->mem.bufspec.cbr_info.buf_size;
		r = copy_to_user((u32 *)arg, addr_info, 11*sizeof(u32));
		break;
	case AMVENC_AVC_IOC_FLUSH_CACHE:
		if (copy_from_user(addr_info, (void *)arg,
				   MAX_ADDR_INFO_SIZE * sizeof(u32))) {
			enc_pr(LOG_ERROR,
				"avc flush cache error, wq: %p.\n", (void *)wq);
			return -1;
		}
		buf_start = getbuffer(wq, addr_info[0]);
		dma_flush(buf_start + addr_info[1],
			addr_info[2] - addr_info[1]);
		break;
	case AMVENC_AVC_IOC_FLUSH_DMA:
		if (copy_from_user(addr_info, (void *)arg,
				MAX_ADDR_INFO_SIZE * sizeof(u32))) {
			enc_pr(LOG_ERROR,
				"avc flush dma error, wq:%p.\n", (void *)wq);
			return -1;
		}
		buf_start = getbuffer(wq, addr_info[0]);
		cache_flush(buf_start + addr_info[1],
			addr_info[2] - addr_info[1]);
		break;
	case AMVENC_AVC_IOC_GET_BUFFINFO:
		put_user(wq->mem.buf_size, (u32 *)arg);
		break;
	case AMVENC_AVC_IOC_GET_DEVINFO:
		if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXL) {
			/* send the same id as GXTVBB to upper*/
			r = copy_to_user((s8 *)arg, AMVENC_DEVINFO_GXTVBB,
				strlen(AMVENC_DEVINFO_GXTVBB));
		} else if (get_cpu_type() == MESON_CPU_MAJOR_ID_GXTVBB) {
			r = copy_to_user((s8 *)arg, AMVENC_DEVINFO_GXTVBB,
				strlen(AMVENC_DEVINFO_GXTVBB));
		} else if (get_cpu_type() == MESON_CPU_MAJOR_ID_GXBB) {
			r = copy_to_user((s8 *)arg, AMVENC_DEVINFO_GXBB,
				strlen(AMVENC_DEVINFO_GXBB));
		} else if (get_cpu_type() == MESON_CPU_MAJOR_ID_MG9TV) {
			r = copy_to_user((s8 *)arg, AMVENC_DEVINFO_G9,
				strlen(AMVENC_DEVINFO_G9));
		} else {
			r = copy_to_user((s8 *)arg, AMVENC_DEVINFO_M8,
				strlen(AMVENC_DEVINFO_M8));
		}
		break;
	case AMVENC_AVC_IOC_SUBMIT:
		get_user(amrisc_cmd, ((u32 *)arg));
		if (amrisc_cmd == ENCODER_IDR) {
			wq->pic.idr_pic_id++;
			if (wq->pic.idr_pic_id > 65535)
				wq->pic.idr_pic_id = 0;
			wq->pic.pic_order_cnt_lsb = 2;
			wq->pic.frame_number = 1;
		} else if (amrisc_cmd == ENCODER_NON_IDR) {
#ifdef H264_ENC_SVC
			/* only update when there is reference frame */
			if (wq->pic.enable_svc == 0 || wq->pic.non_ref_cnt == 0) {
				wq->pic.frame_number++;
				enc_pr(LOG_INFO, "Increase frame_num to %d\n",
					wq->pic.frame_number);
			}
#else
			wq->pic.frame_number++;
#endif

			wq->pic.pic_order_cnt_lsb += 2;
			if (wq->pic.frame_number > 65535)
				wq->pic.frame_number = 0;
		}
#ifdef H264_ENC_SVC
		/* only update when there is reference frame */
		if (wq->pic.enable_svc == 0 || wq->pic.non_ref_cnt == 0) {
			amrisc_cmd = wq->mem.dblk_buf_canvas;
			wq->mem.dblk_buf_canvas = wq->mem.ref_buf_canvas;
			/* current dblk buffer as next reference buffer */
			wq->mem.ref_buf_canvas = amrisc_cmd;
			enc_pr(LOG_INFO,
				"switch buffer enable %d  cnt %d\n",
				wq->pic.enable_svc, wq->pic.non_ref_cnt);
		}
		if (wq->pic.enable_svc) {
			wq->pic.non_ref_cnt ++;
			if (wq->pic.non_ref_cnt > wq->pic.non_ref_limit) {
				enc_pr(LOG_INFO, "Svc clear cnt %d conf %d\n",
					wq->pic.non_ref_cnt,
					wq->pic.non_ref_limit);
				wq->pic.non_ref_cnt = 0;
			} else
			enc_pr(LOG_INFO,"Svc increase non ref counter to %d\n",
				wq->pic.non_ref_cnt );
		}
#else
		amrisc_cmd = wq->mem.dblk_buf_canvas;
		wq->mem.dblk_buf_canvas = wq->mem.ref_buf_canvas;
		/* current dblk buffer as next reference buffer */
		wq->mem.ref_buf_canvas = amrisc_cmd;
#endif
		break;
	case AMVENC_AVC_IOC_READ_CANVAS:
		get_user(argV, ((u32 *)arg));
		canvas = argV;
		if (canvas & 0xff) {
			canvas_read(canvas & 0xff, &dst);
			addr_info[0] = dst.addr;
			if ((canvas & 0xff00) >> 8)
				canvas_read((canvas & 0xff00) >> 8, &dst);
			if ((canvas & 0xff0000) >> 16)
				canvas_read((canvas & 0xff0000) >> 16, &dst);
			addr_info[1] = dst.addr - addr_info[0] +
				dst.width * dst.height;
		} else {
			addr_info[0] = 0;
			addr_info[1] = 0;
		}
		dma_flush(dst.addr, dst.width * dst.height * 3 / 2);
		r = copy_to_user((u32 *)arg, addr_info, 2 * sizeof(u32));
		break;
	case AMVENC_AVC_IOC_MAX_INSTANCE:
		put_user(encode_manager.max_instance, (u32 *)arg);
		break;
	case AMVENC_AVC_IOC_QP_MODE:
		get_user(qp_mode, ((u32 *)arg));
		enc_pr(LOG_INFO, "qp_mode %d\n", qp_mode);
		break;
	case AMVENC_AVC_IOC_GET_CPU_ID:
		cpuid = (u32) get_cpu_major_id();
		enc_pr(LOG_INFO, "AMVENC_AVC_IOC_GET_CPU_ID return %u\n", cpuid);
		put_user(cpuid, (u32 *)arg);
		break;
	default:
		r = -1;
		break;
	}
	return r;
}

#ifdef CONFIG_COMPAT
static long amvenc_avc_compat_ioctl(struct file *filp,
	unsigned int cmd, unsigned long args)
{
	unsigned long ret;

	args = (unsigned long)compat_ptr(args);
	ret = amvenc_avc_ioctl(filp, cmd, args);
	return ret;
}
#endif

static s32 avc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct encode_wq_s *wq = (struct encode_wq_s *)filp->private_data;
	ulong off = vma->vm_pgoff << PAGE_SHIFT;
	ulong vma_size = vma->vm_end - vma->vm_start;

	if (vma_size == 0) {
		enc_pr(LOG_ERROR, "vma_size is 0, wq:%p.\n", (void *)wq);
		return -EAGAIN;
	}
	if (!off)
		off += wq->mem.buf_start;
	enc_pr(LOG_ALL,
		"vma_size is %ld , off is %ld, wq:%p.\n",
		vma_size, off, (void *)wq);
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_IO;
	/* vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); */
	if (remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		enc_pr(LOG_ERROR,
			"set_cached: failed remap_pfn_range, wq:%p.\n",
			(void *)wq);
		return -EAGAIN;
	}
	return 0;
}

static u32 amvenc_avc_poll(struct file *file, poll_table *wait_table)
{
	struct encode_wq_s *wq = (struct encode_wq_s *)file->private_data;

	poll_wait(file, &wq->request_complete, wait_table);

	if (atomic_read(&wq->request_ready)) {
		atomic_dec(&wq->request_ready);
		return POLLIN | POLLRDNORM;
	}
	return 0;
}

static const struct file_operations amvenc_avc_fops = {
	.owner = THIS_MODULE,
	.open = amvenc_avc_open,
	.mmap = avc_mmap,
	.release = amvenc_avc_release,
	.unlocked_ioctl = amvenc_avc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = amvenc_avc_compat_ioctl,
#endif
	.poll = amvenc_avc_poll,
};

/* work queue function */
static s32 encode_process_request(struct encode_manager_s *manager,
	struct encode_queue_item_s *pitem)
{
	s32 ret = 0;
	struct encode_wq_s *wq = pitem->request.parent;
	struct encode_request_s *request = &pitem->request;
	u32 timeout = (request->timeout == 0) ?
		1 : msecs_to_jiffies(request->timeout);
	u32 buf_start = 0;
	u32 size = 0;
	u32 flush_size = ((wq->pic.encoder_width + 31) >> 5 << 5) *
		((wq->pic.encoder_height + 15) >> 4 << 4) * 3 / 2;

	struct enc_dma_cfg *cfg = NULL;
	int i = 0;

#ifdef H264_ENC_CBR
	if (request->cmd == ENCODER_IDR || request->cmd == ENCODER_NON_IDR) {
		if (request->flush_flag & AMVENC_FLUSH_FLAG_CBR
			&& get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB) {
			void *vaddr = wq->mem.cbr_info_ddr_virt_addr;
			ConvertTable2Risc(vaddr, 0xa00);
			//buf_start = getbuffer(wq, ENCODER_BUFFER_CBR);
			codec_mm_dma_flush(vaddr, wq->mem.cbr_info_ddr_size, DMA_TO_DEVICE);
		}
	}
#endif



Again:
	amvenc_avc_start_cmd(wq, request);

	if (no_timeout) {
		wait_event_interruptible(manager->event.hw_complete,
			(manager->encode_hw_status == ENCODER_IDR_DONE
			|| manager->encode_hw_status == ENCODER_NON_IDR_DONE
			|| manager->encode_hw_status == ENCODER_SEQUENCE_DONE
			|| manager->encode_hw_status == ENCODER_PICTURE_DONE));
	} else {
		wait_event_interruptible_timeout(manager->event.hw_complete,
			((manager->encode_hw_status == ENCODER_IDR_DONE)
			|| (manager->encode_hw_status == ENCODER_NON_IDR_DONE)
			|| (manager->encode_hw_status == ENCODER_SEQUENCE_DONE)
			|| (manager->encode_hw_status == ENCODER_PICTURE_DONE)),
			timeout);
	}

	if ((request->cmd == ENCODER_SEQUENCE) &&
	    (manager->encode_hw_status == ENCODER_SEQUENCE_DONE)) {
		wq->sps_size = amlvenc_hcodec_vlc_total_bytes();
		wq->hw_status = manager->encode_hw_status;
		request->cmd = ENCODER_PICTURE;
		goto Again;
	} else if ((request->cmd == ENCODER_PICTURE) &&
		   (manager->encode_hw_status == ENCODER_PICTURE_DONE)) {
		wq->pps_size =
			amlvenc_hcodec_vlc_total_bytes() - wq->sps_size;
		wq->hw_status = manager->encode_hw_status;
		if (request->flush_flag & AMVENC_FLUSH_FLAG_OUTPUT) {
			buf_start = getbuffer(wq, ENCODER_BUFFER_OUTPUT);
			cache_flush(buf_start,
				wq->sps_size + wq->pps_size);
		}
		wq->output_size = (wq->sps_size << 16) | wq->pps_size;
	} else {
		wq->hw_status = manager->encode_hw_status;

		if ((manager->encode_hw_status == ENCODER_IDR_DONE) ||
		    (manager->encode_hw_status == ENCODER_NON_IDR_DONE)) {
			wq->output_size = amlvenc_hcodec_vlc_total_bytes();

			if (request->flush_flag & AMVENC_FLUSH_FLAG_OUTPUT) {
				buf_start = getbuffer(wq, ENCODER_BUFFER_OUTPUT);
				cache_flush(buf_start, wq->output_size);
			}

			if (request->flush_flag & AMVENC_FLUSH_FLAG_DUMP) {
				buf_start = getbuffer(wq, ENCODER_BUFFER_DUMP);
				size = wq->mem.dump_info_ddr_size;
				cache_flush(buf_start, size);
				//enc_pr(LOG_DEBUG, "CBR flush dump_info done");
			}

			if (request->flush_flag & AMVENC_FLUSH_FLAG_REFERENCE) {
				u32 ref_id = ENCODER_BUFFER_REF0;

				if ((wq->mem.ref_buf_canvas & 0xff) == (enc_canvas_offset))
					ref_id = ENCODER_BUFFER_REF0;
				else
					ref_id = ENCODER_BUFFER_REF1;

				buf_start = getbuffer(wq, ref_id);
				cache_flush(buf_start, flush_size);
			}
		} else {
			manager->encode_hw_status = ENCODER_ERROR;
			enc_pr(LOG_DEBUG, "avc encode light reset --- ");
			enc_pr(LOG_DEBUG,
				"frame type: %s, size: %dx%d, wq: %px\n",
				(request->cmd == ENCODER_IDR) ? "IDR" : "P",
				wq->pic.encoder_width,
				wq->pic.encoder_height, (void *)wq);
			enc_pr(LOG_DEBUG,
				"mb info: 0x%x, encode status: 0x%x, dct status: 0x%x ",
				amlvenc_hcodec_mb_info(),
				amlvenc_hcodec_encoder_status(),
				amlvenc_hcodec_qdct_status());
			enc_pr(LOG_DEBUG,
				"vlc status: 0x%x, me status: 0x%x, risc pc:0x%x, debug:0x%x\n",
				amlvenc_hcodec_vlc_status(),
				amlvenc_hcodec_me_status(),
				amlvenc_hcodec_mpc_risc(),
				amlvenc_hcodec_debug());
			amvenc_avc_light_reset(wq, 30);
		}

		for (i = 0; i < request->plane_num; i++) {
			cfg = &request->dma_cfg[i];
			enc_pr(LOG_INFO, "request vaddr %p, paddr %p\n",
				cfg->vaddr, cfg->paddr);
			if (cfg->fd >= 0 && cfg->paddr != NULL)
				enc_dma_buf_unmap(cfg);
		}
	}
	atomic_inc(&wq->request_ready);
	wake_up_interruptible(&wq->request_complete);
	return ret;
}

s32 encode_wq_add_request(struct encode_wq_s *wq)
{
	struct encode_queue_item_s *pitem = NULL;
	struct list_head *head = NULL;
	struct encode_wq_s *tmp = NULL;
	bool find = false;

	spin_lock(&encode_manager.event.sem_lock);

	head =  &encode_manager.wq;
	list_for_each_entry(tmp, head, list) {
		if ((wq == tmp) && (wq != NULL)) {
			find = true;
			break;
		}
	}

	if (find == false) {
		enc_pr(LOG_ERROR, "current wq (%p) doesn't register.\n",
			(void *)wq);
		goto error;
	}

	if (list_empty(&encode_manager.free_queue)) {
		enc_pr(LOG_ERROR, "work queue no space, wq:%p.\n",
			(void *)wq);
		goto error;
	}

	pitem = list_entry(encode_manager.free_queue.next,
		struct encode_queue_item_s, list);

	if (IS_ERR(pitem))
		goto error;

	memcpy(&pitem->request, &wq->request, sizeof(struct encode_request_s));

	enc_pr(LOG_INFO, "new work request %p, vaddr %p, paddr %p\n", &pitem->request,
		pitem->request.dma_cfg[0].vaddr,pitem->request.dma_cfg[0].paddr);

	memset(&wq->request, 0, sizeof(struct encode_request_s));
	wq->request.dma_cfg[0].fd = -1;
	wq->request.dma_cfg[1].fd = -1;
	wq->request.dma_cfg[2].fd = -1;
	wq->hw_status = 0;
	wq->output_size = 0;
	pitem->request.parent = wq;
	list_move_tail(&pitem->list, &encode_manager.process_queue);
	spin_unlock(&encode_manager.event.sem_lock);

	enc_pr(LOG_INFO,
		"add new work ok, cmd:%d, ucode mode: %d, wq:%p.\n",
		pitem->request.cmd, pitem->request.ucode_mode,
		(void *)wq);
	complete(&encode_manager.event.request_in_com);/* new cmd come in */

	return 0;
error:
	spin_unlock(&encode_manager.event.sem_lock);

	return -1;
}

struct encode_wq_s *create_encode_work_queue(void)
{
	struct encode_wq_s *encode_work_queue = NULL;
	bool done = false;
	u32 i, max_instance;
	struct Buff_s *reserve_buff;

	encode_work_queue = kzalloc(sizeof(struct encode_wq_s), GFP_KERNEL);
	if (IS_ERR(encode_work_queue)) {
		enc_pr(LOG_ERROR, "can't create work queue\n");
		return NULL;
	}
	max_instance = encode_manager.max_instance;
	encode_work_queue->pic.init_qppicture = 26;
	encode_work_queue->pic.log2_max_frame_num = 4;
	encode_work_queue->pic.log2_max_pic_order_cnt_lsb = 4;
	encode_work_queue->pic.idr_pic_id = 0;
	encode_work_queue->pic.frame_number = 0;
	encode_work_queue->pic.pic_order_cnt_lsb = 0;
#ifdef H264_ENC_SVC
	/* Get settings from the global*/
	encode_work_queue->pic.enable_svc = svc_enable;
	encode_work_queue->pic.non_ref_limit = svc_ref_conf;
	encode_work_queue->pic.non_ref_cnt = 0;
	enc_pr(LOG_INFO, "svc conf enable %d, duration %d\n",
		encode_work_queue->pic.enable_svc,
		encode_work_queue->pic.non_ref_limit);
#endif
	encode_work_queue->ucode_index = UCODE_MODE_FULL;

#ifdef H264_ENC_CBR
	encode_work_queue->cbr_info.block_w = 16;
	encode_work_queue->cbr_info.block_h = 9;
	encode_work_queue->cbr_info.long_th = CBR_LONG_THRESH;
	encode_work_queue->cbr_info.start_tbl_id = START_TABLE_ID;
	encode_work_queue->cbr_info.short_shift = CBR_SHORT_SHIFT;
	encode_work_queue->cbr_info.long_mb_num = CBR_LONG_MB_NUM;
#endif
	init_waitqueue_head(&encode_work_queue->request_complete);
	atomic_set(&encode_work_queue->request_ready, 0);
	spin_lock(&encode_manager.event.sem_lock);
	if (encode_manager.wq_count < encode_manager.max_instance) {
		list_add_tail(&encode_work_queue->list, &encode_manager.wq);
		encode_manager.wq_count++;
		if (encode_manager.use_reserve == true) {
			for (i = 0; i < max_instance; i++) {
				reserve_buff = &encode_manager.reserve_buff[i];
				if (reserve_buff->used == false) {
					encode_work_queue->mem.buf_start =
						reserve_buff->buf_start;
					encode_work_queue->mem.buf_size =
						reserve_buff->buf_size;
					reserve_buff->used = true;
					done = true;
					break;
				}
			}
		} else
			done = true;
	}
	spin_unlock(&encode_manager.event.sem_lock);
	if (done == false) {
		kfree(encode_work_queue);
		encode_work_queue = NULL;
		enc_pr(LOG_ERROR, "too many work queue!\n");
	}
	return encode_work_queue; /* find it */
}

static void _destroy_encode_work_queue(struct encode_manager_s *manager,
	struct encode_wq_s **wq,
	struct encode_wq_s *encode_work_queue,
	bool *find)
{
	struct list_head *head;
	struct encode_wq_s *wp_tmp = NULL;
	u32 i, max_instance;
	struct Buff_s *reserve_buff;
	u32 buf_start = encode_work_queue->mem.buf_start;

	max_instance = manager->max_instance;
	head =  &manager->wq;
	list_for_each_entry_safe((*wq), wp_tmp, head, list) {
		if ((*wq) && (*wq == encode_work_queue)) {
			list_del(&(*wq)->list);
			if (manager->use_reserve == true) {
				for (i = 0; i < max_instance; i++) {
					reserve_buff =
						&manager->reserve_buff[i];
					if (reserve_buff->used	== true &&
						buf_start ==
						reserve_buff->buf_start) {
						reserve_buff->used = false;
						break;
					}
				}
			}
			*find = true;
			manager->wq_count--;
			enc_pr(LOG_DEBUG,
				"remove  encode_work_queue %p success, %s line %d.\n",
				(void *)encode_work_queue,
				__func__, __LINE__);
			break;
		}
	}
}

s32 destroy_encode_work_queue(struct encode_wq_s *encode_work_queue)
{
	struct encode_queue_item_s *pitem, *tmp;
	struct encode_wq_s *wq = NULL;
	bool find = false;

	struct list_head *head;

	if (encode_work_queue) {
		spin_lock(&encode_manager.event.sem_lock);
		if (encode_manager.current_wq == encode_work_queue) {
			encode_manager.remove_flag = true;
			spin_unlock(&encode_manager.event.sem_lock);
			enc_pr(LOG_DEBUG,
				"warning--Destroy the running queue, should not be here.\n");
			wait_for_completion(
				&encode_manager.event.process_complete);
			spin_lock(&encode_manager.event.sem_lock);
		} /* else we can delete it safely. */

		head =  &encode_manager.process_queue;
		list_for_each_entry_safe(pitem, tmp, head, list) {
			if (pitem && pitem->request.parent ==
				encode_work_queue) {
				pitem->request.parent = NULL;
				enc_pr(LOG_DEBUG,
					"warning--remove not process request, should not be here.\n");
				list_move_tail(&pitem->list,
					&encode_manager.free_queue);
			}
		}

		_destroy_encode_work_queue(&encode_manager, &wq,
			encode_work_queue, &find);
		spin_unlock(&encode_manager.event.sem_lock);
#ifdef CONFIG_CMA
		if (encode_work_queue->mem.buf_start) {
			if (wq->mem.cbr_info_ddr_virt_addr != NULL) {
				codec_mm_unmap_phyaddr(wq->mem.cbr_info_ddr_virt_addr);
				wq->mem.cbr_info_ddr_virt_addr = NULL;
			}
			codec_mm_free_for_dma(
				ENCODE_NAME,
				encode_work_queue->mem.buf_start);
			encode_work_queue->mem.buf_start = 0;

		}
#endif
		kfree(encode_work_queue);
		complete(&encode_manager.event.request_in_com);
	}
	return  0;
}

static s32 encode_monitor_thread(void *data)
{
	struct encode_manager_s *manager = (struct encode_manager_s *)data;
	struct encode_queue_item_s *pitem = NULL;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1 };
	s32 ret = 0;

	enc_pr(LOG_DEBUG, "encode workqueue monitor start.\n");
	sched_setscheduler(current, SCHED_FIFO, &param);
	allow_signal(SIGTERM);

	/* setup current_wq here. */
	while (manager->process_queue_state != ENCODE_PROCESS_QUEUE_STOP) {
		if (kthread_should_stop())
			break;

		ret = wait_for_completion_interruptible(
				&manager->event.request_in_com);

		if (ret == -ERESTARTSYS)
			break;

		if (kthread_should_stop())
			break;

		if (manager->inited == false) {
			spin_lock(&manager->event.sem_lock);

			if (!list_empty(&manager->wq)) {
				struct encode_wq_s *first_wq =
					list_entry(manager->wq.next,
					struct encode_wq_s, list);
				manager->current_wq = first_wq;
				spin_unlock(&manager->event.sem_lock);

				if (first_wq) {
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
					if (!manager->context)
						manager->context =
						create_ge2d_work_queue();
#endif
					avc_init(first_wq);
					manager->inited = true;
				}
				spin_lock(&manager->event.sem_lock);
				manager->current_wq = NULL;
				spin_unlock(&manager->event.sem_lock);
				if (manager->remove_flag) {
					complete(
						&manager
						->event.process_complete);
					manager->remove_flag = false;
				}
			} else
				spin_unlock(&manager->event.sem_lock);
			continue;
		}

		spin_lock(&manager->event.sem_lock);
		pitem = NULL;

		if (list_empty(&manager->wq)) {
			spin_unlock(&manager->event.sem_lock);
			manager->inited = false;
			amvenc_avc_stop();

#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
			if (manager->context) {
				destroy_ge2d_work_queue(manager->context);
				manager->context = NULL;
			}
#endif

			enc_pr(LOG_DEBUG, "power off encode.\n");
			continue;
		} else if (!list_empty(&manager->process_queue)) {
			pitem = list_entry(manager->process_queue.next,
				struct encode_queue_item_s, list);
			list_del(&pitem->list);
			manager->current_item = pitem;
			manager->current_wq = pitem->request.parent;
		}

		spin_unlock(&manager->event.sem_lock);

		if (pitem) {
			encode_process_request(manager, pitem);
			spin_lock(&manager->event.sem_lock);
			list_add_tail(&pitem->list, &manager->free_queue);
			manager->current_item = NULL;
			manager->last_wq = manager->current_wq;
			manager->current_wq = NULL;
			spin_unlock(&manager->event.sem_lock);
		}

		if (manager->remove_flag) {
			complete(&manager->event.process_complete);
			manager->remove_flag = false;
		}
	}
	while (!kthread_should_stop())
		msleep(20);

	enc_pr(LOG_DEBUG, "exit encode_monitor_thread.\n");
	return 0;
}

static s32 encode_start_monitor(void)
{
	s32 ret = 0;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB) {
		y_tnr.mot2alp_nrm_gain = 216;
		y_tnr.mot2alp_dis_gain = 144;
		c_tnr.mot2alp_nrm_gain = 216;
		c_tnr.mot2alp_dis_gain = 144;
	} else {
		/* more tnr */
		y_tnr.mot2alp_nrm_gain = 144;
		y_tnr.mot2alp_dis_gain = 96;
		c_tnr.mot2alp_nrm_gain = 144;
		c_tnr.mot2alp_dis_gain = 96;
	}

	enc_pr(LOG_DEBUG, "encode start monitor.\n");
	encode_manager.process_queue_state = ENCODE_PROCESS_QUEUE_START;
	encode_manager.encode_thread = kthread_run(encode_monitor_thread,
		&encode_manager, "encode_monitor");
	if (IS_ERR(encode_manager.encode_thread)) {
		ret = PTR_ERR(encode_manager.encode_thread);
		encode_manager.process_queue_state = ENCODE_PROCESS_QUEUE_STOP;
		enc_pr(LOG_ERROR,
			"encode monitor : failed to start kthread (%d)\n", ret);
	}
	return ret;
}

static s32  encode_stop_monitor(void)
{
	enc_pr(LOG_DEBUG, "stop encode monitor thread\n");
	if (encode_manager.encode_thread) {
		spin_lock(&encode_manager.event.sem_lock);
		if (!list_empty(&encode_manager.wq)) {
			u32 count = encode_manager.wq_count;

			spin_unlock(&encode_manager.event.sem_lock);
			enc_pr(LOG_ERROR,
				"stop encode monitor thread error, active wq (%d) is not 0.\n",
				 count);
			return -1;
		}
		spin_unlock(&encode_manager.event.sem_lock);
		encode_manager.process_queue_state = ENCODE_PROCESS_QUEUE_STOP;
		send_sig(SIGTERM, encode_manager.encode_thread, 1);
		complete(&encode_manager.event.request_in_com);
		kthread_stop(encode_manager.encode_thread);
		encode_manager.encode_thread = NULL;
		kfree(mc_addr);
		mc_addr = NULL;
	}
	return  0;
}

static s32 encode_wq_init(void)
{
	u32 i = 0;
	struct encode_queue_item_s *pitem = NULL;

	enc_pr(LOG_DEBUG, "encode_wq_init.\n");

	spin_lock_init(&encode_manager.event.sem_lock);
	spin_lock(&encode_manager.event.sem_lock);
	encode_manager.irq_requested = false;
	init_completion(&encode_manager.event.request_in_com);
	init_waitqueue_head(&encode_manager.event.hw_complete);
	init_completion(&encode_manager.event.process_complete);
	INIT_LIST_HEAD(&encode_manager.process_queue);
	INIT_LIST_HEAD(&encode_manager.free_queue);
	INIT_LIST_HEAD(&encode_manager.wq);

	tasklet_init(&encode_manager.encode_tasklet,
		encode_isr_tasklet,
		(ulong)&encode_manager);

	spin_unlock(&encode_manager.event.sem_lock);
	for (i = 0; i < MAX_ENCODE_REQUEST; i++) {
		pitem = kcalloc(1,
			sizeof(struct encode_queue_item_s),
			GFP_KERNEL);
		if (IS_ERR(pitem)) {
			enc_pr(LOG_ERROR, "can't request queue item memory.\n");
			return -1;
		}
		pitem->request.parent = NULL;
		list_add_tail(&pitem->list, &encode_manager.free_queue);
	}
	spin_lock(&encode_manager.event.sem_lock);
	encode_manager.current_wq = NULL;
	encode_manager.last_wq = NULL;
	encode_manager.encode_thread = NULL;
	encode_manager.current_item = NULL;
	encode_manager.wq_count = 0;
	encode_manager.remove_flag = false;
	spin_unlock(&encode_manager.event.sem_lock);

	if (is_support_vdec_canvas())
		enc_canvas_offset = ENC_CANVAS_OFFSET;
	else
		enc_canvas_offset = AMVENC_CANVAS_INDEX;

	amlvenc_h264_init_me(&me);
	if (encode_start_monitor()) {
		enc_pr(LOG_ERROR, "encode create thread error.\n");
		return -1;
	}
	return 0;
}

static s32 encode_wq_uninit(void)
{
	struct encode_queue_item_s *pitem, *tmp;
	struct list_head *head;
	u32 count = 0;
	s32 r = -1;

	enc_pr(LOG_DEBUG, "uninit encode wq.\n");
	if (encode_stop_monitor() == 0) {
		if ((encode_manager.irq_num >= 0) &&
			(encode_manager.irq_requested == true)) {
			free_irq(encode_manager.irq_num, &encode_manager);
			encode_manager.irq_requested = false;
		}
		spin_lock(&encode_manager.event.sem_lock);
		head =  &encode_manager.process_queue;
		list_for_each_entry_safe(pitem, tmp, head, list) {
			if (pitem) {
				list_del(&pitem->list);
				kfree(pitem);
				count++;
			}
		}
		head =  &encode_manager.free_queue;
		list_for_each_entry_safe(pitem, tmp, head, list) {
			if (pitem) {
				list_del(&pitem->list);
				kfree(pitem);
				count++;
			}
		}
		spin_unlock(&encode_manager.event.sem_lock);
		if (count == MAX_ENCODE_REQUEST)
			r = 0;
		else {
			enc_pr(LOG_ERROR, "lost some request item %d.\n",
				MAX_ENCODE_REQUEST - count);
		}
	}
	return  r;
}

static ssize_t encode_status_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	u32 process_count = 0;
	u32 free_count = 0;
	struct encode_queue_item_s *pitem = NULL;
	struct encode_wq_s *current_wq = NULL;
	struct encode_wq_s *last_wq = NULL;
	struct list_head *head = NULL;
	s32 irq_num = 0;
	u32 hw_status = 0;
	u32 process_queue_state = 0;
	u32 wq_count = 0;
	u32 ucode_index;
	bool need_reset;
	bool process_irq;
	bool inited;
	bool use_reserve;
	struct Buff_s reserve_mem;
	u32 max_instance;
#ifdef CONFIG_CMA
	bool check_cma = false;
#endif

	spin_lock(&encode_manager.event.sem_lock);
	head = &encode_manager.free_queue;
	list_for_each_entry(pitem, head, list) {
		free_count++;
		if (free_count > MAX_ENCODE_REQUEST)
			break;
	}

	head = &encode_manager.process_queue;
	list_for_each_entry(pitem, head, list) {
		process_count++;
		if (process_count > MAX_ENCODE_REQUEST)
			break;
	}

	current_wq = encode_manager.current_wq;
	last_wq = encode_manager.last_wq;
	pitem = encode_manager.current_item;
	irq_num = encode_manager.irq_num;
	hw_status = encode_manager.encode_hw_status;
	process_queue_state = encode_manager.process_queue_state;
	wq_count = encode_manager.wq_count;
	ucode_index = encode_manager.ucode_index;
	need_reset = encode_manager.need_reset;
	process_irq = encode_manager.process_irq;
	inited = encode_manager.inited;
	use_reserve = encode_manager.use_reserve;
	reserve_mem.buf_start = encode_manager.reserve_mem.buf_start;
	reserve_mem.buf_size = encode_manager.reserve_mem.buf_size;

	max_instance = encode_manager.max_instance;
#ifdef CONFIG_CMA
	check_cma = encode_manager.check_cma;
#endif

	spin_unlock(&encode_manager.event.sem_lock);

	enc_pr(LOG_DEBUG,
		"encode process queue count: %d, free queue count: %d.\n",
		process_count, free_count);
	enc_pr(LOG_DEBUG,
		"encode curent wq: %p, last wq: %p, wq count: %d, max_instance: %d.\n",
		current_wq, last_wq, wq_count, max_instance);
	if (current_wq)
		enc_pr(LOG_DEBUG,
			"encode curent wq -- encode width: %d, encode height: %d.\n",
			current_wq->pic.encoder_width,
			current_wq->pic.encoder_height);
	enc_pr(LOG_DEBUG,
		"encode curent pitem: %p, ucode_index: %d, hw_status: %d, need_reset: %s, process_irq: %s.\n",
		pitem, ucode_index, hw_status, need_reset ? "true" : "false",
		process_irq ? "true" : "false");
	enc_pr(LOG_DEBUG,
		"encode irq num: %d,  inited: %s, process_queue_state: %d.\n",
		irq_num, inited ? "true" : "false",  process_queue_state);
	if (use_reserve) {
		enc_pr(LOG_DEBUG,
			"encode use reserve memory, buffer start: 0x%x, size: %d MB.\n",
			 reserve_mem.buf_start,
			 reserve_mem.buf_size / SZ_1M);
	} else {
#ifdef CONFIG_CMA
		enc_pr(LOG_DEBUG, "encode check cma: %s.\n",
			check_cma ? "true" : "false");
#endif
	}
	return snprintf(buf, 40, "encode max instance: %d\n", max_instance);
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,13,1)
static struct class_attribute amvenc_class_attrs[] = {
	__ATTR(encode_status,
	S_IRUGO | S_IWUSR,
	encode_status_show,
	NULL),
	__ATTR_NULL
};

static struct class amvenc_avc_class = {
	.name = CLASS_NAME,
	.class_attrs = amvenc_class_attrs,
};
#else /* LINUX_VERSION_CODE <= KERNEL_VERSION(4,13,1)  */
static CLASS_ATTR_RO(encode_status);

static struct attribute *amvenc_avc_class_attrs[] = {
	&class_attr_encode_status.attr,
	NULL
};

ATTRIBUTE_GROUPS(amvenc_avc_class);

static struct class amvenc_avc_class = {
	.name = CLASS_NAME,
	.class_groups = amvenc_avc_class_groups,
};
#endif /* LINUX_VERSION_CODE <= KERNEL_VERSION(4,13,1)  */
s32 init_avc_device(void)
{
	s32  r = 0;

	r = register_chrdev(0, DEVICE_NAME, &amvenc_avc_fops);
	if (r <= 0) {
		enc_pr(LOG_ERROR, "register amvenc_avc device error.\n");
		return  r;
	}
	avc_device_major = r;

	r = class_register(&amvenc_avc_class);
	if (r < 0) {
		enc_pr(LOG_ERROR, "error create amvenc_avc class.\n");
		return r;
	}

	amvenc_avc_dev = device_create(&amvenc_avc_class, NULL,
		MKDEV(avc_device_major, 0), NULL,
		DEVICE_NAME);

	if (IS_ERR(amvenc_avc_dev)) {
		enc_pr(LOG_ERROR, "create amvenc_avc device error.\n");
		class_unregister(&amvenc_avc_class);
		return -1;
	}
	return r;
}

s32 uninit_avc_device(void)
{
	if (amvenc_avc_dev)
		device_destroy(&amvenc_avc_class, MKDEV(avc_device_major, 0));

	class_destroy(&amvenc_avc_class);

	unregister_chrdev(avc_device_major, DEVICE_NAME);
	return 0;
}

static s32 avc_mem_device_init(struct reserved_mem *rmem, struct device *dev)
{
	s32 r;
	struct resource res;

	if (!rmem) {
		enc_pr(LOG_ERROR,
			"Can not obtain I/O memory, and will allocate avc buffer!\n");
		r = -EFAULT;
		return r;
	}
	res.start = (phys_addr_t)rmem->base;
	res.end = res.start + (phys_addr_t)rmem->size - 1;
	encode_manager.reserve_mem.buf_start = res.start;
	encode_manager.reserve_mem.buf_size = res.end - res.start + 1;

	if (encode_manager.reserve_mem.buf_size >=
		amvenc_buffspec[0].min_buffsize) {
		encode_manager.max_instance =
			encode_manager.reserve_mem.buf_size /
			amvenc_buffspec[0].min_buffsize;
		if (encode_manager.max_instance > MAX_ENCODE_INSTANCE)
			encode_manager.max_instance = MAX_ENCODE_INSTANCE;
		encode_manager.reserve_buff = kzalloc(
			encode_manager.max_instance *
			sizeof(struct Buff_s), GFP_KERNEL);
		if (encode_manager.reserve_buff) {
			u32 i;
			struct Buff_s *reserve_buff;
			u32 max_instance = encode_manager.max_instance;

			for (i = 0; i < max_instance; i++) {
				reserve_buff = &encode_manager.reserve_buff[i];
				reserve_buff->buf_start =
					i *
					amvenc_buffspec[0]
					.min_buffsize +
					encode_manager.reserve_mem.buf_start;
				reserve_buff->buf_size =
					encode_manager.reserve_mem.buf_start;
				reserve_buff->used = false;
			}
			encode_manager.use_reserve = true;
			r = 0;
			enc_pr(LOG_DEBUG,
				"amvenc_avc  use reserve memory, buff start: 0x%x, size: 0x%x, max instance is %d\n",
				encode_manager.reserve_mem.buf_start,
				encode_manager.reserve_mem.buf_size,
				encode_manager.max_instance);
		} else {
			enc_pr(LOG_ERROR,
				"amvenc_avc alloc reserve buffer pointer fail. max instance is %d.\n",
				encode_manager.max_instance);
			encode_manager.max_instance = 0;
			encode_manager.reserve_mem.buf_start = 0;
			encode_manager.reserve_mem.buf_size = 0;
			r = -ENOMEM;
		}
	} else {
		enc_pr(LOG_ERROR,
			"amvenc_avc memory resource too small, size is 0x%x. Need 0x%x bytes at least.\n",
			encode_manager.reserve_mem.buf_size,
			amvenc_buffspec[0]
			.min_buffsize);
		encode_manager.reserve_mem.buf_start = 0;
		encode_manager.reserve_mem.buf_size = 0;
		r = -ENOMEM;
	}
	return r;
}

static s32 amvenc_avc_probe(struct platform_device *pdev)
{
	/* struct resource mem; */
	s32 res_irq;
	s32 idx;
	s32 r;

	enc_pr(LOG_INFO, "amvenc_avc probe start.\n");

	encode_manager.this_pdev = pdev;
#ifdef CONFIG_CMA
	encode_manager.check_cma = false;
#endif
	encode_manager.reserve_mem.buf_start = 0;
	encode_manager.reserve_mem.buf_size = 0;
	encode_manager.use_reserve = false;
	encode_manager.max_instance = 0;
	encode_manager.reserve_buff = NULL;

	idx = of_reserved_mem_device_init(&pdev->dev);

	if (idx != 0) {
		enc_pr(LOG_DEBUG,
			"amvenc_avc_probe -- reserved memory config fail.\n");
	}


	if (encode_manager.use_reserve == false) {
#ifndef CONFIG_CMA
		enc_pr(LOG_ERROR,
			"amvenc_avc memory is invalid, probe fail!\n");
		return -EFAULT;
#else
		encode_manager.cma_pool_size =
			(codec_mm_get_total_size() > (MIN_SIZE * 3)) ?
			(MIN_SIZE * 3) : codec_mm_get_total_size();
		enc_pr(LOG_DEBUG,
			"amvenc_avc - cma memory pool size: %d MB\n",
			(u32)encode_manager.cma_pool_size / SZ_1M);
#endif
	}

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
		if (hcodec_clk_prepare(&pdev->dev, &s_hcodec_clks)) {
			//err = -ENOENT;
			enc_pr(LOG_ERROR, "[%s:%d] probe hcodec enc failed\n", __FUNCTION__, __LINE__);
			//goto ERROR_PROBE_DEVICE;
			return -EINVAL;
		}
	}

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
		hcodec_rst = devm_reset_control_get(&pdev->dev, "hcodec_rst");
		if (IS_ERR(hcodec_rst))
			enc_pr(LOG_ERROR, "amvenc probe, hcodec get reset failed: %ld\n", PTR_ERR(hcodec_rst));
	}

	res_irq = platform_get_irq(pdev, 0);
	if (res_irq < 0) {
		enc_pr(LOG_ERROR, "[%s] get irq error!", __func__);
		return -EINVAL;
	}

	encode_manager.irq_num = res_irq;
	if (encode_wq_init()) {
		kfree(encode_manager.reserve_buff);
		encode_manager.reserve_buff = NULL;
		enc_pr(LOG_ERROR, "encode work queue init error.\n");
		return -EFAULT;
	}

	r = init_avc_device();
	enc_pr(LOG_INFO, "amvenc_avc probe end.\n");

	return r;
}

static s32 amvenc_avc_remove(struct platform_device *pdev)
{
	kfree(encode_manager.reserve_buff);
	encode_manager.reserve_buff = NULL;
	if (encode_wq_uninit())
		enc_pr(LOG_ERROR, "encode work queue uninit error.\n");
	uninit_avc_device();
	hcodec_clk_unprepare(&pdev->dev, &s_hcodec_clks);
	enc_pr(LOG_INFO, "amvenc_avc remove.\n");
	return 0;
}

static const struct of_device_id amlogic_avcenc_dt_match[] = {
	{
		.compatible = "amlogic, amvenc_avc",
	},
	{},
};

static struct platform_driver amvenc_avc_driver = {
	.probe = amvenc_avc_probe,
	.remove = amvenc_avc_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = amlogic_avcenc_dt_match,
	}
};

/*
static struct codec_profile_t amvenc_avc_profile = {
	.name = "avc",
	.profile = ""
};
*/

static s32 __init amvenc_avc_driver_init_module(void)
{
	if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7) {
		enc_pr(LOG_ERROR, "T7 doesn't support hcodec avc encoder!!\n");
		return -1;
	}

	enc_pr(LOG_INFO, "amvenc_avc module init\n");

	if (platform_driver_register(&amvenc_avc_driver)) {
		enc_pr(LOG_ERROR,
			"failed to register amvenc_avc driver\n");
		return -ENODEV;
	}
	//vcodec_profile_register(&amvenc_avc_profile);
	return 0;
}

static void __exit amvenc_avc_driver_remove_module(void)
{
	enc_pr(LOG_INFO, "amvenc_avc module remove.\n");

	platform_driver_unregister(&amvenc_avc_driver);
}

static const struct reserved_mem_ops rmem_avc_ops = {
	.device_init = avc_mem_device_init,
};

static s32 __init avc_mem_setup(struct reserved_mem *rmem)
{
	rmem->ops = &rmem_avc_ops;
	enc_pr(LOG_DEBUG, "amvenc_avc reserved mem setup.\n");
	return 0;
}

static int enc_dma_buf_map(struct enc_dma_cfg *cfg)
{
	long ret = -1;
	int fd = -1;
	struct dma_buf *dbuf = NULL;
	struct dma_buf_attachment *d_att = NULL;
	struct sg_table *sg = NULL;
	void *vaddr = NULL;
	struct device *dev = NULL;
	enum dma_data_direction dir;

	if (cfg == NULL || (cfg->fd < 0) || cfg->dev == NULL) {
		enc_pr(LOG_ERROR, "error input param\n");
		return -EINVAL;
	}
	enc_pr(LOG_INFO, "enc_dma_buf_map, fd %d\n", cfg->fd);

	fd = cfg->fd;
	dev = cfg->dev;
	dir = cfg->dir;
	enc_pr(LOG_INFO, "enc_dma_buffer_map fd %d\n", fd);

	dbuf = dma_buf_get(fd);
	if (dbuf == NULL) {
		enc_pr(LOG_ERROR, "failed to get dma buffer,fd %d\n",fd);
		return -EINVAL;
	}

	d_att = dma_buf_attach(dbuf, dev);
	if (d_att == NULL) {
		enc_pr(LOG_ERROR, "failed to set dma attach\n");
		goto attach_err;
	}

	sg = dma_buf_map_attachment(d_att, dir);
	if (sg == NULL) {
		enc_pr(LOG_ERROR, "failed to get dma sg\n");
		goto map_attach_err;
	}

	cfg->dbuf = dbuf;
	cfg->attach = d_att;
	cfg->vaddr = vaddr;
	cfg->sg = sg;

	return 0;

map_attach_err:
	dma_buf_detach(dbuf, d_att);

attach_err:
	dma_buf_put(dbuf);

	return ret;
}

static int enc_dma_buf_get_phys(struct enc_dma_cfg *cfg, unsigned long *addr)
{
	struct sg_table *sg_table;
	struct page *page;
	int ret;
	enc_pr(LOG_INFO, "enc_dma_buf_get_phys in\n");

	ret = enc_dma_buf_map(cfg);
	if (ret < 0) {
		enc_pr(LOG_ERROR, "gdc_dma_buf_map failed\n");
		return ret;
	}
	if (cfg->sg) {
		sg_table = cfg->sg;
		page = sg_page(sg_table->sgl);
		*addr = PFN_PHYS(page_to_pfn(page));
		ret = 0;
	}
	enc_pr(LOG_INFO, "enc_dma_buf_get_phys 0x%lx\n", *addr);
	return ret;
}

static void enc_dma_buf_unmap(struct enc_dma_cfg *cfg)
{
	int fd = -1;
	struct dma_buf *dbuf = NULL;
	struct dma_buf_attachment *d_att = NULL;
	struct sg_table *sg = NULL;
	//void *vaddr = NULL;
	struct device *dev = NULL;
	enum dma_data_direction dir;

	if (cfg == NULL || (cfg->fd < 0) || cfg->dev == NULL
			|| cfg->dbuf == NULL /*|| cfg->vaddr == NULL*/
			|| cfg->attach == NULL || cfg->sg == NULL) {
		enc_pr(LOG_ERROR, "Error input param\n");
		return;
	}

	fd = cfg->fd;
	dev = cfg->dev;
	dir = cfg->dir;
	dbuf = cfg->dbuf;
	d_att = cfg->attach;
	sg = cfg->sg;

	dma_buf_unmap_attachment(d_att, sg, dir);

	dma_buf_detach(dbuf, d_att);

	dma_buf_put(dbuf);
	enc_pr(LOG_DEBUG, "enc_dma_buffer_unmap fd %d\n",fd);
}


module_param_named(fixed_slice_cfg, fixed_slice_cfg, uint, 0664);
MODULE_PARM_DESC(fixed_slice_cfg, "\n fixed_slice_cfg\n");

module_param(clock_level, uint, 0664);
MODULE_PARM_DESC(clock_level, "\n clock_level\n");

module_param(encode_print_level, uint, 0664);
MODULE_PARM_DESC(encode_print_level, "\n encode_print_level\n");

module_param(no_timeout, uint, 0664);
MODULE_PARM_DESC(no_timeout, "\n no_timeout flag for process request\n");

module_param(nr_mode, int, 0664);
MODULE_PARM_DESC(nr_mode, "\n nr_mode option\n");

module_param(qp_table_debug, uint, 0664);
MODULE_PARM_DESC(qp_table_debug, "\n print qp table\n");

module_param(use_reset_control, uint, 0664);
MODULE_PARM_DESC(use_reset_control, "\n use_reset_control\n");

module_param(use_ge2d, uint, 0664);
MODULE_PARM_DESC(use_ge2d, "\n use_ge2d\n");

module_param(dump_input, uint, 0664);
MODULE_PARM_DESC(dump_input, "\n dump_input\n");

#ifdef H264_ENC_SVC
module_param(svc_enable, uint, 0664);
MODULE_PARM_DESC(svc_enable, "\n svc enable\n");
module_param(svc_ref_conf, uint, 0664);
MODULE_PARM_DESC(svc_ref_conf, "\n svc reference duration config\n");
#endif

#ifdef MORE_MODULE_PARAM
module_param_named(me_mv_merge_ctl, me.me_mv_merge_ctl, uint, 0664);
MODULE_PARM_DESC(me_mv_merge_ctl, "\n me_mv_merge_ctl\n");

module_param_named(me_step0_close_mv, me.me_step0_close_mv, uint, 0664);
MODULE_PARM_DESC(me_step0_close_mv, "\n me_step0_close_mv\n");

module_param_named(me_f_skip_sad, me.me_f_skip_sad, uint, 0664);
MODULE_PARM_DESC(me_f_skip_sad, "\n me_f_skip_sad\n");

module_param_named(me_f_skip_weight, me.me_f_skip_weight, uint, 0664);
MODULE_PARM_DESC(me_f_skip_weight, "\n me_f_skip_weight\n");

module_param_named(me_mv_weight_01, me.me_mv_weight_01, uint, 0664);
MODULE_PARM_DESC(me_mv_weight_01, "\n me_mv_weight_01\n");

module_param_named(me_mv_weight_23, me.me_mv_weight_23, uint, 0664);
MODULE_PARM_DESC(me_mv_weight_23, "\n me_mv_weight_23\n");

module_param_named(me_sad_range_inc, me.me_sad_range_inc, uint, 0664);
MODULE_PARM_DESC(me_sad_range_inc, "\n me_sad_range_inc\n");

module_param_named(me_sad_enough_01, me.me_sad_enough_01, uint, 0664);
MODULE_PARM_DESC(me_sad_enough_01, "\n me_sad_enough_01\n");

module_param_named(me_sad_enough_23, me.me_sad_enough_23, uint, 0664);
MODULE_PARM_DESC(me_sad_enough_23, "\n me_sad_enough_23\n");

module_param_named(y_tnr_mc_en, y_tnr.mc_en, uint, 0664);
MODULE_PARM_DESC(y_tnr_mc_en, "\n y_tnr.mc_en option\n");
module_param_named(y_tnr_txt_mode, y_tnr.txt_mode, uint, 0664);
MODULE_PARM_DESC(y_tnr_txt_mode, "\n y_tnr.txt_mode option\n");
module_param_named(y_tnr_mot_sad_margin, y_tnr.mot_sad_margin, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot_sad_margin, "\n y_tnr.mot_sad_margin option\n");
module_param_named(y_tnr_mot_cortxt_rate, y_tnr.mot_cortxt_rate, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot_cortxt_rate, "\n y_tnr.mot_cortxt_rate option\n");
module_param_named(y_tnr_mot_distxt_ofst, y_tnr.mot_distxt_ofst, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot_distxt_ofst, "\n y_tnr.mot_distxt_ofst option\n");
module_param_named(y_tnr_mot_distxt_rate, y_tnr.mot_distxt_rate, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot_distxt_rate, "\n y_tnr.mot_distxt_rate option\n");
module_param_named(y_tnr_mot_dismot_ofst, y_tnr.mot_dismot_ofst, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot_dismot_ofst, "\n y_tnr.mot_dismot_ofst option\n");
module_param_named(y_tnr_mot_frcsad_lock, y_tnr.mot_frcsad_lock, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot_frcsad_lock, "\n y_tnr.mot_frcsad_lock option\n");
module_param_named(y_tnr_mot2alp_frc_gain, y_tnr.mot2alp_frc_gain, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot2alp_frc_gain, "\n y_tnr.mot2alp_frc_gain option\n");
module_param_named(y_tnr_mot2alp_nrm_gain, y_tnr.mot2alp_nrm_gain, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot2alp_nrm_gain, "\n y_tnr.mot2alp_nrm_gain option\n");
module_param_named(y_tnr_mot2alp_dis_gain, y_tnr.mot2alp_dis_gain, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot2alp_dis_gain, "\n y_tnr.mot2alp_dis_gain option\n");
module_param_named(y_tnr_mot2alp_dis_ofst, y_tnr.mot2alp_dis_ofst, uint, 0664);
MODULE_PARM_DESC(y_tnr_mot2alp_dis_ofst, "\n y_tnr.mot2alp_dis_ofst option\n");
module_param_named(y_tnr_alpha_min, y_tnr.alpha_min, uint, 0664);
MODULE_PARM_DESC(y_tnr_alpha_min, "\n y_tnr.alpha_min option\n");
module_param_named(y_tnr_alpha_max, y_tnr.alpha_max, uint, 0664);
MODULE_PARM_DESC(y_tnr_alpha_max, "\n y_tnr.alpha_max option\n");
module_param_named(y_tnr_deghost_os, y_tnr.deghost_os, uint, 0664);
MODULE_PARM_DESC(y_tnr_deghost_os, "\n y_tnr.deghost_os option\n");

module_param_named(c_tnr_mc_en, c_tnr.mc_en, uint, 0664);
MODULE_PARM_DESC(c_tnr_mc_en, "\n c_tnr.mc_en option\n");
module_param_named(c_tnr_txt_mode, c_tnr.txt_mode, uint, 0664);
MODULE_PARM_DESC(c_tnr_txt_mode, "\n c_tnr.txt_mode option\n");
module_param_named(c_tnr_mot_sad_margin, c_tnr.mot_sad_margin, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot_sad_margin, "\n c_tnr.mot_sad_margin option\n");
module_param_named(c_tnr_mot_cortxt_rate, c_tnr.mot_cortxt_rate, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot_cortxt_rate, "\n c_tnr.mot_cortxt_rate option\n");
module_param_named(c_tnr_mot_distxt_ofst, c_tnr.mot_distxt_ofst, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot_distxt_ofst, "\n c_tnr.mot_distxt_ofst option\n");
module_param_named(c_tnr_mot_distxt_rate, c_tnr.mot_distxt_rate, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot_distxt_rate, "\n c_tnr.mot_distxt_rate option\n");
module_param_named(c_tnr_mot_dismot_ofst, c_tnr.mot_dismot_ofst, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot_dismot_ofst, "\n c_tnr.mot_dismot_ofst option\n");
module_param_named(c_tnr_mot_frcsad_lock, c_tnr.mot_frcsad_lock, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot_frcsad_lock, "\n c_tnr.mot_frcsad_lock option\n");
module_param_named(c_tnr_mot2alp_frc_gain, c_tnr.mot2alp_frc_gain, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot2alp_frc_gain, "\n c_tnr.mot2alp_frc_gain option\n");
module_param_named(c_tnr_mot2alp_nrm_gain, c_tnr.mot2alp_nrm_gain, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot2alp_nrm_gain, "\n c_tnr.mot2alp_nrm_gain option\n");
module_param_named(c_tnr_mot2alp_dis_gain, c_tnr.mot2alp_dis_gain, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot2alp_dis_gain, "\n c_tnr.mot2alp_dis_gain option\n");
module_param_named(c_tnr_mot2alp_dis_ofst, c_tnr.mot2alp_dis_ofst, uint, 0664);
MODULE_PARM_DESC(c_tnr_mot2alp_dis_ofst, "\n c_tnr.mot2alp_dis_ofst option\n");
module_param_named(c_tnr_alpha_min, c_tnr.alpha_min, uint, 0664);
MODULE_PARM_DESC(c_tnr_alpha_min, "\n c_tnr.alpha_min option\n");
module_param_named(c_tnr_alpha_max, c_tnr.alpha_max, uint, 0664);
MODULE_PARM_DESC(c_tnr_alpha_max, "\n c_tnr.alpha_max option\n");
module_param_named(c_tnr_deghost_os, c_tnr.deghost_os, uint, 0664);
MODULE_PARM_DESC(c_tnr_deghost_os, "\n c_tnr.deghost_os option\n");

module_param_named(y_snr_err_norm, y_snr.err_norm, uint, 0664);
MODULE_PARM_DESC(y_snr_err_norm, "\n y_snr.err_norm option\n");
module_param_named(y_snr_gau_bld_core, y_snr.gau_bld_core, uint, 0664);
MODULE_PARM_DESC(y_snr_gau_bld_core, "\n y_snr.gau_bld_core option\n");
module_param_named(y_snr_gau_bld_ofst, y_snr.gau_bld_ofst, int, 0664);
MODULE_PARM_DESC(y_snr_gau_bld_ofst, "\n y_snr.gau_bld_ofst option\n");
module_param_named(y_snr_gau_bld_rate, y_snr.gau_bld_rate, uint, 0664);
MODULE_PARM_DESC(y_snr_gau_bld_rate, "\n y_snr.gau_bld_rate option\n");
module_param_named(y_snr_gau_alp0_min, y_snr.gau_alp0_min, uint, 0664);
MODULE_PARM_DESC(y_snr_gau_alp0_min, "\n y_snr.gau_alp0_min option\n");
module_param_named(y_snr_gau_alp0_max, y_snr.gau_alp0_max, uint, 0664);
MODULE_PARM_DESC(y_snr_gau_alp0_max, "\n y_snr.gau_alp0_max option\n");
module_param_named(y_bld_beta2alp_rate, y_snr.beta2alp_rate, uint, 0664);
MODULE_PARM_DESC(y_bld_beta2alp_rate, "\n y_snr.beta2alp_rate option\n");
module_param_named(y_bld_beta_min, y_snr.beta_min, uint, 0664);
MODULE_PARM_DESC(y_bld_beta_min, "\n y_snr.beta_min option\n");
module_param_named(y_bld_beta_max, y_snr.beta_max, uint, 0664);
MODULE_PARM_DESC(y_bld_beta_max, "\n y_snr.beta_max option\n");

module_param_named(c_snr_err_norm, c_snr.err_norm, uint, 0664);
MODULE_PARM_DESC(c_snr_err_norm, "\n c_snr.err_norm option\n");
module_param_named(c_snr_gau_bld_core, c_snr.gau_bld_core, uint, 0664);
MODULE_PARM_DESC(c_snr_gau_bld_core, "\n c_snr.gau_bld_core option\n");
module_param_named(c_snr_gau_bld_ofst, c_snr.gau_bld_ofst, int, 0664);
MODULE_PARM_DESC(c_snr_gau_bld_ofst, "\n c_snr.gau_bld_ofst option\n");
module_param_named(c_snr_gau_bld_rate, c_snr.gau_bld_rate, uint, 0664);
MODULE_PARM_DESC(c_snr_gau_bld_rate, "\n c_snr.gau_bld_rate option\n");
module_param_named(c_snr_gau_alp0_min, c_snr.gau_alp0_min, uint, 0664);
MODULE_PARM_DESC(c_snr_gau_alp0_min, "\n c_snr.gau_alp0_min option\n");
module_param_named(c_snr_gau_alp0_max, c_snr.gau_alp0_max, uint, 0664);
MODULE_PARM_DESC(c_snr_gau_alp0_max, "\n c_snr.gau_alp0_max option\n");
module_param_named(c_bld_beta2alp_rate, c_snr.beta2alp_rate, uint, 0664);
MODULE_PARM_DESC(c_bld_beta2alp_rate, "\n c_snr.beta2alp_rate option\n");
module_param_named(c_bld_beta_min, c_snr.beta_min, uint, 0664);
MODULE_PARM_DESC(c_bld_beta_min, "\n c_snr.beta_min option\n");
module_param_named(c_bld_beta_max, c_snr.beta_max, uint, 0664);
MODULE_PARM_DESC(c_bld_beta_max, "\n c_snr.beta_max option\n");
#endif

module_init(amvenc_avc_driver_init_module);
module_exit(amvenc_avc_driver_remove_module);
RESERVEDMEM_OF_DECLARE(amvenc_avc, "amlogic, amvenc-memory", avc_mem_setup);

MODULE_DESCRIPTION("AMLOGIC AVC Video Encoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("simon.zheng <simon.zheng@amlogic.com>");
