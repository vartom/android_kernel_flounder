/*
 * arch/arm/mach-tegra/board-flounder.c
 *
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/i2c/i2c-hid.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/platform_data/tegra_c2port_platform_data.h>
#include <linux/spi/spi.h>
#include <linux/spi/rm31080a_ts.h>
#include <linux/memblock.h>
#include <linux/spi/spi-tegra.h>
#include <linux/nfc/bcm2079x.h>
#include <linux/rfkill-gpio.h>
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/smb349-charger.h>
#include <linux/max17048_battery.h>
#include <linux/leds.h>
#include <linux/i2c/at24.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/i2c-tegra.h>
#include <linux/tegra-soc.h>
#include <linux/tegra-powergate.h>
#include <linux/platform_data/serial-tegra.h>
#include <linux/edp.h>
#include <linux/usb/tegra_usb_phy.h>
#include <linux/mfd/palmas.h>
#include <linux/clk/tegra.h>
#include <linux/clocksource.h>
#include <linux/irqchip.h>
#include <linux/irqchip/tegra.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-tegra.h>

#include <linux/tegra_fiq_debugger.h>
#include <media/tegra_dtv.h>
#include <linux/pci-tegra.h>

#include <linux/platform/tegra/tegra12_emc.h>
#include <mach/irqs.h>
#include <mach/io_dpd.h>
#include <mach/i2s.h>
#include <linux/platform/tegra/isomgr.h>
#include <mach/tegra_asoc_pdata.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/system_info.h>
#include <mach/xusb.h>

#include <mach/dc.h>
#include <mach/tegra_usb_pad_ctrl.h>

#include <linux/platform_data/tegra_usb.h>
#include <linux/platform_data/tegra_ahci.h>
#include <../../../drivers/staging/android/timed_gpio.h>
#include <linux/platform_data/gpio-tegra.h>

#include <mach/flounder-bdaddress.h>
#include "bcm_gps_hostwake.h"

#include "board.h"
#include "board-flounder.h"
#include "board-common.h"
#include "board-panel.h"
#include <linux/platform/tegra/clock.h>
#include <linux/platform/tegra/common.h>
#include "devices.h"
#include "gpio-names.h"
#include "pm.h"
#include "tegra-board-id.h"
#include "iomap.h"
#include "tegra-of-dev-auxdata.h"
#include <linux/tegra-pm.h>

/*static unsigned int flounder_hw_rev;
static unsigned int flounder_eng_id;

static int __init flounder_hw_revision(char *id)
{
	int ret;
	char *hw_rev;

	hw_rev = strsep(&id, ".");

	ret = kstrtouint(hw_rev, 10, &flounder_hw_rev);
	if (ret < 0) {
		pr_err("Failed to parse flounder hw_revision=%s\n", hw_rev);
		return ret;
	}

	if (id) {
		ret = kstrtouint(id, 10, &flounder_eng_id);
		if (ret < 0) {
			pr_err("Failed to parse flounder eng_id=%s\n", id);
			return ret;
		}
	}

	pr_info("Flounder hardware revision = %d, engineer id = %d\n",
            flounder_hw_rev, flounder_eng_id);

	return 0;
}
early_param("hw_revision", flounder_hw_revision);

int flounder_get_hw_revision(void)
{
	return flounder_hw_rev;
}

int flounder_get_eng_id(void)
{
	return flounder_eng_id;
}*/

static __initdata struct tegra_clk_init_table flounder_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x", "pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	48000000,	false},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s2",	"pll_a_out0",	0,		false},
	{ "i2s3",	"pll_a_out0",	0,		false},
	{ "i2s4",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ "d_audio",	"clk_m",	12000000,	false},
	{ "dam0",	"clk_m",	12000000,	false},
	{ "dam1",	"clk_m",	12000000,	false},
	{ "dam2",	"clk_m",	12000000,	false},
	{ "audio1",	"i2s1_sync",	0,		false},
	{ "audio2",	"i2s2_sync",	0,		false},
	{ "audio3",	"i2s3_sync",	0,		false},
	{ "vi_sensor",	"pll_p",	150000000,	false},
	{ "vi_sensor2",	"pll_p",	150000000,	false},
	{ "cilab",	"pll_p",	150000000,	false},
	{ "cilcd",	"pll_p",	150000000,	false},
	{ "cile",	"pll_p",	150000000,	false},
	{ "i2c1",	"pll_p",	3200000,	false},
	{ "i2c2",	"pll_p",	3200000,	false},
	{ "i2c3",	"pll_p",	3200000,	false},
	{ "i2c4",	"pll_p",	3200000,	false},
	{ "i2c5",	"pll_p",	3200000,	false},
	{ "sbc1",	"pll_p",	25000000,	false},
	{ "sbc2",	"pll_p",	25000000,	false},
	{ "sbc3",	"pll_p",	25000000,	false},
	{ "sbc4",	"pll_p",	25000000,	false},
	{ "sbc5",	"pll_p",	25000000,	false},
	{ "sbc6",	"pll_p",	25000000,	false},
	{ "uarta",	"pll_p",	408000000,	false},
	{ "uartb",	"pll_p",	408000000,	false},
	{ "uartc",	"pll_p",	408000000,	false},
	{ "uartd",	"pll_p",	408000000,	false},
	{ NULL,		NULL,		0,		0},
};

