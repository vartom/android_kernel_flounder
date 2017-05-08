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

/*
static bool reg_requested;
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
	{TEGRA_GPIO_PQ2,	GPIOF_OUT_INIT_HIGH,    "lcmio_1v8"},
	{TEGRA_GPIO_PR0,	GPIOF_OUT_INIT_HIGH,    "avdd_4v"},
	{TEGRA_GPIO_PEE5,	GPIOF_OUT_INIT_HIGH,    "dcdc_en"},
	{TEGRA_GPIO_PH5,	GPIOF_OUT_INIT_HIGH,	"panel_rst"},
};

/*static int dsi_j_qxga_8_9_regulator_get(struct device *dev)
{
	int err = 0;

	if (reg_requested)
		return 0;

	avdd_lcd_3v0 = regulator_get(dev, "avdd_lcd");
	if (IS_ERR_OR_NULL(avdd_lcd_3v0)) {
		pr_err("avdd_lcd regulator get failed\n");
		err = PTR_ERR(avdd_lcd_3v0);
		avdd_lcd_3v0 = NULL;
		goto fail;
	}

	dvdd_lcd_1v8 = regulator_get(dev, "dvdd_lcd");
	if (IS_ERR_OR_NULL(dvdd_lcd_1v8)) {
		pr_err("dvdd_lcd_1v8 regulator get failed\n");
		err = PTR_ERR(dvdd_lcd_1v8);
		dvdd_lcd_1v8 = NULL;
		goto fail;
	}

	vpp_lcd = regulator_get(dev, "outp");
	if (IS_ERR_OR_NULL(vpp_lcd)) {
		pr_err("vpp_lcd regulator get failed\n");
		err = PTR_ERR(vpp_lcd);
		vpp_lcd = NULL;
		goto fail;
	}

	vmm_lcd = regulator_get(dev, "outn");
	if (IS_ERR_OR_NULL(vmm_lcd)) {
		pr_err("vmm_lcd regulator get failed\n");
		err = PTR_ERR(vmm_lcd);
		vmm_lcd = NULL;
		goto fail;
	}

	reg_requested = true;
	return 0;
fail:
	return err;
}*/

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

	err = tegra_panel_gpio_get_dt("j,qxga-8-9", &panel_of);
	if (err < 0) {
		pr_err("dsi gpio request failed\n");
		goto fail;
	}

	gpio_set_value(avdd_4v, 1);
	usleep_range(1 * 1000, 1 * 1000 + 500);
	gpio_set_value(dcdc_en, 1);
	usleep_range(15 * 1000, 15 * 1000 + 500);
	gpio_set_value(lcm_rst, 1);
	usleep_range(15 * 1000, 15 * 1000 + 500);

	return 0;
fail:
	return err;
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

#define ORIG_PWM_MAX 255
#define ORIG_PWM_DEF 133
#define ORIG_PWM_MIN 0

#define MAP_PWM_MAX     255
#define MAP_PWM_DEF     90
#define MAP_PWM_MIN     3

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

static struct pwm_bl_data_dt_ops dsi_j_qxga_8_9_pwm_bl_ops = {
	.notify = dsi_j_qxga_8_9_bl_notify,
	.check_fb = dsi_j_qxga_8_9_check_fb,
	.blnode_compatible = "tegra-dsi-backlight",
};
struct tegra_panel_ops dsi_j_qxga_8_9_ops = {
	.enable = dsi_j_qxga_8_9_enable,
	.disable = dsi_j_qxga_8_9_disable,
	.postpoweron = dsi_j_qxga_8_9_postpoweron,
	.postsuspend = dsi_j_qxga_8_9_postsuspend,
	.pwm_bl_ops = &dsi_j_qxga_8_9_pwm_bl_ops,
};
EXPORT_SYMBOL(dsi_j_qxga_8_9);
