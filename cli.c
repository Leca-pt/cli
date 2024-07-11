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



static void execute_command(Cli_HandlerTypeDef_t *self, const char *line);

static void process_input(Cli_HandlerTypeDef_t *self);

void cli_register_command(const char *name, Cli_state_e (*command)(Cli_HandlerTypeDef_t *cli, int argc, char **argv)) {
    if (command_count < MAX_COMMANDS) {
        strncpy(commands[command_count].name, name, sizeof(commands[command_count].name) - 1);
        commands[command_count].command = command;
        command_count++;
    }
}

void cli_run(Cli_HandlerTypeDef_t *self){
	switch (self->state) {
			case WAITING:
				process_input(self);
				break;
			case EXECUTE_COMMAND:
	        	execute_command(self,self->line);
	        	memset(self->line,'\0',BUFFER_SIZE);
				break;
			case EXECUTING:
				process_input(self);
				self->state = commands[self->commandRunIndex].command(self,0,NULL);
				if(self->asUserInput){
					self->asUserInput=false;
					memset(self->line,'\0',BUFFER_SIZE);
				}
				break;
			case DONE_EXECUTING:
				self->state=WAITING;
				cli_start(self);  // Print the prompt after executing a command
				break;
			default:
				break;
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

char * cli_getUserInput(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;
	if(self->asUserInput){
		return &self->line[0];
	}

	return NULL;
}

bool cli_escape(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;
	if(!self->asEscape){
		return false;
	}
	self->asEscape=false;
	return true;
}

bool cli_ctrlC(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;
	if(!self->asCtrlC){
		return false;
	}
	self->asCtrlC=false;
	return true;
}

int cli_printf(Cli_HandlerTypeDef_t *self,const char * format, ...){
	va_list args;
	va_start(args, format);

	// Use snprintf for limited formatting
	int result = vsnprintf(self->print_Buffer, BUFFER_SIZE-1, format, args);
	self->print_string(self->print_Buffer, strlen(self->print_Buffer));

	va_end(args);
	return result; // Return the number of characters written (excluding null terminator)
}

void cli_hideCursor(Cli_HandlerTypeDef_t *self){
	self->print_string("\033[?25l", strlen("\033[?25l"));
}

void cli_showCursor(Cli_HandlerTypeDef_t *self){
	self->print_string("\033[?25h", strlen("\033[?25h"));

}

void cli_setStyle(Cli_HandlerTypeDef_t *self, Cli_style_e code){
	cli_printf(self, "\033[%dm",code);
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

static void process_input(Cli_HandlerTypeDef_t *self) {

    char c;
    if (!self->read_char(&c)) {
        return; // No input available
    }

    if (c == '\n' || c == '\r') {
    	self->line[self->pos] = '\0';
        self->print_string("\r\n",2);
        if(self->pos>0){

        	switch (self->state) {
				case WAITING:
					self->state=EXECUTE_COMMAND;
					break;
				case EXECUTING:
					self->asUserInput=true;
					break;
				default:
					break;
			}

        }else{
        	if(self->state==WAITING)
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
    } else if(c == 27){	//escape key
    	self->asEscape=true;
    } else if(c == 03){	//control-C
        self->asCtrlC=true;
    }
}