static struct timed_gpio flounder_vib_timed_gpios[] = {
	{
		.name = "vibrator",
		.gpio = TEGRA_GPIO_PU3,
		.max_timeout = 15000,
	},
};

static struct timed_gpio_platform_data flounder_vib_pdata = {
	.num_gpios = ARRAY_SIZE(flounder_vib_timed_gpios),
	.gpios     = flounder_vib_timed_gpios,
};

static struct platform_device flounder_vib_device = {
	.name = TIMED_GPIO_NAME,
	.id   = -1,
	.dev  = {
		.platform_data = &flounder_vib_pdata,
	},
};

static struct platform_device *flounder_devices[] __initdata = {
//	&tegra_rtc_device,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE) && !defined(CONFIG_USE_OF)
	&tegra12_se_device,
#endif
	&tegra_pcm_device,
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device0,
	&tegra_i2s_device1,
	&tegra_i2s_device2,
	&tegra_i2s_device3,
	&tegra_i2s_device4,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&tegra_offload_device,
	&tegra30_avp_audio_device,
	&flounder_vib_device,
};

#ifdef CONFIG_USE_OF
static struct of_dev_auxdata flounder_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("nvidia,tegra124-se", 0x70012000, "tegra12-se", NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-dtv", 0x7000c300, "dtv", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dtv", 0x7000c300, "dtv", NULL),
#if defined(CONFIG_ARM64)
	OF_DEV_AUXDATA("nvidia,tegra132-udc", TEGRA_USB_BASE, "tegra-udc.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-otg", TEGRA_USB_BASE, "tegra-otg",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-ehci", TEGRA_USB_BASE, "tegra-ehci.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-ehci", TEGRA_USB_BASE, "tegra-ehci.1",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-ehci", TEGRA_USB_BASE, "tegra-ehci.2",
		NULL),
#endif
	OF_DEV_AUXDATA("nvidia,tegra124-udc", TEGRA_USB_BASE, "tegra-udc.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-otg", TEGRA_USB_BASE, "tegra-otg",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-host1x", TEGRA_HOST1X_BASE, "host1x",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-gk20a", TEGRA_GK20A_BAR0_BASE,
		"gk20a.0", NULL),
#ifdef CONFIG_ARCH_TEGRA_VIC
	OF_DEV_AUXDATA("nvidia,tegra124-vic", TEGRA_VIC_BASE, "vic03.0", NULL),
#endif
	OF_DEV_AUXDATA("nvidia,tegra124-msenc", TEGRA_MSENC_BASE, "msenc",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-vi", TEGRA_VI_BASE, "vi.0", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-isp", TEGRA_ISP_BASE, "isp.0", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-isp", TEGRA_ISPB_BASE, "isp.1", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-tsec", TEGRA_TSEC_BASE, "tsec", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-xhci", 0x70090000, "tegra-xhci",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-xhci", 0x70090000, "tegra-xhci",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dc", TEGRA_DISPLAY_BASE, "tegradc.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dc", TEGRA_DISPLAY2_BASE, "tegradc.1",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-nvavp", 0x60001000, "nvavp",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dfll", 0x70110000, "tegra_cl_dvfs",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-dfll", 0x70040084, "tegra_cl_dvfs",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-efuse", TEGRA_FUSE_BASE, "tegra-fuse",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-camera", 0, "pcl-generic",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra-bluedroid_pm", 0, "bluedroid_pm",
		NULL),
#ifdef CONFIG_TEGRA_CEC_SUPPORT
	OF_DEV_AUXDATA("nvidia,tegra124-cec", 0x70015000, "tegra_cec", NULL),
