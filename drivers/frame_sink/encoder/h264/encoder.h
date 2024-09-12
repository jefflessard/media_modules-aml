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
#ifndef __H264_H__
#define __H264_H__

#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/slab.h>

#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
#include <linux/amlogic/media/ge2d/ge2d.h>
#endif

#include <linux/dma-buf.h>

#define AMVENC_DEVINFO_M8 "AML-M8"
#define AMVENC_DEVINFO_G9 "AML-G9"
#define AMVENC_DEVINFO_GXBB "AML-GXBB"
#define AMVENC_DEVINFO_GXTVBB "AML-GXTVBB"
#define AMVENC_DEVINFO_GXL "AML-GXL"

#define H264_ENC_SVC

/* M8: 2550/10 = 255M GX: 2000/10 = 200M */
#define HDEC_L0()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (2 << 25) | (1 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))
/* M8: 2550/8 = 319M GX: 2000/8 = 250M */
#define HDEC_L1()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (0 << 25) | (1 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))
/* M8: 2550/7 = 364M GX: 2000/7 = 285M */
#define HDEC_L2()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (3 << 25) | (0 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))
/* M8: 2550/6 = 425M GX: 2000/6 = 333M */
#define HDEC_L3()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (1 << 25) | (1 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))
/* M8: 2550/5 = 510M GX: 2000/5 = 400M */
#define HDEC_L4()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (2 << 25) | (0 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))
/* M8: 2550/4 = 638M GX: 2000/4 = 500M */
#define HDEC_L5()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (0 << 25) | (0 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))
/* M8: 2550/3 = 850M GX: 2000/3 = 667M */
#define HDEC_L6()   WRITE_HHI_REG(HHI_VDEC_CLK_CNTL, \
			 (1 << 25) | (0 << 16) | (1 << 24) | \
			 (0xffff & READ_HHI_REG(HHI_VDEC_CLK_CNTL)))

#define hvdec_clock_enable(level) \
	do { \
		if (level == 0)  \
			HDEC_L0(); \
		else if (level == 1)  \
			HDEC_L1(); \
		else if (level == 2)  \
			HDEC_L2(); \
		else if (level == 3)  \
			HDEC_L3(); \
		else if (level == 4)  \
			HDEC_L4(); \
		else if (level == 5)  \
			HDEC_L5(); \
		else if (level == 6)  \
			HDEC_L6(); \
		WRITE_VREG_BITS(DOS_GCLK_EN0, 0x7fff, 12, 15); \
	} while (0)

#define hvdec_clock_disable() \
	do { \
		WRITE_VREG_BITS(DOS_GCLK_EN0, 0, 12, 15); \
		WRITE_HHI_REG_BITS(HHI_VDEC_CLK_CNTL,  0, 24, 1); \
	} while (0)

#define LOG_ALL 0
#define LOG_INFO 1
#define LOG_DEBUG 2
#define LOG_ERROR 3

#define enc_pr(level, x...) \
	do { \
		if (level >= encode_print_level) \
			printk(x); \
	} while (0)

#define AMVENC_AVC_IOC_MAGIC  'E'

#define AMVENC_AVC_IOC_GET_DEVINFO	_IOW(AMVENC_AVC_IOC_MAGIC, 0xf0, u32)
#define AMVENC_AVC_IOC_MAX_INSTANCE	_IOW(AMVENC_AVC_IOC_MAGIC, 0xf1, u32)
#define AMVENC_AVC_IOC_GET_CPU_ID	_IOW(AMVENC_AVC_IOC_MAGIC, 0xf2, u32)

