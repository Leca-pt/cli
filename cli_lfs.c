/*
 * cli_lfs.c
 *
 *  Created on: Jul 6, 2024
 *      Author: licin
 */

#include "cli_lfs.h"

extern lfs_t lfs;
char current_path[256] = "/";

// Helper function to normalize the path
static void normalize_path(char *path);


char * get_currentPath(void){
	return &current_path[0];
}

//Function to get print current path
Cli_state_e pwd_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
	cli_printf(cli,"%s\r\n", current_path);
    return DONE_EXECUTING;
}

// Echo command function
Cli_state_e echo_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc > 4 || strcmp(argv[1],"-h")==0) {
    	cli_printf(cli,"Usage: echo <\"string\"> >>/> <file>\r\n");

        return DONE_EXECUTING;
    }
    char output_str[1024] = {0};  // Adjust buffer size as needed
    char file_path[512] = {0};    // Buffer for the full file path
    int append_mode = 0;          // 0: No file operation, 1: Overwrite, 2: Append

    // Check for redirection operators
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0) && i + 1 < argc) {
            const char *redirect_operator = argv[i];
            const char *path_argument = argv[i + 1];

            // Determine if the path is absolute or relative
            if (path_argument[0] == '/') {
                // Absolute path
                snprintf(file_path, sizeof(file_path), "%s", path_argument);
            } else {
                // Relative path
                snprintf(file_path, sizeof(file_path), "%s/%s", current_path, path_argument);
            }

            append_mode = (strcmp(redirect_operator, ">>") == 0) ? 2 : 1;
            argc = i;  // Adjust argc to ignore file path arguments
            break;
        }
    }

    // Create the output string from the command arguments
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
        size_t arg_len = strlen(argv[i]);
        if (total_len + arg_len + 1 < sizeof(output_str)) {
            strcat(output_str, argv[i]);
            total_len += arg_len;
            if (i < argc - 1) {
                strcat(output_str, " ");
                total_len += 1;
            }
        } else {
        	cli_printf(cli,"Output string too long\r\n");
            return DONE_EXECUTING;
        }
    }

    // Append "\r\n" to the output string
    if (total_len + 2 < sizeof(output_str)) {
        strcat(output_str, "\r\n");
    } else {
    	cli_printf(cli,"Output string too long\r\n");
        return DONE_EXECUTING;
    }

    // Handle file operations
    if (append_mode > 0) {
    	lfs_file_t file;
        int flags = (append_mode == 1) ? (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) :
                    (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
        int err = lfs_file_open(&lfs, &file, file_path, flags);
        if (err < 0) {
        	cli_printf(cli,"Failed to open file\r\n");
            return DONE_EXECUTING;
        }

        lfs_ssize_t res = lfs_file_write(&lfs, &file, output_str, strlen(output_str));
        if (res < 0) {
        	cli_printf(cli,"Failed to write to file\r\n");
        }

        lfs_file_close(&lfs, &file);
    } else {
        // Print to terminal
        cli_printf(cli,"%s",output_str);
    }

    return DONE_EXECUTING;
}

// Function to print a file
Cli_state_e cat_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
    	cli_printf(cli,"Usage: cat <filename>\r\n");
        return DONE_EXECUTING;
    }
    lfs_file_t file;
    char file_path[512] = {0};    // Buffer for the full file path

    // Determine if the path is absolute or relative
    const char *path_argument = argv[1];
    if (path_argument[0] == '/') {
        // Absolute path
        snprintf(file_path, sizeof(file_path), "%s", path_argument);
    } else {
        // Relative path
        snprintf(file_path, sizeof(file_path), "%s/%s", current_path, path_argument);
    }

    // Open the file
    int err = lfs_file_open(&lfs, &file, file_path, LFS_O_RDONLY);
    if (err < 0) {
    	cli_printf(cli,"Failed to open file\r\n");
        return DONE_EXECUTING;
    }

    // Read and print the file content
    char buffer[128];
    lfs_ssize_t read_size;
    while ((read_size = lfs_file_read(&lfs, &file, buffer, sizeof(buffer))) > 0) {
        cli_print(cli, buffer, read_size);
    }

    if (read_size < 0) {
    	cli_printf(cli,"Failed to read file\r\n");
    }

    lfs_file_close(&lfs, &file);

    return DONE_EXECUTING;
}

