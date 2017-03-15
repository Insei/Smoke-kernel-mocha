/*
 * arch/arm/mach-tegra/board-ardbeg.c
 *
 * Copyright (c) 2013-2014, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (C) 2016 XiaoMi, Inc.
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
#include <linux/nfc/pn544.h>
#include <linux/rfkill-gpio.h>
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
#include <linux/tegra-soc.h>
#include <linux/tegra_fiq_debugger.h>
#include <linux/platform_data/tegra_usb_modem_power.h>
#include <linux/platform_data/tegra_ahci.h>
#include <linux/irqchip/tegra.h>
#include <sound/rt5670.h>

#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/pinmux-t12.h>
#include <mach/io_dpd.h>
#include <mach/i2s.h>
#include <mach/isomgr.h>
#include <mach/tegra_asoc_pdata.h>
#include <mach/dc.h>
#include <mach/tegra_usb_pad_ctrl.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/gpio-tegra.h>
#include <mach/xusb.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/input/synaptics_dsx.h>
#include <linux/firmware.h>

#include <linux/power/bq27x00_battery.h>
#include <linux/platform_data/leds-lp55xx.h>

#include "board.h"
#include "board-ardbeg.h"
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
#include "tegra-of-dev-auxdata.h"

#define BQ27520_INT_SUPPORT

static struct board_info board_info, display_board_info;

static struct rt5670_platform_data rt5671_pdata = {
	.jd_mode = 2,
	.codec_gpio = TEGRA_GPIO_CDC_IRQ,
	.in2_diff = true,
	.in3_diff = false,
	.in4_diff = true,
	.bclk_32fs = {false, true, true, true},
};

static struct i2c_board_info __initdata audio_board_info[] = {
	{
		I2C_BOARD_INFO("rt5671", 0x1c),
		.platform_data = &rt5671_pdata,
	},
	{
		I2C_BOARD_INFO("tfa98xx", 0x34),
	},
	{
		I2C_BOARD_INFO("tfa98xx", 0x37),
	},
};

#ifdef CONFIG_LEDS_LP5521
#define LP5521_CHIP_EN_GPIO        TEGRA_GPIO_PG7
static struct lp55xx_led_config lp5521_led_config[] = {
	{
		.name           = "red",
		.chan_nr        = 0,
		.led_current    = 20,
		.max_current	= 255,
	},
	{
		.name           = "green",
		.chan_nr        = 1,
		.led_current    = 20,
		.max_current	= 255,
	},
	{
		.name           = "blue",
		.chan_nr        = 2,
		.led_current    = 20,
		.max_current	= 255,
	}
};

static int lp5521_setup(void)
{
	return gpio_request_one(LP5521_CHIP_EN_GPIO, GPIOF_DIR_OUT,
			"lp5521_enable");
}

static void lp5521_release(void)
{
	gpio_free(LP5521_CHIP_EN_GPIO);
}

static void lp5521_enable(bool state)
{
	gpio_set_value(LP5521_CHIP_EN_GPIO, !!state);
}

static struct lp55xx_platform_data lp5521_platform_data = {
	.led_config             = lp5521_led_config,
	.num_channels           = ARRAY_SIZE(lp5521_led_config),
	.clock_mode             = LP55XX_CLOCK_AUTO,
	.setup_resources        = lp5521_setup,
	.release_resources      = lp5521_release,
	.enable                 = lp5521_enable,
};
#endif

static struct i2c_board_info __initdata i2c_led_board_info[] = {
#ifdef CONFIG_LEDS_LP5521
	{
		I2C_BOARD_INFO("lp5521", 0x32),
		.platform_data = &lp5521_platform_data,
	},
#endif
};

static __initdata struct tegra_clk_init_table ardbeg_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x", "pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	48000000,	false},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s3",	"pll_a_out0",	0,		false},
	{ "i2s4",	"pll_a_out0",	0,		false},
	{ "i2s0",	"pll_a_out0",	0,		false},
	{ "i2s2",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ "d_audio",	"clk_m",	12000000,	false},
	{ "dam0",	"clk_m",	12000000,	false},
	{ "dam1",	"clk_m",	12000000,	false},
	{ "dam2",	"clk_m",	12000000,	false},
	{ "audio0",	"i2s0_sync",	0,		false},
	{ "audio1",	"i2s1_sync",	0,		false},
	{ "audio2",	"i2s2_sync",	0,		false},
	{ "audio3",	"i2s3_sync",	0,		false},
	{ "audio4",	"i2s4_sync",	0,		false},
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
	{ "audio.emc",	"emc",		50000000,	false},
	{ NULL,		NULL,		0,		0},
};

static struct i2c_hid_platform_data i2c_keyboard_pdata = {
	.hid_descriptor_address = 0x0,
};

static struct i2c_board_info __initdata i2c_keyboard_board_info = {
	I2C_BOARD_INFO("hid", 0x3B),
	.platform_data  = &i2c_keyboard_pdata,
};

static struct i2c_hid_platform_data i2c_touchpad_pdata = {
	.hid_descriptor_address = 0x20,
};

static struct i2c_board_info __initdata i2c_touchpad_board_info = {
	I2C_BOARD_INFO("hid", 0x2C),
	.platform_data  = &i2c_touchpad_pdata,
};

#ifdef CONFIG_BATTERY_BQ27x00
#define BQ27X00_LGC_FIRMWARE	"0105_lgc.dffs"
#define BQ27X00_ATL_FIRMWARE	"0205_atl.dffs"

static unsigned char bq27x00_lgc_config[] = {
	#include "bq27520g4_lgc_0003.h"
};

static unsigned char bq27x00_atl_config[] = {
	#include "bq27520g4_atl_0003.h"
};

DECLARE_BUILTIN_FIRMWARE_SIZE(BQ27X00_LGC_FIRMWARE,
		bq27x00_lgc_config, sizeof(bq27x00_lgc_config) - 1);
DECLARE_BUILTIN_FIRMWARE_SIZE(BQ27X00_ATL_FIRMWARE,
		bq27x00_atl_config, sizeof(bq27x00_atl_config) - 1);

/* Gas gauge board specific configuration filled in at board init */
static struct bq27x00_platform_data bq27520_platform_data = {
	.soc_int_irq = -1,
	.bat_low_irq = -1,
	.fw_name = {BQ27X00_LGC_FIRMWARE, BQ27X00_ATL_FIRMWARE},
};

