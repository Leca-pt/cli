/*
 * cli_lfs.c
 *
 *  Created on: Jul 6, 2024
 *      Author: licin
 */

#include "cli_lfs.h"

extern lfs_t lfs;
extern lfs_file_t file;
extern lfs_dir_t dir;
char current_path[256] = "/";
char message[256];
// Sample command function
void sample_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
    	cli->print_string(argv[i], strlen(argv[i]));
    	cli->print_string(" ",1);
    }
    snprintf(message, sizeof(message),"\r\n");
    cli->print_string(message,strlen(message));
    //cli->print_string("\r\n",2);
}

// Echo command function
void echo_command(Cli_HandlerTypeDef_t *cli,int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
    	cli->print_string(argv[i], strlen(argv[i]));
        if (i < argc - 1) {
        	cli->print_string(" ",1);
        }
    }
    snprintf(message, sizeof(message),"\r\n");
    cli->print_string(message,strlen(message));
    //cli->print_string("\r\n",2);
}

void lfs_ls(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {

	//int err;
    int err = lfs_dir_open(&lfs, &dir, current_path);
    if (err) {
        return;
    }

    struct lfs_info info;
    while (true) {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            return;
        }

        if (res == 0) {
            break;
        }

        switch (info.type) {
            case LFS_TYPE_REG: printf("reg "); break;
            case LFS_TYPE_DIR: printf("dir "); break;
            default:           printf("?   "); break;
        }

        static const char *prefixes[] = {"", "K", "M", "G"};
        for (int i = sizeof(prefixes)/sizeof(prefixes[0])-1; i >= 0; i--) {
            if (info.size >= (1 << 10*i)-1) {
                //printf("%*lu%sB ", 4-(i != 0), info.size >> 10*i, prefixes[i]);
                snprintf(message, sizeof(message),"%*lu%sB ", 4-(i != 0), info.size >> 10*i, prefixes[i]);
                cli->print_string(message, strlen(message));               break;
            }
        }

        //printf("%s\r\n", info.name);
        snprintf(message, sizeof(message),"%s\r\n", info.name);
        cli->print_string(message, strlen(message));
    }

    err = lfs_dir_close(&lfs, &dir);
    if (err) {
        return;
    }

    return;
}


// Helper function to normalize the path
void normalize_path(char *path) {
    char temp[256];
    char *p = path, *q = temp;
    int len;

    while (*p) {
        if (*p == '/') {
            if (*(p + 1) == '/') {
                // Skip duplicate slashes
                p++;
                continue;
            } else if (*(p + 1) == '.') {
                if (*(p + 2) == '/') {
                    // Skip "./"
                    p += 2;
                    continue;
                } else if (*(p + 2) == '.' && (*(p + 3) == '/' || *(p + 3) == '\0')) {
                    // Handle "../"
                    p += 3;
                    if (q > temp) {
                        // Move up one level in the path
                        q--;
                        while (q > temp && *(q - 1) != '/') {
                            q--;
                        }
                    }
                    continue;
                }
            }
        }
        *q++ = *p++;
    }
    *q = '\0';

    // If the path ends with a '/', remove it (unless it's the root "/")
    len = strlen(temp);
    if (len > 1 && temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    strcpy(path, temp);
}

// Function to remove a directory
void rmdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
    	snprintf(message, sizeof(message),"Usage: rmdir <directory>\r\n");
    	cli->print_string(message,strlen(message));
    	//cli->print_string("Usage: rmdir <directory>\r\n", strlen("Usage: rmdir <directory>\r\n"));
        return;
    }

    const char *directory = argv[1];
    char full_path[256];

    // Handle absolute and relative paths
    if (directory[0] == '/') {
        // Absolute path
        strncpy(full_path, directory, sizeof(full_path) - 1);
    } else {
        // Relative path
        snprintf(full_path, sizeof(full_path), "%s/%s", current_path, directory);
    }

    // Normalize the path
    normalize_path(full_path);

    // Remove directory in LittleFS
    if (lfs_remove(&lfs, full_path) < 0) {
    	snprintf(message, sizeof(message),"Failed to remove directory.\r\n");
    	cli->print_string(message,strlen(message));
    	//cli->print_string("Failed to remove directory.\r\n", strlen("Failed to remove directory.\r\n"));
        return;
    }
    snprintf(message, sizeof(message),"Directory removed.\r\n");
    cli->print_string(message,strlen(message));
    //cli->print_string("Directory removed.\r\n", strlen("Directory removed.\r\n"));
}

// Function to make a directory
void mkdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
        snprintf(message, sizeof(message),"Usage: mkdir <directory>\r\n");
        cli->print_string(message,strlen(message));
    	//cli->print_string("Usage: mkdir <directory>\r\n", strlen("Usage: mkdir <directory>\r\n"));
        return;
    }

    const char *directory = argv[1];
    char full_path[256];

    // Handle absolute and relative paths
    if (directory[0] == '/') {
        // Absolute path
        strncpy(full_path, directory, sizeof(full_path) - 1);
    } else {
        // Relative path
        snprintf(full_path, sizeof(full_path), "%s/%s", current_path, directory);
    }

    // Normalize the path
    normalize_path(full_path);

    // Make directory in LittleFS
    if (lfs_mkdir(&lfs, full_path) < 0) {
        snprintf(message, sizeof(message),"Failed to make directory.\r\n");
        cli->print_string(message,strlen(message));
    	//cli->print_string("Failed to make directory.\r\n", strlen("Failed to make directory.\r\n"));
        return;
    }
    snprintf(message, sizeof(message),"Directory created.\r\n");
    cli->print_string(message,strlen(message));
    //cli->print_string("Directory created.\r\n", strlen("Directory created.\r\n"));
}

void change_dir(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
        snprintf(message, sizeof(message),"Usage: cd <path>\r\n");
        cli->print_string(message,strlen(message));
    	//cli->print_string("Usage: change_dir <path>\r\n",strlen("Usage: change_dir <path>\r\n"));
        return;
    }

    const char *path = argv[1];
    char new_path[256];

    // Handle absolute and relative paths
    if (path[0] == '/') {
        // Absolute path
        strncpy(new_path, path, sizeof(new_path) - 1);
    } else {
        // Relative path
        snprintf(new_path, sizeof(new_path), "%s/%s", current_path, path);
    }

    // Normalize the path
    normalize_path(new_path);

    // Check if the directory exists
    int err = lfs_dir_open(&lfs, &dir, new_path);
    if (err) {
        snprintf(message, sizeof(message),"Error opening directory '%s': %d\r\n", new_path, err);
        cli->print_string(message,strlen(message));
        //printf("Error opening directory '%s': %d\r\n", new_path, err);
        return;
    }

    // Close the directory (just checking its existence)
    lfs_dir_close(&lfs, &dir);

    // Update the current path
    strncpy(current_path, new_path, sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';

    snprintf(message, sizeof(message),"Changed directory to '%s'\r\n");
    cli->print_string(message,strlen(message));
    //printf("Changed directory to '%s'\r\n", current_path);
}
