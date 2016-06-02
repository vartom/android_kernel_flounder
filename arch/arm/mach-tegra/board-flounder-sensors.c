/*
 * arch/arm/mach-tegra/board-flounder-sensors.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
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

#include <asm/atomic.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mpu.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/nct1008.h>
#include <linux/of_platform.h>
#include <linux/pid_thermal_gov.h>
#include <linux/tegra-fuse.h>
#include <mach/edp.h>

#include <mach/io_dpd.h>
#include <media/camera.h>
#include <media/ar0261.h>
#include <media/imx135.h>
#include <media/dw9718.h>
#include <media/as364x.h>
#include <media/ov5693.h>
#include <media/ov7695.h>
#include <media/mt9m114.h>
#include <media/ad5823.h>
#include <media/max77387.h>
#include <media/imx219.h>
#include <media/ov9760.h>
#include <media/drv201.h>
#include <media/tps61310.h>

#include <linux/platform_device.h>
#include <linux/input/cy8c_sar.h>
#include <media/soc_camera.h>
#include <media/soc_camera_platform.h>
#include <media/tegra_v4l2_camera.h>
#include <linux/generic_adc_thermal.h>
#include <mach/board_htc.h>

#include <linux/platform/tegra/cpu-tegra.h>
#include "devices.h"
#include "board.h"
#include "board-common.h"
#include "board-flounder.h"
#include "tegra-board-id.h"


int cy8c_sar1_reset(void)
{
	pr_debug("[SAR] %s Enter\n", __func__);
	gpio_set_value_cansleep(TEGRA_GPIO_PG6, 1);
	msleep(5);
	gpio_set_value_cansleep(TEGRA_GPIO_PG6, 0);
	msleep(50);/*wait chip reset finish time*/
	return 0;
}

int cy8c_sar_reset(void)
{
	pr_debug("[SAR] %s Enter\n", __func__);
	gpio_set_value_cansleep(TEGRA_GPIO_PG7, 1);
	msleep(5);
	gpio_set_value_cansleep(TEGRA_GPIO_PG7, 0);
	msleep(50);/*wait chip reset finish time*/
	return 0;
}

int cy8c_sar_powerdown(int activate)
{
	int gpio = TEGRA_GPIO_PO5;
	int ret = 0;

	if (!is_mdm_modem()) {
		pr_debug("[SAR]%s:!is_mdm_modem()\n", __func__);
		return ret;
	}

	if (activate) {
		pr_debug("[SAR]:%s:gpio high,activate=%d\n",
			__func__, activate);
		ret = gpio_direction_output(gpio, 1);
		if (ret < 0)
			pr_debug("[SAR]%s: calling gpio_free(sar_modem)\n",
				__func__);
	} else {
		pr_debug("[SAR]:%s:gpio low,activate=%d\n", __func__, activate);
		ret = gpio_direction_output(gpio, 0);
		if (ret < 0)
			pr_debug("[SAR]%s: calling gpio_free(sar_modem)\n",
				__func__);
	}
	return ret;
}

static struct i2c_board_info flounder_i2c_board_info_cm32181[] = {
	{
		I2C_BOARD_INFO("cm32181", 0x48),
	},
};

struct cy8c_i2c_sar_platform_data sar1_cy8c_data[] = {
	{
		.gpio_irq = TEGRA_GPIO_PCC5,
		.reset    = cy8c_sar1_reset,
		.position_id = 1,
		.bl_addr = 0x61,
		.ap_addr = 0x5d,
		.powerdown    = cy8c_sar_powerdown,
	},
};

struct cy8c_i2c_sar_platform_data sar_cy8c_data[] = {
	{
		.gpio_irq = TEGRA_GPIO_PC7,
		.reset    = cy8c_sar_reset,
		.position_id = 0,
		.bl_addr = 0x60,
		.ap_addr = 0x5c,
		.powerdown    = cy8c_sar_powerdown,
	},
};

struct i2c_board_info flounder_i2c_board_info_cypress_sar[] = {
	{
		I2C_BOARD_INFO("CYPRESS_SAR", 0xB8 >> 1),
		.platform_data = &sar_cy8c_data,
		.irq = -1,
	},
};

struct i2c_board_info flounder_i2c_board_info_cypress_sar1[] = {
	{
		I2C_BOARD_INFO("CYPRESS_SAR1", 0xBA >> 1),
		.platform_data = &sar1_cy8c_data,
		.irq = -1,
	},
};

