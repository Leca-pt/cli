/*
 * xmodem.h
 *
 *  Created on: Jan 26, 2025
 *      Author: licin
 */

#ifndef XMODEM_H_
#define XMODEM_H_

#include <stdint.h>
#include "lfs.h"
#include "cli.h"

#define SOH  0x01  // Start of Header
#define EOT  0x04  // End of Transmission
#define ACK  "\x06"  // Acknowledge
#define NAK  "\x15"  // Negative Acknowledge
#define CAN  "\x18"  // Cancel

#define XMODEM_PACKET_SIZE 128



// Functions
int xmodem_receive_file(Cli_HandlerTypeDef_t *cli, const char *path);


#endif /* XMODEM_H_ */
