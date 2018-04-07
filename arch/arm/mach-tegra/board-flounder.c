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
#include <linux/htc_headset_mgr.h>
#include <linux/htc_headset_pmic.h>
#include <linux/htc_headset_one_wire.h>
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
/*
#include "../../../sound/soc/codecs/rt5506.h"
#include "../../../sound/soc/codecs/rt5677.h"
#include "../../../sound/soc/codecs/tfa9895.h"*/

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

struct aud_sfio_data {
	const char *name;
	int id;
};
static struct aud_sfio_data audio_sfio_pins[] = {
	[0] = {
		.name = "I2S1_LRCK",
		.id   = TEGRA_GPIO_PA2,
	},
	[1] = {
		.name = "I2S1_SCLK",
		.id   = TEGRA_GPIO_PA3,
	},
	[2] = {
		.name = "I2S1_SDATA_IN",
		.id   = TEGRA_GPIO_PA4,
	},
	[3] = {
		.name = "I2S1_SDATA_OUT",
		.id   = TEGRA_GPIO_PA5,
	},
	[4] = {
		.name = "I2S2_LRCK",
		.id   = TEGRA_GPIO_PP0,
	},
	[5] = {
		.name = "I2S2_SDATA_IN",
		.id   = TEGRA_GPIO_PP1,
	},
	[6] = {
		.name = "I2S2_SDATA_OUT",
		.id   = TEGRA_GPIO_PP2,
	},
	[7] = {
		.name = "I2S2_SCLK",
		.id   = TEGRA_GPIO_PP3,
	},
	[8] = {
		.name = "extperiph1_clk",
		.id   = TEGRA_GPIO_PW4,
	},
	[9] = {
		.name = "SPI_MOSI",
		.id   = TEGRA_GPIO_PY0,
	},
	[10] = {
		.name = "SPI_MISO",
		.id   = TEGRA_GPIO_PY1,
	},
	[11] = {
		.name = "SPI_SCLK",
		.id   = TEGRA_GPIO_PY2,
	},
	[12] = {
		.name = "SPI_CS",
		.id   = TEGRA_GPIO_PY3,
	},
};
/*
static struct tfa9895_platform_data tfa9895_data = {
	.tfa9895_power_enable = TEGRA_GPIO_PX5,
};
struct rt5677_priv rt5677_data = {
	.vad_clock_en = TEGRA_GPIO_PX3,
};

static struct i2c_board_info __initdata rt5677_board_info = {
	I2C_BOARD_INFO("rt5677", 0x2d),
	.platform_data = &rt5677_data,
};
static struct i2c_board_info __initdata tfa9895_board_info = {
	I2C_BOARD_INFO("tfa9895", 0x34),
	.platform_data = &tfa9895_data,
};
static struct i2c_board_info __initdata tfa9895l_board_info = {
	I2C_BOARD_INFO("tfa9895l", 0x35),
};*/

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

static void flounder_i2c_init(void)
{
/*	i2c_register_board_info(1, &rt5677_board_info, 1);
	i2c_register_board_info(1, &tfa9895_board_info, 1);
	i2c_register_board_info(1, &tfa9895l_board_info, 1);*/

	bcm2079x_board_info.irq = gpio_to_irq(TEGRA_GPIO_PR7),
	i2c_register_board_info(0, &bcm2079x_board_info, 1);
}

/*static struct tegra_asoc_platform_data flounder_audio_pdata_rt5677 = {
	.gpio_hp_det = -1,
	.gpio_ldo1_en = TEGRA_GPIO_PK0,
	.gpio_ldo2_en = TEGRA_GPIO_PQ3,
	.gpio_reset = TEGRA_GPIO_PX4,
	.gpio_irq1 = TEGRA_GPIO_PS4,
	.gpio_wakeup = TEGRA_GPIO_PO0,
	.gpio_spkr_en = -1,
	.gpio_spkr_ldo_en = TEGRA_GPIO_PX5,
	.gpio_int_mic_en = TEGRA_GPIO_PV3,
	.gpio_ext_mic_en = TEGRA_GPIO_PS3,
	.gpio_hp_mute = -1,
	.gpio_hp_en = TEGRA_GPIO_PX1,
	.gpio_hp_ldo_en = -1,
	.gpio_codec1 = -1,
	.gpio_codec2 = -1,
	.gpio_codec3 = -1,
	.i2s_param[HIFI_CODEC]       = {
		.audio_port_id = 1,
		.is_i2s_master = 1,
		.i2s_mode = TEGRA_DAIFMT_I2S,
	},
	.i2s_param[SPEAKER]       = {
		.audio_port_id = 2,
		.is_i2s_master = 1,
		.i2s_mode = TEGRA_DAIFMT_I2S,
	},
	.i2s_param[BT_SCO] = {
		.audio_port_id = 3,
		.is_i2s_master = 1,
		.i2s_mode = TEGRA_DAIFMT_DSP_A,
	}
};*/

