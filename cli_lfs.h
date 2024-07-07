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

// Sample command function
void sample_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

// Echo command function
void echo_command(Cli_HandlerTypeDef_t *cli,int argc, char **argv);

void lfs_ls(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

// Helper function to normalize the path
void normalize_path(char *path);
// Function to remove a directory
void rmdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv);
// Function to make a directory
void mkdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv);

void change_dir(Cli_HandlerTypeDef_t *cli, int argc, char **argv);
#endif /* CLI_LFS_H_ */
