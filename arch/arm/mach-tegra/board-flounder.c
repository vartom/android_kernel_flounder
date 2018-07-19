/*
 * arch/arm/mach-tegra/board-flounder.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
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
#include <linux/spi/spi.h>
#include <linux/spi/rm31080a_ts.h>
#include <linux/maxim_sti.h>
#include <linux/memblock.h>
#include <linux/spi/spi-tegra.h>
#include <linux/rfkill-gpio.h>
#include <linux/nfc/bcm2079x.h>
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
#include <linux/regulator/consumer.h>
#include <linux/smb349-charger.h>
#include <linux/max17048_battery.h>
#include <linux/leds.h>
#include <linux/i2c/at24.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/i2c-tegra.h>
#include <linux/platform_data/serial-tegra.h>
#include <linux/edp.h>
#include <linux/usb/tegra_usb_phy.h>
#include <linux/mfd/palmas.h>
#include <linux/clk/tegra.h>
#include <media/tegra_dtv.h>
#include <linux/clocksource.h>
#include <linux/irqchip.h>
#include <linux/irqchip/tegra.h>
#include <linux/pci-tegra.h>
#include <linux/max1187x.h>
#include <linux/tegra-soc.h>

#include <mach/irqs.h>
#include <linux/tegra_fiq_debugger.h>

#include <mach/pinmux.h>
#include <mach/pinmux-t12.h>
#include <mach/io_dpd.h>
#include <mach/i2s.h>
#include <mach/isomgr.h>
#include <mach/tegra_asoc_pdata.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/gpio-tegra.h>
#include <mach/xusb.h>
#include <linux/platform_data/tegra_ahci.h>
#include <linux/irqchip/tegra.h>
#include <linux/htc_headset_mgr.h>
#include <linux/htc_headset_pmic.h>
#include <linux/htc_headset_one_wire.h>
#include <../../../drivers/staging/android/timed_gpio.h>

#include <mach/flounder-bdaddress.h>
#include "bcm_gps_hostwake.h"
#include "board.h"
#include "board-flounder.h"
#include "board-common.h"
#include "board-touch-raydium.h"
#include "board-touch-maxim_sti.h"
#include "clock.h"
#include "common.h"
#include "devices.h"
#include "gpio-names.h"
#include "iomap.h"
#include "pm.h"
#include "tegra-board-id.h"

static unsigned int flounder_hw_rev;
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
}

static struct resource flounder_bluedroid_pm_resources[] = {
	[0] = {
		.name   = "shutdown_gpio",
		.start  = TEGRA_GPIO_PR1,
		.end    = TEGRA_GPIO_PR1,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "host_wake",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
	[2] = {
		.name = "gpio_ext_wake",
		.start  = TEGRA_GPIO_PEE1,
		.end    = TEGRA_GPIO_PEE1,
		.flags  = IORESOURCE_IO,
	},
	[3] = {
		.name = "gpio_host_wake",
		.start  = TEGRA_GPIO_PU6,
		.end    = TEGRA_GPIO_PU6,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device flounder_bluedroid_pm_device = {
	.name = "bluedroid_pm",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(flounder_bluedroid_pm_resources),
	.resource       = flounder_bluedroid_pm_resources,
};

static noinline void __init flounder_setup_bluedroid_pm(void)
{
	flounder_bluedroid_pm_resources[1].start =
		flounder_bluedroid_pm_resources[1].end =
				gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&flounder_bluedroid_pm_device);
}

static struct bcm2079x_platform_data bcm2079x_pdata = {
	.irq_gpio = TEGRA_GPIO_PR7,
	.en_gpio = TEGRA_GPIO_PB1,
	.wake_gpio= TEGRA_GPIO_PS1,
};

static struct i2c_board_info __initdata bcm2079x_board_info = {
	I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
	.platform_data = &bcm2079x_pdata,
};

static __initdata struct tegra_clk_init_table flounder_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x", "pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	3187500,	false},
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

static void flounder_i2c_init(void)
{
	bcm2079x_board_info.irq = gpio_to_irq(TEGRA_GPIO_PR7),
	i2c_register_board_info(0, &bcm2079x_board_info, 1);
}

#ifndef CONFIG_USE_OF
static struct platform_device *flounder_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
	&tegra_uartd_device,
};

static struct tegra_serial_platform_data flounder_uarta_pdata = {
	.dma_req_selector = 8,
	.modem_interrupt = false,
};

static struct tegra_serial_platform_data flounder_uartb_pdata = {
	.dma_req_selector = 9,
	.modem_interrupt = false,
};

static struct tegra_serial_platform_data flounder_uartc_pdata = {
	.dma_req_selector = 10,
	.modem_interrupt = false,
};

static struct tegra_serial_platform_data flounder_uartd_pdata = {
	.dma_req_selsctor = 19,
	.modem_interrupt = false,
};
#endif
static struct tegra_serial_platform_data flounder_uarta_pdata = {
	.dma_req_selector = 8,
	.modem_interrupt = false,
};

static void __init flounder_uart_init(void)
{

#ifndef CONFIG_USE_OF
	tegra_uarta_device.dev.platform_data = &flounder_uarta_pdata;
	tegra_uartb_device.dev.platform_data = &flounder_uartb_pdata;
	tegra_uartc_device.dev.platform_data = &flounder_uartc_pdata;
	tegra_uartd_device.dev.platform_data = &flounder_uartd_pdata;
	platform_add_devices(flounder_uart_devices,
			ARRAY_SIZE(flounder_uart_devices));
#endif
	tegra_uarta_device.dev.platform_data = &flounder_uarta_pdata;
	if (!is_tegra_debug_uartport_hs()) {
		int debug_port_id = uart_console_debug_init(0);
		if (debug_port_id < 0)
			return;

#ifdef CONFIG_TEGRA_FIQ_DEBUGGER
#ifndef CONFIG_TRUSTY_FIQ
		tegra_serial_debug_init_irq_mode(TEGRA_UARTA_BASE, INT_UARTA, NULL, -1, -1);
#endif
#else
		platform_device_register(uart_console_debug_device);
#endif
	} else {
		tegra_uarta_device.dev.platform_data = &flounder_uarta_pdata;
		platform_device_register(&tegra_uarta_device);
	}
}

static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
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
	&tegra_pmu_device,
	&tegra_rtc_device,
	&tegra_udc_device,
#if defined(CONFIG_TEGRA_WATCHDOG)
#ifndef CONFIG_TRUSTY_FIQ
	&tegra_wdt0_device,
#endif
#endif
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
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
#if IS_ENABLED(CONFIG_SND_SOC_TEGRA_OFFLOAD)
	&tegra_offload_device,
#endif
#if IS_ENABLED(CONFIG_SND_SOC_TEGRA30_AVP)
	&tegra30_avp_audio_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif
	&flounder_vib_device,
};

static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev = {
		.vbus_pmu_irq = 0,
		.vbus_gpio = -1,
		.charging_supported = true,
		.remote_wakeup_supported = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_hsslew_lsb = 3,
		.xcvr_hsslew_msb = 3,
		.xcvr_setup_offset = 3,
		.xcvr_use_fuses = 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 15,
		.xcvr_lsfslew = 0,
		.xcvr_lsrslew = 3,
		.xcvr_hsslew_lsb = 3,
		.xcvr_hsslew_msb = 3,
		.xcvr_setup_offset = 3,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x4,
		.xcvr_hsslew_lsb = 2,
	},
};

static struct tegra_usb_platform_data tegra_ehci2_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x5,
	},
};

static struct tegra_usb_platform_data tegra_ehci3_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
	.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x5,
	},
};

static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci1_utmi_pdata,
	.id_det_gpio = TEGRA_GPIO_PW2,
};

static void flounder_usb_init(void)
{
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	/* Device cable is detected through PMU Interrupt */
	tegra_udc_pdata.support_pmu_vbus = true;
	tegra_ehci1_utmi_pdata.support_pmu_vbus = true;
	tegra_ehci1_utmi_pdata.vbus_extcon_dev_name = "palmas-extcon";
	/* Host cable is detected through GPIO Interrupt */
	tegra_udc_pdata.id_det_type = TEGRA_USB_GPIO_ID;
	tegra_udc_pdata.vbus_extcon_dev_name = "palmas-extcon";
	tegra_ehci1_utmi_pdata.id_det_type = TEGRA_USB_GPIO_ID;

	tegra_ehci1_utmi_pdata.u_data.host.turn_off_vbus_on_lp0 = true;

	/* Enable Y-Cable support */
	tegra_ehci1_utmi_pdata.u_data.host.support_y_cable = true;

	/* charger needs to be set to 2A - h/w will do 1.8A */
	tegra_udc_pdata.u_data.dev.dcp_current_limit_ma = 2000;

	if (!is_mdm_modem())
		tegra_ehci1_utmi_pdata.u_cfg.utmi.xcvr_setup_offset = -3;

	if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB)) {
		tegra_otg_pdata.is_xhci = false;
		tegra_udc_pdata.u_data.dev.is_xhci = false;
	} else {
		tegra_otg_pdata.is_xhci = true;
		tegra_udc_pdata.u_data.dev.is_xhci = true;
	}
	if (!is_mdm_modem())
		tegra_udc_pdata.u_cfg.utmi.xcvr_setup_offset = -3;
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);
	/* Setup the udc platform data */
	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;

	if ((!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB))
		/* tegra_ehci2_device will reserve for mdm9x25 modem */
		&& (!is_mdm_modem()))
	{
		tegra_ehci2_device.dev.platform_data =
			&tegra_ehci2_utmi_pdata;
		platform_device_register(&tegra_ehci2_device);
	}
	if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB)) {
		tegra_ehci3_device.dev.platform_data = &tegra_ehci3_utmi_pdata;
		platform_device_register(&tegra_ehci3_device);
	}
}

