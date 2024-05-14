#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
4. (0,5) Quando a entrada do usuário não é um programa ou comando válido, deve ser
mostrada uma mensagem de erro adequada.
5. (1,0) Os argumentos digitados na linha de comando devem ser passados ao programa
que será executado.
12. (1,0) O processador de comandos deve permitir o uso de pipes. O símbolo | indica a
separação entre cada programa, conectando a saída padrão do programa à esquerda
com a entrada padrão do programa à direita através de um pipe.
13. (2,0) O processador de comandos deve permitir o uso de múltiplos pipes na mesma
linha de comando.
*/

#define INPUT_SIZE 5000

void prompt()
{
	char username[201];
	strcpy(username, getlogin());	//acha o nome de usuário
	
	char hostname[201];
	hostname[201] = '\0';
	gethostname(hostname, 200);	//acha o nome do host
	
	char dir[1001];
	getcwd(dir, 1001);			//acha o diretório atual
	// fprintf(stderr, "dir: %s\n", dir);
	if(!strncmp("/home", dir, 5)){		//compara se o diretório começa com /home/
		int pos = 6;
		if (!strncmp(username, dir+6, strlen(username))){
			pos += strlen(username)+1;	//pula o nome de usuario e a /
			if (dir[pos-1] == '\0')
				sprintf(dir, "~");	//
			else
				sprintf(dir, "~/%s", dir+pos);
		}
	}

	fprintf(stderr, "[MySh] %s@%s:%s$ ", username, hostname, dir);
	for(int i = 0; i < 1001; i++)
		dir[i] = '\0';	//zera a string do diretório
}

/*
To-do:
- Separar os argumentos e colocar em uma lista
- Separar cada comando pelos pipes
*/

void separa_argumentos(char *input, char *comando, char **args)
{
	char *str_aux = strtok(input, " ");
	
	strcpy(comando, str_aux);
	int i = 0;
	do{
		str_aux = strtok(NULL, " ");

		/*
		Mudar aq pra if (str_aux != NULL)
						strcpy(...)
		e adicionar o null no fim da lista
		*/
		if (str_aux == NULL){
			// strcpy(args[i], "");
			free(args[i]);
			args[i] = NULL;
		}
		else
			strcpy(args[i], str_aux);
		i++;
	}while(str_aux != NULL);
}

void executa(char *input)
{
	int status;
	char comando[INPUT_SIZE], **args;	//mudar isso aq pra memória dinâmica: args[1][INPUT_SIZE]
	
	//separa o nome do comando e os argumentos
	args = malloc(sizeof(char *)*20);			//aloca 20 parâmetros
	for (int i = 0; i < 20; i++)
		args[i] = malloc(sizeof(char)*101);	//aloca 101 bytes pra cada parâmetro
	separa_argumentos(input, comando, args);
	
	for (int i = 0; i < 20; i++){
		if (args[i] == NULL)
			break;
		printf("%s\n", args[i]);
	}

	pid_t pid = fork();

	if (pid < 0)			//erro
		printf("Erro ao criar processo\n");
	else if (pid == 0){		//filho
		execvp(comando, args);
		fprintf(stderr, "Erro: %s\n", strerror(errno));	//Por enquanto essa mensagem de erro é suficiente
		exit(1);
	}
	else{					//pai
		wait(&status);
		if (WIFEXITED(status))
			printf("Exit status: %d\n", WEXITSTATUS(status));
	}

	//liberar a memória dos args
}

void cd(char *input)
{
	//cd (Vai pra home do usuário)
	if ((strlen(input) == 2 && !strcmp("cd", input)) || (strlen(input) == 4 && input[3] == '~')){
		char dir[208], username[201];
		strcpy(username, getlogin());	//acha o nome do usuário
		sprintf(dir, "/home/%s", username);	//concatena /home/ com o nome do usuário
		chdir(dir);	//cd ~
		return;
	}

	//o tamanho desse vetor deveria ser maior, mas n faz diferença pra esse programa
	char aux[INPUT_SIZE];
	if (input[3] == '~')	//troca o ~ pela home do usuário
		sprintf(aux, "/home/%s/%s", getlogin(), input+5);	//input = "cd ~/[nome do diretorio]"
	else
		strcpy(aux, input+3);	//copia a string a partir do nome do diretório

	//cd dir/qualquer/...
	if (chdir(aux))
		fprintf(stderr, "Erro: %s\n", strerror(errno));
}

void ignora()
{
	printf("\n");
}

int main ()
{
	char input[INPUT_SIZE], *c;
	
	struct sigaction ctrlC, ctrlZ;
	memset(&ctrlC, 0, sizeof (ctrlC));
	memset(&ctrlZ, 0, sizeof (ctrlZ));
	
	
	ctrlC.sa_handler = &ignora;
	ctrlZ.sa_handler = &ignora;
	
	sigaction(SIGINT, &ctrlC, NULL);
	sigaction(SIGTSTP, &ctrlZ, NULL);

	do{
		prompt();
		
		for (int i = 0; i < INPUT_SIZE; i++)	//zera o buffer de input
			input[i] = 0;
		if (!read(STDIN_FILENO, input, INPUT_SIZE))	//recebe a entrada ou sai do loop caso receba o sinal do ctrl+d
			break;
		input[strlen(input)-1] = '\0';			//tira o \n do fim da string
		
		if (!strcmp("exit", input))
			break;
		
		executa(input);							//manda executar comando

		//Ach q essa comparação n tem sentido, pq se der erro na hora de executar o cd ele vai avisar
		//Talvez eu botei ela aq pra n ter alguma forma de "injetar" um cd
		//Se mais pra frente eu n ver sentido, vou tirar 
		if (!strncmp("cd", input, 2) && (strlen(input) == 2 || input[2] == ' '))	//compara se o comando começa com cd e se tem ou não algum argumento na frente
			cd(input);

	}while(1);
	
	printf("\n");
	return 0;
}