static struct i2c_board_info __initdata bq27520_boardinfo[] = {
	{
		I2C_BOARD_INFO("bq27520", 0x55),
		.platform_data = &bq27520_platform_data,
	},
};

#ifdef BQ27520_INT_SUPPORT
#define BQ27520_BATTERY_INT	TEGRA_GPIO_PQ5
/* BQ27520 is on I2C1 with the PMIC; this init needs to happen before that bus is initialized. */
static int __init ardbeg_bq27520_init(void)
{
	int soc_int_gpio, soc_int_irq;
	int res;

	soc_int_gpio = BQ27520_BATTERY_INT;

	res = gpio_request(soc_int_gpio, "battery_int_n");
	if (res) {
		pr_err("%s: Failed to get soc_int gpio: %d\n", __func__, res);
		goto error;
	}

	soc_int_irq = gpio_to_irq(soc_int_gpio);

	res = irq_set_irq_wake(soc_int_irq, 1);
	if (res) {
		pr_err("%s: Failed to set irq wake for soc_int: %d\n", __func__, res);
		goto error;
	}

	pr_warn("%s: soc_int_gpio=%d soc_int_irq=%d\n",
			__func__, soc_int_gpio, soc_int_irq);

	bq27520_platform_data.soc_int_irq = soc_int_irq;

	return 0;

error:
	return res;
}
#endif
#endif

static void ardbeg_i2c_init(void)
{
	struct board_info board_info;
	tegra_get_board_info(&board_info);

	i2c_register_board_info(0, audio_board_info, ARRAY_SIZE(audio_board_info));

	if (board_info.board_id == BOARD_PM359 ||
			board_info.board_id == BOARD_PM358 ||
			board_info.board_id == BOARD_PM363) {
		i2c_keyboard_board_info.irq = gpio_to_irq(I2C_KB_IRQ);
		i2c_register_board_info(1, &i2c_keyboard_board_info , 1);

		i2c_touchpad_board_info.irq = gpio_to_irq(I2C_TP_IRQ);
		i2c_register_board_info(1, &i2c_touchpad_board_info , 1);
	}

	i2c_register_board_info(0, i2c_led_board_info, ARRAY_SIZE(i2c_led_board_info));

#ifdef BQ27520_INT_SUPPORT
	ardbeg_bq27520_init();
#endif
	i2c_register_board_info(1, bq27520_boardinfo, ARRAY_SIZE(bq27520_boardinfo));
}

#ifndef CONFIG_USE_OF
static struct platform_device *ardbeg_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
};

static struct tegra_serial_platform_data ardbeg_uarta_pdata = {
	.dma_req_selector = 8,
	.modem_interrupt = false,
};

static struct tegra_serial_platform_data ardbeg_uartb_pdata = {
	.dma_req_selector = 9,
	.modem_interrupt = false,
};

static struct tegra_serial_platform_data ardbeg_uartc_pdata = {
	.dma_req_selector = 10,
	.modem_interrupt = false,
};
#endif

static struct tegra_serial_platform_data ardbeg_uartd_pdata = {
	.dma_req_selector = 19,
	.modem_interrupt = false,
};

static struct tegra_asoc_platform_data ardbeg_audio_pdata_rt5671 = {
	.gpio_hp_det = -1,
	.gpio_ldo1_en = -1,
	.gpio_spkr_en = -1,
	.gpio_int_mic_en = -1,
	.gpio_ext_mic_en = -1,
	.gpio_hp_mute = TEGRA_GPIO_HP_MUTE,
	.gpio_codec1 = -1,
	.gpio_codec2 = -1,
	.gpio_codec3 = -1,
	.i2s_param[HIFI_CODEC]       = {
		.audio_port_id = 0,
		.is_i2s_master = 0,
		.i2s_mode = TEGRA_DAIFMT_I2S,
		.sample_size	= 16,
		.channels       = 2,
		.rate		= 48000,
	},
};

static struct platform_device ardbeg_audio_device_rt5671 = {
	.name = "tegra-snd-rt5671",
	.id = 0,
	.dev = {
		.platform_data = &ardbeg_audio_pdata_rt5671,
	},
};

static void __init ardbeg_uart_init(void)
{

#ifndef CONFIG_USE_OF
	tegra_uarta_device.dev.platform_data = &ardbeg_uarta_pdata;
	tegra_uartb_device.dev.platform_data = &ardbeg_uartb_pdata;
	tegra_uartc_device.dev.platform_data = &ardbeg_uartc_pdata;
	platform_add_devices(ardbeg_uart_devices,
			ARRAY_SIZE(ardbeg_uart_devices));
#endif
	tegra_uartd_device.dev.platform_data = &ardbeg_uartd_pdata;
	if (!is_tegra_debug_uartport_hs()) {
		int debug_port_id = uart_console_debug_init(3);
		if (debug_port_id < 0)
			return;

#ifdef CONFIG_TEGRA_FIQ_DEBUGGER
		tegra_serial_debug_init(TEGRA_UARTD_BASE, INT_WDT_CPU, NULL, -1, -1);
#endif
		platform_device_register(uart_console_debug_device);
	} else {
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_GPIO_PJ7,
						TEGRA_PUPD_PULL_DOWN);
		tegra_pinmux_set_tristate(TEGRA_PINGROUP_GPIO_PJ7,
						TEGRA_TRI_TRISTATE);
		tegra_pinmux_set_io(TEGRA_PINGROUP_GPIO_PB0,
						TEGRA_PIN_OUTPUT);
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_GPIO_PB0,
						TEGRA_PUPD_PULL_DOWN);
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

