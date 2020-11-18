/*
	Rui Miguel Freitas Rocha
	tempo de inicio 15-10-2017
	tempo de fim 10-10-2017
	tempo de elaboraçao 5h por dia * 56 dias totais = 280 horas usadas
*/

#include "externas.h"

char *pt;
void *fichLog;
char *fichEstatisticas;
int descritorRegistos;
char registos[tamanhoLog];
int capacidade;

char RegistaEstatisticas[tamanhoLog];
//ponteiros das estruturas de dados
configs configuracao;




//semaforos
sem_t *semConfiguracao;



pthread_mutex_t lock;
pthread_mutexattr_t attrmutex;
struct queue *fila;



// THREADS 
pthread_t  pthreadsTriagem[10];
int idtriagem[10];


// memoria partilhada
int shmid;


/* Id da fila de mensagens MQ */
key_t key;
int mqid;		
int tamanhof=0;
int tamanhoMaximo=0;
struct msqid_ds valoresMQ;



pid_t pidConfiguracao , pidEstatisticas;
pid_t pidDoutores,pid_temporario;

int flagProcTemp = 0;


int mmapfd;
estaticas estaticasMemoriaMapeada;
int pacienteTriados=0;
int numeroPacientesAtendidos =0;
int nPaciente = 0;
int *ptrNpacientes;
int *ptrTriados;
time_t chegadaSistema;
time_t saidaSistema;
time_t incioTriagem;
time_t fimTriagem;
time_t incioDoAtendimento;
time_t fimDoAtendimento;
time_t tempoSaidaQueue;
double tempoTotal;

int numeroEntradas=0;


int ficherioMMP;




time_t tempo;

struct queueNO *n, *temporiario;


sigset_t unblock_ctrlc; //nao bloqueia SIGINT
sigset_t unblock_hup; //nao bloqueia SIGHUP e SIGTERM
sigset_t block_all; //bloqueia tudo - enquanto realiza trabalho

int main(int argc, char ** argv){

	int status;
	time(&tempo);
	char *horaArranqueSistema = asctime(localtime(&tempo));

	key=1234;

	ptrNpacientes=&nPaciente;
	ptrTriados = &pacienteTriados;
	printf("hora de horaArranqueSistema %s\n", horaArranqueSistema);
	configuracao = (configs)malloc(sizeof(configs));

	sem_unlink("CONFIG");
	semConfiguracao = sem_open("CONFIG", O_CREAT|O_EXCL, 0700, 0);

	#ifdef DEBUG
	printf("INICIALIZAÇÃO (init) Pid %d \n", getpid());
	#endif
	fila = criaQueue();
	lerFicheiro();

	iniciarMemorias();
	criaPipe();



	
	sigfillset(&unblock_ctrlc);
	

	if(criarFicheiroMemoriaMapeada() != 0){
		#ifdef DEBUG
		printf("erro ao mapear o ficheiroLog\n");
		#endif
	}

	//signal(SIGINT, catch_ctrlc);
	//bloquear ctrl-Z
	signal(SIGTSTP, SIG_IGN);
	pidConfiguracao=getpid();
	
	if(SIG_IGN){
		printf("sinal CTRL-Z bloqueado\n");
	}
	
	if((pidEstatisticas = fork()) == -1)
    {

		#ifdef DEBUG
		printf("erro\n");
		#endif

        perror("fork");

        return 0;
    }


    if(pidEstatisticas == 0)
    {

		pidEstatisticas=getpid();
		#ifdef DEBUG
		printf("processo estaticas iniciado com o PID = %d \n",pidEstatisticas );
		#endif
		gestorEstatisticas(0);

		sleep(1);		

    }else{
				
		sem_unlink("CONFIG");
		semConfiguracao = sem_open("CONFIG", O_CREAT|O_EXCL, 0700, 0);

		
		#if DEBUG
		printf("ficheiro de configuracoes carregado\n");
		#endif
		int j = 0;
		
		for( ; j< configuracao->processosDoutor;j++){
		
			pidDoutores = fork();
	        #ifdef DEBUG
	        printf("vou criar processo para novo turno \n ");
	        #endif
			if(pidDoutores == 0){
				atenderPaciente();
				exit(1);
			}
		    
		}
		
		int i = 0;

		for( ; i <= configuracao->threadsTriagem ; i++) {
			idtriagem[i]=i;
			if(i==0){
				
				
				if(pthread_create(&pthreadsTriagem[i], NULL, lePipe, &idtriagem[i]) != 0) {
					#ifdef DEBUG
					printf("erro ao criar threads triagem\n");
					#endif
					terminar(1,1,1,1,1,1,1,1);
				}
				
			}
			else {
				
				if(pthread_create(&pthreadsTriagem[i], NULL, servico, fila) != 0) {


					#ifdef DEBUG
					printf("erro ao criar threads\n");
					#endif
					terminar(1,1,1,1,1,1,1,1);
				}	
			}
		}
	}
	
	
	while ((pidConfiguracao = wait(&status)) > 0)
    {

        sleep(1);
       
        pidDoutores = fork();
        #ifdef DEBUG
        printf("vou criar um novo processo para o turno\n");
        #endif
		if(pidDoutores == 0){
			atenderPaciente();
			exit(1);
		}
    }

		
    

    	
   
	
	
	return 0;
}