// Function to remove a file
Cli_state_e rm_command(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
    	cli_printf(cli,"Usage: rm <filename>\r\n");
        return DONE_EXECUTING;
    }

    char file_path[512] = {0};  // Buffer for the full file path

    // Determine if the path is absolute or relative
    const char *path_argument = argv[1];
    if (path_argument[0] == '/') {
        // Absolute path
        snprintf(file_path, sizeof(file_path), "%s", path_argument);
    } else {
        // Relative path
        if (current_path[strlen(current_path) - 1] == '/') {
            snprintf(file_path, sizeof(file_path), "%s%s", current_path, path_argument);
        } else {
            snprintf(file_path, sizeof(file_path), "%s/%s", current_path, path_argument);
        }
    }

    // Attempt to remove the file
    int err = lfs_remove(&lfs, file_path);
    if (err < 0) {
    	cli_printf(cli,"Failed to delete file\r\n");
    } else {
    	cli_printf(cli,"File deleted successfully\r\n");
    }
    return DONE_EXECUTING;
}

// Function to list a directory
Cli_state_e lfs_ls(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {

	lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, current_path);
    if (err) {
        return DONE_EXECUTING;
    }

    struct lfs_info info;
    while (true) {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            return DONE_EXECUTING;
        }

        if (res == 0) {
            break;
        }

        switch (info.type) {
            case LFS_TYPE_REG: cli_printf(cli,"reg "); break;
            case LFS_TYPE_DIR: cli_printf(cli,"dir "); break;
            default:           cli_printf(cli,"?   "); break;
        }

        static const char *prefixes[] = {"", "K", "M", "G"};
        for (int i = sizeof(prefixes)/sizeof(prefixes[0])-1; i >= 0; i--) {
            if (info.size >= (1 << 10*i)-1) {
            	cli_printf(cli,"%*lu%sB ", 4-(i != 0), info.size >> 10*i, prefixes[i]);
               break;
            }
        }

        cli_printf(cli,"%s\r\n", info.name);
    }

    err = lfs_dir_close(&lfs, &dir);
    if (err) {
        return DONE_EXECUTING;
    }

    return DONE_EXECUTING;
}

// Function to remove a directory
Cli_state_e rmdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
    	cli_printf(cli,"Usage: rmdir <directory>\r\n");

        return DONE_EXECUTING;
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
    	cli_printf(cli,"Failed to remove directory.\r\n");
        return DONE_EXECUTING;
    }
    cli_printf(cli,"Directory removed.\r\n");
    return DONE_EXECUTING;
}

// Function to make a directory
Cli_state_e mkdir(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
    	cli_printf(cli,"Usage: mkdir <directory>\r\n");
        return DONE_EXECUTING;
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
    	cli_printf(cli,"Failed to make directory.\r\n");
        return DONE_EXECUTING;
    }
    cli_printf(cli,"Directory created.\r\n");

    return DONE_EXECUTING;
}

Cli_state_e change_dir(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc < 2) {
    	cli_printf(cli,"Usage: cd <path>\r\n");
        return DONE_EXECUTING;
    }

    lfs_dir_t dir;
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
    	cli_printf(cli,"Error opening directory '%s': %d\r\n", new_path, err);
        //printf("Error opening directory '%s': %d\r\n", new_path, err);
        return DONE_EXECUTING;
    }

    // Close the directory (just checking its existence)
    lfs_dir_close(&lfs, &dir);

    // Update the current path
    strncpy(current_path, new_path, sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';

    cli_printf(cli,"Changed directory to '%s'\r\n",current_path);
    //printf("Changed directory to '%s'\r\n", current_path);
    return DONE_EXECUTING;
}

// Function to create a new file using LittleFS
Cli_state_e create_new_file(Cli_HandlerTypeDef_t *cli, int argc, char **argv)  {
    if (argc != 2) {
    	cli_printf(cli,"Usage: touch <filename>\r\n");
        return DONE_EXECUTING;
    }

    char full_path[256];
    const char *filename = argv[1];

    // Check if the provided path is absolute or relative
    if (filename[0] == '/') {
        // Absolute path
        strncpy(full_path, filename, 256);
    } else {
        // Relative path
        if (current_path[strlen(current_path) - 1] == '/') {
            snprintf(full_path, 256, "%s%s", current_path, filename);
        } else {
            snprintf(full_path, 256, "%s/%s", current_path, filename);
        }
    }

    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, full_path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL);
    if (err) {
        cli_printf(cli,"Error creating file.\r\n");
        return DONE_EXECUTING;
    }

    lfs_file_close(&lfs, &file);
    cli_printf(cli,"File created successfully.\r\n");
    return DONE_EXECUTING;
}

