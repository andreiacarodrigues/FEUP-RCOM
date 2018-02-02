#ifndef URL_PARSER_H
#define URL_PARSER_H

#include "shared.h"

typedef struct {
	char username[MAX_SIZE];
	char password[MAX_SIZE];
	char host[MAX_SIZE];
	unsigned int port;
	char path[MAX_SIZE];
} url_data;

int parseURL(char *urlStr, url_data *urlData);

#endif //URL_PARSER_H
