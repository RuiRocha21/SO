/*
	Rui Miguel Freitas Rocha
	tempo de inicio 15-10-2017
	tempo de fim 10-10-2017
	tempo de elaboraçao 5h por dia * 56 dias totais = 280 horas usadas
*/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

// caminho de ficheiro de configuração
#define ficheiroConfiguracao "ficheiros/config.txt"

// caminho ficheiro memoria mapeada
#define ficheiroLog "ficheiros/ficheiroMemoriaMapeada.txt"

#define log "ficheiros/ficheiroLog.txt"
#define tamanhoLog 1024*1024*4 // 4MB para ficheiro de registos de sistema
#define maximo (tamanhoLog * sizeof (struct queueNO))

#define DEBUG 1

#define tamanhoBuf	1024 

//definiçao do pipe
#define INPUT_PIPE "/tmp/my_fifo"

#define tamanho 20
#define MAX (tamanho * sizeof(struct noEstatisticas))

#define recebe 100


//estrutura do no a armazenar configuraçoes

typedef struct no* configs;
typedef struct no {
	int threadsTriagem;                   // Nº de threads de triagem
	int processosDoutor;           		// Nº de processos de Doutor
	int tempoTurno;						//tempo de turno
	int tamanhoFilaAtendimento;             // tamanho da fila de atendimentos
}noConfiguracao;


//estrutura de estatisticas
typedef struct noEstatisticas* estaticas;
typedef struct noEstatisticas{
	long tipoMensagem;
	int numeroPacientesTriados;			//numero total de pacientes triados
	int numeroPacientesAtendidos;		//numero total de pacientes atendidos            		
	double tempoMedioAntesTriagem;			//tempo medio de espera antes do inicio da triagem
	double tempoMedioAteAtendimento;		//tempo medio de espera entre o fim da triagem e o inicio do atendimento
	double mediaTempoTotalUtilizador;		//media do tempo de cada utilizador gastou desde que chegou ao sistema ate sair 
}memoriaPartilhadaSistema;




//estrutura de dados de triagem
struct queueNO 
{
	char nome[20];
	int tempoTriagem;
	int tempoAtendimento;
	int prioridade;
	time_t chegadaSistema;
	time_t tempoInicioQ;
	time_t tempoFimQ;
	time_t inicioAtendimento;
	time_t fimAtendimento;
	double tempoTotal;
	struct queueNO *proximo;
};








struct queue
{
	struct queueNO *primeiro, *ultimo;
};

struct filaMensagens{
	long mtype;
	int tamanhoFila;
	struct queueNO paciente; //tirar ponteiro
}mensagens;




//servidor
void catch_ctrlc(int signum);
void criaPipe();
void lerFicheiro();
void iniciarMemorias();
int criaMemoriaPartilhada();
void limpaMemoriaPartilhada();
void destroiMemoriaMapeada();
void terminar(int shm, int sem, int proc, int threadTriagem,int processoDoutor, int mmap,int mmap2,int mq);

void imprimeEstatisticas();
int criarFicheiroMemoriaMapeada();
void* lePipe();
void criaThreadsTriagem(int n);
void  *servico(void *ptr);
void gestorEstatisticas(int sinal);
void gravarFicheiroLog();
void inicializarBuffer();
int criaNovoProcesso();
void atenderPaciente();

void destroiMMFregistos();
/*---------------------------------------------------------metodos da queue*/

void insereQueue(struct queue *q, char *nome, int triagem, int atendimento, int prioridades,time_t chegadaSistema);
struct queueNO* novoNo(char nome[30], int triagem, int atendimento, int prioridades,time_t chegadaSistema);
struct queue *criaQueue();
struct queueNO *retiraQueue(struct queue *q);
void imprimeQueue(struct queue *q);
//cliente
void menu();
void escrevePipe(char* op, char* str);