// Function to move a file using LittleFS
Cli_state_e move_file(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc != 3) {
    	cli_printf(cli,"Usage: mv <source> <destination>\r\n");
        return DONE_EXECUTING;
    }

    char source_path[256];
    char destination_path[256];
    const char *source = argv[1];
    const char *destination = argv[2];

    // Handle source path
    if (source[0] == '/') {
        // Absolute path
        //strncpy(source_path, source, 256);
        snprintf(source_path, sizeof(source_path), "%s", source);
    } else {
        // Relative path
        if (current_path[strlen(current_path) - 1] == '/') {
            snprintf(source_path, 256, "%s%s", current_path, source);
        } else {
            snprintf(source_path, 256, "%s/%s", current_path, source);
        }
    }

    // Handle destination path
    if (destination[0] == '/') {
        // Absolute path
        strncpy(destination_path, destination, 256);
    } else {
        // Relative path
        if (current_path[strlen(current_path) - 1] == '/') {
            snprintf(destination_path, 256, "%s%s", current_path, destination);
        } else {
            snprintf(destination_path, 256, "%s/%s", current_path, destination);
        }
    }

    // Perform the file move
    int err = lfs_rename(&lfs, source_path, destination_path);
    if (err) {
    	cli_printf(cli,"Error moving file.\r\n");
        return DONE_EXECUTING;
    }

    cli_printf(cli,"File moved successfully.\r\n");
    return DONE_EXECUTING;
}

// Function to copy a file using LittleFS
Cli_state_e copy_file(Cli_HandlerTypeDef_t *cli, int argc, char **argv) {
    if (argc != 3) {
    	cli_printf(cli,"Usage: cp <source> <destination>\r\n");
        return DONE_EXECUTING;
    }

    char source_path[256];
    char destination_path[256];
    const char *source = argv[1];
    const char *destination = argv[2];

    // Handle source path
    if (source[0] == '/') {
        // Absolute path
        strncpy(source_path, source, 256);
    } else {
        // Relative path
        if (current_path[strlen(current_path) - 1] == '/') {
            snprintf(source_path, 256, "%s%s", current_path, source);
        } else {
            snprintf(source_path, 256, "%s/%s", current_path, source);
        }
    }

    // Handle destination path
    if (destination[0] == '/') {
        // Absolute path
        strncpy(destination_path, destination, 256);
    } else {
        // Relative path
        if (current_path[strlen(current_path) - 1] == '/') {
            snprintf(destination_path, 256, "%s%s", current_path, destination);
        } else {
            snprintf(destination_path, 256, "%s/%s", current_path, destination);
        }
    }

    // Open source file for reading
    lfs_file_t src_file;
    int err = lfs_file_open(&lfs, &src_file, source_path, LFS_O_RDONLY);
    if (err) {
    	cli_printf(cli,"Error opening source file.\r\n");
        return DONE_EXECUTING;
    }

    // Open destination file for writing
    lfs_file_t dest_file;
    err = lfs_file_open(&lfs, &dest_file, destination_path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err) {
    	cli_printf(cli,"Error creating destination file.\r\n");
        lfs_file_close(&lfs, &src_file);
        return DONE_EXECUTING;
    }

    // Buffer for copying data
    char buffer[128];
    int bytes_read, bytes_written;

    // Copy data from source to destination
    while ((bytes_read = lfs_file_read(&lfs, &src_file, buffer, sizeof(buffer))) > 0) {
        bytes_written = lfs_file_write(&lfs, &dest_file, buffer, bytes_read);
        if (bytes_written < 0) {
        	cli_printf(cli,"Error writing to destination file.\r\n");
            lfs_file_close(&lfs, &src_file);
            lfs_file_close(&lfs, &dest_file);
            return DONE_EXECUTING;
        }
    }

    // Close files
    lfs_file_close(&lfs, &src_file);
    lfs_file_close(&lfs, &dest_file);

    cli_printf(cli,"File copied successfully.\r\n");
    return DONE_EXECUTING;
}


// Helper function to normalize the path
static void normalize_path(char *path) {
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
