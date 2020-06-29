
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#define BUF_SIZE 128

int main(void){

int ret;
char *ftp_cmd = NULL, *ftp_arg = NULL;
char command[BUF_SIZE];
char ftp_reply_text[BUF_SIZE];
int ftp_ret;
ftp_cmd = (char *)calloc(10, sizeof(char));
ftp_arg = (char *)calloc(10, sizeof(char));


printf("\nEnter a command:\n");
fgets(command, BUF_SIZE, stdin);

ret = sscanf(command, "%s %s", ftp_cmd, ftp_arg);
if (ret < 1)
	printf("Command wasn't read");
printf("\n%s %s",ftp_cmd ,ftp_arg);
}
