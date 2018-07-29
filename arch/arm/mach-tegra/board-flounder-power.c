/*
 * arch/arm/mach-tegra/board-flounder-power.c
 *
 * Copyright (c) 2013-2014, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/io.h>
#include <mach/irqs.h>
#include <mach/edp.h>
#include <linux/platform_data/tegra_edp.h>
#include <linux/pid_thermal_gov.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/regulator/tegra-dfll-bypass-regulator.h>
#include <linux/tegra-fuse.h>
#include <linux/tegra-pmc.h>
#include <linux/pinctrl/pinconf-tegra.h>

#include <linux/system-wakeup.h>
#include <linux/syscore_ops.h>
#include <linux/delay.h>
#include <linux/mfd/palmas.h>
#include <linux/power/power_supply_extcon.h>

#include <asm/mach-types.h>
#include <linux/tegra_soctherm.h>

#include "pm.h"
#include <linux/platform/tegra/dvfs.h>
#include "board.h"
#include <linux/platform/tegra/common.h>
#include "tegra-board-id.h"
#include "board-common.h"
#include "board-flounder.h"
#include "board-pmu-defines.h"
#include "devices.h"
#include "iomap.h"
#include <linux/platform/tegra/tegra_cl_dvfs.h>

#if defined(CONFIG_ARCH_TEGRA_21x_SOC)

#define PMC_CTRL                0x0
#define PMC_CTRL_INTR_LOW       (1 << 17)

/************************ FLOUNDER CL-DVFS DATA *********************/
#define FLOUNDER_DEFAULT_CVB_ALIGNMENT	10000

#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
static struct tegra_cl_dvfs_cfg_param e1736_flounder_cl_dvfs_param = {
	.sample_rate = 12500, /* i2c freq */

	.force_mode = TEGRA_CL_DVFS_FORCE_FIXED,
	.cf = 10,
	.ci = 0,
	.cg = 2,

	.droop_cut_value = 0xF,
	.droop_restore_ramp = 0x0,
	.scale_out_ramp = 0x0,
};

/* E1736 volatge map. Fixed 10mv steps from 700mv to 1400mv */
#define E1736_CPU_VDD_MAP_SIZE ((1400000 - 700000) / 10000 + 1)
static struct voltage_reg_map e1736_cpu_vdd_map[E1736_CPU_VDD_MAP_SIZE];
static inline void e1736_fill_reg_map(void)
{
	int i;
	for (i = 0; i < E1736_CPU_VDD_MAP_SIZE; i++) {
		/* 0.7V corresponds to 0b0011010 = 26 */
		/* 1.4V corresponds to 0b1100000 = 96 */
		e1736_cpu_vdd_map[i].reg_value = i + 26;
		e1736_cpu_vdd_map[i].reg_uV = 700000 + 10000 * i;
	}
}

static struct tegra_cl_dvfs_platform_data e1736_cl_dvfs_data = {
	.dfll_clk_name = "dfll_cpu",
	.pmu_if = TEGRA_CL_DVFS_PMU_I2C,
	.u.pmu_i2c = {
		.fs_rate = 400000,
		.slave_addr = 0xb0, /* pmu i2c address */
		.reg = 0x23,        /* vdd_cpu rail reg address */
	},
	.vdd_map = e1736_cpu_vdd_map,
	.vdd_map_size = E1736_CPU_VDD_MAP_SIZE,

	.cfg_param = &e1736_flounder_cl_dvfs_param,
};

static int __init flounder_cl_dvfs_init(void)
{
	struct tegra_cl_dvfs_platform_data *data = NULL;
		struct device_node *dn = of_find_compatible_node(
			NULL, NULL, "nvidia,tegra132-dfll");
	/*
	 * flounder platforms maybe used with different DT variants. Some of them
	 * include DFLL data in DT, some - not. Check DT here, and continue with
	 * platform device registration only if DT DFLL node is not present.
	 */
	if (dn) {
		bool available = of_device_is_available(dn);
		of_node_put(dn);

		if (available)
			return 0;
	}

	e1736_fill_reg_map();
	data = &e1736_cl_dvfs_data;

	if (data) {
		data->flags = TEGRA_CL_DVFS_DYN_OUTPUT_CFG;
		tegra_cl_dvfs_device.dev.platform_data = data;
		platform_device_register(&tegra_cl_dvfs_device);
	}
	return 0;
}
#else
static inline int flounder_cl_dvfs_init()
{ return 0; }
#endif

int __init flounder_regulator_init(void)
{
	flounder_cl_dvfs_init();
	return 0;
}
#endif

static struct pid_thermal_gov_params soctherm_pid_params = {
	.max_err_temp = 9000,
	.max_err_gain = 1000,

	.gain_p = 1000,
	.gain_d = 0,

	.up_compensation = 20,
	.down_compensation = 20,
};

static struct thermal_zone_params soctherm_tzp = {
	.governor_name = "pid_thermal_gov",
	.governor_params = &soctherm_pid_params,
};

static struct tegra_thermtrip_pmic_data tpdata_palmas = {
	.reset_tegra = 1,
	.pmu_16bit_ops = 0,
	.controller_type = 0,
	.pmu_i2c_addr = 0x58,
	.i2c_controller_id = 4,
	.poweroff_reg_addr = 0xa0,
	.poweroff_reg_data = 0x0,
};

