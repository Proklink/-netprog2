#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <ctype.h>



char *remove_symbol(char *oldmass, size_t *size_new_mass, char *symbol)
{//копируем исходный массив

	char *old_mass = (char *)malloc(strlen(oldmass) * sizeof(char));

	memcpy(old_mass, oldmass, sizeof(char) * strlen(oldmass));

	char *new_mass = NULL;
	//отделяем исходную строку от ненужных символов
	char *token = strtok(old_mass, symbol);

	while ((token != NULL) && (*token != '\n')) {
		*size_new_mass += strlen(token);//с каждым новым опреатором расширяем массив
		new_mass = (char *)realloc(new_mass, (*size_new_mass) * sizeof(char));

		//printf("\nnew_mass: %s\n", new_mass);
		//переносим данные в массив
		int t = 0;
		for (int j = (*size_new_mass) - 1; j < (*size_new_mass); j++) {
			new_mass[j] = token[t];
			t++;
		}
		token = strtok(NULL, symbol);
	}
	free(old_mass);
	return new_mass;
}

int *remove_symbol_i(char *oldmass, size_t *size_new_mass, char *symbol)
{	//копируем исходный массив
	char *old_mass = (char *)malloc(strlen(oldmass) * sizeof(char));
	//printf("memory: %p", old_mass);
	memcpy(old_mass, oldmass, sizeof(char) * strlen(oldmass));

	int *new_mass = NULL;
	//отделяем исходную строку от ненужных символов
	char *token = strtok(old_mass, symbol);

	while ((token != NULL) && (*token != '\n')) {
		*size_new_mass = *size_new_mass + 1;//с каждым новой цифрой расширяем массив
		new_mass = (int *)realloc(new_mass, (*size_new_mass) * sizeof(int));
		new_mass[*size_new_mass - 1] = atoi(token);

		token = strtok(NULL, symbol);
	}
	free(old_mass);
	return new_mass;
}

int init_buf(char *buffer, int bufsize)
{

	if (buffer == NULL) {
		perror("Unable to allocate buffer");
		return -1;
	}

	while (1) {
		int flag = 1;

		printf("Print a expression:\n");
		fgets(buffer, bufsize, stdin);
		if (strlen(buffer) < 4)
			flag = 0;
		for (int i = 0; i < strlen(buffer); i++) {
			if (!(isdigit(buffer[i]) || (buffer[i] == 43) ||
			(buffer[i] == 42) || buffer[i] == '\n' || buffer[i] == 32)) {
				flag = 0;
				break;
			}
		}
		if (flag == 0)
			printf("input error, you can only enter numbers and operators +, *\n");
		else
			break;
	}
	return 1;
}

int main(void)
{

	size_t bufsize = 64;//размер введённой строки
	char *buffer;//entered string

	while (1) {
		int result = 0;
		char *operators = NULL;//выражение для операторов без пробелов
		size_t len_op = 0; //длина массива с операторами
		int *numbers = NULL;//выражение для чисел без пробелов и операторов
		size_t len_num = 0;//длина массива с числами

		buffer = (char *)malloc(bufsize * sizeof(char));
		//инициализируем исходную строку
		if (init_buf(buffer, bufsize) == -1)
			return -1;
		char *symbols = " 0123456789";

		operators = remove_symbol(buffer, &len_op, symbols);//получаем массив операторов
		printf("op: %s\n", operators);
		symbols = " +*";

		numbers = remove_symbol_i(buffer, &len_num, symbols);//получаем массив цифр
		printf("\nnum: ");
		for (int i = 0; i < len_num; i++)
			printf("%d ", numbers[i]);

		//исходная строка нам больше не нужна
		free(buffer);
		int comp_len = 0;
		int *compositions = (int *)malloc(comp_len);
		char *oper = NULL;

		while (1) {//сначала ищем вхождения оператора главных по приоритету
			oper = strchr(operators, '*');

			if (oper == NULL)
				//если звёздочки закончились значит остались одни плюсы
				break;

			//если расположить один массив над другим:
			// numbers:   |5|2|3|6|
			// operators: |+|*|*|
			//если изначально правильно составить исходную строку, получим, что все операторы всегда будут
			//между цифрами, значит можно точно определить между какими цифрами стоит какой знак
			++comp_len;
			int star_p = oper - operators;

			compositions = (int *)realloc(compositions,
			sizeof(int)*(comp_len));
			compositions[comp_len - 1] = numbers[star_p] *
			numbers[star_p + 1];
			printf("comp:\n");
			for (int i = 0; i < comp_len; i++)
				printf(" %d", compositions[i]);
			numbers[star_p] = 0;
			numbers[star_p + 1] = 0;
			printf("numbers:\n");
			for (int i = 0; i < len_num; i++)
				printf(" %d", numbers[i]);

			operators[star_p] = 'x';
		}

		for (int i = 0; i < len_num; i++)
			result += numbers[i];
		for (int i = 0; i < comp_len; i++)
			result += compositions[i];
		printf("\nresult = %d\n", result);
		//printf("\nFinish? 1/0\n");
		//int ans;

		//scanf("%d", &ans);
		//getchar();
		//if (ans)
		//	break;
		//else
		//	continue;
	}
	return 0;
}
