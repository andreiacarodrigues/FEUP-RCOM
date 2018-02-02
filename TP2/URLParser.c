#include <stdio.h> //printf
#include <string.h> //strlen
#include <strings.h> //strncasecmp
#include <stddef.h> //ptrdiff_t
#include <errno.h> //errno
#include <stdlib.h> //strtol

#include "URLParser.h"
#include "utilInterface.h"

#define URL_FTP_HEADER "ftp://"
#define URL_FTP_HEADER_SIZE 6

#define URL_DFL_USERNAME "anonymous"
#define URL_DFL_PASSWORD "mail@domain"
#define URL_DFL_PORT 21

int parseURL(char *urlStr, url_data *urlData)
{
	//Step 1: Check that the URL has the correct header
	//This comparison is case insensitive, according to RFC 1738
	if(strncasecmp(urlStr,URL_FTP_HEADER,URL_FTP_HEADER_SIZE) != 0)
	{
		printf(RED "URL does not start with 'ftp://'.\n" RESET);
		return 1;
	}
	
	char *curStr = &urlStr[URL_FTP_HEADER_SIZE]; //string with the rest of urlStr to parse
	
	//Step 2: Parse the username and password
	//This comparison is case insensitive, according to RFC 1738
	char *atPtr = strchr(curStr,'@');
	if(atPtr != NULL) //username and password have been specified
	{
		char *colonPtr = strchr(curStr,':');
		if(colonPtr == NULL)
		{
			printf(RED "URL username and password are not separated by a ':'.\n" RESET);
			return 1;
		}
		if(colonPtr > atPtr)
		{
			printf(RED "URL username and password are not separated by a ':'.\n" RESET);
			return 1;
		}
		
		//Retrieve the username
		ptrdiff_t usernameSize = colonPtr - curStr;
		memcpy(urlData->username,curStr,usernameSize);
		urlData->username[usernameSize] = '\0';
		
		//Retrieve the password
		ptrdiff_t passwordSize = atPtr - (colonPtr + 1);
		memcpy(urlData->password,colonPtr + 1,passwordSize);
		urlData->password[passwordSize] = '\0';

		curStr = atPtr + 1;
 	}
	else
	{
		strcpy(urlData->username,URL_DFL_USERNAME);
		strcpy(urlData->password,URL_DFL_PASSWORD);
	}
		
	//Step 3: Parse the host
	char *portColonPtr = strchr(curStr,':');
	char *pathDashPtr = strchr(curStr,'/');
	
	if(pathDashPtr == NULL)
	{
		printf(RED "URL host and path are not separated by a '/'.\n" RESET);
		return 1; 
	}
	else if(portColonPtr != NULL) //port has been specified
	{
		ptrdiff_t hostSize = portColonPtr - curStr;
		memcpy(urlData->host,curStr,hostSize);
		urlData->host[hostSize] = '\0';
		
		//Parse the port
		char portStr[MAX_SIZE];
		ptrdiff_t portStrSize = pathDashPtr - (portColonPtr + 1);
		memcpy(portStr,portColonPtr + 1, portStrSize);
		portStr[portStrSize] = '\0';
		
		errno = 0;
		/* "the calling  program should set errno to 0 before the call, and then deter‐
		mine if an error occurred by checking whether errno has a nonzero value
		after the call." (man strtol) */
    	urlData->port = strtol(portStr, NULL, 10);
    	if (errno != 0)
    	{
			printf(RED "Invalid value for the port.\n" RESET);
        	return 1;
    	}
	}
	else
	{
		ptrdiff_t hostSize = pathDashPtr - curStr;
		memcpy(urlData->host,curStr,hostSize);
		urlData->host[hostSize] = '\0';
		
		urlData->port = URL_DFL_PORT;
	}
		
	curStr = pathDashPtr + 1;
	
	//Step 4: Parse the path
	strcpy(urlData->path,curStr);
	
	return 0;
}