/*
 * Soc Camera platform driver for testing
 */
#if IS_ENABLED(CONFIG_SOC_CAMERA_PLATFORM)
static int flounder_soc_camera_add(struct soc_camera_device *icd);
static void flounder_soc_camera_del(struct soc_camera_device *icd);

static int flounder_soc_camera_set_capture(struct soc_camera_platform_info *info,
		int enable)
{
	/* TODO: probably add clk opertaion here */
	return 0; /* camera sensor always enabled */
}

static struct soc_camera_platform_info flounder_soc_camera_info = {
	.format_name = "RGB4",
	.format_depth = 32,
	.format = {
		.code = V4L2_MBUS_FMT_RGBA8888_4X8_LE,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.field = V4L2_FIELD_NONE,
		.width = 1280,
		.height = 720,
	},
	.set_capture = flounder_soc_camera_set_capture,
};

static struct tegra_camera_platform_data flounder_camera_platform_data = {
	.flip_v                 = 0,
	.flip_h                 = 0,
	.port                   = TEGRA_CAMERA_PORT_CSI_A,
	.lanes                  = 4,
	.continuous_clk         = 0,
};

static struct soc_camera_link flounder_soc_camera_link = {
	.bus_id         = 0, /* This must match the .id of tegra_vi01_device */
	.add_device     = flounder_soc_camera_add,
	.del_device     = flounder_soc_camera_del,
	.module_name    = "soc_camera_platform",
	.priv		= &flounder_camera_platform_data,
	.dev_priv	= &flounder_soc_camera_info,
};

static struct platform_device *flounder_pdev;

static void flounder_soc_camera_release(struct device *dev)
{
	soc_camera_platform_release(&flounder_pdev);
}

static int flounder_soc_camera_add(struct soc_camera_device *icd)
{
	return soc_camera_platform_add(icd, &flounder_pdev,
			&flounder_soc_camera_link,
			flounder_soc_camera_release, 0);
}

static void flounder_soc_camera_del(struct soc_camera_device *icd)
{
	soc_camera_platform_del(icd, flounder_pdev, &flounder_soc_camera_link);
}

static struct platform_device flounder_soc_camera_device = {
	.name   = "soc-camera-pdrv",
	.id     = 0,
	.dev    = {
		.platform_data = &flounder_soc_camera_link,
	},
};
#endif

static struct tegra_io_dpd csia_io = {
	.name			= "CSIA",
	.io_dpd_reg_index	= 0,
	.io_dpd_bit		= 0,
};

static struct tegra_io_dpd csib_io = {
	.name			= "CSIB",
	.io_dpd_reg_index	= 0,
	.io_dpd_bit		= 1,
};

static struct tegra_io_dpd csie_io = {
	.name			= "CSIE",
	.io_dpd_reg_index	= 1,
	.io_dpd_bit		= 12,
};

static atomic_t shared_gpios_refcnt = ATOMIC_INIT(0);

static void flounder_enable_shared_gpios(void)
{
	if (1 == atomic_add_return(1, &shared_gpios_refcnt)) {
		gpio_set_value(CAM_VCM2V85_EN, 1);
		usleep_range(100, 120);
		gpio_set_value(CAM_1V2_EN, 1);
		gpio_set_value(CAM_A2V85_EN, 1);
		gpio_set_value(CAM_1V8_EN, 1);
		pr_debug("%s\n", __func__);
	}
}

static void flounder_disable_shared_gpios(void)
{
	if (atomic_dec_and_test(&shared_gpios_refcnt)) {
		gpio_set_value(CAM_1V8_EN, 0);
		gpio_set_value(CAM_A2V85_EN, 0);
		gpio_set_value(CAM_1V2_EN, 0);
		gpio_set_value(CAM_VCM2V85_EN, 0);
		pr_debug("%s\n", __func__);
	}
}

static int flounder_imx219_power_on(struct imx219_power_rail *pw)
{
	/* disable CSIA/B IOs DPD mode to turn on camera for flounder */
	tegra_io_dpd_disable(&csia_io);
	tegra_io_dpd_disable(&csib_io);

	gpio_set_value(CAM_PWDN, 0);

	flounder_enable_shared_gpios();

	usleep_range(1, 2);
	gpio_set_value(CAM_PWDN, 1);

	usleep_range(300, 310);
	pr_debug("%s\n", __func__);
	return 1;
}