#define AMVENC_AVC_IOC_GET_ADDR _IOW(AMVENC_AVC_IOC_MAGIC, 0x00, u32)
#define AMVENC_AVC_IOC_INPUT_UPDATE	_IOW(AMVENC_AVC_IOC_MAGIC, 0x01, u32)
#define AMVENC_AVC_IOC_NEW_CMD _IOW(AMVENC_AVC_IOC_MAGIC, 0x02, u32)
#define AMVENC_AVC_IOC_GET_STAGE _IOW(AMVENC_AVC_IOC_MAGIC, 0x03, u32)
#define AMVENC_AVC_IOC_GET_OUTPUT_SIZE _IOW(AMVENC_AVC_IOC_MAGIC, 0x04, u32)
#define AMVENC_AVC_IOC_CONFIG_INIT _IOW(AMVENC_AVC_IOC_MAGIC, 0x05, u32)
#define AMVENC_AVC_IOC_FLUSH_CACHE _IOW(AMVENC_AVC_IOC_MAGIC, 0x06, u32)
#define AMVENC_AVC_IOC_FLUSH_DMA _IOW(AMVENC_AVC_IOC_MAGIC, 0x07, u32)
#define AMVENC_AVC_IOC_GET_BUFFINFO _IOW(AMVENC_AVC_IOC_MAGIC, 0x08, u32)
#define AMVENC_AVC_IOC_SUBMIT	_IOW(AMVENC_AVC_IOC_MAGIC, 0x09, u32)
#define AMVENC_AVC_IOC_READ_CANVAS _IOW(AMVENC_AVC_IOC_MAGIC, 0x0a, u32)
#define AMVENC_AVC_IOC_QP_MODE _IOW(AMVENC_AVC_IOC_MAGIC, 0x0b, u32)

enum amvenc_mem_type_e {
	LOCAL_BUFF = 0,
	CANVAS_BUFF,
	PHYSICAL_BUFF,
	DMA_BUFF,
	MAX_BUFF_TYPE
};

enum amvenc_frame_fmt_e {
	FMT_YUV422_SINGLE = 0,
	FMT_YUV444_SINGLE,
	FMT_NV21,
	FMT_NV12,
	FMT_YUV420,
	FMT_YUV444_PLANE,
	FMT_RGB888,
	FMT_RGB888_PLANE,
	FMT_RGB565,
	FMT_RGBA8888,
	FMT_YUV422_12BIT,
	FMT_YUV444_10BIT,
	FMT_YUV422_10BIT,
	FMT_BGR888,
	MAX_FRAME_FMT
};

#define MAX_ENCODE_REQUEST  8   /* 64 */

#define MAX_ENCODE_INSTANCE  8   /* 64 */

#define ENCODE_PROCESS_QUEUE_START	0
#define ENCODE_PROCESS_QUEUE_STOP	1

#define AMVENC_FLUSH_FLAG_INPUT			0x1
#define AMVENC_FLUSH_FLAG_OUTPUT		0x2
#define AMVENC_FLUSH_FLAG_REFERENCE		0x4
#define AMVENC_FLUSH_FLAG_INTRA_INFO	0x8
#define AMVENC_FLUSH_FLAG_INTER_INFO	0x10
#define AMVENC_FLUSH_FLAG_QP				0x20
#define AMVENC_FLUSH_FLAG_DUMP			0x40
#define AMVENC_FLUSH_FLAG_CBR				0x80

#define ENCODER_BUFFER_INPUT              0
#define ENCODER_BUFFER_REF0                1
#define ENCODER_BUFFER_REF1                2
#define ENCODER_BUFFER_OUTPUT           3
#define ENCODER_BUFFER_INTER_INFO          4
#define ENCODER_BUFFER_INTRA_INFO          5
#define ENCODER_BUFFER_QP		          6
#define ENCODER_BUFFER_DUMP              7
#define ENCODER_BUFFER_CBR              8

struct encode_wq_s;

struct enc_dma_cfg {
	int fd;
	void *dev;
	void *vaddr;
	void *paddr;
	struct dma_buf *dbuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sg;
	enum dma_data_direction dir;
};

struct encode_request_s {
	u32 quant;
	u32 cmd;
	u32 ucode_mode;
	u32 src;
	u32 framesize;

	u32 me_weight;
	u32 i4_weight;
	u32 i16_weight;

	u32 crop_top;
	u32 crop_bottom;
	u32 crop_left;
	u32 crop_right;
	u32 src_w;
	u32 src_h;
	u32 scale_enable;

	u32 nr_mode;
	u32 flush_flag;
	u32 timeout;
	enum amvenc_mem_type_e type;
	enum amvenc_frame_fmt_e fmt;
	struct encode_wq_s *parent;
	struct enc_dma_cfg dma_cfg[3];
	u32 plane_num;
};

struct encode_queue_item_s {
	struct list_head list;
	struct encode_request_s request;
};

struct Buff_s {
	u32 buf_start;
	u32 buf_size;
	bool used;
};

