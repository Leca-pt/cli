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
static bool cliDEBUG = true;


static void execute_command(Cli_HandlerTypeDef_t *self, const char *line) {
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
    cli_printf(self,"Command not found\r\n");
    self->state=DONE_EXECUTING;
}

static void process_input(Cli_HandlerTypeDef_t *self) {

    char c;
    if (!self->read_char(&c)) {
        return; // No input available
    }

    if (c == '\n' || c == '\r') {
    	self->line[self->pos] = '\0';
    	cli_printf(self,"\r\n");
        if(self->pos>0){

        	switch (self->state) {
				case WAITING:
					self->state=EXECUTE_COMMAND;
					break;
				case EXECUTING:
				case LOGIN_PROMPT:
				case PSW_PROMPT:
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
        	cli_printf(self,"\b \b");
        }
    } else if (c >= 32 && c <= 126) { // Printable characters
        if (self->pos < BUFFER_SIZE - 1) {
        	self->line[self->pos++] = c;
        	if(self->state != PSW_PROMPT){
        		self->print_string((uint8_t *)&c,1);
        	}

        }
    } else if(c == 27){	//escape key
    	self->asEscape=true;
    } else if(c == 03){	//control-C
        self->asCtrlC=true;
    }
}

Cli_state_e exit_cli(Cli_HandlerTypeDef_t *self,int argc, char **argv){
	cli_printf(self, "Bye!!!\r\n");
	return LOCKED;
}


void cli_register_command(const char *name, Cli_state_e (*command)(Cli_HandlerTypeDef_t *cli, int argc, char **argv)) {
    if (command_count < MAX_COMMANDS) {
        strncpy(commands[command_count].name, name, sizeof(commands[command_count].name) - 1);
        commands[command_count].command = command;
        command_count++;
    }
}

void cli_run(Cli_HandlerTypeDef_t *self){
	switch (self->state) {
			case LOCKED:
				cli_printf(self, "login as:");
				self->state = LOGIN_PROMPT;
				break;
			case LOGIN_PROMPT:
				char *input = cli_getUserInput(self);
				if(input!=NULL){
					strcpy(self->user, input);
					cli_printf(self, "password:");
					self->asUserInput=false;
					self->state = PSW_PROMPT;
				}
				break;
			case PSW_PROMPT:
				char *psw = cli_getUserInput(self);
				if(psw!=NULL){
					strcpy(self->psw, psw);
					if(strcmp(self->psw, "pass")==0 && strcmp(self->user, "admin")==0){
						cli_start(self);
						self->state = WAITING;
					}else{
						cli_printf(self,"Wrong login! try again.\r\n" );
						self->state = LOCKED;
					}
					self->asUserInput=false;
				}
				break;
			case WAITING:
				process_input(self);
				break;
			case EXECUTE_COMMAND:
	        	execute_command(self,self->line);
	        	memset(self->line,'\0',BUFFER_SIZE);
				break;
			case EXECUTING:
				if(self->processInputWhileRunning){
					process_input(self);
				}
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

void cli_init(Cli_HandlerTypeDef_t *self, bool (*read_func)(char *), void (*print_func)(uint8_t *data, uint16_t size), bool Locked) {
	self->read_char = read_func;
    self->print_string = print_func;
    self->pos=0;
    self->commandRunIndex=0;
    self->processInputWhileRunning = false;
    if(Locked){
    	self->state=LOCKED;
    	cli_register_command("exit", exit_cli);
    }

    strcpy(self->current_path , "/");

}

void cli_start(Cli_HandlerTypeDef_t *self) {
	cli_printf(self, "%s>", self->current_path);
}

char * cli_getUserInput(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;
	process_input(self);
	if(self->asUserInput){
		return &self->line[0];
	}

	return NULL;
}

bool cli_escape(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;
	process_input(self);
	if(!self->asEscape){
		return false;
	}
	self->asEscape=false;
	return true;
}

bool cli_ctrlC(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;
	process_input(self);
	if(!self->asCtrlC){
		return false;
	}
	self->asCtrlC=false;
	return true;
}

int cli_write(Cli_HandlerTypeDef_t *self, uint8_t *data, size_t size){
	if(self==NULL)return -1;
	self->print_string(data, size);

	return 0;
}

int cli_writeByte(Cli_HandlerTypeDef_t *self, uint8_t byte){
	if(self==NULL)return -1;
	self->print_string(&byte, 1);

	return 0;
}

int cli_printf(Cli_HandlerTypeDef_t *self,const char * format, ...){
	va_list args;
	va_start(args, format);

	// Use snprintf for limited formatting
	int result = vsnprintf(self->print_Buffer, BUFFER_SIZE-1, format, args);
	self->print_string((uint8_t *)self->print_Buffer, strlen(self->print_Buffer));

	va_end(args);
	return result; // Return the number of characters written (excluding null terminator)
}

void cli_hideCursor(Cli_HandlerTypeDef_t *self){
	cli_printf(self,"\033[?25l");
}

void cli_showCursor(Cli_HandlerTypeDef_t *self){
	cli_printf(self,"\033[?25h");

}

void cli_setStyle(Cli_HandlerTypeDef_t *self, Cli_style_e code){
	cli_printf(self, "\033[%dm",code);
}

int cli_logMessage(Cli_HandlerTypeDef_t *self, Cli_logLevel_e level,const char * format, ...){
	va_list args;
	va_start(args, format);
	int result = 0;

	switch (level) {
		case LOG_LEVEL_DEBUG:
			if(cliDEBUG){
				cli_setStyle(self, CLI_Magenta);
				cli_printf(self,DEBUG_HEADER);
				cli_setStyle(self, CLI_Reset);
				result = vsnprintf(self->print_Buffer, BUFFER_SIZE-1, format, args);
				self->print_string((uint8_t *)self->print_Buffer, strlen(self->print_Buffer));
				return result;
			}
			return 0;
			break;
		case LOG_LEVEL_ERROR:
			cli_setStyle(self, CLI_Red);
			cli_printf(self,ERROR_HEADER);
			break;
		case LOG_LEVEL_INFO:
			cli_setStyle(self, CLI_Cyan);
			cli_printf(self,INFO_HEADER);
			break;
		case LOG_LEVEL_WARNING:
			cli_setStyle(self, CLI_Yellow);
			cli_printf(self,WARNING_HEADER);
			break;
		case LOG_LEVEL_FAIL:
			cli_setStyle(self, CLI_Red);
			cli_printf(self,FAIL_HEADER);
			break;
		case LOG_LEVEL_OK:
			cli_setStyle(self, CLI_Green);
			cli_printf(self,OK_HEADER);
			break;
		default:
			break;
	}

	cli_setStyle(self, CLI_Reset);
	result = vsnprintf(self->print_Buffer, BUFFER_SIZE-1, format, args);
	self->print_string((uint8_t *)self->print_Buffer, strlen(self->print_Buffer));

	va_end(args);
	return result;

}

int cli_styled_printf(Cli_HandlerTypeDef_t *self, Cli_style_e style, const char * format, ...){
	if(self==NULL)return 0;

	cli_setStyle(self, style);
	va_list args;
	va_start(args, format);

	// Use snprintf for limited formatting
	int result = vsnprintf(self->print_Buffer, BUFFER_SIZE-1, format, args);
	self->print_string((uint8_t *)self->print_Buffer, strlen(self->print_Buffer));

	va_end(args);
	cli_setStyle(self, CLI_Reset);
	return result; // Return the number of characters written (excluding null terminator)
}

void print_progress_bar(Cli_HandlerTypeDef_t *self,int min_val, int max_val, int current_val, int bar_width, const char *title, char trailing_char, char after_char, char current_val_char) {

    // Ensure current_val is within range
    if (current_val < min_val) current_val = min_val;
    if (current_val > max_val) current_val = max_val;

    // Calculate progress as a percentage
    int range = max_val - min_val;
    int progress = current_val - min_val;
    float percentage = (float)progress / range;

    // Determine the number of completed segments
    int completed = (int)(percentage * bar_width);
    cli_hideCursor(self);
    if(title!=NULL){
    	cli_printf(self, "%s ", title);
    }

    // Print the progress bar
    cli_printf(self,"\033[0m["); // Reset any formatting and print the opening bracket
    for (int i = 0; i < bar_width; i++) {
        if (i < completed ) {
        	cli_printf(self,"%c", trailing_char);
        } else if (i == completed && current_val < max_val) {
        	cli_printf(self,"%c", current_val_char); // Special character for the current value
        } else {
        	cli_printf(self,"%c", after_char);
        }
    }
    cli_printf(self,"] %3.0f%%\r", percentage * 100.0); // Print the percentage
    cli_showCursor(self);
}

void cli_clearScreen(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return;

	cli_printf(self, "\x1b[2J\x1b[H");

	return;
}

void cli_setDebug(bool val){
	cliDEBUG = val;
}

char *cli_getCurrentPath(Cli_HandlerTypeDef_t *self){
	if(self==NULL)return NULL;

	return self->current_path;
}
