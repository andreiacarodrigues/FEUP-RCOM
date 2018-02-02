#ifndef _UTIL_INTERFACE_H_
#define _UTIL_INTERFACE_H_

//SGR (Select Graphic Rendition) parameters
#define RESET "\x1B[0m"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"

#define RED_BG   "\x1B[41m"
#define GRN_BG   "\x1B[42m"
#define YEL_BG   "\x1B[43m"
#define BLU_BG   "\x1B[44m"
#define MAG_BG   "\x1B[45m"
#define CYN_BG   "\x1B[46m"
#define WHT_BG   "\x1B[47m"


void cleanBuffer();

void clearScreen();


#endif //_UTIL_INTERFACE_H_