static struct tegra_xusb_platform_data xusb_pdata = {
	.portmap = TEGRA_XUSB_SS_P0 | TEGRA_XUSB_USB2_P0 | TEGRA_XUSB_SS_P1 |
			TEGRA_XUSB_USB2_P1 | TEGRA_XUSB_USB2_P2,
};

static void flounder_xusb_init(void)
{
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	xusb_pdata.lane_owner = (u8) tegra_get_lane_owner_info();

	if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB))
		xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P0 |
			TEGRA_XUSB_SS_P0);

	if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB))
		xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P1 |
			TEGRA_XUSB_USB2_P2 | TEGRA_XUSB_SS_P1);

	if (usb_port_owner_info & HSIC1_PORT_OWNER_XUSB)
		xusb_pdata.portmap |= TEGRA_XUSB_HSIC_P0;

	if (usb_port_owner_info & HSIC2_PORT_OWNER_XUSB)
		xusb_pdata.portmap |= TEGRA_XUSB_HSIC_P1;
}

#ifndef CONFIG_USE_OF
static struct platform_device *flounder_spi_devices[] __initdata = {
	&tegra11_spi_device1,
	&tegra11_spi_device4,
};

static struct tegra_spi_platform_data flounder_spi1_pdata = {
	.dma_req_sel		= 15,
	.spi_max_frequency	= 25000000,
	.clock_always_on	= false,
};