/*static struct tegra_spi_device_controller_data dev_cdata_rt5677 = {
	.rx_clk_tap_delay = 0,
	.tx_clk_tap_delay = 16,
};
struct spi_board_info rt5677_flounder_spi_board[1] = {
	{
	 .modalias = "rt5677_spidev",
	 .bus_num = 4,
	 .chip_select = 0,
	 .max_speed_hz = 12 * 1000 * 1000,
	 .mode = SPI_MODE_0,
	 .controller_data = &dev_cdata_rt5677,
	 },
};*/

static void flounder_audio_init(void)
{
	int i;

//	flounder_audio_pdata_rt5677.codec_name = "rt5677.1-002d";
//	flounder_audio_pdata_rt5677.codec_dai_name = "rt5677-aif1";
//	spi_register_board_info(&rt5677_flounder_spi_board[0],
//	ARRAY_SIZE(rt5677_flounder_spi_board));
	for (i = 0; i < ARRAY_SIZE(audio_sfio_pins); i++)
		if (tegra_is_gpio(audio_sfio_pins[i].id)) {
			gpio_request(audio_sfio_pins[i].id, audio_sfio_pins[i].name);
			gpio_free(audio_sfio_pins[i].id);
			pr_info("%s: gpio_free for gpio[%d] %s\n",
				__func__, audio_sfio_pins[i].id, audio_sfio_pins[i].name);
		}
};

/*static struct platform_device flounder_audio_device_rt5677 = {
	.name = "tegra-snd-rt5677",
	.id = 0,
	.dev = {
		.platform_data = &flounder_audio_pdata_rt5677,
	},
};*/

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

static void flounder_panel(void)
{
	struct tegra_panel *panel;

	panel = &dsi_j_qxga_8_9;

	if (panel) {
		if (panel->register_bl_dev)
			panel->register_bl_dev();
	}
};

static struct platform_device *flounder_devices[] __initdata = {
	&tegra_rtc_device,
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
//	&flounder_audio_device_rt5677,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&tegra_offload_device,
	&tegra30_avp_audio_device,
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

#if !defined(CONFIG_ARM64)
static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
		.support_y_cable = true,
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
};
#else
static struct tegra_usb_otg_data tegra_otg_pdata;
#endif

static void flounder_usb_init(void)
{
#if !defined(CONFIG_ARM64)
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	tegra_ehci1_utmi_pdata.u_data.host.turn_off_vbus_on_lp0 = true;
	/* Device cable is detected through PMU Interrupt */
	tegra_udc_pdata.support_pmu_vbus = true;
	tegra_udc_pdata.vbus_extcon_dev_name = "palmas-extcon";
	tegra_ehci1_utmi_pdata.support_pmu_vbus = true;
	tegra_ehci1_utmi_pdata.vbus_extcon_dev_name = "palmas-extcon";
	/* Host cable is detected through PMU Interrupt */
	tegra_udc_pdata.id_det_type = TEGRA_USB_VIRTUAL_ID;
	tegra_udc_pdata.vbus_extcon_dev_name = "palmas-extcon";
	tegra_ehci1_utmi_pdata.id_det_type = TEGRA_USB_VIRTUAL_ID;
	tegra_ehci1_utmi_pdata.id_extcon_dev_name = "palmas-extcon";

	/*
	 * Set the maximum voltage that can be supplied
	 * over USB vbus that the board supports if we use
	 * a quick charge 2 wall charger.
	 *
	tegra_udc_pdata.qc2_voltage = TEGRA_USB_QC2_9V;
	tegra_udc_pdata.u_data.dev.qc2_current_limit_ma = 1200;*/

	/* charger needs to be set to 3A - h/w will do 2A */
	tegra_udc_pdata.u_data.dev.dcp_current_limit_ma = 3000;

	if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB)) {
		tegra_otg_pdata.is_xhci = false;
		tegra_udc_pdata.u_data.dev.is_xhci = false;
	} else {
		tegra_otg_pdata.is_xhci = true;
		tegra_udc_pdata.u_data.dev.is_xhci = true;
	}

	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	/* Setup the udc platform data */
	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;
