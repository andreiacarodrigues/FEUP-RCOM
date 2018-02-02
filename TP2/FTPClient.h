#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

typedef struct {
	int controlSocketFd;
	int dataSocketFd;
} ftp_data;

int ftpConnect(ftp_data *ftpData, const char* ip, const unsigned int port);
int ftpLogin(ftp_data *ftpData, const char *username, const char *password);
int ftpSetPasv(ftp_data *ftpData);
int ftpDownload(ftp_data *ftpData, const char *path);
int ftpLogout(ftp_data *ftpData);

#endif //FTP_CLIENT_H
