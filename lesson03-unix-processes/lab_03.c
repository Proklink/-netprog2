
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <sys/signal.h>
#include <signal.h>

#define BUF_SIZE 128
#define MAX_CHILDREN 64
//#define _XOPEN_SOURCE 700
//#define _GNU_SOURCE



bool terminate; /* true - программа получила сигнал SIGTERM или SIGINT и должна
		 * завершиться
		 */

int nchildren = 0; /* текущее количество дочерних процессов */
struct {
	pid_t pid;
	bool valid; /* true - ячейка содержит PID дочернего процесса, false -
		     * ячейка свободна.
		     */
} children[MAX_CHILDREN];

/*
 * Функция разбивает строку @command на массив аргументов argv, где argv[0] -
 * имя команды. Для анализа строки можно использовать sscanf(). Далее функция
 * исполняет указанную команду с помощью вызова execvp().
 */
static int exec_command(char *command)
{
	char *argv[6] = {NULL}; /* Можно считать, что аргументов не больше 4,
				 * ячейка argv после последнего аргумента
				 * должна содержать NULL.
				 */

	//char temp[5][BUF_SIZE];
	//int ret;
	if (sscanf(command, "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]) < 1) {
		printf("\nFailed to scan the command");
		return -1;
	}
	printf("\nExecvp is startig...");
	if (-1 == execvp(argv[0], argv))
		printf("\nFailed to run execvp");
	//for (int i = 0; i < ret; i++) {
	//	argv[i] = (char *)malloc(sizeof(char) * strlen(temp[i]));
	//	memcpy(argv[i], temp[i], sizeof(char) * strlen(temp[i]));
	//}

	return 0;
}

/*
 * Фунция создает новый процесс с помощью вызова fork(). Далее родительский
 * процесс запоминает PID нового процесса в массиве children и инкрементирует
 * nchildren, а дочерний процесс переходит в функцию exec_command().
 */
static int fork_and_exec_command(char *command)
{
	printf("\nCommand: %s", command);
	pid_t pid;

	if (nchildren == MAX_CHILDREN) {
		printf("\nThe maximum number of child processes has been reached");
		return -1;
	}
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
		return -1;
	}
	children[++nchildren].pid = pid;
	children[nchildren].valid = true;
	if (pid == 0) {
		exec_command(command);

		return 0;
	}

	return 0;
}

/*
 * Если (nchildren > 0), функция определяет статус всех завершившихся процессов
 * с помощью вызова waitpid(). Для каждого процесса выводится причина
 * завершения (макросы WIFEXITED, WIFSIGNALED) и код статуса (макрос
 * WEXITSTATUS), а также очищается ячейка в массиве children.  @blocking - true
 * означает, что функция должна ждать завершения всех процессов, false -
 * функция должна вернуться немедленно, даже если ни один процесс не
 * завершился.
 */
