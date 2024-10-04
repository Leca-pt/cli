main {
  cli_init(&cli,read_char_stdin, print_string_stdout);

  // Register commands
	cli_register_command("ls", lfs_ls);
	cli_register_command("rmdir", rmdir);
	cli_register_command("mkdir", mkdir);
	cli_register_command("cd", change_dir);
	cli_register_command("cat", cat_command);
	cli_register_command("echo", echo_command);
	cli_register_command("rm", rm_command);
	cli_register_command("touch", create_new_file);
	cli_register_command("mv", move_file);
	cli_register_command("cp", copy_file);
	cli_register_command("edit", editor_run);
	cli_register_command("pwd", pwd_command);

 while(1){
   cli_run(&cli);
 }
}

bool read_char_stdin(char *data){
    if(fifo_available(&fifo)){
    	fifo_read(&fifo, (uint8_t *)data, 1);
    	return true;
    }
    return false;
}

void print_string_stdout(char *data, uint16_t size) {
      HAL_UART_Transmit(&huart3, (uint8_t *)data, size, HAL_MAX_DELAY);
}
