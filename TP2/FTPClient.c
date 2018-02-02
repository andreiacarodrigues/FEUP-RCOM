#include <stdlib.h> //NULL

#include <arpa/inet.h> //inet_addr

#include <sys/types.h> //socket
#include <sys/socket.h> //socket

#include <stdio.h> //errno
#include <strings.h> //bzero

#include <unistd.h> //read

#include <string.h> //strlen

#include <libgen.h> //basename

#include <fcntl.h> //open

#include <sys/times.h> //times

#include "FTPClient.h"
#include "shared.h" //MAX_SIZE
#include "utilInterface.h"

int connectSocket(int *socketFd, const char* ip, const unsigned int port)
{
	//Step 1: Open an TCP socket
	if ((*socketFd = socket(AF_INET,SOCK_STREAM,0)) < 0) 
	{
    	perror("socket");
        return 1;
    }

	//Step 2: Handle server address
	struct	sockaddr_in server_addr;
	bzero((char*)&server_addr,sizeof(server_addr)); //clear all the fields
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); //32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);	//server TCP port must be network byte ordered */
    
	//Step 3: Connect to the server
    if(connect(*socketFd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
        perror("connect");
		return 1;
	}
	
	return 0;
}

int ftpSendCmd(int socketFd, const char *cmd, const char *cmdArg)
{
	//Step 1: Define the command string
	const size_t CMD_BUF_SIZE = MAX_SIZE + 13; //max length for a command
	//command_buf_size = url_data_field_max_size + cmd_max_size{=10} + strlen(" \r\n"){=3}
	char cmdBuf[CMD_BUF_SIZE];
	
	if(sprintf(cmdBuf, "%s %s\r\n", cmd, cmdArg) < 0)
	{
		printf(RED "sprintf failed.\n" RESET);
		return 1;
	}
	
	//Step 2: Write to the socket
	if(write(socketFd, cmdBuf, strlen(cmdBuf)) == -1)
	{
		perror("write");
		return 1;
	}
	
	return 0;
}

int ftpReceiveReplies(int socketFd, char *replyBuf, size_t replyBufSize)
{
	//Step 1: Associate a stream with the socket file descriptor (so we can use fgets)
	FILE *socketFp = fdopen(socketFd, "r");
	if(socketFp == NULL)
	{
		perror("fdopen");
		return 1;
	}
	
	//Step 2: Read the connection greetings from the server
	char replyCode[4];
	replyCode[3] = '\0';
	bzero(replyBuf, replyBufSize);
	if(fgets(replyBuf, replyBufSize, socketFp) == NULL)
	{
		printf(RED "fgets failed or EOF was found.\n" RESET);
		return 1;
	}
	printf(YEL "%s" RESET, replyBuf);
	
	//Store the reply code
	strncpy(replyCode,replyBuf,strlen(replyCode));
	
	//For multi-line replies
	if(replyBuf[3] == '-')
	{
		do
		{
			bzero(replyBuf, replyBufSize);
			if(fgets(replyBuf, replyBufSize, socketFp) == NULL)
			{
				printf(RED "fgets failed or EOF was found.\n" RESET);
				return 1;
			}
			printf(YEL "%s" RESET, replyBuf);
		} while(strncmp(replyBuf,replyCode,strlen(replyCode)) != 0);
	}
	
	return 0;
}

void printProgressBar(double ratio)
{
	double remaining = ratio;
	while(remaining > 0)
	{
		if(ratio < 0.5)
		{
			printf(RED_BG " " RESET);
		}
		else if(ratio < 1)
		{
			printf(YEL_BG " " RESET);
		}
		else
		{
			printf(GRN_BG " " RESET);
		}
		remaining -= 0.05;
	}
}