void atenderPaciente(){

	
	time_t inicio, fim;
    time(&inicio);
	time(&fim); 

    double seconds; 
    seconds=inicio-fim;
	
    
	while((double)(configuracao->tempoTurno)>seconds){
		
		if (msgctl(mqid,IPC_STAT,&valoresMQ) == -1){
			perror("erro no  msgctl()");
		}
		else if(valoresMQ.msg_qnum >= 1){
	        if(msgrcv(mqid, &mensagens, sizeof(mensagens)-sizeof(long),-3,0) <0){//-sizeof(long)
	        	printf("erro\n");
	        }else{
	        	printf("doutor numero %d \n",getpid());
	        	pthread_mutex_lock(&lock);
	        	printf("retirei da message queue %d %s %d %d %d \n",mensagens.tamanhoFila,mensagens.paciente.nome,mensagens.paciente.tempoTriagem,mensagens.paciente.tempoAtendimento,mensagens.paciente.prioridade);
	        	time(&incioDoAtendimento);
	        	
		    	
		    	sleep(mensagens.paciente.tempoAtendimento);
		    	
		    	
		    	time(&fimDoAtendimento);
		    	
		    	
		    	mensagens.paciente.inicioAtendimento = incioDoAtendimento/1000000000;
		    	mensagens.paciente.fimAtendimento = fimDoAtendimento/1000000000;
		    	chegadaSistema = mensagens.paciente.chegadaSistema/1000000000;
		    	tempoTotal=mensagens.paciente.fimAtendimento-mensagens.paciente.chegadaSistema;
		    	
		    	
		    	mensagens.paciente.tempoTotal = tempoTotal;
		    	
	        	tempoSaidaQueue=mensagens.paciente.tempoFimQ/1000000000;
	        	double mediaAntesAtendimento;
	        	
	        	
		    	
		    	//printf("paciente %s  tempo de atendimento %d\n",mensagens.paciente.nome,  mensagens.paciente.tempoAtendimento);

	        	mediaAntesAtendimento = mensagens.paciente.inicioAtendimento-tempoSaidaQueue;
	        	estaticasMemoriaMapeada->tipoMensagem = mensagens.paciente.prioridade;
	        	estaticasMemoriaMapeada->numeroPacientesAtendidos += 1;
	        	estaticasMemoriaMapeada->tempoMedioAteAtendimento =estaticasMemoriaMapeada->tempoMedioAteAtendimento+ mediaAntesAtendimento;
	        	estaticasMemoriaMapeada->mediaTempoTotalUtilizador += tempoTotal;
		    	
	        	

	        	sprintf(fichLog,"utente %s foi atendido pelo doutor %d com tempo de atendimento %d nivel de prioridade %d esteve no sistema no total %lf s\n",mensagens.paciente.nome,getpid(),mensagens.paciente.tempoAtendimento,mensagens.paciente.prioridade,mensagens.paciente.tempoTotal);

	        	

	        	time(&fim);
		    	seconds = fim - inicio;
	 			pthread_mutex_unlock(&lock);
	 			
	    		

	        }
	    }else{
	    	pthread_mutex_unlock(&lock);
	    	
	    }
        
       
        sleep(mensagens.paciente.tempoAtendimento);
        
    }
    printf("Sou o doutor %d o meu turno acabou\n", getpid());
    
}


