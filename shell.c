#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
3. (0,5) Quando a entrada do usuário é o nome de um programa no path ou o caminho
completo ou relativo de um programa, o mesmo deve ser executado.
4. (0,5) Quando a entrada do usuário não é um programa ou comando válido, deve ser
mostrada uma mensagem de erro adequada.
5. (1,0) Os argumentos digitados na linha de comando devem ser passados ao programa
que será executado.
12. (1,0) O processador de comandos deve permitir o uso de pipes. O símbolo | indica a
separação entre cada programa, conectando a saída padrão do programa à esquerda
com a entrada padrão do programa à direita através de um pipe.
13. (2,0) O processador de comandos deve permitir o uso de múltiplos pipes na mesma
linha de comando.
* */

#define INPUT_SIZE 5000

void prompt()
{
	char username[501];
	strcpy(username, getlogin());
	
	char hostname[501];
	hostname[501] = '\0';
	gethostname(hostname, 500);
	
	char aux[1001], dir[1001];
	getcwd(aux, 1001);
	if(!strncmp("/home/", aux, 6)){
		int pos = 6;
		if (!strncmp(username, aux+6, strlen(username))){
			pos += strlen(username)+1;
			sprintf(dir, "~/%s", aux+pos);
		}
	}
	
	fprintf(stderr, "[MySh] %s@%s:%s$ ", getlogin(), hostname, dir);
}

/*
8. (0,5) Implemente o comando cd para mudar diretórios (quando recebe argumento) ou
voltar ao diretório home do usuário (sem argumentos ou com o argumento ~).
9. (0,5) Mostre uma mensagem de erro adequada se cd falhar.
*/

void cd(char input[INPUT_SIZE])
{
	
}

void ignora()
{
	printf("\n");
}

int main ()
{
	char input[INPUT_SIZE], *c;
	
	struct sigaction ctrlC, ctrlZ;
	memset (&ctrlC, 0, sizeof (ctrlC));
	memset (&ctrlZ, 0, sizeof (ctrlZ));
	
	
	ctrlC.sa_handler = &ignora;
	ctrlZ.sa_handler = &ignora;
	
	sigaction (SIGINT, &ctrlC, NULL);
	sigaction (SIGTSTP, &ctrlZ, NULL);

	do{
		prompt();
		
		if (!read(STDIN_FILENO, c, 1))
			break;
		
		fgets(input, INPUT_SIZE, stdin);
		//printf("tam: %d\n", strlen(input));
		input[strlen(input)-1] = '\0';
		
		if (!strcmp("exit", input))
			break;
		
		if (!strncmp("cd", input, 2))
			cd(input);
		
		
	}while(1);
	
	printf("\n");
	return 0;
}
