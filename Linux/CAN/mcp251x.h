#ifndef __CAN_PLATFORM_MCP251X_H__
#define __CAN_PLATFORM_MCP251X_H__

/*
 *
 * CAN bus driver for Microchip 251x CAN Controller with SPI Interface
 *
 */

#include <linux/spi/spi.h>
#include <linux/wait.h>
/**
 * struct mcp251x_platform_data - MCP251X SPI CAN controller platform data
 * @oscillator_frequency:       - oscillator frequency in Hz
 * @board_specific_setup:       - called before probing the chip (power,reset)
 * @transceiver_enable:         - called to power on/off the transceiver
 * @power_enable:               - called to power on/off the mcp *and* the
 *                                transceiver
 *
 * Please note that you should define power_enable or transceiver_enable or
 * none of them. Defining both of them is no use.
 *
 */

#define SLEEPING_FLAG 		0x00
#define WAKE_UP_FLAG 		0x01


struct mcp251x_platform_data {
	unsigned long oscillator_frequency;
	int (*board_specific_setup)(struct spi_device *spi);
	int (*transceiver_enable)(int enable);
	int (*power_enable) (int enable);
	wait_queue_head_t wait_queue;
	int flag;
};

#endif /* __CAN_PLATFORM_MCP251X_H__ */
