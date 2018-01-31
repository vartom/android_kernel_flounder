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


/*static bool reg_requested;
static struct regulator *avdd_lcd_3v0;
static struct regulator *dvdd_lcd_1v8;
static struct regulator *vpp_lcd;
static struct regulator *vmm_lcd;
static struct device *dc_dev;
static u16 en_panel_rst;*/

enum panel_gpios {
	IOVDD_1V8 = 0,
	AVDD_4V,
	DCDC_EN,
	LCM_RST,
	NUM_PANEL_GPIOS,
};

static bool gpio_requested;
static bool gpio_init;
//static struct platform_device *disp_device;

static int iovdd_1v8, avdd_4v, dcdc_en, lcm_rst;

static struct gpio panel_init_gpios[] = {
	{TEGRA_GPIO_PQ2,	GPIOF_OUT_INIT_HIGH,    "iovdd_1v8"},
	{TEGRA_GPIO_PR0,	GPIOF_OUT_INIT_HIGH,    "avdd_4v"},
	{TEGRA_GPIO_PEE5,	GPIOF_OUT_INIT_HIGH,    "dcdc_en"},
	{TEGRA_GPIO_PH5,	GPIOF_OUT_INIT_HIGH,	"lcm_rst"},
};

static int dsi_j_qxga_8_9_gpio_get(void)
{
	int err;

	if (gpio_requested)
		return 0;

	err = gpio_request_array(panel_init_gpios, ARRAY_SIZE(panel_init_gpios));
	if(err) {
		pr_err("gpio array request failed\n");
		return err;
	}

	gpio_requested = true;

	return 0;
}

static int dsi_j_qxga_8_9_postpoweron(struct device *dev)
{
	int err;

	err = dsi_j_qxga_8_9_gpio_get();
	if (err) {
		pr_err("failed to get panel gpios\n");
		return err;
	}

/*	pr_info("panel dsi_j_qxga_8_9_postpoweron\n");*/
	gpio_set_value(avdd_4v, 1);
	usleep_range(1 * 1000, 1 * 1000 + 500);
	gpio_set_value(dcdc_en, 1);
	usleep_range(15 * 1000, 15 * 1000 + 500);
	gpio_set_value(lcm_rst, 1);
	usleep_range(15 * 1000, 15 * 1000 + 500);

	return 0;
}

static int dsi_j_qxga_8_9_enable(struct device *dev)
{
	int i, err;
	struct device_node *np;

	if (!gpio_init) {
		np = of_find_node_by_name(NULL, "panel_jdi_qxga_8_9");
		if (np == NULL) {
			pr_info("can't find device node\n");
		} else {
			for (i=0; i<NUM_PANEL_GPIOS; i++) {
				panel_init_gpios[i].gpio =
					of_get_gpio_flags(np, i, NULL);
				pr_info("gpio pin = %d\n", panel_init_gpios[i].gpio);
			}
		}

		iovdd_1v8 = panel_init_gpios[IOVDD_1V8].gpio;
		avdd_4v = panel_init_gpios[AVDD_4V].gpio;
		dcdc_en = panel_init_gpios[DCDC_EN].gpio;
		lcm_rst = panel_init_gpios[LCM_RST].gpio;

		gpio_init = true;
	}

	err = dsi_j_qxga_8_9_gpio_get();
	if (err) {
		pr_err("failed to get panel gpios\n");
		return err;
	}

/*	pr_info("panel dsi_j_qxga_8_9_enable\n");*/
	gpio_set_value(iovdd_1v8, 1);
	usleep_range(15 * 1000, 15 * 1000 + 500);
	return 0;
}

static int dsi_j_qxga_8_9_disable(struct device *dev)
{
	int err;

	err = dsi_j_qxga_8_9_gpio_get();
	if (err) {
		pr_err("failed to get panel gpios\n");
		return err;
	}

/*	pr_info("panel dsi_j_qxga_8_9_disable\n");*/
	gpio_set_value(lcm_rst, 0);
	msleep(1);
	gpio_set_value(dcdc_en, 0);
	msleep(15);
	gpio_set_value(avdd_4v, 0);
	gpio_set_value(iovdd_1v8, 0);
	msleep(10);

/*	pr_info("panel dsi_j_qxga_8_9_disable 2\n");*/
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
