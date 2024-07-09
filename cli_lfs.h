/*
 * cli_lfs.h
 *
 *  Created on: Jul 6, 2024
 *      Author: licin
 */

#ifndef CLI_LFS_H_
#define CLI_LFS_H_

#include "cli.h"
#include "lfs.h"

#define COPY_BUFFER_SIZE 64

char * get_currentPath(void);
// Echo command function
Cli_state_e echo_command(Cli_HandlerTypeDef_t *cli,int argc, char **argv);

Cli_state_e cat_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

Cli_state_e rm_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

Cli_state_e lfs_ls(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

// Function to remove a directory
Cli_state_e rmdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv);
// Function to make a directory
Cli_state_e mkdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

Cli_state_e change_dir(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

Cli_state_e create_new_file(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

Cli_state_e move_file(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

Cli_state_e copy_file(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

#endif /* CLI_LFS_H_ */
