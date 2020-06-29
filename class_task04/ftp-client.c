
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

#define FIXME 0
#define BUF_SIZE 128

int main(int argc, char *argv[])
{
	FILE *control_stream = NULL;
	int sock = -1;
	struct sockaddr_in addr;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(21);
	inet_aton("127.0.0.1", &addr.sin_addr);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(stderr, "Failed to create TCP socket: %s\n", strerror(errno));
		goto on_error;
	}

	ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		fprintf(stderr, "Failed to connect to server: %s\n",
				strerror(errno));
		goto on_error;
	}

	control_stream = fdopen(sock, "r+");

	{
		char rx_buf[BUF_SIZE];
		int ftp_ret;
		int len;
		char ftp_reply_text[BUF_SIZE];

		if (fgets(rx_buf, sizeof(rx_buf), control_stream) == NULL) {
			printf("Server closed connection\n");
			goto on_error;
		}

		ret = sscanf(rx_buf, "%d %n", &ftp_ret, &len);
		if (ret < 0) {
			fprintf(stderr, "Invalid response '%s'\n", rx_buf);
			goto on_error;
		}
		strcpy(ftp_reply_text, rx_buf);

		printf("Code: %d. Reply text: %s", ftp_ret, ftp_reply_text);
	}

	fprintf(control_stream, "%s\r\n", "QUIT");
	fflush(control_stream);

	fclose(control_stream);

	return 0;
on_error:
	if (control_stream)
		fclose(control_stream);
	else if  (sock >= 0)
		close(sock);
	return 1;
}
