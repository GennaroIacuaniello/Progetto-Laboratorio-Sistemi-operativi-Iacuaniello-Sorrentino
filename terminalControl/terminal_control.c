#include <stdio.h>
#include "terminal_control.h"
#include "color.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void TC_clear_screen(){ 
	write(STDOUT_FILENO, "\x1b[H\x1b[2J\x1b[3J", 11);
}

void TC_enter_screen() 
{
	puts("\x1B[?1049h\x1B[H");
}

void TC_exit_screen() 
{
	puts("\x1B[?1049l");
}

void TC_set_echo_off(){

	system("stty -echo"); //removes echo
	printf("\e[?25l"); //removes visible cursor
}
void TC_set_echo_on(){

	system("stty echo"); //restores echo
	printf("\e[?25h"); //restores visible cursor
}

void TC_set_cursor(int x, int y){ 
	printf("\x1B[%d;%dH", y, x);
}

void TC_set_raw_input_mode(){ 
	system("/bin/stty raw");
}
void TC_set_cooked_input_mode(){ 
	system("/bin/stty cooked");
}

void TC_get_cols_rows(int *cols, int *rows){

	char bash_cmd[256] = "tput cols lines";
	FILE *pipe;
	int len; 

	pipe = popen(bash_cmd, "r");

	if (NULL == pipe) {
		perror("pipe");
		exit(1);
	} 
	
	fscanf(pipe, "%d", cols);
	fscanf(pipe, "%d", rows);

	pclose(pipe);
}

void TC_color_print(char *msg, color_t fg_color, color_t bg_color){

	printf("\x1B[38;2;%d;%d;%dm\x1B[48;2;%d;%d;%dm%s\x1b[0m",
		fg_color.r, fg_color.g, fg_color.b, 
		bg_color.r, bg_color.g, bg_color.b, msg);
}


int TC_read_movement_input(){
    
    char c;
    
    if ((c = getchar()) == '\x1B') { //Escape sequence
        getchar(); //Skip the [
        return getchar(); //The real arrow value
    }

    switch (c)
    {
        case 'w':
            return MOVE_UP;
        case 'a':
            return MOVE_LEFT;
        case 's':
            return MOVE_DOWN;
        case 'd':
            return MOVE_RIGHT;
    }

    return c;
}