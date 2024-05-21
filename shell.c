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

#define INPUT_SIZE 5000
#define N_COMANDS 20
#define N_PARAMS 20

typedef struct s_comando{
	char comando[51];
	char **args;
}Comando;

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

void separa_argumentos(char *input, char *comando, char **args)
{
	char *str_aux = strtok(input, " ");
	
	// printf("Args:\n");
	// printf("[%s]\n", str_aux);

	strcpy(comando, str_aux);
	strcpy(args[0], str_aux);	//coloca o nome do programa no começo
	
	int i = 1;
	do{
		str_aux = strtok(NULL, " ");

		if (str_aux != NULL){
			strcpy(args[i], str_aux);	//copia cada argumento pra lista
			// printf("[%s]\n", str_aux);
		}
		
		i++;
	}while(str_aux != NULL);
	
	i--;

	free(args[i]);
	args[i] = NULL;	//coloca null no fim
}

int separa_comandos(char *input, struct s_comando **comandos)
{
	char *str, *str_aux;
	int i = 0, cont = 0;	//contador de comandos
	
	str = input;
	while(str[i] != '\0'){
		if (str[i] == '|'){
			str[i] = '\0';
			str_aux = str+i+1;
			separa_argumentos(str, comandos[cont]->comando, comandos[cont]->args);	//separa os argumentos do comando
			str = str_aux;
			cont++;
			i = 0;
		}

		i++;
	}

	separa_argumentos(str, comandos[cont]->comando, comandos[cont]->args);	//adiciona no fim do vetor
	cont++;
	
	return cont;
}

Comando** aloca_comandos()
{
	Comando **c = malloc(sizeof(Comando *)*N_COMANDS);	//aloca N comandos
	for (int i = 0; i < N_COMANDS; i++){
		c[i] = malloc(sizeof(Comando));
		c[i]->args = malloc(sizeof(char *)*N_PARAMS);	//aloca M parâmetros
		for (int j = 0; j < N_PARAMS; j++){
			c[i]->args[j] = malloc(sizeof(char)*101);	//aloca 101 bytes pra cada parâmetro
		}
	}
	
	return c;
}

/*
Todo:
- Fazer a função de desalocar a struct de comandos
*/
void executa(char *input)
{
	int status;
	char comando[INPUT_SIZE], **args;
	Comando **comandos = aloca_comandos();
	
	int n_comandos = separa_comandos(input, comandos);	//Coloca cada comando e seus argumentos num vetor
	int n_pipes = n_comandos-1;
	int j = 0;

	int fd[n_pipes][2];	//cria n-1 pipes

	// printf("Nº de comandos: %d\n", n_comandos);
	if (n_comandos <= 0){
		fprintf(stderr, "Erro ao executar comandos\n");
		return;
	}

	for (int i = 0; i < n_pipes; i++)
		pipe(fd[i]);	//inicializa os pipes

	//primeiro processo
	pid_t pid = fork();
	if (pid < 0)
		fprintf(stderr, "Erro: %s\n", strerror(errno));
	else if (pid == 0){	//executa o primeiro comando
		for (int i = 0; i < n_pipes; i++){
			close(fd[i][0]);
			if (i != 0)
				close(fd[i][1]);	//deixa o fd[0][1] aberto
		}

		dup2(fd[0][1], STDOUT_FILENO);
		execvp(comandos[j]->comando, comandos[j]->args);
		return;
	}
	j++;

	//processos do meio
	for (j = 1; j < n_pipes; j++){
		// printf("Meio\n");
		pid = fork();
		if (pid < 0)
			fprintf(stderr, "Erro: %s\n", strerror(errno));
		else if (pid == 0){
			for (int i = 0; i < n_pipes; i++){
				if (i != j-1)
					close(fd[i][0]);
				if (i == j)
					close(fd[i][1]);	//fecha os pipes não usados
			}

			// printf("pipes do processo %d: %d - %d\n", pid, fd[j-1][0], fd[j][1]);
			dup2(fd[j-1][0], STDIN_FILENO);
			dup2(fd[j][1], STDOUT_FILENO);
			execvp(comandos[j]->comando, comandos[j]->args);
			return;
		}
	}
	
	//último processo
	if (n_comandos > 1){
		// printf("Fim\n");
		pid = fork();
		if (pid < 0)
			fprintf(stderr, "Erro: %s\n", strerror(errno));
		else if (pid == 0){	//executa o primeiro comando
			for (int i = 0; i < n_pipes; i++){
				if (i != n_pipes-1)
					close(fd[i][0]);	//deixa o fd[n_pipes-1][0] aberto
				close(fd[i][1]);
			}

			// printf("n_pipes-1: %d\n", n_pipes-1);
			dup2(fd[n_pipes-1][0], STDIN_FILENO);
			execvp(comandos[j]->comando, comandos[j]->args);
			return;
		}
	}

	for (int i = 0; i < n_pipes; i++){
		close(fd[i][0]);
		close(fd[i][1]);
	}

	for (int i = 0; i < n_comandos; i++){
		// printf("i: %d\n", i);
		wait(&status);
		// if (WIFEXITED(status))
			// printf("Exit status: %d\n", WEXITSTATUS(status));
	}

	/*
	printf("Nº de comandos: %d\n", n_comando);
	for (int i = 0; i < n_comando; i++){
		pid_t pid = fork();

		if (pid < 0)			//erro
			fprintf(stderr, "Erro ao criar processo\n");
		else if (pid == 0){		//filho
			sleep(2);
			execvp(comandos[i]->comando, comandos[i]->args);
			fprintf(stderr, "Erro: %s\n", strerror(errno));	//Por enquanto essa mensagem de erro é suficiente
			exit(1);
		}
		else{					//pai
			//mudar aqui pra while(wait(NULL) == -1);
			wait(&status);
			if (WIFEXITED(status))
				printf("Exit status: %d\n", WEXITSTATUS(status));
		}
	}
	//
	pid_t pid = fork();

	if (pid < 0)			//erro
		fprintf(stderr, "Erro ao criar processo\n");
	else if (pid == 0){		//filho
		execvp(comandos[0]->comando, comandos[0]->args);
		fprintf(stderr, "Erro: %s\n", strerror(errno));	//Por enquanto essa mensagem de erro é suficiente
		exit(1);
	}
	else{					//pai
		wait(&status);
		if (WIFEXITED(status))
			printf("Exit status: %d\n", WEXITSTATUS(status));
	}*/


	//libera a memória dos comandos
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
		if (input[0] == '\n' || input[0] == '\0')
			continue;
		input[strlen(input)-1] = '\0';	//tira o \n do fim da string
		
		if (!strcmp("exit", input))
			break;
		
		executa(input);

		if (!strncmp("cd", input, 2) && (strlen(input) == 2 || input[2] == ' '))	//compara se o comando começa com cd e se tem ou não algum argumento na frente
			cd(input);

	}while(1);
	
	printf("\n");
	return 0;
}
