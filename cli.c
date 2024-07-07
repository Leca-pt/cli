/*
 * cli.c
 *
 *  Created on: Jul 5, 2024
 *      Author: licin
 */


#include "cli.h"


// Command array and count
static Command commands[MAX_COMMANDS];
static int command_count = 0;


void register_command(const char *name, Cli_state_e (*command)(Cli_HandlerTypeDef_t *cli, int argc, char **argv)) {
    if (command_count < MAX_COMMANDS) {
        strncpy(commands[command_count].name, name, sizeof(commands[command_count].name) - 1);
        commands[command_count].command = command;
        command_count++;
    }
}

void execute_command(Cli_HandlerTypeDef_t *self, const char *line) {
    char args[MAX_ARGS][MAX_ARG_LEN];
    char *argv[MAX_ARGS];
    int argc = 0;
    bool in_quotes = false;
    const char *p = line;
    int arg_pos = 0;

    // Parse input line into arguments
    while (*p && argc < MAX_ARGS) {
        if (*p == ' ' && !in_quotes) {
            if (arg_pos > 0) {
                args[argc][arg_pos] = '\0';
                argv[argc] = args[argc];
                argc++;
                arg_pos = 0;
            }
        } else if (*p == '"') {
            in_quotes = !in_quotes;
        } else {
            if (arg_pos < MAX_ARG_LEN - 1) {
                args[argc][arg_pos++] = *p;
            }
        }
        p++;
    }
    if (arg_pos > 0) {
        args[argc][arg_pos] = '\0';
        argv[argc] = args[argc];
        argc++;
    }

    // Find and execute command
    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, argv[0]) == 0) {
            self->state = commands[i].command(self,argc, argv);
            self->commandRunIndex = i;
            return;
        }
    }
    self->print_string("Command not found\r\n", strlen("Command not found\r\n"));
    cli_start(self);  // Print the prompt if the command was not found
}

void cli_run(Cli_HandlerTypeDef_t *self){
	switch (self->state) {
			case WAITING:
				process_input(self);
				break;
			case EXECUTING:
				self->state = commands[self->commandRunIndex].command(self,0,NULL);
				break;
			case DONE_EXECUTING:
				self->state=WAITING;
				cli_start(self);  // Print the prompt after executing a command
				break;
			default:
				break;
		}
}

void process_input(Cli_HandlerTypeDef_t *self) {


    char c;
    if (!self->read_char(&c)) {
        return; // No input available
    }

    if (c == '\n' || c == '\r') {
    	self->line[self->pos] = '\0';
        self->print_string("\r\n",2);
        if(self->pos>0){
        	execute_command(self,self->line);
        	memset(self->line,'\0',BUFFER_SIZE);
        }else{
        	cli_start(self);
        }

        self->pos = 0;
    } else if (c == '\b' || c == 127) { // back space
        if (self->pos > 0) {
        	self->pos--;
            self->print_string("\b \b",strlen("\b \b"));
        }
    } else if (c >= 32 && c <= 126) { // Printable characters
        if (self->pos < BUFFER_SIZE - 1) {
        	self->line[self->pos++] = c;
            self->print_string(&c,1);
        }
    }
}

void cli_init(Cli_HandlerTypeDef_t *self, bool (*read_func)(char *), void (*print_func)(char *data, uint16_t size)) {
	self->read_char = read_func;
    self->print_string = print_func;
    self->pos=0;
    self->commandRunIndex=0;
    self->state=WAITING;
}

void cli_start(Cli_HandlerTypeDef_t *self) {
    self->print_string("> ",2);
}