static struct tegra_pci_platform_data laguna_pcie_platform_data = {
	.port_status[0]	= 1,
	.port_status[1]	= 0,
	/* Laguna platforms does not support CLKREQ# feature */
	.has_clkreq	= 0,
	.gpio_hot_plug	= TEGRA_GPIO_PO1,
	.gpio_wake	= TEGRA_GPIO_PDD3,
	.gpio_x1_slot	= -1,
};

static void laguna_pcie_init(void)
{
	struct board_info board_info;
	int lane_owner = tegra_get_lane_owner_info() >> 1;

	tegra_get_board_info(&board_info);
	/* root port 1(x1 slot) is supported only on of ERS-S board */
	if (board_info.board_id == BOARD_PM359) {
		laguna_pcie_platform_data.port_status[1] = 1;
		/* enable x1 slot for PM359 if all lanes config'd for PCIe */
		if (lane_owner == PCIE_LANES_X4_X1)
			laguna_pcie_platform_data.gpio_x1_slot =
					PMU_TCA6416_GPIO(8);
	}
	tegra_pci_device.dev.platform_data = &laguna_pcie_platform_data;
	platform_device_register(&tegra_pci_device);
}

static struct platform_device *ardbeg_devices[] __initdata = {
	&tegra_pmu_device,
	&tegra_rtc_device,
#if defined(CONFIG_TEGRA_WAKEUP_MONITOR)
	&tegratab_tegra_wakeup_monitor_device,
#endif
	&tegra_udc_device,
#if defined(CONFIG_TEGRA_WATCHDOG)
	&tegra_wdt0_device,
#endif
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE) && !defined(CONFIG_USE_OF)
	&tegra12_se_device,
#endif
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device0,
	&tegra_i2s_device1,
	&tegra_i2s_device3,
	&tegra_i2s_device4,
	&ardbeg_audio_device_rt5671,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&baseband_dit_device,
	&fm_dit_device,
	&tegra_hda_device,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif
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
		.dcp_current_limit_ma = 2000,
		.charging_supported = false,
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
		.xcvr_setup_offset = -6,
		.xcvr_use_fuses = 1,
		.xcvr_hsslew_lsb = 1,
		.xcvr_hsslew_msb = 2,
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
		.xcvr_setup_offset = 0,
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

static struct gpio modem_gpios[] = { /* Bruce modem */
	{MODEM_EN, GPIOF_OUT_INIT_HIGH, "MODEM EN"},
	{MDM_RST, GPIOF_OUT_INIT_LOW, "MODEM RESET"},
	{MDM_SAR0, GPIOF_OUT_INIT_LOW, "MODEM SAR0"},
};

static struct tegra_usb_platform_data tegra_ehci2_hsic_baseband_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_HSIC,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
};

static struct tegra_usb_platform_data tegra_ehci2_hsic_smsc_hub_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_HSIC,
	.op_mode	= TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
};


static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci1_utmi_pdata,
};

static void ardbeg_usb_init(void)
{
	int usb_port_owner_info = tegra_get_usb_port_owner_info();
	int modem_id = tegra_get_modem_id();
	struct board_info bi;
	tegra_get_pmu_board_info(&bi);

	if (board_info.sku == 1100 || board_info.board_id == BOARD_P1761 ||
					board_info.board_id == BOARD_E1784)
		tegra_ehci1_utmi_pdata.u_data.host.turn_off_vbus_on_lp0 = true;

	if (board_info.board_id == BOARD_PM359 ||
			board_info.board_id == BOARD_PM358 ||
			board_info.board_id == BOARD_PM370 ||
			board_info.board_id == BOARD_PM374 ||
			board_info.board_id == BOARD_PM363) {
		/* Laguna */
		/* Host cable is detected through AMS PMU Interrupt */
		tegra_udc_pdata.id_det_type = TEGRA_USB_PMU_ID;
		tegra_ehci1_utmi_pdata.id_det_type = TEGRA_USB_PMU_ID;
		tegra_ehci1_utmi_pdata.id_extcon_dev_name = "as3722-extcon";
	} else {
		/* Ardbeg and TN8 */

		/*
		 * TN8 supports vbus changing and it can handle
		 * vbus voltages larger then 5V.  Enable this.
		 */
		if (board_info.board_id == BOARD_P1761 ||
			board_info.board_id == BOARD_E1784 ||
			board_info.board_id == BOARD_E1780) {

			/*
			 * Set the maximum voltage that can be supplied
			 * over USB vbus that the board supports if we use
			 * a quick charge 2 wall charger.
			 */
			tegra_udc_pdata.qc2_voltage = TEGRA_USB_QC2_9V;
			tegra_udc_pdata.u_data.dev.qc2_current_limit_ma = 1200;

			/* charger needs to be set to 2A - h/w will do 1.8A */
			tegra_udc_pdata.u_data.dev.dcp_current_limit_ma = 2000;
		}

		switch (bi.board_id) {
		case BOARD_E1733:
			/* Host cable is detected through PMU Interrupt */
			tegra_udc_pdata.id_det_type = TEGRA_USB_PMU_ID;
			tegra_ehci1_utmi_pdata.id_det_type = TEGRA_USB_PMU_ID;
			tegra_ehci1_utmi_pdata.id_extcon_dev_name =
							 "as3722-extcon";
			break;
		case BOARD_E1736:
		case BOARD_E1769:
		case BOARD_E1735:
		case BOARD_E1936:
		case BOARD_P1761:
			/* Device cable is detected through PMU Interrupt */
			tegra_udc_pdata.support_pmu_vbus = true;
			tegra_udc_pdata.vbus_extcon_dev_name = "palmas-extcon";
			tegra_ehci1_utmi_pdata.support_pmu_vbus = true;
			tegra_ehci1_utmi_pdata.vbus_extcon_dev_name =
							 "palmas-extcon";
			/* Host cable is detected through PMU Interrupt */
			tegra_udc_pdata.id_det_type = TEGRA_USB_PMU_ID;
			tegra_ehci1_utmi_pdata.id_det_type = TEGRA_USB_PMU_ID;
			tegra_ehci1_utmi_pdata.id_extcon_dev_name =
							 "palmas-extcon";
		}
	}

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

	if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB)) {
		if (!modem_id) {
			if ((bi.board_id != BOARD_P1761) &&
			    (bi.board_id != BOARD_E1922) &&
			    (bi.board_id != BOARD_E1784)) {
				tegra_ehci2_device.dev.platform_data =
					&tegra_ehci2_utmi_pdata;
				platform_device_register(&tegra_ehci2_device);
			}
		}
	}

	if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB)) {
		if ((bi.board_id != BOARD_P1761) &&
		    (bi.board_id != BOARD_E1922) &&
		    (bi.board_id != BOARD_E1784)) {
			tegra_ehci3_device.dev.platform_data =
				&tegra_ehci3_utmi_pdata;
			platform_device_register(&tegra_ehci3_device);
		}
	}

}