static struct tegra_spi_platform_data flounder_spi4_pdata = {
	.dma_req_sel		= 18,
	.spi_max_frequency	= 25000000,
	.clock_always_on	= false,
};
static void __init flounder_spi_init(void)
{
	tegra11_spi_device1.dev.platform_data = &flounder_spi1_pdata;
	tegra11_spi_device4.dev.platform_data = &flounder_spi4_pdata;
	platform_add_devices(flounder_spi_devices,
			ARRAY_SIZE(flounder_spi_devices));
}
#else
static void __init flounder_spi_init(void)
{
}
#endif

#ifdef CONFIG_USE_OF
static struct of_dev_auxdata flounder_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("nvidia,tegra114-spi", 0x7000d400, "spi-tegra114.0",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-spi", 0x7000d600, "spi-tegra114.1",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-spi", 0x7000d800, "spi-tegra114.2",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-spi", 0x7000da00, "spi-tegra114.3",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-spi", 0x7000dc00, "spi-tegra114.4",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-spi", 0x7000de00, "spi-tegra114.5",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-apbdma", 0x60020000, "tegra-apbdma",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-se", 0x70012000, "tegra12-se", NULL),
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
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006000, "serial-tegra.0",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006040, "serial-tegra.1",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006200, "serial-tegra.2",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006300, "serial-tegra.3",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-i2c", 0x7000c000, "tegra12-i2c.0",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-i2c", 0x7000c400, "tegra12-i2c.1",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-i2c", 0x7000c500, "tegra12-i2c.2",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-i2c", 0x7000c700, "tegra12-i2c.3",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-i2c", 0x7000d000, "tegra12-i2c.4",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-i2c", 0x7000d100, "tegra12-i2c.5",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-xhci", 0x70090000, "tegra-xhci",
				&xusb_pdata),
	OF_DEV_AUXDATA("nvidia,tegra124-nvavp", 0x60001000, "nvavp",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-camera", 0, "pcl-generic",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dfll", 0x70110000, "tegra_cl_dvfs",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-dfll", 0x70040084, "tegra_cl_dvfs",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-efuse", TEGRA_FUSE_BASE, "tegra-fuse",
			NULL),
	OF_DEV_AUXDATA("nvidia,tegra-audio-rt5677", 0x0, "tegra-snd-rt5677.0",
		NULL),
	{}
};
#endif

