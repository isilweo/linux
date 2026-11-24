// SPDX-License-Identifier: GPL-2.0-only
/*
 * drivers/media/i2c/ccs/ccs-quirk.c
 *
 * Generic driver for MIPI CCS/SMIA/SMIA++ compliant camera sensors
 *
 * Copyright (C) 2020 Intel Corporation
 * Copyright (C) 2011--2012 Nokia Corporation
 * Contact: Sakari Ailus <sakari.ailus@linux.intel.com>
 */

#include <linux/delay.h>

#include "ccs.h"
#include "ccs-limits.h"

static int ccs_write_addr_8s(struct ccs_sensor *sensor,
			     const struct ccs_reg_8 *regs, int len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->src->sd);
	int rval;

	for (; len > 0; len--, regs++) {
		rval = ccs_write_addr(sensor, regs->reg, regs->val);
		if (rval < 0) {
			dev_err(&client->dev,
				"error %d writing reg 0x%4.4x, val 0x%2.2x",
				rval, regs->reg, regs->val);
			return rval;
		}
	}

	return 0;
}

static int jt8ew9_limits(struct ccs_sensor *sensor)
{
	if (sensor->minfo.revision_number < 0x0300)
		sensor->frame_skip = 1;

	/* Below 24 gain doesn't have effect at all, */
	/* but ~59 is needed for full dynamic range */
	ccs_replace_limit(sensor, CCS_L_ANALOG_GAIN_CODE_MIN, 0, 59);
	ccs_replace_limit(sensor, CCS_L_ANALOG_GAIN_CODE_MAX, 0, 6000);

	return 0;
}

static int jt8ew9_post_poweron(struct ccs_sensor *sensor)
{
	static const struct ccs_reg_8 regs[] = {
		{ 0x30a3, 0xd8 }, /* Output port control : LVDS ports only */
		{ 0x30ae, 0x00 }, /* 0x0307 pll_multiplier maximum value on PLL input 9.6MHz ( 19.2MHz is divided on pre_pll_div) */
		{ 0x30af, 0xd0 }, /* 0x0307 pll_multiplier maximum value on PLL input 9.6MHz ( 19.2MHz is divided on pre_pll_div) */
		{ 0x322d, 0x04 }, /* Adjusting Processing Image Size to Scaler Toshiba Recommendation Setting */
		{ 0x3255, 0x0f }, /* Horizontal Noise Reduction Control Toshiba Recommendation Setting */
		{ 0x3256, 0x15 }, /* Horizontal Noise Reduction Control Toshiba Recommendation Setting */
		{ 0x3258, 0x70 }, /* Analog Gain Control Toshiba Recommendation Setting */
		{ 0x3259, 0x70 }, /* Analog Gain Control Toshiba Recommendation Setting */
		{ 0x325f, 0x7c }, /* Analog Gain Control Toshiba Recommendation Setting */
		{ 0x3302, 0x06 }, /* Pixel Reference Voltage Control Toshiba Recommendation Setting */
		{ 0x3304, 0x00 }, /* Pixel Reference Voltage Control Toshiba Recommendation Setting */
		{ 0x3307, 0x22 }, /* Pixel Reference Voltage Control Toshiba Recommendation Setting */
		{ 0x3308, 0x8d }, /* Pixel Reference Voltage Control Toshiba Recommendation Setting */
		{ 0x331e, 0x0f }, /* Black Hole Sun Correction Control Toshiba Recommendation Setting */
		{ 0x3320, 0x30 }, /* Black Hole Sun Correction Control Toshiba Recommendation Setting */
		{ 0x3321, 0x11 }, /* Black Hole Sun Correction Control Toshiba Recommendation Setting */
		{ 0x3322, 0x98 }, /* Black Hole Sun Correction Control Toshiba Recommendation Setting */
		{ 0x3323, 0x64 }, /* Black Hole Sun Correction Control Toshiba Recommendation Setting */
		{ 0x3325, 0x83 }, /* Read Out Timing Control Toshiba Recommendation Setting */
		{ 0x3330, 0x18 }, /* Read Out Timing Control Toshiba Recommendation Setting */
		{ 0x333c, 0x01 }, /* Read Out Timing Control Toshiba Recommendation Setting */
		{ 0x3345, 0x2f }, /* Black Hole Sun Correction Control Toshiba Recommendation Setting */
		{ 0x33de, 0x38 }, /* Horizontal Noise Reduction Control Toshiba Recommendation Setting */
		/* Taken from v03. No idea what the rest are. */
		{ 0x32e0, 0x05 },
		{ 0x32e1, 0x05 },
		{ 0x32e2, 0x04 },
		{ 0x32e5, 0x04 },
		{ 0x32e6, 0x04 },

	};

	return ccs_write_addr_8s(sensor, regs, ARRAY_SIZE(regs));
}