static struct tegra_xusb_platform_data xusb_pdata = {
	.portmap = TEGRA_XUSB_SS_P0 | TEGRA_XUSB_USB2_P0 | TEGRA_XUSB_SS_P1 |
			TEGRA_XUSB_USB2_P1 | TEGRA_XUSB_USB2_P2,
};

static void ardbeg_xusb_init(void)
{
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	xusb_pdata.lane_owner = (u8) tegra_get_lane_owner_info();

	if (board_info.board_id == BOARD_PM359 ||
			board_info.board_id == BOARD_PM358 ||
			board_info.board_id == BOARD_PM374 ||
			board_info.board_id == BOARD_PM370 ||
			board_info.board_id == BOARD_PM363) {
		/* Laguna */
		pr_info("Laguna ERS. 0x%x\n", board_info.board_id);

		if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB))
			xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P0 |
				TEGRA_XUSB_SS_P0);

		if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB))
			xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P1 |
				TEGRA_XUSB_SS_P1 | TEGRA_XUSB_USB2_P2);

		/* FIXME Add for UTMIP2 when have odmdata assigend */
	} else {
		/* Ardbeg */
		if (board_info.board_id == BOARD_E1781) {
			pr_info("Shield ERS-S. 0x%x\n", board_info.board_id);
			/* Shield ERS-S */
			if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB))
				xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P0);

			if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB))
				xusb_pdata.portmap &= ~(
					TEGRA_XUSB_USB2_P1 | TEGRA_XUSB_SS_P0 |
					TEGRA_XUSB_USB2_P2 | TEGRA_XUSB_SS_P1);
		} else {
			pr_info("Shield ERS 0x%x\n", board_info.board_id);
			/* Shield ERS */
			if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB))
				xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P0 |
					TEGRA_XUSB_SS_P0);

			if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB))
				xusb_pdata.portmap &= ~(TEGRA_XUSB_USB2_P1 |
					TEGRA_XUSB_USB2_P2 | TEGRA_XUSB_SS_P1);
		}
		/* FIXME Add for UTMIP2 when have odmdata assigend */
	}

	if (usb_port_owner_info & HSIC1_PORT_OWNER_XUSB)
		xusb_pdata.portmap |= TEGRA_XUSB_HSIC_P0;

	if (usb_port_owner_info & HSIC2_PORT_OWNER_XUSB)
		xusb_pdata.portmap |= TEGRA_XUSB_HSIC_P1;
}

static int baseband_init(void)
{
	int ret;

	ret = gpio_request_array(modem_gpios, ARRAY_SIZE(modem_gpios));
	if (ret) {
		pr_warn("%s:gpio request failed\n", __func__);
		return ret;
	}

	/* enable pull-down for MDM_COLD_BOOT */
	tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_ULPI_DATA4,
				    TEGRA_PUPD_PULL_DOWN);

	/* export GPIO for user space access through sysfs */
	gpio_export(MDM_RST, false);
	gpio_export(MDM_SAR0, false);

	return 0;
}

static const struct tegra_modem_operations baseband_operations = {
	.init = baseband_init,
};

static struct tegra_usb_modem_power_platform_data baseband_pdata = {
	.ops = &baseband_operations,
	.regulator_name = "vdd_wwan_mdm",
	.wake_gpio = -1,
	.boot_gpio = MDM_COLDBOOT,
	.boot_irq_flags = IRQF_TRIGGER_RISING |
				    IRQF_TRIGGER_FALLING |
				    IRQF_ONESHOT,
	.autosuspend_delay = 2000,
	.short_autosuspend_delay = 50,
	.tegra_ehci_device = &tegra_ehci2_device,
	.tegra_ehci_pdata = &tegra_ehci2_hsic_baseband_pdata,
};

static struct platform_device icera_bruce_device = {
	.name = "tegra_usb_modem_power",
	.id = -1,
	.dev = {
		.platform_data = &baseband_pdata,
	},
};

