/*
 * cli.h
 *
 *  Created on: Jul 5, 2024
 *      Author: licin
 */

#ifndef CLI_H_
#define CLI_H_

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>



// Maximum number of commands and arguments
#define MAX_COMMANDS 100
#define MAX_ARGS 100
#define MAX_ARG_LEN 128
#define BUFFER_SIZE 1024

typedef enum{
	WAITING=0,
	EXECUTE_COMMAND,
	EXECUTING,
	DONE_EXECUTING
}Cli_state_e;

// Function pointers for reading a character and printing a string
typedef bool (*read_char)(char *data);
typedef void (*print_string)(char *data, uint16_t size);



typedef struct _Cli_HandlerTypeDef{
	char line[BUFFER_SIZE];
	char print_Buffer[BUFFER_SIZE];
	int pos;
	read_char read_char;
	print_string print_string;
	uint8_t commandRunIndex;
	bool asUserInput;
	bool asEscape;
	bool asCtrlC;
	Cli_state_e state;
}Cli_HandlerTypeDef_t;


// Command structure
typedef struct {
    char name[16];
    Cli_state_e (*command)(Cli_HandlerTypeDef_t *cli, int argc, char **argv);
} Command;





// Function prototypes
void register_command(const char *name, Cli_state_e (*command)(Cli_HandlerTypeDef_t *cli, int argc, char **argv));
void execute_command(Cli_HandlerTypeDef_t *self, const char *line);
void process_input(Cli_HandlerTypeDef_t *self);
void cli_init(Cli_HandlerTypeDef_t *self, bool (*read_func)(char *), void (*print_func)(char *data, uint16_t size));
void cli_start(Cli_HandlerTypeDef_t *self);
void cli_run(Cli_HandlerTypeDef_t *self);
char * cli_getUserInput(Cli_HandlerTypeDef_t *self);
bool cli_escape(Cli_HandlerTypeDef_t *self);
bool cli_ctrlC(Cli_HandlerTypeDef_t *self);
int cli_printf(Cli_HandlerTypeDef_t *self,const char * format, ...);
void cli_hideCursor(Cli_HandlerTypeDef_t *self);
void cli_showCursor(Cli_HandlerTypeDef_t *self);

#endif /* CLI_H_ */
