/*
	Rui Miguel Freitas Rocha
	tempo de inicio 15-10-2017
	tempo de fim 10-10-2017
	tempo de elaboraçao 5h por dia * 56 dias totais = 280 horas usadas
*/


//gcc -Wall -g administrador.c -o administrador

#include "externas.h"




int main(int argc, char ** argv){

	menu();
	
	return 0;
}

void menu(){
	char op[256];
	char aux[256];
	char menssagem[tamanho];
	char nome[tamanho];
	char triagem[50];
	char atendimento[50];
	char prioridade[50];
	char poolthreadsTriagem[100];

	while (1){
		printf("Menu:\n");
		printf("1 -> Alterar o número de threads de triagem\n");
		printf("2 -> adicionar paciente\n");
		printf("3 -> imprimir estatisticas do sistema \n");
		printf("4 -> sair\n");
		scanf("%s", op);

		if (strcmp(op, "1") == 0){
			printf("Número de threads a adicionar a triagem: ");
			scanf("%s", poolthreadsTriagem);
			sprintf(aux, "%s", poolthreadsTriagem);
			strcpy(menssagem,"TRIAGE=");
			strcat(menssagem, aux);
			//printf("mensagem %s\n",menssagem);
			escrevePipe(op, menssagem);
		}

		else if (strcmp(op, "2") == 0){
			scanf("%s %s %s %s",nome,triagem,atendimento,prioridade);
			//printf(" aqui %s %d %d %d\n", nome,triagem,atendimento,prioridade);
			//sprintf(aux,"%s",nome);
			strcpy(menssagem,"ATENDIMENTO=");
			sprintf(aux,"%s",nome);
			strcat(menssagem,aux);
			sprintf(aux," %s",triagem);
			strcat(menssagem,aux);
			sprintf(aux," %s",atendimento);
			strcat(menssagem,aux);
			sprintf(aux," %s",prioridade);
			strcat(menssagem,aux);
			//printf("mensagem %s\n",menssagem);
			printf("mensagem para o pipe %s\n",menssagem);

			escrevePipe(op, menssagem);
			
		}
		else if (strcmp(op, "3") == 0){
			escrevePipe(op, "estatisticas");
		}
		else if (strcmp(op, "4") == 0){
			escrevePipe(op, "sair");
			break;
		}
		else{
			printf("opcao invalida\n");
		}
	}	
}

void escrevePipe(char* op, char* str){
	int pipe_fd;
	int res;
	int modoAbertura = O_WRONLY;
	printf("Process %d opening FIFO O_WRONLY\n", getpid());
	pipe_fd = open(INPUT_PIPE, modoAbertura);
	printf("Processo %d result %d   mensagem  %s\n", getpid(), pipe_fd, str);

	if (pipe_fd != -1) {
		res = write(pipe_fd, str, (strlen(str)));
		if (res == -1) {
			fprintf(stderr, "erro ao escrever no pipe\n");
			exit(EXIT_FAILURE);
		}
		(void)close(pipe_fd);
	}
	else {
		exit(EXIT_FAILURE);
	}
	printf("Processo %d saiu \n", getpid());
}