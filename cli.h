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

typedef enum{
	CLI_Reset		=0,
	CLI_Underline	=4,
	CLI_Black		=30,
	CLI_Red			=31,
	CLI_Green		=32,
	CLI_Yellow		=33,
	CLI_Blue		=34,
	CLI_Magenta		=35,
	CLI_Cyan		=36,
	CLI_White		=37,
	CLI_Default_col	=39,
	CLI_Black_bg	=40,
	CLI_Red_bg		=41,
	CLI_Green_bg	=42,
	CLI_Yellow_bg	=43,
	CLI_Blue_bg		=44,
	CLI_Magenta_bg	=45,
	CLI_Cyan_bg		=46,
	CLI_White_bg	=47,
	CLI_Default_bg	=49
}Cli_style_e;




// Function pointers for reading a character and printing a string
typedef bool (*read_char)(char *data);
typedef void (*print_string)(char *data, uint16_t size);

//CLI control structure
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
void cli_register_command(const char *name, Cli_state_e (*command)(Cli_HandlerTypeDef_t *cli, int argc, char **argv));
void cli_init(Cli_HandlerTypeDef_t *self, bool (*read_func)(char *), void (*print_func)(char *data, uint16_t size));
void cli_start(Cli_HandlerTypeDef_t *self);
void cli_run(Cli_HandlerTypeDef_t *self);
char * cli_getUserInput(Cli_HandlerTypeDef_t *self);
bool cli_escape(Cli_HandlerTypeDef_t *self);
bool cli_ctrlC(Cli_HandlerTypeDef_t *self);
int cli_printf(Cli_HandlerTypeDef_t *self,const char * format, ...);
void cli_print(Cli_HandlerTypeDef_t *self,char *data, uint16_t size);
void cli_hideCursor(Cli_HandlerTypeDef_t *self);
void cli_showCursor(Cli_HandlerTypeDef_t *self);
void cli_setStyle(Cli_HandlerTypeDef_t *self, Cli_style_e code);

#endif /* CLI_H_ */