void catch_ctrlc(int signum){
	printf("a limprar sistema\n");
	terminar(1,1,1,1,1,1,1,1);
	
}

void inicializarBuffer(){
	
	
	mqid = msgget(recebe, IPC_CREAT|0777);
  	if(mqid < 0){
      perror("Erro ao criar a message queue\n");
      exit(0);
    }

    pthread_mutexattr_init(&attrmutex);
    pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
    

	if (pthread_mutex_init(&lock, &attrmutex) != 0)
    {
        printf("\n erro ao criar mutx\n");
        exit(0);
    }


	#ifdef DEBUG
		printf("Fila de mensagens criada.\n");
	#endif
	
	#ifdef DEBUG
		printf("Mutex criado.\n");
	#endif


	#ifdef DEBUG
		printf("Buffer criado.\n");
	#endif
}


void gestorEstatisticas(int sinal) {
	signal(SIGUSR1, imprimeEstatisticas);
	#ifdef DEBUG
	printf("gestor de estatisticas a espera de sinal\n");
	#endif
	sleep(1);
	
}

void gravarFicheiroLog(){

	FILE *fLog;
	char linha[tamanhoBuf];

	/******* Abrir ficheiro de Estatisticas *******/
	fLog = fopen(ficheiroLog, "a+");
	if(fLog == NULL){
		#ifdef DEBUG
		printf("Erro ao abrir Ficheiro Log\n");
		#endif
		terminar(1,1,1,1,1,1,1,1);
	}
	#ifdef DEBUG
	printf("Guardando as Estatisticas...\n");
	#endif
	sprintf(linha,"%d %d %lf %lf %lf \n", estaticasMemoriaMapeada->numeroPacientesTriados, estaticasMemoriaMapeada->numeroPacientesAtendidos, estaticasMemoriaMapeada->tempoMedioAntesTriagem, estaticasMemoriaMapeada->tempoMedioAteAtendimento,estaticasMemoriaMapeada->mediaTempoTotalUtilizador);
	if(fputs(linha, fLog) == EOF){
		#ifdef DEBUG
		fprintf(stderr, "Falhou escrita para o ficheiro Log.\n");
		#endif
		terminar(1,1,1,1,1,1,1,1);
	}
	
	fclose(fLog);
}






void criaThreadsTriagem(int n){
	int i;
	int sig=0;
	// Shutdown a todas as threads
	for(i = 1; i <= configuracao->threadsTriagem; i++) {
		pthread_detach(pthreadsTriagem[i]);
		if ( pthread_kill(pthreadsTriagem[i], sig) == 0){
			#ifdef DEBUG
			printf("thread triagem %d terminada com sucesso\n", i);
			#endif
		}
		else{
			#ifdef DEBUG
			printf("erro ao terminar as threads %d\n", i);
			#endif
		}
	}
	configuracao->threadsTriagem = n;
	#ifdef DEBUG
	printf("numero de threads da triagem alterado: %d\n", configuracao->threadsTriagem);
	#endif

	for(i = 1; i <= configuracao->threadsTriagem ; i++) {

		idtriagem[i]=i;
		if (i > 0){
			// cada thread vai executar um servico
			if(pthread_create(&pthreadsTriagem[i], NULL, servico, fila) != 0) {
				#ifdef DEBUG
				printf("erro ao criar as threads\n");
				#endif
				terminar(1,1,1,1,1,1,1,1);
			}
		}		
	}

}