const struct ccs_quirk smiapp_jt8ew9_quirk = {
	.limits = jt8ew9_limits,
	.post_poweron = jt8ew9_post_poweron,
};

static int imx125es_post_poweron(struct ccs_sensor *sensor)
{
	/* Taken from v02. No idea what the other two are. */
	static const struct ccs_reg_8 regs[] = {
		/*
		 * 0x3302: clk during frame blanking:
		 * 0x00 - HS mode, 0x01 - LP11
		 */
		{ 0x3302, 0x01 },
		{ 0x302d, 0x00 },
		{ 0x3b08, 0x8c },
	};

	return ccs_write_addr_8s(sensor, regs, ARRAY_SIZE(regs));
}

const struct ccs_quirk smiapp_imx125es_quirk = {
	.post_poweron = imx125es_post_poweron,
};

static int jt8ev1_limits(struct ccs_sensor *sensor)
{
	ccs_replace_limit(sensor, CCS_L_X_ADDR_MAX, 0, 4271);
	ccs_replace_limit(sensor, CCS_L_MIN_LINE_BLANKING_PCK_BIN, 0, 184);

	return 0;
}

static int jt8ev1_post_poweron(struct ccs_sensor *sensor)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->src->sd);
	int rval;
	static const struct ccs_reg_8 regs[] = {
		{ 0x3031, 0xcd }, /* For digital binning (EQ_MONI) */
		{ 0x30a3, 0xd0 }, /* FLASH STROBE enable */
		{ 0x3237, 0x00 }, /* For control of pulse timing for ADC */
		{ 0x3238, 0x43 },
		{ 0x3301, 0x06 }, /* For analog bias for sensor */
		{ 0x3302, 0x06 },
		{ 0x3304, 0x00 },
		{ 0x3305, 0x88 },
		{ 0x332a, 0x14 },
		{ 0x332c, 0x6b },
		{ 0x3336, 0x01 },
		{ 0x333f, 0x1f },
		{ 0x3355, 0x00 },
		{ 0x3356, 0x20 },
		{ 0x33bf, 0x20 }, /* Adjust the FBC speed */
		{ 0x33c9, 0x20 },
		{ 0x33ce, 0x30 }, /* Adjust the parameter for logic function */
		{ 0x33cf, 0xec }, /* For Black sun */
		{ 0x3328, 0x80 }, /* Ugh. No idea what's this. */
	};
	static const struct ccs_reg_8 regs_96[] = {
		{ 0x30ae, 0x00 }, /* For control of ADC clock */
		{ 0x30af, 0xd0 },
		{ 0x30b0, 0x01 },
	};

	rval = ccs_write_addr_8s(sensor, regs, ARRAY_SIZE(regs));
	if (rval < 0)
		return rval;

	switch (sensor->hwcfg.ext_clk) {
	case 9600000:
		return ccs_write_addr_8s(sensor, regs_96,
				       ARRAY_SIZE(regs_96));
	default:
		dev_warn(&client->dev, "no MSRs for %d Hz ext_clk\n",
			 sensor->hwcfg.ext_clk);
		return 0;
	}
}

static int jt8ev1_pre_streamon(struct ccs_sensor *sensor)
{
	return ccs_write_addr(sensor, 0x3328, 0x00);
}