static void ardbeg_modem_init(void)
{
	int modem_id = tegra_get_modem_id();
	struct board_info board_info;
	struct board_info pmu_board_info;
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	tegra_get_board_info(&board_info);
	tegra_get_pmu_board_info(&pmu_board_info);
	pr_info("%s: modem_id = %d\n", __func__, modem_id);

	switch (modem_id) {
	case TEGRA_BB_BRUCE:
		if (!(usb_port_owner_info & HSIC1_PORT_OWNER_XUSB)) {
			/* Set specific USB wake source for Ardbeg */
			if (board_info.board_id == BOARD_E1780)
				tegra_set_wake_source(42, INT_USB2);
			if (pmu_board_info.board_id == BOARD_E1736 ||
				pmu_board_info.board_id == BOARD_E1769)
				baseband_pdata.regulator_name = NULL;
			platform_device_register(&icera_bruce_device);
		}
		break;
	case TEGRA_BB_HSIC_HUB: /* HSIC hub */
		if (!(usb_port_owner_info & HSIC1_PORT_OWNER_XUSB)) {
			tegra_ehci2_device.dev.platform_data =
				&tegra_ehci2_hsic_smsc_hub_pdata;
			/* Set specific USB wake source for Ardbeg */
			if (board_info.board_id == BOARD_E1780)
				tegra_set_wake_source(42, INT_USB2);
			platform_device_register(&tegra_ehci2_device);
		} else
			xusb_pdata.pretend_connect_0 = true;
		break;
	default:
		return;
	}
}

#ifdef CONFIG_USE_OF
static struct of_dev_auxdata ardbeg_auxdata_lookup[] __initdata = {
	T124_SPI_OF_DEV_AUXDATA,
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
	OF_DEV_AUXDATA("nvidia,tegra124-pwm", TEGRA_PWFM_BASE, "tegra-pwm", NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-tsec", TEGRA_TSEC_BASE, "tsec", NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006000, "serial-tegra.0",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006040, "serial-tegra.1",
				NULL),
	OF_DEV_AUXDATA("nvidia,tegra114-hsuart", 0x70006200, "serial-tegra.2",
				NULL),
	T124_I2C_OF_DEV_AUXDATA,
	OF_DEV_AUXDATA("nvidia,tegra124-xhci", 0x70090000, "tegra-xhci",
				&xusb_pdata),
	OF_DEV_AUXDATA("nvidia,tegra124-dc", TEGRA_DISPLAY_BASE, "tegradc.0",
		NULL),
	OF_DEV_AUXDATA("nvidia,tegra124-dc", TEGRA_DISPLAY2_BASE, "tegradc.1",
		NULL),
	{}
};
#endif

struct maxim_sti_pdata maxim_sti_pdata = {
	.touch_fusion         = "/vendor/bin/touch_fusion",
	.config_file          = "/vendor/firmware/touch_fusion.cfg",
	.fw_name              = "maxim_fp35.bin",
	.nl_family            = TF_FAMILY_NAME,
	.nl_mc_groups         = 5,
	.chip_access_method   = 2,
	.default_reset_state  = 0,
	.tx_buf_size          = 4100,
	.rx_buf_size          = 4100,
	.gpio_reset           = TOUCH_GPIO_RST_MAXIM_STI_SPI,
	.gpio_irq             = TOUCH_GPIO_IRQ_MAXIM_STI_SPI
};

struct maxim_sti_pdata maxim_sti_pdata_rd = {
	.touch_fusion         = "/vendor/bin/touch_fusion_rd",
	.config_file          = "/vendor/firmware/touch_fusion.cfg",
	.fw_name              = "maxim_fp35.bin",
	.nl_family            = TF_FAMILY_NAME,
	.nl_mc_groups         = 5,
	.chip_access_method   = 2,
	.default_reset_state  = 0,
	.tx_buf_size          = 4100,
	.rx_buf_size          = 4100,
	.gpio_reset           = TOUCH_GPIO_RST_MAXIM_STI_SPI,
	.gpio_irq             = TOUCH_GPIO_IRQ_MAXIM_STI_SPI
};

static struct tegra_spi_device_controller_data maxim_dev_cdata = {
	.rx_clk_tap_delay = 0,
	.is_hw_based_cs = true,
	.tx_clk_tap_delay = 0,
};

struct spi_board_info maxim_sti_spi_board = {
	.modalias = MAXIM_STI_NAME,
	.bus_num = TOUCH_SPI_ID,
	.chip_select = TOUCH_SPI_CS,
	.max_speed_hz = 12 * 1000 * 1000,
	.mode = SPI_MODE_0,
	.platform_data = &maxim_sti_pdata,
	.controller_data = &maxim_dev_cdata,
};

static __initdata struct tegra_clk_init_table touch_clk_init_table[] = {
	/* name         parent          rate            enabled */
	{ "extern2",    "pll_p",        41000000,       false},
	{ "clk_out_2",  "extern2",      40800000,       false},
	{ NULL,         NULL,           0,              0},
};

static struct rm_spi_ts_platform_data rm31080ts_ardbeg_data = {
	.gpio_reset = TOUCH_GPIO_RST_RAYDIUM_SPI,
	.config = 0,
	.platform_id = RM_PLATFORM_A010,
	.name_of_clock = "clk_out_2",
	.name_of_clock_con = "extern2",
};

static struct rm_spi_ts_platform_data rm31080ts_norrin_data = {
	.gpio_reset = TOUCH_GPIO_RST_RAYDIUM_SPI,
	.config = 0,
	.platform_id = RM_PLATFORM_P140,
	.name_of_clock = "clk_out_2",
	.name_of_clock_con = "extern2",
};

static struct tegra_spi_device_controller_data dev_cdata = {
	.rx_clk_tap_delay = 0,
	.tx_clk_tap_delay = 16,
};

static struct spi_board_info rm31080a_ardbeg_spi_board[1] = {
	{
		.modalias = "rm_ts_spidev",
		.bus_num = TOUCH_SPI_ID,
		.chip_select = TOUCH_SPI_CS,
		.max_speed_hz = 12 * 1000 * 1000,
		.mode = SPI_MODE_0,
		.controller_data = &dev_cdata,
		.platform_data = &rm31080ts_ardbeg_data,
	},
};