static int flounder_imx219_power_off(struct imx219_power_rail *pw)
{
	gpio_set_value(CAM_PWDN, 0);
	usleep_range(100, 120);
	pr_debug("%s\n", __func__);

	flounder_disable_shared_gpios();

	/* put CSIA/B IOs into DPD mode to save additional power for flounder */
	tegra_io_dpd_enable(&csia_io);
	tegra_io_dpd_enable(&csib_io);
	return 0;
}

static struct imx219_platform_data flounder_imx219_pdata = {
	.power_on = flounder_imx219_power_on,
	.power_off = flounder_imx219_power_off,
};

static int flounder_ov9760_power_on(struct ov9760_power_rail *pw)
{
	/* disable CSIE IO DPD mode to turn on camera for flounder */
	tegra_io_dpd_disable(&csie_io);

	gpio_set_value(CAM2_RST, 0);

	flounder_enable_shared_gpios();

	usleep_range(100, 120);
	gpio_set_value(CAM2_RST, 1);
	pr_debug("%s\n", __func__);

	return 1;
}

static int flounder_ov9760_power_off(struct ov9760_power_rail *pw)
{
	gpio_set_value(CAM2_RST, 0);
	usleep_range(100, 120);
	pr_debug("%s\n", __func__);

	flounder_disable_shared_gpios();

	/* put CSIE IOs into DPD mode to save additional power for flounder */
	tegra_io_dpd_enable(&csie_io);

	return 0;
}

static struct ov9760_platform_data flounder_ov9760_pdata = {
	.power_on = flounder_ov9760_power_on,
	.power_off = flounder_ov9760_power_off,
	.mclk_name = "mclk2",
};

static int flounder_drv201_power_on(struct drv201_power_rail *pw)
{
	gpio_set_value(CAM_VCM_PWDN, 0);

	flounder_enable_shared_gpios();

	gpio_set_value(CAM_VCM_PWDN, 1);
	usleep_range(100, 120);
	pr_debug("%s\n", __func__);

	return 1;
}

static int flounder_drv201_power_off(struct drv201_power_rail *pw)
{
	gpio_set_value(CAM_VCM_PWDN, 0);
	usleep_range(100, 120);
	pr_debug("%s\n", __func__);

	flounder_disable_shared_gpios();

	return 1;
}

static struct nvc_focus_cap flounder_drv201_cap = {
	.version = NVC_FOCUS_CAP_VER2,
	.settle_time = 15,
	.focus_macro = 810,
	.focus_infinity = 50,
	.focus_hyper = 50,
};

static struct drv201_platform_data flounder_drv201_pdata = {
	.cfg = 0,
	.num = 0,
	.sync = 0,
	.dev_name = "focuser",
	.cap = &flounder_drv201_cap,
	.power_on	= flounder_drv201_power_on,
	.power_off	= flounder_drv201_power_off,
};

static struct nvc_torch_pin_state flounder_tps61310_pinstate = {
	.mask		= 1 << (CAM_FLASH_STROBE - TEGRA_GPIO_PBB0), /* VGP4 */
	.values		= 1 << (CAM_FLASH_STROBE - TEGRA_GPIO_PBB0),
};

static struct tps61310_platform_data flounder_tps61310_pdata = {
	.dev_name	= "torch",
	.pinstate	= &flounder_tps61310_pinstate,
};

static struct camera_data_blob flounder_camera_lut[] = {
	{"flounder_imx219_pdata", &flounder_imx219_pdata},
	{"flounder_drv201_pdata", &flounder_drv201_pdata},
	{"flounder_tps61310_pdata", &flounder_tps61310_pdata},
	{"flounder_ov9760_pdata", &flounder_ov9760_pdata},
	{},
};

void __init flounder_camera_auxdata(void *data)
{
	struct of_dev_auxdata *aux_lut = data;
	while (aux_lut && aux_lut->compatible) {
		if (!strcmp(aux_lut->compatible, "nvidia,tegra124-camera")) {
			pr_info("%s: update camera lookup table.\n", __func__);
			aux_lut->platform_data = flounder_camera_lut;
		}
		aux_lut++;
	}
}