#endif
	OF_DEV_AUXDATA("nvidia,ptm", 0x7081c000, "ptm", NULL),
	OF_DEV_AUXDATA("nvidia,tegra-audio-rt5677", 0x0, "tegra-snd-rt5677.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-sdhci", TEGRA_SDMMC4_BASE, "sdhci-tegra.3", 
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-sdhci", TEGRA_SDMMC3_BASE, "sdhci-tegra.2", 
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-sdhci", TEGRA_SDMMC1_BASE, "sdhci-tegra.0", 
		NULL),
	{}
};
#endif

static void __init tegra_flounder_early_init(void)
{
	tegra_clk_init_from_table(flounder_clk_init_table);
	tegra_clk_verify_parents();
	if (of_machine_is_compatible("nvidia,flounder"))
		tegra_soc_device_init("flounder");
	else if (of_machine_is_compatible("nvidia,tn8"))
		tegra_soc_device_init("tn8");
	else
		tegra_soc_device_init("flounder");
}

static struct tegra_io_dpd pexbias_io = {
	.name			= "PEX_BIAS",
	.io_dpd_reg_index	= 0,
	.io_dpd_bit		= 4,
};
static struct tegra_io_dpd pexclk1_io = {
	.name			= "PEX_CLK1",
	.io_dpd_reg_index	= 0,
	.io_dpd_bit		= 5,
};
static struct tegra_io_dpd pexclk2_io = {
	.name			= "PEX_CLK2",
	.io_dpd_reg_index	= 0,
	.io_dpd_bit		= 6,
};

static struct tegra_suspend_platform_data flounder_suspend_data = {	
	.cpu_timer      = 500,	
	.cpu_off_timer  = 300,	
	.cpu_suspend_freq = 408000,	
	.suspend_mode   = TEGRA_SUSPEND_LP0,	
	.core_timer     = 0x157e,	
	.core_off_timer = 2000,	
	.corereq_high   = true,	
	.sysclkreq_high = true,	
	.cpu_lp2_min_residency = 1000,	
	.min_residency_vmin_fmin = 1000,	
	.min_residency_ncpu_fast = 8000,	
	.min_residency_ncpu_slow = 5000,	
	.min_residency_mclk_stop = 5000,	
	.min_residency_crail = 20000,	
};

int __init flounder_suspend_init(void)
{
	tegra_init_suspend(&flounder_suspend_data);
	return 0;
}

static void __init tegra_flounder_late_init(void)
{
	platform_add_devices(flounder_devices, ARRAY_SIZE(flounder_devices));

	tegra_io_dpd_init();

	tegra12_emc_init();
	flounder_suspend_init();

	isomgr_init();

	/* put PEX pads into DPD mode to save additional power */
	tegra_io_dpd_enable(&pexbias_io);
	tegra_io_dpd_enable(&pexclk1_io);
	tegra_io_dpd_enable(&pexclk2_io);

//	flounder_sensors_init();

//	flounder_soctherm_init();

//	flounder_sysedp_dynamic_capping_init();

}

static void __init tegra_flounder_init_early(void)
{
	tegra12x_init_early();
}

static void __init tegra_flounder_dt_init(void)
{
	pr_info("regulator_has_full_constraints ");
	regulator_has_full_constraints();
	tegra_flounder_early_init();
#ifdef CONFIG_USE_OF
//	flounder_camera_auxdata(flounder_auxdata_lookup);
	of_platform_populate(NULL,
		of_default_bus_match_table, flounder_auxdata_lookup,
		&platform_bus);
#endif

	tegra_flounder_late_init();
        bt_export_bd_address();
}

static void __init tegra_flounder_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM) || \
		defined(CONFIG_TEGRA_NO_CARVEOUT)
	ulong carveout_size = 0;
#else
	ulong carveout_size = SZ_1G;
#endif
	ulong vpr_size = 186 * SZ_1M;

	tegra_reserve4(carveout_size, 0, 0, vpr_size);
}

static const char * const flounder_dt_board_compat[] = {
	"google,flounder",
	"google,flounder_lte",
	"google,flounder64",
	"google,flounder64_lte",
	NULL
};

static const char * const tn8_dt_board_compat[] = {
	"nvidia,tn8",
	NULL
};


DT_MACHINE_START(FLOUNDER, "flounder")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_flounder_reserve,
	.init_early	= tegra_flounder_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_flounder_dt_init,
	.dt_compat	= flounder_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END

DT_MACHINE_START(TN8, "tn8")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_flounder_reserve,
	.init_early	= tegra_flounder_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_flounder_dt_init,
	.dt_compat	= tn8_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END