static int jt8ev1_post_streamoff(struct ccs_sensor *sensor)
{
	int rval;

	/* Workaround: allows fast standby to work properly */
	rval = ccs_write_addr(sensor, 0x3205, 0x04);
	if (rval < 0)
		return rval;

	/* Wait for 1 ms + one line => 2 ms is likely enough */
	usleep_range(2000, 2050);

	/* Restore it */
	rval = ccs_write_addr(sensor, 0x3205, 0x00);
	if (rval < 0)
		return rval;

	return ccs_write_addr(sensor, 0x3328, 0x80);
}

static int jt8ev1_init(struct ccs_sensor *sensor)
{
	sensor->pll.flags |= CCS_PLL_FLAG_LANE_SPEED_MODEL;
	sensor->pll.vt_lanes = 1;
	sensor->pll.op_lanes = sensor->pll.csi2.lanes;

	return 0;
}

const struct ccs_quirk smiapp_jt8ev1_quirk = {
	.limits = jt8ev1_limits,
	.post_poweron = jt8ev1_post_poweron,
	.pre_streamon = jt8ev1_pre_streamon,
	.post_streamoff = jt8ev1_post_streamoff,
	.init = jt8ev1_init,
};

static int tcm8500md_limits(struct ccs_sensor *sensor)
{
	ccs_replace_limit(sensor, CCS_L_MIN_PLL_IP_CLK_FREQ_MHZ, 0, 2700000);

	return 0;
}

const struct ccs_quirk smiapp_tcm8500md_quirk = {
	.limits = tcm8500md_limits,
};

/*
 * Samsung S5KC505A: incomplete clock limit registers, dual PLL, and a vendor
 * init sequence taken from the Lenovo libmmcamera_s5kc505a.so.
 */
static int s5kc505a_limits(struct ccs_sensor *sensor)
{
	ccs_replace_limit(sensor, CCS_L_CLOCK_TREE_PLL_CAPABILITY, 0,
			  CCS_CLOCK_TREE_PLL_CAPABILITY_DUAL_PLL);
	ccs_replace_limit(sensor, CCS_L_CLOCK_CALCULATION, 0,
			  CCS_CLOCK_CALCULATION_LANE_SPEED);
	/* vendor mode timing; shorter blanking makes the VFE drop lines */
	ccs_replace_limit(sensor, CCS_L_MIN_LINE_BLANKING_PCK, 0, 504);
	ccs_replace_limit(sensor, CCS_L_MIN_FRAME_BLANKING_LINES, 0, 70);
	/* lower VT PLL rates clip white; the *_MHZ limits are IEEE 754 floats */
	ccs_replace_limit(sensor, CCS_L_MIN_PLL_OP_CLK_FREQ_MHZ, 0, 0x44098000); /* 550 */
	ccs_replace_limit(sensor, CCS_L_MAX_PLL_OP_CLK_FREQ_MHZ, 0, 0x44160000); /* 600 */
	ccs_replace_limit(sensor, CCS_L_MIN_VT_SYS_CLK_DIV, 0, 1);
	ccs_replace_limit(sensor, CCS_L_MAX_VT_SYS_CLK_DIV, 0, 1);
	ccs_replace_limit(sensor, CCS_L_MIN_VT_SYS_CLK_FREQ_MHZ, 0, 0x42c80000); /* 100 */
	ccs_replace_limit(sensor, CCS_L_MAX_VT_SYS_CLK_FREQ_MHZ, 0, 0x447a0000); /* 1000 */
	ccs_replace_limit(sensor, CCS_L_MAX_VT_PIX_CLK_FREQ_MHZ, 0, 0x43480000); /* 200 */
	ccs_replace_limit(sensor, CCS_L_MIN_OP_SYS_CLK_DIV, 0, 1);
	ccs_replace_limit(sensor, CCS_L_MAX_OP_SYS_CLK_DIV, 0, 1);
	ccs_replace_limit(sensor, CCS_L_MIN_OP_SYS_CLK_FREQ_MHZ, 0, 0x42c80000); /* 100 */
	ccs_replace_limit(sensor, CCS_L_MAX_OP_SYS_CLK_FREQ_MHZ, 0, 0x447a0000); /* 1000 */
	ccs_replace_limit(sensor, CCS_L_MIN_OP_PIX_CLK_DIV, 0, 8);
	ccs_replace_limit(sensor, CCS_L_MAX_OP_PIX_CLK_DIV, 0, 10);
	ccs_replace_limit(sensor, CCS_L_MIN_OP_PIX_CLK_FREQ_MHZ, 0, 0x41200000); /* 10 */
	ccs_replace_limit(sensor, CCS_L_MAX_OP_PIX_CLK_FREQ_MHZ, 0, 0x43480000); /* 200 */
	return 0;
}