void  *servico(void *ptr){
	
	
	
	
	
	while(1){
		
		
		pthread_mutex_lock(&lock);

		n= retiraQueue(fila);
		
	    if(n != NULL){
	    	
			time(&incioTriagem);
			mensagens.mtype = n->prioridade;
			strcpy(mensagens.paciente.nome,n->nome);
			mensagens.paciente.tempoAtendimento = n->tempoAtendimento;
			mensagens.paciente.tempoTriagem = n->tempoTriagem;
			mensagens.paciente.prioridade = n->prioridade;
			mensagens.paciente.tempoInicioQ = incioTriagem;
			mensagens.tamanhoFila +=1 ;
			
			tamanhof++;
			
			if (msgsnd(mqid,&mensagens,sizeof(mensagens)-sizeof(long),0)<0){
				printf("Erro \n");
			}
			
			else{
				if (msgctl(mqid,IPC_STAT,&valoresMQ) == -1){
		        	perror("erro no  msgctl()");
			    }else{
			    	if((valoresMQ.msg_qnum >=((tamanhoMaximo*0.8)+tamanhoMaximo)) && flagProcTemp==0 ){
			    		pid_temporario=fork();
			    		flagProcTemp = 1;
			    		#ifdef DEBUG
				        printf("PROCESSO TEMPORARIO VAI COMEÇAR O TURNO\n");
				        #endif
				        atenderPaciente();
			    	}if((flagProcTemp ==1) && (pid_temporario > 0) && (valoresMQ.msg_qnum <tamanhoMaximo*0.8)){
			    		flagProcTemp = 0;
			    		#ifdef DEBUG
				        printf("PROCESSO TEMPORARIO VAI TERMINAR O TURNO\n");
				        #endif
			    		wait(NULL);
			    	}
			    }
				
				time(&fimTriagem);
				mensagens.paciente.tempoFimQ = fimTriagem;
				
				printf("enviei na message queue %d %s %d %d %d \n",mensagens.tamanhoFila,mensagens.paciente.nome,mensagens.paciente.tempoTriagem,mensagens.paciente.tempoAtendimento,mensagens.paciente.prioridade);
				
				
				
				pacienteTriados+=1;
				ptrTriados = &pacienteTriados;
				
				
				
				double mediaTriados;

				mediaTriados = (mensagens.paciente.tempoInicioQ-mensagens.paciente.chegadaSistema)/1000000000;
				

				estaticasMemoriaMapeada->tempoMedioAntesTriagem += mediaTriados;
				estaticasMemoriaMapeada->numeroPacientesTriados= *ptrTriados;
				sprintf(fichLog,"utente %s deu entrada na triagem , tempo na triagem %d com nivel de prioridade %d\n",n->nome,n->tempoTriagem,n->prioridade);
				temporiario = n;
				n = n->proximo;
				
			}	

	  	}
	  	
	  
	  	
  		pthread_mutex_unlock(&lock);
		if (temporiario !=NULL){
			
			sleep(temporiario->tempoTriagem);
			temporiario=NULL;
		}
	}
	
	

}



void criaPipe(){
	int res;
	if (access(INPUT_PIPE, F_OK) == -1) {
		res = mkfifo(INPUT_PIPE, 0660);
		if (res != 0) {
			#ifdef DEBUG
			fprintf(stderr, "erro ao criar o pipe %s\n", INPUT_PIPE);
			#endif
			exit(EXIT_FAILURE);
		}
	}
	#ifdef DEBUG
	printf("PIPE CRIADO\n");
	#endif
}




