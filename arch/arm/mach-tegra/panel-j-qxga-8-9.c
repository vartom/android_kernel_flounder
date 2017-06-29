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
	int i, err;
	struct device_node *np;

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

	err = dsi_j_qxga_8_9_gpio_get();
	if (err) {
		pr_err("failed to get panel gpios\n");
		return err;
	}

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
	int err;

	err = dsi_j_qxga_8_9_gpio_get();
	if (err) {
		pr_err("failed to get panel gpios\n");
		return err;
	}

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

	gpio_set_value(lcm_rst, 0);
	msleep(1);
	gpio_set_value(dcdc_en, 0);
	msleep(15);
	gpio_set_value(avdd_4v, 0);
	gpio_set_value(iovdd_1v8, 0);
	msleep(10);

	return 0;
}

static int dsi_j_qxga_8_9_postsuspend(void)
{
	return 0;
}

static int dsi_j_qxga_8_9_bl_notify(struct device *dev, int brightness)
{
	int cur_sd_brightness;
	struct backlight_device *bl = NULL;
	struct pwm_bl_data *pb = NULL;
	bl = (struct backlight_device *)dev_get_drvdata(dev);
	pb = (struct pwm_bl_data *)dev_get_drvdata(&bl->dev);

	cur_sd_brightness = atomic_read(&sd_brightness);
	/* SD brightness is a percentage */
	brightness = (brightness * cur_sd_brightness) / 255;

	/* Apply any backlight response curve */
	if (brightness > 255)
		pr_info("Error: Brightness > 255!\n");
	else if (pb->bl_measured)
		brightness = pb->bl_measured[brightness];

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

static struct pwm_bl_data_dt_ops dsi_j_qxga_8_9_pwm_bl_ops = {
	.notify = dsi_j_qxga_8_9_bl_notify,
	.check_fb = dsi_j_qxga_8_9_check_fb,
	.blnode_compatible = "j,qxga-8-9-bl",
};
struct tegra_panel_ops dsi_j_qxga_8_9_ops = {
	.enable = dsi_j_qxga_8_9_enable,
	.disable = dsi_j_qxga_8_9_disable,
	.postpoweron = dsi_j_qxga_8_9_postpoweron,
	.postsuspend = dsi_j_qxga_8_9_postsuspend,
	.pwm_bl_ops = &dsi_j_qxga_8_9_pwm_bl_ops,
};
EXPORT_SYMBOL(dsi_j_qxga_8_9);