static struct device *gps_dev;
static struct class *gps_class;

extern int tegra_get_hw_rev(void);

#define GPS_HOSTWAKE_GPIO 69
static struct bcm_gps_hostwake_platform_data gps_hostwake_data = {
	.gpio_hostwake = GPS_HOSTWAKE_GPIO,
};

static struct platform_device bcm_gps_hostwake = {
	.name   = "bcm-gps-hostwake",
	.id     = -1,
	.dev    = {
		.platform_data  = &gps_hostwake_data,
	},
};

#define PRJ_F	302
static int __init flounder_gps_init(void)
{

	int ret;
	int gps_onoff;
	int product_id;

	pr_info("[GPS]%s init gps onoff\n", __func__);
	of_property_read_u32(
		of_find_node_by_path("/chosen/board_info"),
		"pid",
		&product_id);

	if (product_id == PRJ_F && flounder_get_hw_revision() <= FLOUNDER_REV_EVT1_1  ){
		gps_onoff = TEGRA_GPIO_PH5; // XB
	} else {
		gps_onoff = TEGRA_GPIO_PB4; // XC
	}

	gps_class = class_create(THIS_MODULE, "gps");
	if (IS_ERR(gps_class)){
		pr_err("[GPS] %s: gps class create fail \n", __func__);
		return PTR_ERR(gps_class);
	}

	gps_dev = device_create(gps_class, NULL, 0, NULL, "bcm47521");
	if (IS_ERR(gps_dev)){
		pr_err("[GPS] %s: gps device create fail \n", __func__);
		return PTR_ERR(gps_dev);
	}

	ret = gpio_request(gps_onoff, "gps_onoff");
	if (ret < 0){
		pr_err("[GPS] %s: gpio_request failed for gpio %s\n",
			__func__, "gps_onoff");
	}

	gpio_direction_output(gps_onoff, 0);
	gpio_export (gps_onoff, 1);
	gpio_export_link(gps_dev,"gps_onoff", gps_onoff);

	if (product_id == PRJ_F) {
		pr_info("GPS: init gps hostwake\n");
		platform_device_register(&bcm_gps_hostwake);
	}

	return 0;
}
#undef PRJ_F