int ftpConnect(ftp_data *ftpData, const char* ip, const unsigned int port)
{
	//Step 1: Connect the control socket
	if(connectSocket(&(ftpData->controlSocketFd), ip, port) != 0)
	{
		printf(RED "connectSocket failed.\n" RESET);
		return 1;
	}
	
	//Step 2: Receive a reply from the server
	//RFC 959 - Section 5.4: "Under normal circumstances, a server will send a 220 reply,
	//"awaiting input", when the connection is completed."
	const size_t REPLY_BUF_SIZE = 300;
	char replyBuf[REPLY_BUF_SIZE];
	if(ftpReceiveReplies(ftpData->controlSocketFd, replyBuf, REPLY_BUF_SIZE) != 0)
	{
		printf(RED "ftpReceiveReplies failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	const char SERVICE_READY[4] = "220";
	if(strncmp(replyBuf,SERVICE_READY,strlen(SERVICE_READY)) != 0)
	{
		printf(RED "ftpReceiveReplies returned with a code different from the one expected.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	return 0;
}

int ftpLogin(ftp_data *ftpData, const char *username, const char *password)
{
	//Step 1: Send the username and wait for reply
	if(ftpSendCmd(ftpData->controlSocketFd, "USER", username) != 0)
	{
		printf(RED "ftpSendCmd failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	const size_t REPLY_BUF_SIZE = 300;
	char replyBuf[REPLY_BUF_SIZE];
	if(ftpReceiveReplies(ftpData->controlSocketFd, replyBuf, REPLY_BUF_SIZE) != 0)
	{
		printf(RED "ftpReceiveReplies failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	const char PROCEED[4] = "230";
	const char NEED_PASSWORD[4] = "331";
	if((strncmp(replyBuf,PROCEED,strlen(PROCEED)) != 0) &&
	   (strncmp(replyBuf,NEED_PASSWORD,strlen(NEED_PASSWORD)) != 0))
	{
		printf(RED "ftpReceiveReplies returned with a code different from the one expected.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	//Step 2: Send the password and wait for reply
	if(ftpSendCmd(ftpData->controlSocketFd, "PASS", password) != 0)
	{
		printf(RED "ftpSendCmd failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	if(ftpReceiveReplies(ftpData->controlSocketFd, replyBuf, REPLY_BUF_SIZE) != 0)
	{
		printf(RED "ftpReceiveReplies failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	const char CMD_SUPERFLUOS[4] = "202";
	if((strncmp(replyBuf,PROCEED,strlen(PROCEED)) != 0) &&
	   (strncmp(replyBuf,CMD_SUPERFLUOS,strlen(CMD_SUPERFLUOS)) != 0))
	{
		printf(RED "ftpReceiveReplies returned with a code different from the one expected.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	return 0;
}

int ftpSetPasv(ftp_data *ftpData)
{
	//Step 1: Send the PASV command and wait for reply
	if(ftpSendCmd(ftpData->controlSocketFd, "PASV", "") != 0)
	{
		printf(RED "ftpSendCmd failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	const size_t REPLY_BUF_SIZE = 300;
	char replyBuf[REPLY_BUF_SIZE];
	if(ftpReceiveReplies(ftpData->controlSocketFd, replyBuf, REPLY_BUF_SIZE) != 0)
	{
		printf(RED "ftpReceiveReplies failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	//Step 2: Process the reply to find the data socket's ip address and port
	unsigned int ip1, ip2, ip3, ip4;
	unsigned int port1, port2;
	if(sscanf(replyBuf, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
			  &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6)
	{
		printf(RED "sscanf failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	//Find the ip address
	const size_t IP_SIZE = 16;
	char ip[IP_SIZE];
	if(sprintf(ip, "%u.%u.%u.%u", ip1, ip2, ip3, ip4) < 0)
	{
		printf(RED "sprintf failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	//Compute the port
	unsigned int port = 256*port1 + port2;
	
	//Step 3: Connect the data socket
	if(connectSocket(&(ftpData->dataSocketFd), ip, port) != 0)
	{
		printf(RED "connectSocket failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	return 0;
}

int ftpDownload(ftp_data *ftpData, const char *path)
{
	//Step 1: Send the RETR command and wait for reply
	if(ftpSendCmd(ftpData->controlSocketFd, "RETR", path) != 0)
	{
		printf(RED "ftpSendCmd failed.\n" RESET);
		close(ftpData->controlSocketFd);
		close(ftpData->dataSocketFd);
		return 1;
	}
	
	const size_t REPLY_BUF_SIZE = 300;
	char replyBuf[REPLY_BUF_SIZE];
	if(ftpReceiveReplies(ftpData->controlSocketFd, replyBuf, REPLY_BUF_SIZE) != 0)
	{
		printf(RED "ftpReceiveReplies failed.\n" RESET);
		close(ftpData->controlSocketFd);
		close(ftpData->dataSocketFd);
		return 1;
	}
	
	char filename2[MAX_SIZE];
	ssize_t fileSize;
	if(sscanf(replyBuf, "150 Opening BINARY mode data connection for %s (%zu bytes).",
			  filename2, &fileSize) != 2)
	{
		printf(RED "sscanf failed.\n" RESET);
		close(ftpData->controlSocketFd);
		close(ftpData->dataSocketFd);
		return 1;
	}
	
	//Step 2: Open the output file
	char pathCopy[MAX_SIZE]; 
	strcpy(pathCopy,path); //because basename() may modify the contents of path
	char *filename = basename(pathCopy);
	
	const mode_t OUTPUT_FILE_MODE = 0666;
	int outputFd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, OUTPUT_FILE_MODE);
	if(outputFd == -1)
	{
		perror("open");
		close(ftpData->controlSocketFd);
		close(ftpData->dataSocketFd);
		return 1;
	}
	
	//Step 3: Read from the data socket
	const size_t DATA_BUF_SIZE = 1024;
	char dataBuf[DATA_BUF_SIZE];
	ssize_t totalRead = 0;
	ssize_t nRead;
	clock_t start = times(NULL);
	const long int TICKS_PER_SEC = sysconf(_SC_CLK_TCK);
	while((nRead = read(ftpData->dataSocketFd, dataBuf, DATA_BUF_SIZE)) > 0)
	{
		if(write(outputFd, dataBuf, nRead) == -1)
		{
			perror("write");
			close(outputFd);
			close(ftpData->controlSocketFd);
			close(ftpData->dataSocketFd);
			return 1;
		}
		
		totalRead += nRead;
		double ratio = (double)totalRead/(double)fileSize;
		
		printf("\r");
		printf("Progress: %zu/%zu Bytes (%f%%) ",totalRead,fileSize,ratio*100.0);
		printProgressBar(ratio);
	}
	if(nRead == -1)
	{
		perror("read");
		close(outputFd);
		close(ftpData->controlSocketFd);
		close(ftpData->dataSocketFd);
		return 1;
	}
	clock_t end = times(NULL);
	double secsElapsed = (double) (end - start) / TICKS_PER_SEC;
	
	printf("\nFile download complete! Time elapsed: %f seconds. \n", secsElapsed);
	
	//Step 4: Close the output file descriptor
	if(close(outputFd) == -1)
	{
		perror("close");
		close(ftpData->controlSocketFd);
		close(ftpData->dataSocketFd);
		return 1;
	}
	
	//Step 5: Close the data socket file descriptor
	if(close(ftpData->dataSocketFd) == -1)
	{
		perror("close");
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	return 0;
}

int ftpLogout(ftp_data *ftpData)
{
	//Step 1: Send the QUIT command and wait for reply (goodbye!)
	if(ftpSendCmd(ftpData->controlSocketFd, "QUIT", "") != 0)
	{
		printf(RED "ftpSendCmd failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	const size_t REPLY_BUF_SIZE = 300;
	char replyBuf[REPLY_BUF_SIZE];
	if(ftpReceiveReplies(ftpData->controlSocketFd, replyBuf, REPLY_BUF_SIZE) != 0)
	{
		printf(RED "ftpReceiveReplies failed.\n" RESET);
		close(ftpData->controlSocketFd);
		return 1;
	}
	
	//Step 2: Close the control socket file descriptor
	if(close(ftpData->controlSocketFd) == -1)
	{
		perror("close");
		return 1;
	}
	
	return 0;
}