#define TP_GPIO_POWER			TEGRA_GPIO_PK1
#define TP_GPIO_RESET			TEGRA_GPIO_PK4
#define TP_GPIO_INTR			TEGRA_GPIO_PR7

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_DSX

#define S7040_FIRMWARE		"synaptics_s7040"
#define S7040_OFILM_TEST_DATA	"synaptics_s7040_ofilm_test_data"

static unsigned char s7040_ofilm_firmware_data[] = {
	#include "synaptics_7040_ofilm_firmware.h"
};

static unsigned char s7040_ofilm_test_data[] = {
	#include "synaptics_7040_ofilm_test_data.h"
};

DECLARE_BUILTIN_FIRMWARE_SIZE(S7040_FIRMWARE,
			s7040_ofilm_firmware_data, sizeof(s7040_ofilm_firmware_data));

DECLARE_BUILTIN_FIRMWARE_SIZE(S7040_OFILM_TEST_DATA,
			s7040_ofilm_test_data, sizeof(s7040_ofilm_test_data));

static unsigned int key_map[] = {
	KEY_MENU, KEY_HOME, KEY_BACK
};

static struct synaptics_dsx_cap_button_map button_map = {
	.nbuttons		= 3,
	.map			= key_map,
};

static struct synaptics_dsx_board_data s7040_platform_data = {
	.x_flip		= false,
	.y_flip		= false,
	.swap_axes		= false,
	.irq_gpio		= TP_GPIO_INTR,
	.irq_on_state		= 0,
	.power_gpio		= -1,
	.dcdc_gpio		= -1,
	.power_on_state		= 1,
	.reset_gpio		= TP_GPIO_RESET,
	.reset_on_state		= 0,
	.irq_flags		= IRQF_TRIGGER_LOW | IRQF_ONESHOT,
	.fw_name		= S7040_FIRMWARE,
	.self_test_name		= S7040_OFILM_TEST_DATA,
	.panel_x		= 1536,
	.panel_y		= 2048,
	.power_delay_ms		= 160,
	.reset_delay_ms		= 100,
	.reset_active_ms		= 20,
	.byte_delay_us		= 20,
	.block_delay_us		= 20,
	.regulator_name		= "vdd-touch",
	.cap_button_map		= &button_map,
};

static struct i2c_board_info s7040_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("synaptics_dsx_i2c", 0x20),
		.platform_data = &s7040_platform_data,
	},
};

#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT

#include "mxT1664T.h"
#include "mxT1066T.h"
#define MXT1664T_FIRMWARE		"mxt1664t_fw"
#define MXT1066T_FIRMWARE		"mxt1066t_fw"
#define MXT1664T_LENS_CONFIG_NO_DUMMY		"mxt1664t_lens_config_no_dummy.cfg"
#define MXT1664T_LENS_CONFIG_WITH_DUMMY		"mxt1664t_lens_config_with_dummy.cfg"
#define MXT1066T_LENS_CONFIG			"mxt1066t_lens_config.cfg"

static unsigned char mXT1664T_lens_config_no_dummy[] = {
	#include "mxt_lens_1664t_config_no_dummy.h"
};

static unsigned char mXT1664T_lens_config_with_dummy[] = {
	#include "mxt_lens_1664t_config_with_dummy.h"
};

static unsigned char mXT1066T_lens_config[] = {
	#include "mxt_lens_1066t_config.h"
};

DECLARE_BUILTIN_FIRMWARE_SIZE(MXT1664T_FIRMWARE, mXT1664Tfw, sizeof(mXT1664Tfw)-1);
DECLARE_BUILTIN_FIRMWARE_SIZE(MXT1066T_FIRMWARE, mXT1066Tfw, sizeof(mXT1066Tfw)-1);
DECLARE_BUILTIN_FIRMWARE_SIZE(MXT1664T_LENS_CONFIG_NO_DUMMY, \
						mXT1664T_lens_config_no_dummy, \
						sizeof(mXT1664T_lens_config_no_dummy)-1);
DECLARE_BUILTIN_FIRMWARE_SIZE(MXT1664T_LENS_CONFIG_WITH_DUMMY, \
						mXT1664T_lens_config_with_dummy, \
						sizeof(mXT1664T_lens_config_with_dummy)-1);
DECLARE_BUILTIN_FIRMWARE_SIZE(MXT1066T_LENS_CONFIG, \
						mXT1066T_lens_config, \
						sizeof(mXT1066T_lens_config)-1);

static int mxt_lens_1664t_key_codes[MXT_KEYARRAY_MAX_KEYS] = {
	KEY_BACK, KEY_HOME, KEY_MENU, KEY_POWER,
};

static int mxt_lens_1066t_key_codes[MXT_KEYARRAY_MAX_KEYS] = {
	KEY_MENU, KEY_HOME, KEY_BACK, KEY_POWER,
};

static struct mxt_config_info mxt_config_array[] = {
	{
		.family_id	= 0xA4,
		.variant_id	= 0x04,
		.version	= 0x10,
		.build		= 0xAA,
		.user_id	= 0x00,
		.bootldr_id	= 0x48,
		.mxt_cfg_name	= MXT1664T_LENS_CONFIG_NO_DUMMY,
		.vendor_id	= 0x4,
		.key_codes		= mxt_lens_1664t_key_codes,
		.key_num		= 4,
		.mxt_fw_name		= MXT1664T_FIRMWARE,
	},
	{
		.family_id	= 0xA4,
		.variant_id	= 0x04,
		.version	= 0x10,
		.build		= 0xAA,
		.user_id	= 0xAA,
		.bootldr_id	= 0x48,
		.mxt_cfg_name	= MXT1664T_LENS_CONFIG_WITH_DUMMY,
		.vendor_id	= 0x4,
		.key_codes		= mxt_lens_1664t_key_codes,
		.key_num		= 4,
		.mxt_fw_name		= MXT1664T_FIRMWARE,
	},
	{
		.family_id	= 0xA4,
		.variant_id	= 0x0B,
		.version	= 0x12,
		.build		= 0xAA,
		.user_id	= 0x00,
		.bootldr_id	= 0x51,
		.mxt_cfg_name	= MXT1066T_LENS_CONFIG,
		.vendor_id	= 0x4,
		.key_codes		= mxt_lens_1066t_key_codes,
		.key_num		= 4,
		.mxt_fw_name		= MXT1066T_FIRMWARE,
	},
};