struct BuffInfo_s {
	u32 lev_id;
	u32 min_buffsize;
	u32 max_width;
	u32 max_height;
	struct Buff_s dct;
	struct Buff_s dec0_y;
	struct Buff_s dec0_uv;
	struct Buff_s dec1_y;
	struct Buff_s dec1_uv;
	struct Buff_s assit;
	struct Buff_s bitstream;
	struct Buff_s scale_buff;
	struct Buff_s dump_info;
	struct Buff_s cbr_info;
};

struct encode_meminfo_s {
	u32 buf_start;
	u32 buf_size;

	u32 BitstreamStart;
	u32 BitstreamEnd;

	/*input buffer define*/
	u32 dct_buff_start_addr;
	u32 dct_buff_end_addr;

	/*microcode assitant buffer*/
	u32 assit_buffer_offset;

	u32 scaler_buff_start_addr;

	u32 dump_info_ddr_start_addr;
	u32 dump_info_ddr_size;

	u32 cbr_info_ddr_start_addr;
	u32 cbr_info_ddr_size;

	u8 * cbr_info_ddr_virt_addr;

	s32 dblk_buf_canvas;
	s32 ref_buf_canvas;
	struct BuffInfo_s bufspec;
#ifdef CONFIG_CMA
	struct page *venc_pages;
#endif
};

struct encode_picinfo_s {
	u32 encoder_width;
	u32 encoder_height;

	u32 rows_per_slice;

	u32 idr_pic_id;  /* need reset as 0 for IDR */
	u32 frame_number;   /* need plus each frame */
	/* need reset as 0 for IDR and plus 2 for NON-IDR */
	u32 pic_order_cnt_lsb;

	u32 log2_max_pic_order_cnt_lsb;
	u32 log2_max_frame_num;
	u32 init_qppicture;
#ifdef H264_ENC_SVC
	u32 enable_svc;
	u32 non_ref_limit;
	u32 non_ref_cnt;
#endif
	u32 color_space;
};

struct encode_cbr_s {
	u16 block_w;
	u16 block_h;
	u16 long_th;
	u8 start_tbl_id;
	u8 short_shift;
	u8 long_mb_num;
};

struct encode_wq_s {
	struct list_head list;

	/* dev info */
	u32 ucode_index;
	u32 hw_status;
	u32 output_size;

	u32 sps_size;
	u32 pps_size;

	u32 me_weight;
	u32 i4_weight;
	u32 i16_weight;

	u32 quant_tbl_i4[8];
	u32 quant_tbl_i16[8];
	u32 quant_tbl_me[8];

	struct encode_meminfo_s mem;
	struct encode_picinfo_s pic;
	struct encode_request_s request;
	struct encode_cbr_s cbr_info;
	atomic_t request_ready;
	wait_queue_head_t request_complete;
};

struct encode_event_s {
	wait_queue_head_t hw_complete;
	struct completion process_complete;
	spinlock_t sem_lock; /* for queue switch and create destroy queue. */
	struct completion request_in_com;
};

struct encode_manager_s {
	struct list_head wq;
	struct list_head process_queue;
	struct list_head free_queue;

	u32 encode_hw_status;
	u32 process_queue_state;
	s32 irq_num;
	u32 wq_count;
	u32 ucode_index;
	u32 max_instance;
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
	struct ge2d_context_s *context;
#endif
	bool irq_requested;
	bool need_reset;
	bool process_irq;
	bool inited; /* power on encode */
	bool remove_flag; /* remove wq; */
	bool uninit_flag; /* power off encode */
	bool use_reserve;

#ifdef CONFIG_CMA
	bool check_cma;
	ulong cma_pool_size;
#endif
	struct platform_device *this_pdev;
	struct Buff_s *reserve_buff;
	struct encode_wq_s *current_wq;
	struct encode_wq_s *last_wq;
	struct encode_queue_item_s *current_item;
	struct task_struct *encode_thread;
	struct Buff_s reserve_mem;
	struct encode_event_s event;
	struct tasklet_struct encode_tasklet;
};

extern s32 encode_wq_add_request(struct encode_wq_s *wq);
extern struct encode_wq_s *create_encode_work_queue(void);
extern s32 destroy_encode_work_queue(struct encode_wq_s *encode_work_queue);

extern bool amvenc_avc_on(void);
#endif
