
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

typedef int ftp_code_t; /* -1 - ошибка, >= 0 - код от FTP-сервера */

/* Функция создает потоковый сетевой сокет и устанавливает соединение с
 * локальным сервером (IP-адрес 127.0.0.1) на указанном порте @port. Далее
 * функция открывает поток ввода-вывода на этом сокете и отдает его в качестве
 * возвращаемого значения. Функция использует библиотечные вызовы socket(),
 * connect(), fdopen() и inet_aton().
 */
static FILE *ftp_connect(unsigned short port)
{
	int sock_fd = -1;
	struct sockaddr_in addr;
	int ret;
	FILE *control_stream = NULL;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_aton("127.0.0.1", &addr.sin_addr);

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		fprintf(stderr, "Failed to create TCP socket: %s\n", strerror(errno));
		return NULL;
	}

	ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		fprintf(stderr, "Failed to connect to server: %s\n",
				strerror(errno));
		return NULL;
	}
	control_stream = fdopen(sock_fd, "r+");

	return control_stream;
}

/* Функция формирует FTP-запрос из аргументов @ftp_cmd и @ftp_arg и передает
 * его через управляющее соединение @control_stream в FTP-сервер. После этого
 * функция читает и анализирует ответ от сервера. Код статуса из ответа
 * используется как возвращаемое значение, остальной текст ответа помещается в
 * строку @ftp_reply_text.
 * Аргументы @ftp_cmd и @ftp_arg являются опциональными. Если @ftp_cmd равен
 * NULL, функция сразу ждет ответ от сервера.
 * Функция использует вызовы fprintf(), fgets(), sscanf() и fflush().
 */
static ftp_code_t ftp_command(FILE *control_stream,
			      const char *ftp_cmd,
			      const char *ftp_arg,
			      char *ftp_reply_text)
{
	ftp_code_t ftp_ret;
	int ret = -1;
	char rx_buf[BUF_SIZE];
	bool flag = false;

	if (ftp_reply_text == NULL) {
		flag = true;
		ftp_reply_text = (char *)calloc(BUF_SIZE, sizeof(char));
	}

	if (ftp_cmd != NULL) {
		if (ftp_arg != NULL) {
			ret = fprintf(control_stream, "%s %s\r\n", ftp_cmd, ftp_arg);
			if (ret < 0) {
				fprintf(stderr, "Failed sending request: %s\n", strerror(errno));
				return -1;
			}
		} else {
			ret = fprintf(control_stream, "%s\r\n", ftp_cmd);
			if (ret < 0) {
				fprintf(stderr, "Failed sending request: %s\n", strerror(errno));
				return -1;
			}
		}
	}
	fflush(control_stream);

	if (fgets(rx_buf, sizeof(rx_buf), control_stream) == NULL) {
		printf("Server closed connection\n");
		return -1;
	}

	ret = sscanf(rx_buf, "%d", &ftp_ret);
	if (ret < 0) {
		fprintf(stderr, "Invalid response '%s'\n", rx_buf);
		return -1;
	}

	strcpy(ftp_reply_text, rx_buf + 4);
	printf("\nIts check:  %d %s", ftp_ret, ftp_reply_text);

	if (flag == true)
		free(ftp_reply_text);

	return ftp_ret;
}

static int ftp_login(FILE *control_stream, const char *user, const char *password)
{
	ftp_code_t ftp_ret;
	char ftp_reply_text[BUF_SIZE];

	ftp_ret = ftp_command(control_stream, "USER", user, ftp_reply_text);
	if (ftp_ret < 0) {
		return -1;
	} else if (ftp_ret != 331) {
		fprintf(stderr, "Invalid user name: %s\n", ftp_reply_text);
		return 0;
	}

	ftp_ret = ftp_command(control_stream, "PASS", password, ftp_reply_text);
	if (ftp_ret < 0) {
		return -1;
	} else if (ftp_ret != 230) {
		fprintf(stderr, "Invalid password: %s\n", ftp_reply_text);
		return 0;
	}
	return 0;
}

/* Функция устанавливает дополнительное соединение для передачи данных от
 * FTP-сервера. Для этого отправляется команда PASV. Из ответа от сервера
 * извлекается номер порта, на котором сервер ждет подключения. После установки
 * соединения функция отправляет серверу команду RETR с аргументом @file_name и
 * читает данные файла из дополнительного соединения. Полученные данные
 * записываются в файл с таким же именем в текущем каталоге.
 */