/* cci_write() needs the register width, unlike ccs_write_addr_8s() */
static int s5kc505a_write(struct ccs_sensor *sensor,
			  const struct ccs_reg_8 *regs, int len)
{
	int rval;

	for (; len > 0; len--, regs++) {
		rval = ccs_write_addr(sensor, CCI_REG8(regs->reg), regs->val);
		if (rval)
			return rval;
	}

	return 0;
}

static int s5kc505a_post_poweron(struct ccs_sensor *sensor)
{
	static const struct ccs_reg_8 init[] = {
		{ 0x0a02, 0x0f },
		{ 0x340f, 0x80 },
		{ 0x3457, 0x04 },
		{ 0x3419, 0x00 },
		{ 0x3416, 0x40 },
		{ 0x3418, 0x00 },
		{ 0x3419, 0x04 },
		{ 0x3419, 0x00 },
		{ 0x3416, 0x40 },
		{ 0x3418, 0x24 },
		{ 0x3419, 0x04 },
		{ 0x3419, 0x00 },
		{ 0x3416, 0x40 },
		{ 0x3418, 0x48 },
		{ 0x3419, 0x04 },
		{ 0x3419, 0x00 },
		{ 0x3416, 0x40 },
		{ 0x3418, 0x6c },
		{ 0x3419, 0x04 },
		{ 0x3419, 0x00 },
		{ 0x3457, 0x0c },
		{ 0x3457, 0x04 },
		{ 0x307c, 0x16 },
		{ 0x3087, 0xdc },
		{ 0x3089, 0x0a },
		{ 0x308b, 0x08 },
		{ 0x308c, 0x0b },
		{ 0x308e, 0x36 },
		{ 0x3075, 0x4d },
		{ 0x306f, 0xa8 },
		{ 0x306a, 0x79 },
		{ 0x3077, 0x10 },
		{ 0x30b0, 0xc8 },
		{ 0x3078, 0x17 },
		{ 0x3070, 0x0f },
		{ 0x0b00, 0x01 },
		{ 0x0801, 0x10 },
		{ 0x0804, 0x10 },
		{ 0x3931, 0x02 },
		{ 0x3930, 0x80 },
		{ 0x3932, 0x60 },
	};
	static const struct ccs_reg_8 mode[] = {
		{ 0x3c17, 0x00 },
		{ 0x3c1f, 0x00 },
		{ 0x0820, 0x03 },
		{ 0x0821, 0xc0 },
		{ 0x0b05, 0x01 },
		{ 0x3303, 0x02 },
		{ 0x3301, 0x00 },
		{ 0x3400, 0x00 },
		{ 0x0b00, 0x00 },
		{ 0x3459, 0x75 },
		{ 0x3c0f, 0x01 },
		{ 0x300f, 0x01 },
		{ 0x3010, 0x77 },
		{ 0x3014, 0x01 },
		{ 0x3015, 0x86 },
		{ 0x3929, 0x07 },
		{ 0x3906, 0x76 },
		{ 0x3c02, 0x00 },
		{ 0x3c03, 0xff },
	};
	int rval;

	/* the vendor sequence waits 10 ms after the first write */
	rval = s5kc505a_write(sensor, init, 1);
	if (rval)
		return rval;
	usleep_range(10000, 11000);

	rval = s5kc505a_write(sensor, init + 1, ARRAY_SIZE(init) - 1);
	if (rval)
		return rval;

	return s5kc505a_write(sensor, mode, ARRAY_SIZE(mode));
}

const struct ccs_quirk smiapp_s5kc505a_quirk = {
	.limits = s5kc505a_limits,
	.post_poweron = s5kc505a_post_poweron,
};
