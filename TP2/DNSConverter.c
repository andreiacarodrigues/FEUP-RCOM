#include <netdb.h> //gethostbyname
#include <stdlib.h> //NULL

//#include <sys/socket.h> //inet_ntoa
//#include <netinet/in.h> //inet_ntoa
#include <arpa/inet.h> //inet_ntoa

#include "DNSConverter.h"

char *host2Ip(const char* host)
{
	//Step 1: Get the host info
	struct hostent *h = gethostbyname(host);
	if (h == NULL) 
	{  
		herror("gethostbyname");
		return NULL;
	}
	
	//Step 2: Retrieve the IP char pointer
	char *IPStr = inet_ntoa(*((struct in_addr *)(h->h_addr)));
	return IPStr;
}