static int ftp_retrieve(FILE *control_stream, const char *file_name)
{
	int port = 0;
	int temp = 0;
	char ftp_reply_text[BUF_SIZE];
	ftp_code_t ftp_ret;
	int zpt_count = 0;
	FILE *file_stream = NULL;
	char text_port[10];
	char retr_data[BUF_SIZE];

	memset(text_port, 0, 10);

	ftp_ret = ftp_command(control_stream, "PASV", NULL, ftp_reply_text);
	if (ftp_ret < 0)
		return -1;

	if (ftp_ret == 227) {
		for (int i = 0; i < strlen(ftp_reply_text); i++) {
			if (ftp_reply_text[i] == ',') {
				zpt_count++;
				if (zpt_count == 4)
					temp = i;
				if (zpt_count == 5) {
					memmove(text_port, ftp_reply_text + temp + 1, i - temp - 1);
					port = atoi(text_port) * 256;
					temp = i;
					memset(text_port, 0, 10);
				}
			}
			if (ftp_reply_text[i] == ')') {
				memmove(text_port, ftp_reply_text + temp + 1, i - temp - 1);
				port += atoi(text_port);
			}
		}
	} else
		return -1;

	file_stream = ftp_connect(port);
	if (!file_stream) {
		fprintf(stderr, "Failed to create file_stream\n");
		return -1;
	}

	ftp_ret = ftp_command(control_stream, "RETR", file_name, ftp_reply_text);
	if (ftp_ret < 0)
		return -1;

	if (fgets(retr_data, sizeof(retr_data), file_stream) == NULL) {
		printf("Failed to retrieve file\n");
		return -1;
	}

	return ftp_ret;
}

int main(int argc, char *argv[])
{
	FILE *control_stream = NULL;
	int ret;

	control_stream = ftp_connect(21);
	if (!control_stream) {
		fprintf(stderr, "Failed to create control stream\n");
		return 1;
	}
	ret = ftp_command(control_stream, NULL, NULL, NULL);
	if (ret < 0)
		goto on_error;

	if (argc > 1) {
		ret = ftp_login(control_stream, argv[1], argc > 2 ? argv[2] : NULL);
		if (ret < 0) {
			fprintf(stderr, "Failed to log in to FTP server\n");
			goto on_error;
		}
		printf("Logged in as %s\n", argv[1]);
	}

	while (1) {
		char *ftp_cmd = NULL, *ftp_arg = NULL;
		char command[BUF_SIZE];
		char ftp_reply_text[BUF_SIZE];
		ftp_code_t ftp_ret;

		/* Фрагмент кода читает и анализирует команду пользователя с
		 * помощью fgets() и sscanf(). Далее если введена команда RETR,
		 * вызывается функция ftp_retrieve(). В остальных случаях -
		 * ftp_command(). По результатам выполнения выводится код
		 * статуса и дополнительное сообщение от сервера.
		 */
		ftp_cmd = (char *)calloc(BUF_SIZE, sizeof(char));
		ftp_arg = (char *)calloc(BUF_SIZE, sizeof(char));

		printf("\nEnter a command:\n");
		fgets(command, BUF_SIZE, stdin);

		ret = sscanf(command, "%s %s", ftp_cmd, ftp_arg);
		printf("\n%s, %s", ftp_cmd, ftp_arg);
		// if (ret < 1) {
		// 	printf("Command wasn't read");
		// 	continue;
		// } else if (ret == 1) {
		// 	ftp_cmd[strlen(ftp_cmd)] = '\0';
		// 	ftp_arg = NULL;
		// } else if (ret == 2) {
		// 	ftp_cmd[strlen(ftp_cmd)] = '\0';
		// 	ftp_arg[strlen(ftp_arg)] = '\0';
		// }

		if (!strcmp(ftp_cmd, "RETR")) {
			ftp_ret = ftp_retrieve(control_stream, ftp_arg);
			if (ftp_ret < 0)
				goto on_error;
		} else {
			ftp_ret = ftp_command(control_stream, ftp_cmd, ftp_arg, ftp_reply_text);
			if (ftp_ret < 0)
				goto on_error;
		}
		printf("Code: %d. Reply text: %s\n", ftp_ret, ftp_reply_text);
	}

	return 0;
on_error:
	fclose(control_stream);
	return 1;
}