/*
 * @PSKIP_CONFIG_NOTE: For T132, throttling config of PSKIP is no longer
 * done in soctherm registers. These settings are now done via registers in
 * denver:ccroc module which are at a different register offset. More
 * importantly, there are _only_ three levels of throttling: 'low',
 * 'medium' and 'heavy' and are selected via the 'throttling_depth' field
 * in the throttle->devs[] section of the soctherm config. Since the depth
 * specification is per device, it is necessary to manually make sure the
 * depths specified alongwith a given level are the same across all devs,
 * otherwise it will overwrite a previously set depth with a different
 * depth. We will refer to this comment at each relevant location in the
 * config sections below.
 */
static struct soctherm_platform_data flounder_soctherm_data = {
	.oc_irq_base = TEGRA_SOC_OC_IRQ_BASE,
	.num_oc_irqs = TEGRA_SOC_OC_NUM_IRQ,
	.therm = {
		[THERM_CPU] = {
			.zone_enable = true,
			.passive_delay = 1000,
			.hotspot_offset = 10000,
			.num_trips = 3,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000,
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-heavy",
					.trip_temp = 99000,
					.trip_type = THERMAL_TRIP_HOT,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "cpu-balanced",
					.trip_temp = 85000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
		[THERM_GPU] = {
			.zone_enable = true,
			.passive_delay = 1000,
			.hotspot_offset = 5000,
			.num_trips = 3,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000,
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-heavy",
					.trip_temp = 99000,
					.trip_type = THERMAL_TRIP_HOT,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "gpu-balanced",
					.trip_temp = 85000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
		[THERM_MEM] = {
			.zone_enable = true,
			.num_trips = 1,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000, /* = GPU shut */
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
		[THERM_PLL] = {
			.zone_enable = true,
			.num_trips = 1,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000, /* = GPU shut */
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
	},
	.throttle = {
		[THROTTLE_HEAVY] = {
			.priority = 100,
			.devs = {
				[THROTTLE_DEV_CPU] = {
					.enable = true,
					.depth = 80,
					/* see @PSKIP_CONFIG_NOTE */
					.throttling_depth = "medium_throttling",
				},
				[THROTTLE_DEV_GPU] = {
					.enable = true,
					.throttling_depth = "heavy_throttling",
				},
			},
		},
	},
};

static struct soctherm_throttle battery_oc_throttle_t13x = {
	.throt_mode = BRIEF,
	.polarity = SOCTHERM_ACTIVE_LOW,
	.priority = 50,
	.intr = true,
	.alarm_cnt_threshold = 15,
	.alarm_filter = 5100000,
	.devs = {
		[THROTTLE_DEV_CPU] = {
			.enable = true,
			.depth = 50,
			/* see @PSKIP_CONFIG_NOTE */
			.throttling_depth = "low_throttling",
		},
		[THROTTLE_DEV_GPU] = {
			.enable = true,
			.throttling_depth = "low_throttling",
		},
	},
};

int __init flounder_soctherm_init(void)
{
	const int t13x_cpu_edp_temp_margin = 8000,
		t13x_gpu_edp_temp_margin = 8000;
	int cpu_edp_temp_margin, gpu_edp_temp_margin;
	int cp_rev, ft_rev;
	enum soctherm_therm_id therm_cpu = THERM_CPU;

	cp_rev = tegra_fuse_calib_base_get_cp(NULL, NULL);
	ft_rev = tegra_fuse_calib_base_get_ft(NULL, NULL);

	/* TODO: remove this part once bootloader changes merged */
	tegra_gpio_disable(TEGRA_GPIO_PJ2);
	tegra_gpio_disable(TEGRA_GPIO_PS7);

	cpu_edp_temp_margin = t13x_cpu_edp_temp_margin;
	gpu_edp_temp_margin = t13x_gpu_edp_temp_margin;

	/* do this only for supported CP,FT fuses */
	if ((cp_rev >= 0) && (ft_rev >= 0)) {
		tegra_platform_edp_init(
			flounder_soctherm_data.therm[therm_cpu].trips,
			&flounder_soctherm_data.therm[therm_cpu].num_trips,
			cpu_edp_temp_margin);
		tegra_platform_gpu_edp_init(
			flounder_soctherm_data.therm[THERM_GPU].trips,
			&flounder_soctherm_data.therm[THERM_GPU].num_trips,
			gpu_edp_temp_margin);
		tegra_add_cpu_vmax_trips(
			flounder_soctherm_data.therm[therm_cpu].trips,
			&flounder_soctherm_data.therm[therm_cpu].num_trips);
		tegra_add_tgpu_trips(
			flounder_soctherm_data.therm[THERM_GPU].trips,
			&flounder_soctherm_data.therm[THERM_GPU].num_trips);
		tegra_add_core_vmax_trips(
			flounder_soctherm_data.therm[THERM_PLL].trips,
			&flounder_soctherm_data.therm[THERM_PLL].num_trips);
	}

	tegra_add_cpu_vmin_trips(
		flounder_soctherm_data.therm[therm_cpu].trips,
		&flounder_soctherm_data.therm[therm_cpu].num_trips);
	tegra_add_gpu_vmin_trips(
		flounder_soctherm_data.therm[THERM_GPU].trips,
		&flounder_soctherm_data.therm[THERM_GPU].num_trips);
	tegra_add_core_vmin_trips(
		flounder_soctherm_data.therm[THERM_PLL].trips,
		&flounder_soctherm_data.therm[THERM_PLL].num_trips);

	flounder_soctherm_data.tshut_pmu_trip_data = &tpdata_palmas;
	/* Enable soc_therm OC throttling on selected platforms */
	memcpy(&flounder_soctherm_data.throttle[THROTTLE_OC4],
		       &battery_oc_throttle_t13x,
		       sizeof(battery_oc_throttle_t13x));
	return tegra_soctherm_init(&flounder_soctherm_data);
}