static struct mxt_platform_data mxt_platform_data = {
	.config_array		= mxt_config_array,
	.config_array_size		= ARRAY_SIZE(mxt_config_array),
	.irqflags		= IRQF_TRIGGER_LOW | IRQF_ONESHOT,
	.power_gpio		= -1,
	.reset_gpio		= TP_GPIO_RESET,
	.irq_gpio		= TP_GPIO_INTR,
	.read_chg		= NULL,
	.gpio_mask		= 0xc,
	.vendor_info		= 0x03eb,
	.product_info		= 0x214f,
};

static struct i2c_board_info mxt_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x4a),
		.platform_data = &mxt_platform_data,
	},
};

#endif

static struct spi_board_info rm31080a_norrin_spi_board[1] = {
	{
		.modalias = "rm_ts_spidev",
		.bus_num = NORRIN_TOUCH_SPI_ID,
		.chip_select = NORRIN_TOUCH_SPI_CS,
		.max_speed_hz = 12 * 1000 * 1000,
		.mode = SPI_MODE_0,
		.controller_data = &dev_cdata,
		.platform_data = &rm31080ts_norrin_data,
	},
};

static int __init ardbeg_touch_init(void)
{
	int i;

	pr_info(" %s init atmel touch\n", __func__);
	for (i = 0; i < ARRAY_SIZE(mxt_device_info); i++) {
		mxt_device_info[i].irq = gpio_to_irq(TP_GPIO_INTR);
	}
	i2c_register_board_info(3, mxt_device_info,
				ARRAY_SIZE(mxt_device_info));

	i2c_register_board_info(3, s7040_device_info,
				ARRAY_SIZE(s7040_device_info));

	return 0;
}

static void __init ardbeg_sysedp_init(void)
{
	struct board_info bi;

	tegra_get_board_info(&bi);

	switch (bi.board_id) {
	case BOARD_E1780:
		if (bi.sku == 1100) {
			tn8_new_sysedp_init();
		}
		else
			shield_new_sysedp_init();
		break;
	case BOARD_E1922:
	case BOARD_E1784:
	case BOARD_P1761:
		tn8_new_sysedp_init();
		break;
	case BOARD_PM358:
	case BOARD_PM359:
	default:
		break;
	}
}

static void __init ardbeg_sysedp_dynamic_capping_init(void)
{
	struct board_info bi;

	tegra_get_board_info(&bi);

	switch (bi.board_id) {
	case BOARD_E1780:
		if (bi.sku == 1100)
			tn8_sysedp_dynamic_capping_init();
		else
			shield_sysedp_dynamic_capping_init();
		break;
	case BOARD_E1922:
	case BOARD_E1784:
	case BOARD_P1761:
		tn8_sysedp_dynamic_capping_init();
		break;
	case BOARD_PM358:
	case BOARD_PM359:
	default:
		break;
	}
}

static void __init ardbeg_sysedp_batmon_init(void)
{
	struct board_info bi;

	if (!IS_ENABLED(CONFIG_SYSEDP_FRAMEWORK))
		return;

	tegra_get_board_info(&bi);

	switch (bi.board_id) {
	case BOARD_E1780:
		if (bi.sku != 1100)
			shield_sysedp_batmon_init();
		break;
	case BOARD_PM358:
	case BOARD_PM359:
	default:
		break;
	}
}



static void __init edp_init(void)
{
	struct board_info bi;

	tegra_get_board_info(&bi);

	switch (bi.board_id) {
	case BOARD_E1780:
		if (bi.sku == 1100)
			tn8_edp_init();
		else
			ardbeg_edp_init();
		break;
	case BOARD_P1761:
			tn8_edp_init();
			break;
	case BOARD_PM358:
	case BOARD_PM359:
			laguna_edp_init();
			break;
	default:
			ardbeg_edp_init();
			break;
	}
}

static void __init tegra_ardbeg_early_init(void)
{
	ardbeg_sysedp_init();
	tegra_clk_init_from_table(ardbeg_clk_init_table);
	tegra_clk_verify_parents();
	if (of_machine_is_compatible("nvidia,laguna"))
		tegra_soc_device_init("laguna");
	else if (of_machine_is_compatible("nvidia,tn8"))
		tegra_soc_device_init("tn8");
	else if (of_machine_is_compatible("nvidia,mocha"))
		tegra_soc_device_init("mocha");
	else if (of_machine_is_compatible("nvidia,ardbeg_sata"))
		tegra_soc_device_init("ardbeg_sata");
	else if (of_machine_is_compatible("nvidia,norrin"))
		tegra_soc_device_init("norrin");
	else
		tegra_soc_device_init("ardbeg");
}

static struct tegra_dtv_platform_data ardbeg_dtv_pdata = {
	.dma_req_selector = 11,
};