#endif
#if !defined(CONFIG_ARM64)
	platform_device_register(&tegra_udc_device);

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
#endif
}

static struct tegra_xusb_platform_data xusb_pdata = {
	.portmap = TEGRA_XUSB_SS_P0 | TEGRA_XUSB_USB2_P0 | TEGRA_XUSB_SS_P1 |
			TEGRA_XUSB_USB2_P1 | TEGRA_XUSB_USB2_P2,
};

#ifdef CONFIG_TEGRA_XUSB_PLATFORM
static void flounder_xusb_init(void)
{
#if !defined(CONFIG_ARM64)
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
#endif
}
#endif

#ifdef CONFIG_USE_OF
static struct of_dev_auxdata flounder_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("nvidia,tegra124-se", 0x70012000, "tegra12-se", NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-dtv", 0x7000c300, "dtv", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dtv", 0x7000c300, "dtv", NULL),
#if defined(CONFIG_ARM64)
	OF_DEV_AUXDATA("nvidia,tegra132-udc", 0x7d000000, "tegra-udc.0",
			&tegra_udc_pdata.u_data.dev),
	OF_DEV_AUXDATA("nvidia,tegra132-otg", 0x7d000000, "tegra-otg",
			&tegra_otg_pdata),
	OF_DEV_AUXDATA("nvidia,tegra132-ehci", 0x7d004000, "tegra-ehci.1",
			NULL),
	OF_DEV_AUXDATA("nvidia,tegra132-ehci", 0x7d008000, "tegra-ehci.2",
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
				&xusb_pdata),
	OF_DEV_AUXDATA("nvidia,tegra132-xhci", 0x70090000, "tegra-xhci",
				&xusb_pdata),
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
	OF_DEV_AUXDATA("nvidia,tegra-audio-rt5677", 0x0, "tegra-snd-rt5677.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-sdhci", 0x700b0600, "sdhci-tegra.3", 
			NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-sdhci", 0x700b0400, "sdhci-tegra.2", 
			NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-sdhci", 0x700b0000, "sdhci-tegra.0", 
			NULL),
	{}
};
#endif


#define	EARPHONE_DET TEGRA_GPIO_PW3
#define	HSMIC_2V85_EN TEGRA_GPIO_PS3
#define AUD_REMO_PRES TEGRA_GPIO_PS2
#define	AUD_REMO_TX_OE TEGRA_GPIO_PQ4
#define	AUD_REMO_TX TEGRA_GPIO_PJ7
#define	AUD_REMO_RX TEGRA_GPIO_PB0

static void headset_init(void)
{
	int ret ;

	ret = gpio_request(HSMIC_2V85_EN, "HSMIC_2V85_EN");
	if (ret < 0){
		pr_err("[HS] %s: gpio_request failed for gpio %s\n",
                        __func__, "HSMIC_2V85_EN");
	}

	ret = gpio_request(AUD_REMO_TX_OE, "AUD_REMO_TX_OE");
	if (ret < 0){
		pr_err("[HS] %s: gpio_request failed for gpio %s\n",
                        __func__, "AUD_REMO_TX_OE");
	}

	ret = gpio_request(AUD_REMO_TX, "AUD_REMO_TX");
	if (ret < 0){
		pr_err("[HS] %s: gpio_request failed for gpio %s\n",
                        __func__, "AUD_REMO_TX");
	}

	ret = gpio_request(AUD_REMO_RX, "AUD_REMO_RX");
	if (ret < 0){
		pr_err("[HS] %s: gpio_request failed for gpio %s\n",
                        __func__, "AUD_REMO_RX");
	}

	gpio_direction_output(HSMIC_2V85_EN, 0);
	gpio_direction_output(AUD_REMO_TX_OE, 1);

}

static void headset_power(int enable)
{
	pr_info("[HS_BOARD] (%s) Set MIC bias %d\n", __func__, enable);

	if (enable)
		gpio_set_value(HSMIC_2V85_EN, 1);
	else {
		gpio_set_value(HSMIC_2V85_EN, 0);
	}
}

#ifdef CONFIG_HEADSET_DEBUG_UART
#define	AUD_DEBUG_EN TEGRA_GPIO_PK5
static int headset_get_debug(void)
{
	int ret = 0;
	ret = gpio_get_value(AUD_DEBUG_EN);
	pr_info("[HS_BOARD] (%s) AUD_DEBUG_EN=%d\n", __func__, ret);

	return ret;
}
#endif