void* lePipe(){//certo
	int pipe_fd;
	int modoAbertura = O_RDONLY;
	char buf[tamanhoBuf + 1];
	char *aux1;
	char *aux2;
	char *nome;
	char *tempoTriagem;
	char *tempoAtendimento;
	char *prioridade;
	char nomes[20];
	int triagem;
	int atendimento;
	int prioridades;

	aux2 =malloc(sizeof (char*));
	#ifdef DEBUG
	printf("READ PIPE (PID = %d)\n", getpid());
	#endif
	
	while(1){//certo
		;
    	pipe_fd = open(INPUT_PIPE, modoAbertura);
		memset(buf, '\0', sizeof(buf));
		
		if (pipe_fd != -1) {
			if (read(pipe_fd, buf, tamanhoBuf)< 0){
				#ifdef DEBUG
				printf("erro ao ler o pipe\n");
				#endif
				terminar(1,1,1,1,1,1,1,1);
			}
			if (strcmp(buf, "sair") == 0){
				#ifdef DEBUG
				printf("ADMINISTRADOR SAIU DO SISTEMA\n");
				#endif
				
				
			}
			else if(strcmp(buf, "estatisticas")==0){
				printf("imprimir estatisticas\n");

			
				imprimeEstatisticas();
				
				
			} 
			else {
				
				pt = strtok (buf, "=");
				aux1=pt;
				
				while (pt != NULL) {
					aux2=pt;
					pt = strtok (NULL, "=");
					
					while (pt && *pt == '\040'){
						pt++;
					}
				}
		
				if (strcmp(aux1, "TRIAGE")==0){
					criaThreadsTriagem(atoi(aux2));
				}
				else if(strcmp(aux1, "ATENDIMENTO") ==0){
				
					char *horaInicio = asctime(localtime(&tempo));
					horaInicio[strlen(horaInicio) - 1] = 0;
					
					if((nome=strtok (aux2, " ")) != NULL){
						
						if ((tempoTriagem = strtok (NULL, " ")) != NULL){
							if((tempoAtendimento  = strtok (NULL, " ")) != NULL){
								prioridade = strtok (NULL, " ");
								
								size_t tam= strlen(nome);
								size_t i;
								int controlo=0;

								
								for (i=0; i<tam; i++){
									if (!isdigit(nome[i])){
										controlo=1;
										break;
									}
								}
								
								if (controlo == 1){
									strcpy(nomes,nome);
									triagem=atoi(tempoTriagem);;
									atendimento=atoi(tempoAtendimento);
									prioridades=atoi(prioridade);
									time(&chegadaSistema);
									insereQueue(fila,nomes,triagem, atendimento, prioridades,chegadaSistema);
								

										if (sigprocmask (SIG_UNBLOCK, &block_all, NULL)<0){
										fprintf(stderr, "Failed to unblock signals in worker thread.\n");
										}


								}
								else{
									int a;

									if (nome[tam] == '\n'){
										nome[tam]='\0';
									}
									for (a=1; a <= atoi(nome); a++){
										char novoNome[40];

										
										sprintf(novoNome,"Utente_%d", a);

										strcpy(nomes,novoNome);
										triagem=atoi(tempoTriagem);;
										atendimento=atoi(tempoAtendimento);
										prioridades=atoi(prioridade);
										time(&chegadaSistema);
										insereQueue(fila,nomes,triagem, atendimento, prioridades,chegadaSistema);
										numeroEntradas+=1;
										

										
									}
								}
						
								
							}
							else{
								#ifdef DEBUG
								printf("mensagem com erros!!\n");
								#endif
							}
						}
						else{
							#ifdef DEBUG
							printf("mensagem com erros!!\n");
							#endif
						}
					}
					else{
						#ifdef DEBUG
						printf("mensagem com erros!!\n");
						#endif
					}
					
					fflush(stdin);
					
    				

				}
				else{
					#ifdef DEBUG
					printf("Mensagem com erros\n");
					#endif
				}
			}
		}
		else {
			#ifdef DEBUG
			printf("leitura do pipe com sucesso %d  pipe_fd %d \n", getpid(), pipe_fd);
			#endif
			
		}
		time(&tempo);
		char *horaFim = asctime(localtime(&tempo));
		horaFim[strlen(horaFim) - 1] = 0;
		
		
		close(pipe_fd);
		
		fflush(stdin);
	}
	
	
}


struct queue *criaQueue(){
    struct queue *q = (struct queue*)malloc(sizeof(struct queue));
    q->primeiro = q->ultimo = NULL;
    return q;
}



