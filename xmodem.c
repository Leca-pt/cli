/*
 * xmodem.c
 *
 *  Created on: Jan 26, 2025
 *      Author: licin
 */


#include "xmodem.h"
#include "stm32f7xx_hal.h"

extern lfs_t lfs;

static bool wait_for_char(Cli_HandlerTypeDef_t *cli, uint8_t *ch, uint32_t timeout_ms) {
    uint32_t start_time = HAL_GetTick();  // Assuming HAL_GetTick() is available
    while ((HAL_GetTick() - start_time) < timeout_ms) {
        if (cli->read_char((char *)ch)) {
            return true;  // Character received
        }
    }
    return false;  // Timeout
}


int xmodem_receive_file(Cli_HandlerTypeDef_t *cli, const char *path) {
    uint8_t packet[XMODEM_PACKET_SIZE + 5];  // SOH + Seq + ~Seq + Data + Checksum
    uint8_t buffer[XMODEM_PACKET_SIZE];
    uint8_t seq_num = 1;
    uint8_t ch;
    bool transfer_complete = false;
    lfs_file_t file;

    lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC | LFS_O_APPEND);

    cli_printf(cli, "Waiting for XMODEM transfer...\r\n");
    HAL_Delay(15000);

    // Signal readiness to receive data
    cli_printf(cli,NAK);

    while (!transfer_complete) {
        if (!wait_for_char(cli, &ch, 10)) {  // Wait up to 10 seconds for a character
        	cli_printf(cli, CAN);
        	lfs_file_close(&lfs, &file);
            return -1;  // Exit on timeout
        }

        if (ch == SOH) {
            // Receive full packet
            for (int i = 0; i < (XMODEM_PACKET_SIZE + 4); i++) {
                if (!wait_for_char(cli, &packet[i], 1000)) {  // Wait for each byte
                	cli_printf(cli,CAN);
                	cli_printf(cli, "Timeout during packet reception.\n");
                	continue;
                }
            }

            // Validate sequence number
            if (packet[0] != seq_num || packet[1] != (255 - seq_num)) {
            	cli_printf(cli,CAN);
            	cli_printf(cli, "Sequence number mismatch.\n");
            	continue;
            }

            // Validate checksum
            uint16_t checksum = 0;
            for (int i = 0; i < XMODEM_PACKET_SIZE; i++) {
                checksum += packet[2 + i];
            }

            if ((checksum & 0xFF) != packet[XMODEM_PACKET_SIZE + 2]) {
                cli_printf(cli,CAN);
                cli_printf(cli, "Checksum error.\n");
                continue;
            }

            // Packet is valid; process data
            memcpy(buffer, &packet[2], XMODEM_PACKET_SIZE);
            lfs_file_write(&lfs, &file, buffer, XMODEM_PACKET_SIZE);
//            save_to_flash(cli,buffer, XMODEM_PACKET_SIZE);

            seq_num++;
            cli_printf(cli,ACK);  // Acknowledge the packet

        } else if (ch == EOT) {
        	cli_printf(cli,ACK);  // Finalize transfer
            transfer_complete = true;
        } else {
        	cli_printf(cli,NAK);  // Unexpected input
        	lfs_file_close(&lfs, &file);
        	return -1;
        }
    }
    lfs_file_close(&lfs, &file);
    return 0;
}