static void __init flounder_force_recovery_gpio(void)
{
	int ret;

	if(flounder_get_hw_revision() != FLOUNDER_REV_PVT)
		return ;
	ret = gpio_request(TEGRA_GPIO_PI1, "force_recovery");
	if (ret < 0){
		pr_err("force_recovery: gpio_request failed for force_recovery %s\n", "force_recovery");
	}
	gpio_direction_input(TEGRA_GPIO_PI1);
	tegra_pinctrl_pg_set_pullupdown(TEGRA_PINGROUP_GPIO_PI1, TEGRA_PUPD_PULL_UP);
}

static void __init sysedp_init(void)
{
	flounder_new_sysedp_init();
}

static void __init edp_init(void)
{
	flounder_edp_init();
}

static void __init sysedp_dynamic_capping_init(void)
{
	flounder_sysedp_dynamic_capping_init();
}

static void __init tegra_flounder_early_init(void)
{
	sysedp_init();
	tegra_clk_init_from_table(flounder_clk_init_table);
	tegra_clk_verify_parents();
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

static void __init tegra_flounder_late_init(void)
{

	flounder_display_init();
	flounder_uart_init();
	flounder_usb_init();
#ifdef CONFIG_QCT_9K_MODEM
	if(is_mdm_modem())
		flounder_mdm_9k_init();
#endif
	flounder_xusb_init();
	flounder_i2c_init();
	flounder_spi_init();

	platform_add_devices(flounder_devices, ARRAY_SIZE(flounder_devices));
	tegra_io_dpd_init();
	flounder_sdhci_init();
	flounder_regulator_init();

	flounder_suspend_init();

	flounder_emc_init();
	edp_init();
	isomgr_init();

	flounder_panel_init();
	flounder_kbc_init();
	flounder_gps_init();

	/* put PEX pads into DPD mode to save additional power */
	tegra_io_dpd_enable(&pexbias_io);
	tegra_io_dpd_enable(&pexclk1_io);
	tegra_io_dpd_enable(&pexclk2_io);

#ifdef CONFIG_TEGRA_WDT_RECOVERY
	tegra_wdt_recovery_init();
#endif

	flounder_sensors_init();

	flounder_soctherm_init();

	flounder_setup_bluedroid_pm();
	sysedp_dynamic_capping_init();
	flounder_force_recovery_gpio();
}

static void __init tegra_flounder_init_early(void)
{
	flounder_rail_alignment_init();
	tegra12x_init_early();
}

static void __init tegra_flounder_dt_init(void)
{
	tegra_flounder_early_init();
#ifdef CONFIG_NVMAP_USE_CMA_FOR_CARVEOUT
	carveout_linear_set(&tegra_generic_cma_dev);
	carveout_linear_set(&tegra_vpr_cma_dev);
#endif
#ifdef CONFIG_USE_OF
	flounder_camera_auxdata(flounder_auxdata_lookup);
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
	/* 1536*2048*4*2 = 25165824 bytes */
	ulong carveout_size = 0;
	ulong fb2_size = 0;
#else
	ulong carveout_size = SZ_1G;
	ulong fb2_size = 0;
#endif
	ulong fb1_size = SZ_16M + SZ_8M;
	ulong vpr_size = 186 * SZ_1M;

	tegra_reserve4(carveout_size, fb1_size, fb2_size, vpr_size);
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
	.restart	= tegra_assert_system_reset,
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
	.restart	= tegra_assert_system_reset,
	.dt_compat	= tn8_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END