void insereQueue(struct queue *q, char *nome, int triagem, int atendimento, int prioridades,time_t chegadaSistema){
    
    struct queueNO *temp = novoNo(nome, triagem, atendimento, prioridades,chegadaSistema);
 
    
    if (q->ultimo == NULL)
    {
       q->primeiro = q->ultimo = temp;
       return;
    }
 
    
    q->ultimo->proximo = temp;
    q->ultimo = temp;
}

struct queueNO *retiraQueue(struct queue *q){
    
    if (q->primeiro == NULL){
       return NULL;
    }
 
    
    struct queueNO *temp = q->primeiro;
    q->primeiro = q->primeiro->proximo;
 
   
    if (q->primeiro == NULL){
       q->ultimo = NULL;
    }
    return temp;
}

struct queueNO* novoNo(char nome[30], int triagem, int atendimento, int prioridades,time_t chegadaSistema){
	struct queueNO *temp = (struct queueNO*)malloc(sizeof(struct queueNO));
    strcpy(temp->nome,nome);
    temp->tempoTriagem= triagem;
    temp->tempoAtendimento = atendimento;
    temp->prioridade = prioridades;
    temp->chegadaSistema = chegadaSistema;
    temp->proximo = NULL;
    return temp; 
}



void lerFicheiro(){
	sigfillset(&unblock_hup);
	signal(SIGHUP, lerFicheiro);
	
	if (sigdelset(&unblock_hup, SIGHUP)<0){
		#ifdef DEBUG
		fprintf(stderr, "Failed to delete SIGHUP from signal block set.\n");
		#endif
	}
	if(sigdelset(&unblock_hup, SIGTERM)<0){
		#ifdef DEBUG
		fprintf(stderr, "Failed to delete SIGTERM from signal block set.\n");
		#endif
	}
	if (sigprocmask (SIG_BLOCK, &unblock_ctrlc, NULL)<0){
		#ifdef DEBUG
		fprintf(stderr, "Failed to apply mask on blocked signals.\n");
		#endif
	}

	FILE *fp;
	int a;
	char buff[255];

	fp = fopen(ficheiroConfiguracao, "r");
	if (fscanf(fp, "%s", buff)>0){
		pt = strtok (buff,"=");
		while (pt != NULL) {
			a = atoi(pt);
			pt = strtok (NULL, "=");
		}
		
		configuracao->threadsTriagem = a;
		#ifdef DEBUG
		printf("numero de threads na triagem: %d\n", configuracao->threadsTriagem);
		#endif
	}
	if (fscanf(fp, "%s", buff)>0){
		pt = strtok (buff,"=");
		while (pt != NULL) {
			a = atoi(pt);
			pt = strtok (NULL, "=");
		}
		
		configuracao->processosDoutor = a;
		#ifdef DEBUG
		printf("numero de processos doutor: %d\n", configuracao->processosDoutor);
		#endif
	}
	if (fscanf(fp, "%s", buff)>0){
		pt = strtok (buff,"=");
		while (pt != NULL) {
			a = atoi(pt);
			pt = strtok (NULL, "=");
		}
		
		configuracao->tempoTurno = a;
		#ifdef DEBUG
		printf("duracao de turno de cada processo doutor em segundos: %d\n", configuracao->tempoTurno);
		#endif
	}
	if (fscanf(fp, "%s", buff)>0){
		pt = strtok (buff,"=");
		while (pt != NULL) {
			a = atoi(pt);
			pt = strtok (NULL, "=");
		}
		
		configuracao->tamanhoFilaAtendimento = a;
		tamanhoMaximo = configuracao->tamanhoFilaAtendimento;
		#ifdef DEBUG
		printf("tamanho maximo da fila de mensagens: %d\n", configuracao->tamanhoFilaAtendimento);
		#endif
	}
	fclose(fp);
}



void iniciarMemorias() {
// criaçao de momoria partilhada
	if(criaMemoriaPartilhada() == 1) {
		#ifdef DEBUG
		printf("memoria partilhada com sucesso\n");
		#endif
	} else {
		#ifdef DEBUG
		printf("erro ao criar memoria partilhada\n");
		#endif
		terminar(1, 0, 1, 0, 0, 0,0,0);
	}
	
	
	inicializarBuffer();
	
}