static int reap_dead_children(bool blocking)
{
	pid_t pid;
	int child_status;
	int i = nchildren - 1;
	printf("\nreap_dead_children is starting...");

	/*
		if (blocking == true)
			while (nchildren) {
				pid = waitpid(-1, &child_status, 0);
				if (WIFEXITED(child_status))
					printf("Process %d terminated with status %d\n",
					pid, WEXITSTATUS(child_status));
				else if (WIFSIGNALED(child_status))
					printf("Process %d has been interrupted with status%d\n",
					pid, WTERMSIG(child_status));
				nchildren--;
			}
		for (int i = 0; i < nchildren; i++) {
			pid = waitpid(children[i].pid, &child_status, WNOHANG);
			if (pid == 0)
				continue;
			else {
				if (WIFEXITED(child_status))
					printf("Process %d terminated with status %d\n",
					pid, WEXITSTATUS(child_status));
				else if (WIFSIGNALED(child_status))
					printf("Process %d has been interrupted with status%d\n",
					pid, WTERMSIG(child_status));
				children[i].valid = false;
			}
		}
	*/
	// если @blocking - true, то программа завершается и нужно завершить все дочерние
	//процессы
	//если false, то нужно просто проверить статус всех процессов и если кто то
	//завершился, то декрементировать nchildren и "убрать" соответствующий процесс
	//из children[MAX_CHILDREN]
	//(вообще я не очень понимаю зачем нам нужен blocking, выше я изложил свои предположения)
	// while (i) {
	// 	if (blocking) {
	// 		pid = waitpid(-1, &child_status, 0);

	// 		if (pid == 0) {
	// 			continue;
	// 		} else {
	// 			if (WIFEXITED(child_status))
	// 				printf("Process %d terminated with status %d\n",
	// 				pid, WEXITSTATUS(child_status));
	// 			else if (WIFSIGNALED(child_status))
	// 				printf("Process %d has been interrupted with status%d\n",
	// 				pid, WTERMSIG(child_status));
	// 			nchildren--;
	// 			i--;
	// 		}
	// 	} else {//возможно, стоит проверить children[i].valid = true
	// 		pid = waitpid(children[i].pid, &child_status, WNOHANG);

	// 		if (pid == 0) {
	// 			continue;
	// 			i--;
	// 		} else {
	// 			if (WIFEXITED(child_status))
	// 				printf("Process %d terminated with status %d\n",
	// 				pid, WEXITSTATUS(child_status));
	// 			else if (WIFSIGNALED(child_status))
	// 				printf("Process %d has been interrupted with status%d\n",
	// 				pid, WTERMSIG(child_status));
	// 			children[i].valid = false;
	// 			nchildren--;
	// 			i--;
	// 		}
	// 	}
	// }

	return 0;
}

static void sigchld_handler(int sig)
{
	printf("\nsigchld_handler is starting...");
	/* Обработчик ничего не делает. Мы рассчитываем на завершение
	 * вызова read() с кодом errno EINTR.
	 *
	 * (получается, если в родительский процесс придёт сигнал SIGCHILD,
	 * то read сразу прервётся,если он запущен?)
	 */
}

/* Общий обработчик для сигналов SIGTERM и SIGINT. Обработчик отправляет всем
 * дочерним процессам сигнал SIGTERM с помощью вызова kill().
 */
static void sigterm_sigint_handler(int sig)
{
	int i;
	printf("\nsigterm_sigint_handler is starting...");
	terminate = true;
	for (i = 0; i < MAX_CHILDREN; i++) {
		if (children[i].valid)
			kill(children[i].pid, SIGTERM);
	}
}

int main(int argc, char *argv[])
{
	char chain[BUF_SIZE];
	int ret;

	struct sigaction sa1;
	struct sigaction sa2;

	memset(&sa1, 0, sizeof(sa1));
	memset(&sa2, 0, sizeof(sa2));
	sa1.sa_handler = sigterm_sigint_handler;
	sa2.sa_handler = sigchld_handler;

	sigaction(SIGTERM, &sa1, NULL);
	sigaction(SIGINT, &sa1, NULL);
	sigaction(SIGCHLD, &sa2, NULL);
	/* Фрагмент кода регистрирует обработчики сигналов SIGCHLD, SIGTERM и
	 * SIGINT с помощью вызова sigaction().
	 */

	/* ... УСЛОЖНЕННАЯ ЗАДАЧА ... */


	while (1) {
		if (terminate)
			break;
		printf("\nEnter the command: ");
		ret = read(STDIN_FILENO, chain, sizeof(chain));
		if (ret < 0 && errno == EINTR) {
			printf("Interrupted by signal\n");
			ret = reap_dead_children(false);
		} else if (ret < 0) {
			fprintf(stderr, "Failed to read data from terminal: %s\n",
					strerror(errno));
			goto on_error;
		} else if (ret) {
			char *cmd;
			/*
			 * Фрагмент кода разбивает строку chain на отдельные
			 * команды ('|' - разделитель) и передает их в функцию
			 * fork_and_exec_command(). Для разбиения строки можно
			 * использовать strtok().
			 */
			cmd = strtok(chain, "|");

			while ((cmd != NULL)) {
				if (fork_and_exec_command(cmd) == -1) {
					fprintf(stderr, "Failed to run fork_and_exec_command(): %s\n", strerror(errno));
					goto on_error;
				}
				cmd = strtok(NULL, "|");
			}

		}
	}
	reap_dead_children(true);

	return 0;
on_error:
	return 1;
}
