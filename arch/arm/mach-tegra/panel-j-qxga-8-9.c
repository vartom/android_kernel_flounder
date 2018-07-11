/*
 * arch/arm/mach-tegra/panel-s-wuxga-8-0.c
 *
 * Copyright (c) 2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mach/dc.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/tegra_dsi_backlight.h>
#include <linux/leds.h>
#include <linux/ioport.h>
#include <linux/export.h>
#include <linux/of_gpio.h>

#include <generated/mach-types.h>

#include "board.h"
#include "board-panel.h"
#include "devices.h"
#include "gpio-names.h"

#define DSI_PANEL_RESET		1

static bool reg_requested;
static struct regulator *avdd_lcd_3v3;
static struct regulator *vdd_lcd_bl;
static struct regulator *vdd_lcd_bl_en;
static struct regulator *dvdd_lcd_1v8;
static u16 en_panel_rst;

static int dsi_j_qxga_8_9_dsi_regulator_get(struct device *dev)
{
	int err = 0;

	if (reg_requested)
		return 0;
	dvdd_lcd_1v8 = regulator_get(dev, "dvdd_lcd");
	if (IS_ERR(dvdd_lcd_1v8)) {
		pr_err("dvdd_lcd regulator get failed\n");
		err = PTR_ERR(dvdd_lcd_1v8);
		dvdd_lcd_1v8 = NULL;
		goto fail;
	}
	avdd_lcd_3v3 = regulator_get(dev, "avdd_lcd");
	if (IS_ERR(avdd_lcd_3v3)) {
		pr_err("avdd_lcd regulator get failed\n");
		err = PTR_ERR(avdd_lcd_3v3);
		avdd_lcd_3v3 = NULL;
		goto fail;
	}

	vdd_lcd_bl = regulator_get(dev, "vdd_lcd_bl");
	if (IS_ERR(vdd_lcd_bl)) {
		pr_err("vdd_lcd_bl regulator get failed\n");
		err = PTR_ERR(vdd_lcd_bl);
		vdd_lcd_bl = NULL;
		goto fail;
	}

	vdd_lcd_bl_en = regulator_get(dev, "vdd_lcd_bl_en");
	if (IS_ERR(vdd_lcd_bl_en)) {
		pr_err("vdd_lcd_bl_en regulator get failed\n");
		err = PTR_ERR(vdd_lcd_bl_en);
		vdd_lcd_bl_en = NULL;
		goto fail;
	}
	reg_requested = true;
	return 0;
fail:
	return err;
}

static int dsi_j_qxga_8_9_postpoweron(struct device *dev)
{
	int err = 0;

	pr_info("panel dsi_j_qxga_8_9_postpoweron\n");

	err = dsi_j_qxga_8_9_dsi_regulator_get(dev);
	if (err < 0) {
		pr_err("dsi regulator get failed\n");
		goto fail;
	}

	err = tegra_panel_gpio_get_dt("j,qxga-8-9", &panel_of);
	if (err < 0) {
		pr_err("dsi gpio request failed\n");
		goto fail;
	}

	/* If panel rst gpio is specified in device tree,
	 * use that.
	 */
	if (gpio_is_valid(panel_of.panel_gpio[TEGRA_GPIO_RESET]))
		en_panel_rst = panel_of.panel_gpio[TEGRA_GPIO_RESET];

	if (avdd_lcd_3v3) {
		err = regulator_enable(avdd_lcd_3v3);
		if (err < 0) {
			pr_err("avdd_lcd regulator enable failed\n");
			goto fail;
		}
	}

	usleep_range(1 * 1000, 1 * 1000 + 500);

	if (dvdd_lcd_1v8) {
		err = regulator_enable(dvdd_lcd_1v8);
		if (err < 0) {
			pr_err("dvdd_lcd regulator enable failed\n");
			goto fail;
		}
	}

	usleep_range(15 * 1000, 15 * 1000 + 500);

	if (avdd_lcd_3v3) {
		err = regulator_enable(avdd_lcd_3v3);
		if (err < 0) {
			pr_err("avdd_lcd regulator enable failed\n");
			goto fail;
		}
	}

	/* panel ic requirement after vcc enable */
	usleep_range(1 * 1000, 1 * 1000 + 500);

	if (vdd_lcd_bl) {
		err = regulator_enable(vdd_lcd_bl);
		if (err < 0) {
			pr_err("vdd_lcd_bl regulator enable failed\n");
			goto fail;
		}
	}

	usleep_range(15 * 1000, 15 * 1000 + 500);

	if (vdd_lcd_bl_en) {
		err = regulator_enable(vdd_lcd_bl_en);
		if (err < 0) {
			pr_err("vdd_lcd_bl_en regulator enable failed\n");
			goto fail;
		}
	}

	usleep_range(15 * 1000, 15 * 1000 + 500);

#if DSI_PANEL_RESET
	/* use platform data */
	err = gpio_direction_output(en_panel_rst, 1);
	if (err) {
		pr_err("gpio_direction_output for en_panel_rst failed\n");
		goto fail;
	}
	usleep_range(1000, 5000);
	gpio_set_value(en_panel_rst, 1);
	usleep_range(15 * 1000, 15 * 1000 + 500);