//cria memoria partilhada
int criaMemoriaPartilhada() {
	//alocar estrutura na memoria partilhada
	printf("cria de memoria partilhada com pid %d\n", getpid());
	// criaçao de momoria partilhada
	if((shmid = shmget(IPC_PRIVATE, sizeof(memoriaPartilhadaSistema), IPC_CREAT | 0660)) < 0 ) {
		#ifdef DEBUG
		printf("erro ao criar memoria partilhada ID\n");
		#endif
		return 0;
	}
	//estaticas estaticasMemoriaMapeada;
	// memoria mapeada e configuraçao
	if((estaticasMemoriaMapeada = (estaticas) shmat(shmid, NULL, 0)) == (estaticas) -1 ) {
		#ifdef DEBUG
		printf("erro na criacao da memoria mapeada\n");
		#endif
		return 0;
	}

	return 1;
}



void limpaMemoriaPartilhada() {

	
	

		// remover memoria partilhada (IPC_RMID) 
		if (shmctl(shmid, IPC_RMID, NULL) <0) {
			#ifdef DEBUG
				printf("erro a limpar memoria partilhada\n");
			#endif
			exit(EXIT_FAILURE);
		}

	#ifdef DEBUG
	printf("memoria partilhada limpa\n");
	#endif
}

/**
	destroiMemoriaMapeada: memoria mapeada do ficheiro log
**/
void destroiMemoriaMapeada() {

	// destroi memoria mapeada do ficheiro
    if(munmap(estaticasMemoriaMapeada, MAX) <0) {
        close(mmapfd);
        #ifdef DEBUG
        	printf("Erro ao destroir memoria mapeada do ficheiro\n");
        #endif
        exit(EXIT_FAILURE);
    }

    #ifdef DEBUG
    	printf("memoria mapeada limpa\n");
    #endif

    // fecha o ficheiro
    close(mmapfd);
}
void destroiMMFregistos(){
	if(munmap(fichLog, tamanhoLog) <0) {
        close(descritorRegistos);
        #ifdef DEBUG
        	printf("Erro ao destroir memoria mapeada do ficheiro\n");
        #endif
        exit(EXIT_FAILURE);
    }

    #ifdef DEBUG
    	printf("memoria mapeada limpa\n");
    #endif

    // fecha o ficheiro
    close(descritorRegistos);
}

/**
	terminar: termina as threads, fecha socket, liberta a memoria, termina o processo estatisticas, e o processo principal
	* shm: 1 remove memoria partilhada, 0 caso contrario
	* sem: 1 fecha semaforos, 0 caso contrario
	* proc: 1 mata processo filho, 0 caso contrario
	* threadTriagem: 1 fecha threads triagem, 0 caso contrario
	* threadsDoutor: 1 fecha threads doutor, 0 caso contrario
	* mmap: 1 fecha memoria mapeada do ficheiro, 0 caso contrario
**/
void terminar(int shm, int sem, int proc, int threadTriagem,int processoDoutor, int mmap,int mmap2,int mq) {
	int i;
	int sig=0;

	waitpid(-1, NULL, 0);
	if (getpid() == pidConfiguracao){
		#ifdef DEBUG
			printf("\n\nAplicacao a fechar todas as configuracoes  %d\n", getpid());
		#endif
	}
	if(threadTriagem == 1) {
		// Shutdown todas as threaDS	
		//fazer wait de espera por cada uma das threads
		for(i = 0; i <= configuracao->threadsTriagem; i++) {
			pthread_detach(pthreadsTriagem[i]);
			if ( pthread_kill(pthreadsTriagem[i], sig) == 0){
				#ifdef DEBUG
					printf("THREADS Triagem %d terminado com sucesso\n", i);
				#endif
			}
			else{
				#ifdef DEBUG
					printf("erro ao terminar as threads triagem %d\n", i);
				#endif
			}
		}
	}
	
	// Kill processes
	if(proc == 1) {

		sleep(2);
		kill(pidEstatisticas, SIGKILL);
		#ifdef DEBUG
		printf("processo de estatisticas fechado \n");
		#endif
	}
	
	if(processoDoutor == 1) {

		sleep(2);
		int i = 1;
		for(i = 1;i<=configuracao->processosDoutor;i++){
			kill(pidDoutores, SIGKILL);
			#ifdef DEBUG
			printf("processo doutor fechado \n");
			#endif
		}
		
		
	}

	// remover memoria partilhada
	if(shm == 1) limpaMemoriaPartilhada();
	// remover memoria mapeada
	if(mmap == 1) destroiMemoriaMapeada();
	if(mmap2 == 1) destroiMMFregistos();
	if(mq==1){
		msgctl(mqid,IPC_RMID,NULL);
	}
	pthread_mutex_destroy(&lock);
	pthread_mutexattr_destroy(&attrmutex); 


	sleep(1);

	#ifdef DEBUG
	printf("Aplicacao fechada com sucesso\n");
	#endif
	// Shutdown
	exit(0);
}




