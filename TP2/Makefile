all: download getip clientTCP

download: main.c FTPClient.c FTPClient.h DNSConverter.c DNSConverter.h URLParser.c URLParser.h shared.h utilInterface.h
	gcc -o ../../download main.c FTPClient.c DNSConverter.c URLParser.c -Wall

getip: demos/getip.c
	gcc -o ../../getip demos/getip.c -Wall

clientTCP: demos/clientTCP.c
	gcc -o ../../clientTCP demos/clientTCP.c -Wall

clean:
	rm -f ../../download ../../getip ../../clientTCP