static int flounder_camera_init(void)
{
	pr_debug("%s: ++\n", __func__);

	tegra_io_dpd_enable(&csia_io);
	tegra_io_dpd_enable(&csib_io);
	tegra_io_dpd_enable(&csie_io);

#if IS_ENABLED(CONFIG_SOC_CAMERA_PLATFORM)
	platform_device_register(&flounder_soc_camera_device);
#endif
	return 0;
}

static struct pid_thermal_gov_params cpu_pid_params = {
	.max_err_temp = 4000,
	.max_err_gain = 1000,

	.gain_p = 1000,
	.gain_d = 0,

	.up_compensation = 15,
	.down_compensation = 15,
};

static struct thermal_zone_params cpu_tzp = {
	.governor_name = "pid_thermal_gov",
	.governor_params = &cpu_pid_params,
};

static struct thermal_zone_params board_tzp = {
	.governor_name = "pid_thermal_gov"
};

static struct nct1008_platform_data flounder_nct72_pdata = {
	.loc_name = "tegra",
	.supported_hwrev = true,
	.conv_rate = 0x06, /* 4Hz conversion rate */
	.offset = 0,
	.extended_range = true,

	.sensors = {
		[LOC] = {
			.tzp = &board_tzp,
			.shutdown_limit = 120, /* C */
			.passive_delay = 1000,
			.num_trips = 1,
			.trips = {
				{
					.cdev_type = "therm_est_activ",
					.trip_temp = 40000,
					.trip_type = THERMAL_TRIP_ACTIVE,
					.hysteresis = 1000,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
					.mask = 1,
				},
			},
		},
		[EXT] = {
			.tzp = &cpu_tzp,
			.shutdown_limit = 95, /* C */
			.passive_delay = 1000,
			.num_trips = 2,
			.trips = {
				{
					.cdev_type = "shutdown_warning",
					.trip_temp = 93000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
					.mask = 0,
				},
				{
					.cdev_type = "cpu-balanced",
					.trip_temp = 83000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
					.hysteresis = 1000,
					.mask = 1,
				},
			}
		}
	}
};

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
static struct pid_thermal_gov_params skin_pid_params = {
	.max_err_temp = 4000,
	.max_err_gain = 1000,

	.gain_p = 1000,
	.gain_d = 0,

	.up_compensation = 15,
	.down_compensation = 15,
};

static struct thermal_zone_params skin_tzp = {
	.governor_name = "pid_thermal_gov",
	.governor_params = &skin_pid_params,
};

static struct nct1008_platform_data flounder_nct72_tskin_pdata = {
	.loc_name = "skin",

	.supported_hwrev = true,
	.conv_rate = 0x06, /* 4Hz conversion rate */
	.offset = 0,
	.extended_range = true,

	.sensors = {
		[LOC] = {
			.shutdown_limit = 95, /* C */
			.num_trips = 0,
			.tzp = NULL,
		},
		[EXT] = {
			.shutdown_limit = 85, /* C */
			.passive_delay = 10000,
			.polling_delay = 1000,
			.tzp = &skin_tzp,
			.num_trips = 1,
			.trips = {

	{
		.cdev_type = "skin-balanced",


					.trip_temp = 50000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
					.mask = 1,
				},
			},
		}
	}
};
#endif

static struct i2c_board_info flounder_i2c_nct72_board_info[] = {
	{
		I2C_BOARD_INFO("nct72", 0x4c),
		.platform_data = &flounder_nct72_pdata,
		.irq = -1,
	},
#ifdef CONFIG_TEGRA_SKIN_THROTTLE
	{
		I2C_BOARD_INFO("nct72", 0x4d),
		.platform_data = &flounder_nct72_tskin_pdata,
		.irq = -1,
	}
#endif
};