void imprimeEstatisticas(){
	gravarFicheiroLog();
	#ifdef DEBUG
	printf("numero de pacientes triados: %d\n", estaticasMemoriaMapeada->numeroPacientesTriados);	
	printf("numero de pacientes atendidos: %d\n", estaticasMemoriaMapeada->numeroPacientesAtendidos	);
	printf("Tempo medio de espera antes do inicio da triagem : %lf \n", estaticasMemoriaMapeada->tempoMedioAntesTriagem);	
	printf("Tempo medio de espera entre o fim da triagem e o inicio do atendimento: %lf\n", estaticasMemoriaMapeada->tempoMedioAteAtendimento);
	printf("media do tempo total de cada utilizador gastou desde que chegou ao sistema ate sair: %lf\n",estaticasMemoriaMapeada->mediaTempoTotalUtilizador);
	#endif
}

int criarFicheiroMemoriaMapeada(){
	//------------ficheiro log de registos
	
	sprintf(registos, "registosSistema.txt");
	
	if ( (descritorRegistos = open(registos, O_RDWR | O_CREAT | O_TRUNC,0600)) < 0){
		fprintf(stderr,"can't creat %s for writing\n", registos);
		exit(1);
	}
	/* set size of output file */
	if (lseek(descritorRegistos, tamanhoLog , SEEK_SET) == -1)
	{
		fprintf(stderr,"lseek error\n");
		exit(1);
	}
	if (write(descritorRegistos, "", 1) != 1)
	{
		fprintf(stderr,"write error\n");
		exit(1);
	}
	if ( (fichLog = mmap(NULL, tamanhoLog, PROT_READ | PROT_WRITE,MAP_SHARED,descritorRegistos, 0)) == (caddr_t) -1)
	{
		fprintf(stderr,"mmap error for output\n");
		exit(1);
	}

	/*----------registar estatisticas-----------*/
	sprintf(RegistaEstatisticas, "registosEstatisticas.txt");
	if((mmapfd = open(RegistaEstatisticas, O_RDWR | O_CREAT | O_TRUNC,0600)) < 0){
		#ifdef DEBUG
			printf("erro ao criar escritor do ficheiro memoria mapeada\n");
		#endif
		close(mmapfd);
		
	}

	//set size of output file 
	if (lseek(mmapfd, sizeof(estaticasMemoriaMapeada) , SEEK_SET) == -1)
	{
		fprintf(stderr,"lseek error\n");
		exit(1);
	}
	if (write(mmapfd, "", 1) != 1)
	{
		fprintf(stderr,"write error\n");
		exit(1);
	}
	if ( (fichEstatisticas = mmap(NULL, sizeof(estaticasMemoriaMapeada), PROT_READ | PROT_WRITE,MAP_SHARED,mmapfd, 0)) == (caddr_t) -1)
	{
		fprintf(stderr,"mmap error for output\n");
		exit(1);
	}
   
   

	
	return 0;
}