/* HTC_HEADSET_PMIC Driver */ 
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_ADC,
	.hpin_gpio		= EARPHONE_DET,
	.hpin_irq		= 0,
	.key_gpio		= AUD_REMO_PRES,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote 	= {0, 117, 118, 230, 231, 414, 415, 829},
	.hs_controller		= 0,
	.hs_switch		= 0,
	.iio_channel_name = "hs_channel",
#ifdef CONFIG_HEADSET_DEBUG_UART
	.debug_gpio		= AUD_DEBUG_EN,
	.debug_irq		= 0,
	.headset_get_debug	= headset_get_debug,
#endif
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_pmic_data,
	},
};

static struct htc_headset_1wire_platform_data htc_headset_1wire_data = {
	.tx_level_shift_en	= AUD_REMO_TX_OE,
	.uart_sw		= 0,
	.one_wire_remote	={0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.remote_press		= 0,
	.onewire_tty_dev	= "/dev/ttyTHS3",
};

static struct platform_device htc_headset_one_wire = {
	.name	= "HTC_HEADSET_1WIRE",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_1wire_data,
	},
};

#ifdef CONFIG_HEADSET_DEBUG_UART
static void uart_tx_gpo(int mode)
{
	pr_info("[HS_BOARD] (%s) Set uart_tx_gpo mode = %d\n", __func__, mode);
	switch (mode) {
		case 0:
			gpio_direction_output(AUD_REMO_TX, 0);
			break;
		case 1:
			gpio_direction_output(AUD_REMO_TX, 1);
			break;
		case 2:
			tegra_gpio_disable(AUD_REMO_TX);
			break;
	}
}

static void uart_lv_shift_en(int enable)
{
	pr_info("[HS_BOARD] (%s) Set uart_lv_shift_en %d\n", __func__, enable);
	gpio_direction_output(AUD_REMO_TX_OE, enable);
}
#endif

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	&htc_headset_one_wire,
	/* Please put the headset detection driver on the last */
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 3680,
		.adc_min = 621,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 620,
		.adc_min = 0,
	},
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_FLOAT_DET,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config		= htc_headset_mgr_config,
	.headset_init		= headset_init,
	.headset_power		= headset_power,
#ifdef CONFIG_HEADSET_DEBUG_UART
	.uart_tx_gpo		= uart_tx_gpo,
	.uart_lv_shift_en	= uart_lv_shift_en,
#endif

};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_mgr_data,
	},
};

static int __init flounder_headset_init(void)
{
	pr_info("[HS]%s Headset device register enter\n", __func__);
	platform_device_register(&htc_headset_mgr);
	return 0;
}

static void __init tegra_flounder_early_init(void)
{
	flounder_new_sysedp_init();
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

static void __init tegra_flounder_late_init(void)
{
	flounder_usb_init();
//	if(is_mdm_modem())
//		flounder_mdm_9k_init();
#ifdef CONFIG_TEGRA_XUSB_PLATFORM
	flounder_xusb_init();
#endif
	flounder_i2c_init();
//	flounder_spi_init(); in DT
	flounder_audio_init();
	platform_add_devices(flounder_devices, ARRAY_SIZE(flounder_devices));

	tegra_io_dpd_init();
	flounder_sdhci_init(); // in DT
	flounder_regulator_init();
	flounder_suspend_init();
	tegra12_emc_init();

	isomgr_init();
	flounder_headset_init();
	flounder_panel();
//	flounder_kbc_init(); in DT

	/* put PEX pads into DPD mode to save additional power */
	tegra_io_dpd_enable(&pexbias_io);
	tegra_io_dpd_enable(&pexclk1_io);
	tegra_io_dpd_enable(&pexclk2_io);

	flounder_sensors_init();

	flounder_soctherm_init();

//	flounder_setup_bluedroid_pm(); in DT

	flounder_sysedp_dynamic_capping_init();


}

static void __init tegra_flounder_init_early(void)
{
	flounder_rail_alignment_init();/* Old*/
	tegra12x_init_early();
}

static void __init tegra_flounder_dt_init(void)
{
	tegra_flounder_early_init();
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
	ulong carveout_size = 0;
#else
	ulong carveout_size = SZ_1G;
#endif
/*	ulong fb1_size = SZ_16M + SZ_8M;
	ulong fb2_size = SZ_16M + SZ_8M;*/
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