static void __init ardbeg_dtv_init(void)
{
	tegra_dtv_device.dev.platform_data = &ardbeg_dtv_pdata;
	platform_device_register(&tegra_dtv_device);
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

#ifdef CONFIG_BLUEDROID_PM
static struct resource ardbeg_bluedroid_pm_resources[] = {
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

static struct platform_device ardbeg_bluedroid_pm_device = {
	.name = "bluedroid_pm",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(ardbeg_bluedroid_pm_resources),
	.resource       = ardbeg_bluedroid_pm_resources,
};

static noinline void __init ardbeg_setup_bluedroid_pm(void)
{
	ardbeg_bluedroid_pm_resources[1].start =
		ardbeg_bluedroid_pm_resources[1].end =
				gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&ardbeg_bluedroid_pm_device);
}
#endif

static void __init tegra_ardbeg_late_init(void)
{
	struct board_info board_info;
	tegra_get_board_info(&board_info);
	pr_info("board_info: id:sku:fab:major:minor = 0x%04x:0x%04x:0x%02x:0x%02x:0x%02x\n",
		board_info.board_id, board_info.sku,
		board_info.fab, board_info.major_revision,
		board_info.minor_revision);

	ardbeg_display_init();
	ardbeg_uart_init();
	ardbeg_usb_init();
	ardbeg_xusb_init();
	ardbeg_i2c_init();
	platform_add_devices(ardbeg_devices, ARRAY_SIZE(ardbeg_devices));
	tegra_io_dpd_init();
	ardbeg_sdhci_init();
	if (board_info.board_id == BOARD_PM359 ||
			board_info.board_id == BOARD_PM358 ||
			board_info.board_id == BOARD_PM370 ||
			board_info.board_id == BOARD_PM363)
		laguna_regulator_init();
	else if (board_info.board_id == BOARD_PM374)
		norrin_regulator_init();
	else
		ardbeg_regulator_init();
	ardbeg_suspend_init();
	ardbeg_emc_init();
	edp_init();
	isomgr_init();
	ardbeg_touch_init();
	ardbeg_panel_init();
	switch (board_info.board_id) {
	case BOARD_PM358:
		laguna_pm358_pmon_init();
		break;
	case BOARD_E1784:
	case BOARD_P1761:
		tn8_p1761_pmon_init();
		break;
	default:
		ardbeg_pmon_init();
		break;
	}
	if (board_info.board_id == BOARD_PM359 ||
			board_info.board_id == BOARD_PM358 ||
			board_info.board_id == BOARD_PM363)
		laguna_pcie_init();
	else {
		/* put PEX pads into DPD mode to save additional power */
		tegra_io_dpd_enable(&pexbias_io);
		tegra_io_dpd_enable(&pexclk1_io);
		tegra_io_dpd_enable(&pexclk2_io);
	}


	ardbeg_sensors_init();

	ardbeg_soctherm_init();

#ifdef CONFIG_BLUEDROID_PM
	ardbeg_setup_bluedroid_pm();
#endif
	tegra_register_fuse();

	ardbeg_sysedp_dynamic_capping_init();
	ardbeg_sysedp_batmon_init();
}

static void __init ardbeg_ramconsole_reserve(unsigned long size)
{
	tegra_reserve_ramoops_memory(size);
}
static void __init tegra_ardbeg_init_early(void)
{
	ardbeg_rail_alignment_init();
	tegra12x_init_early();
}

static void __init tegra_ardbeg_dt_init(void)
{
	tegra_get_board_info(&board_info);
	tegra_get_display_board_info(&display_board_info);

	tegra_ardbeg_early_init();
#ifdef CONFIG_USE_OF
	of_platform_populate(NULL,
		of_default_bus_match_table, ardbeg_auxdata_lookup,
		&platform_bus);
#endif

	tegra_ardbeg_late_init();
}

static void __init tegra_ardbeg_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM) || \
		defined(CONFIG_TEGRA_NO_CARVEOUT)
	/* 1920*1200*4*2 = 18432000 bytes */
	tegra_reserve(0, SZ_16M + SZ_2M, SZ_16M);
#else
	tegra_reserve(SZ_1G, SZ_16M + SZ_2M, SZ_4M);
#endif
	ardbeg_ramconsole_reserve(SZ_2M);
}

static const char * const ardbeg_dt_board_compat[] = {
	"nvidia,ardbeg",
	NULL
};

static const char * const laguna_dt_board_compat[] = {
	"nvidia,laguna",
	NULL
};

static const char * const tn8_dt_board_compat[] = {
	"nvidia,tn8",
	NULL
};

static const char * const mocha_dt_board_compat[] = {
	"nvidia,mocha",
	NULL
};

static const char * const ardbeg_sata_dt_board_compat[] = {
	"nvidia,ardbeg_sata",
	NULL
};

static const char * const norrin_dt_board_compat[] = {
	"nvidia,norrin",
	NULL
};

DT_MACHINE_START(LAGUNA, "laguna")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_ardbeg_reserve,
	.init_early	= tegra_ardbeg_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_ardbeg_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= laguna_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END

DT_MACHINE_START(TN8, "tn8")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_ardbeg_reserve,
	.init_early	= tegra_ardbeg_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_ardbeg_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= tn8_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END

DT_MACHINE_START(MOCHA, "mocha")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_ardbeg_reserve,
	.init_early	= tegra_ardbeg_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_ardbeg_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= mocha_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END

DT_MACHINE_START(NORRIN, "norrin")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_ardbeg_reserve,
	.init_early	= tegra_ardbeg_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_ardbeg_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= norrin_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END

DT_MACHINE_START(ARDBEG, "ardbeg")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_ardbeg_reserve,
	.init_early	= tegra_ardbeg_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_ardbeg_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= ardbeg_dt_board_compat,
	.init_late      = tegra_init_late
MACHINE_END

DT_MACHINE_START(ARDBEG_SATA, "ardbeg_sata")
	.atag_offset	= 0x100,
	.smp		= smp_ops(tegra_smp_ops),
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_ardbeg_reserve,
	.init_early	= tegra_ardbeg_init_early,
	.init_irq	= irqchip_init,
	.init_time	= clocksource_of_init,
	.init_machine	= tegra_ardbeg_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= ardbeg_sata_dt_board_compat,
	.init_late      = tegra_init_late

MACHINE_END
