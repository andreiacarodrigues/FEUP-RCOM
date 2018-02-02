#include <stdio.h>

#include "URLParser.h"
#include "DNSConverter.h"
#include "FTPClient.h"
#include "utilInterface.h"

int main(int argc, char **argv)
{
	//Step 1: Read the arguments
	if(argc != 2)
	{
		printf("Usage: %s <file URL>\n",argv[0]);
		return 1;
	}
	
	//Step 2: Parse the URL
	url_data urlData;
	if(parseURL(argv[1],&urlData) != 0)
	{
		printf(RED "parseURL failed.\n" RESET);
		return 1;
	}
	printf(GRN "parseURL: success!\n" RESET);

	/*
	Tests
	1- "ftP://speedtest.tele2.net/1KB.zip"
	2- "ftp://demo:password@test.rebex.net/readme.txt"
	*/
	printf(CYN "username: %s\n", urlData.username);
	printf("password: %s\n", urlData.password);
	printf("host: %s\n", urlData.host);
	printf("port: %u\n", urlData.port);
	printf("path: %s\n" RESET, urlData.path);
	
	//Step 3: Retrieve the IP address using the DNS
	char* ip = host2Ip(urlData.host);
	if(ip == NULL)
	{
		printf(RED "host2Ip failed.\n" RESET);
		return 1;
	}
	printf(GRN "host2Ip: success!\n" RESET);
	printf(CYN "IP address of %s: %s\n" RESET, urlData.host, ip);
	
	//The following steps are according to RFC 959 - Section 5.4.
	ftp_data ftpData;
	
	//Step 4: Establish connection with the server
	if(ftpConnect(&ftpData, ip, urlData.port) != 0)
	{
		printf(RED "ftpConnect failed.\n" RESET);
		return 1;
	}
	printf(GRN "ftpConnect: success!\n" RESET);
	
	//Step 5: Login
	if(ftpLogin(&ftpData, urlData.username, urlData.password) != 0)
	{
		printf(RED "ftpLogin failed.\n" RESET);
		return 1;
	}
	printf(GRN "ftpLogin: success!\n" RESET);
	
	//Step 6: Set the DTP (data transfer process) to passive
	if(ftpSetPasv(&ftpData) != 0)
	{
		printf(RED "ftpSetPasv failed.\n" RESET);
		return 1;
	}
	printf(GRN "ftpSetPasv: success!\n" RESET);
	
	//Step 7: Download the file
	if(ftpDownload(&ftpData, urlData.path) != 0)
	{
		printf(RED "ftpDownload failed.\n" RESET);
		return 1;
	}
	printf(GRN "ftpDownload: success!\n" RESET);
	
	//Step 8: Logout (disconnect from the server)
	if(ftpLogout(&ftpData) != 0)
	{
		printf(RED "ftpLogout failed.\n" RESET);
		return 1;
	}
	printf(GRN "ftpLogout: success!\n" RESET);
	
	return 0;
}