#endif

	return 0;
fail:
	return err;
}

static int dsi_j_qxga_8_9_enable(struct device *dev)
{
	int err = 0;

	pr_info("panel dsi_j_qxga_8_9_enable\n");

	if (dvdd_lcd_1v8) {
		err = regulator_enable(dvdd_lcd_1v8);
		if (err < 0) {
			pr_err("dvdd_lcd regulator enable failed\n");
		}
	}

	usleep_range(15 * 1000, 15 * 1000 + 500);
	return 0;
}

static int dsi_j_qxga_8_9_disable(struct device *dev)
{

	pr_info("panel dsi_j_qxga_8_9_disable\n");

	gpio_set_value(en_panel_rst, 0);
	msleep(1);
	if (vdd_lcd_bl)
		regulator_disable(vdd_lcd_bl);

	msleep(15);

	if (vdd_lcd_bl_en)
		regulator_disable(vdd_lcd_bl_en);

	msleep(15);
	if (avdd_lcd_3v3)
		regulator_disable(avdd_lcd_3v3);

	if (dvdd_lcd_1v8)
		regulator_disable(dvdd_lcd_1v8);
	msleep(10);

	return 0;
}

static int dsi_j_qxga_8_9_postsuspend(void)
{
	return 0;
}

#define ORIG_PWM_MAX 255
#define ORIG_PWM_DEF 133
#define ORIG_PWM_MIN 10

#define MAP_PWM_MAX     255
#define MAP_PWM_DEF     90
#define MAP_PWM_MIN     7

static unsigned char shrink_pwm(int val)
{
	unsigned char shrink_br;

	/* define line segments */
	if (val <= ORIG_PWM_MIN)
		shrink_br = MAP_PWM_MIN;
	else if (val > ORIG_PWM_MIN && val <= ORIG_PWM_DEF)
		shrink_br = MAP_PWM_MIN +
			(val-ORIG_PWM_MIN)*(MAP_PWM_DEF-MAP_PWM_MIN)/(ORIG_PWM_DEF-ORIG_PWM_MIN);
	else
	shrink_br = MAP_PWM_DEF +
	(val-ORIG_PWM_DEF)*(MAP_PWM_MAX-MAP_PWM_DEF)/(ORIG_PWM_MAX-ORIG_PWM_DEF);

	return shrink_br;
}

static int dsi_j_qxga_8_9_bl_notify(struct device *unused, int brightness)
{
	/* Apply any backlight response curve */
	if (brightness > 255)
		pr_info("Error: Brightness > 255!\n");
	else if (brightness > 0 && brightness <= 255)
		brightness = shrink_pwm(brightness);

	return brightness;
}

static int dsi_j_qxga_8_9_check_fb(struct device *dev,
	struct fb_info *info)
{
	struct platform_device *pdev = NULL;
	pdev = to_platform_device(bus_find_device_by_name(
		&platform_bus_type, NULL, "tegradc.0"));
	return info->device == &pdev->dev;
}

static struct tegra_dsi_cmd dsi_j_qxga_8_9_backlight_cmd[] = {
       DSI_CMD_VBLANK_SHORT(DSI_DCS_WRITE_1_PARAM, 0x51, 0xFF, CMD_NOT_CLUBBED),
};

static struct tegra_dsi_bl_platform_data dsi_j_qxga_8_9_bl_data = {
	.dsi_backlight_cmd = dsi_j_qxga_8_9_backlight_cmd,
	.n_backlight_cmd = ARRAY_SIZE(dsi_j_qxga_8_9_backlight_cmd),
	.dft_brightness	= 127,
	.notify		= dsi_j_qxga_8_9_bl_notify,
	/* Only toggle backlight on fb blank notifications for disp1 */
	.check_fb	= dsi_j_qxga_8_9_check_fb,
};

static struct platform_device __maybe_unused
		dsi_j_qxga_8_9_bl_device = {
	.name	= "tegra-dsi-backlight",
	.dev	= {
		.platform_data = &dsi_j_qxga_8_9_bl_data,
	},
};

static struct platform_device __maybe_unused
			*dsi_j_qxga_8_9_bl_devices[] __initdata = {
	&dsi_j_qxga_8_9_bl_device,
};

static int __init dsi_j_qxga_8_9_register_bl_dev(void)
{
	int err = 0;
	err = platform_add_devices(dsi_j_qxga_8_9_bl_devices,
				ARRAY_SIZE(dsi_j_qxga_8_9_bl_devices));
	if (err) {
		pr_err("disp1 bl device registration failed");
		return err;
	}
	return err;
}
device_initcall(dsi_j_qxga_8_9_register_bl_dev);

struct tegra_panel_ops dsi_j_qxga_8_9_ops = {
	.enable = dsi_j_qxga_8_9_enable,
	.disable = dsi_j_qxga_8_9_disable,
	.postpoweron = dsi_j_qxga_8_9_postpoweron,
	.postsuspend = dsi_j_qxga_8_9_postsuspend,
};

struct tegra_panel __initdata dsi_j_qxga_8_9 = {
	.register_bl_dev = dsi_j_qxga_8_9_register_bl_dev,
};
EXPORT_SYMBOL(dsi_j_qxga_8_9);