static int flounder_nct72_init(void)
{
	int nct72_port = TEGRA_GPIO_PI6;
	int ret = 0;
	int i;
	struct thermal_trip_info *trip_state;

	/* raise NCT's thresholds if soctherm CP,FT fuses are ok */
	if ((tegra_fuse_calib_base_get_cp(NULL, NULL) >= 0) &&
	    (tegra_fuse_calib_base_get_ft(NULL, NULL) >= 0)) {
		flounder_nct72_pdata.sensors[EXT].shutdown_limit += 20;
		for (i = 0; i < flounder_nct72_pdata.sensors[EXT].num_trips;
			 i++) {
			trip_state = &flounder_nct72_pdata.sensors[EXT].trips[i];
			if (!strncmp(trip_state->cdev_type, "cpu-balanced",
					THERMAL_NAME_LENGTH)) {
				trip_state->cdev_type = "_none_";
				break;
			}
		}
	} else {
		tegra_platform_edp_init(
			flounder_nct72_pdata.sensors[EXT].trips,
			&flounder_nct72_pdata.sensors[EXT].num_trips,
					12000); /* edp temperature margin */
		tegra_add_cpu_vmax_trips(
			flounder_nct72_pdata.sensors[EXT].trips,
			&flounder_nct72_pdata.sensors[EXT].num_trips);
		tegra_add_tgpu_trips(
			flounder_nct72_pdata.sensors[EXT].trips,
			&flounder_nct72_pdata.sensors[EXT].num_trips);
		tegra_add_vc_trips(
			flounder_nct72_pdata.sensors[EXT].trips,
			&flounder_nct72_pdata.sensors[EXT].num_trips);
		tegra_add_core_vmax_trips(
			flounder_nct72_pdata.sensors[EXT].trips,
			&flounder_nct72_pdata.sensors[EXT].num_trips);
	}

	tegra_add_all_vmin_trips(flounder_nct72_pdata.sensors[EXT].trips,
		&flounder_nct72_pdata.sensors[EXT].num_trips);

	flounder_i2c_nct72_board_info[0].irq = gpio_to_irq(nct72_port);

	ret = gpio_request(nct72_port, "temp_alert");
	if (ret < 0)
		return ret;

	ret = gpio_direction_input(nct72_port);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(nct72_port)", __func__);
		gpio_free(nct72_port);
	}

	i2c_register_board_info(0, flounder_i2c_nct72_board_info,
	ARRAY_SIZE(flounder_i2c_nct72_board_info));

	return ret;
}

static int powerdown_gpio_init(void){
	int ret = 0;
	static int done;
	if (!is_mdm_modem()) {
		pr_debug("[SAR]%s:!is_mdm_modem()\n", __func__);
		return ret;
	}

	if (done == 0) {
		if (!gpio_request(TEGRA_GPIO_PO5, "sar_modem")) {
			pr_debug("[SAR]%s:gpio_request success\n", __func__);
			ret = gpio_direction_output(TEGRA_GPIO_PO5, 0);
			if (ret < 0) {
				pr_debug(
					"[SAR]%s: calling gpio_free(sar_modem)",
					__func__);
				gpio_free(TEGRA_GPIO_PO5);
			}
			done = 1;
		}
	}
	return ret;
}

static int flounder_sar_init(void){
	int sar_intr = TEGRA_GPIO_PC7;
	int ret;
	pr_info("%s: GPIO pin:%d\n", __func__, sar_intr);
	flounder_i2c_board_info_cypress_sar[0].irq = gpio_to_irq(sar_intr);
	ret = gpio_request(sar_intr, "sar_interrupt");
	if (ret < 0)
		return ret;
	ret = gpio_direction_input(sar_intr);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(sar_intr)", __func__);
		gpio_free(sar_intr);
	}
	powerdown_gpio_init();
	i2c_register_board_info(1, flounder_i2c_board_info_cypress_sar,
			ARRAY_SIZE(flounder_i2c_board_info_cypress_sar));
	return 0;
}

static int flounder_sar1_init(void){
	int sar1_intr = TEGRA_GPIO_PCC5;
	int ret;
	pr_info("%s: GPIO pin:%d\n", __func__, sar1_intr);
	flounder_i2c_board_info_cypress_sar1[0].irq = gpio_to_irq(sar1_intr);
	ret  = gpio_request(sar1_intr, "sar1_interrupt");
	if (ret < 0)
		return ret;
	ret = gpio_direction_input(sar1_intr);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(sar1_intr)", __func__);
		gpio_free(sar1_intr);
	}
	powerdown_gpio_init();
	i2c_register_board_info(1, flounder_i2c_board_info_cypress_sar1,
			ARRAY_SIZE(flounder_i2c_board_info_cypress_sar1));
	return 0;
}

int __init flounder_sensors_init(void)
{
	flounder_camera_init();
	flounder_nct72_init();
	flounder_sar_init();
	flounder_sar1_init();

	i2c_register_board_info(0, flounder_i2c_board_info_cm32181,
		ARRAY_SIZE(flounder_i2c_board_info_cm32181));

	return 0;
}
