#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>

#include <sys/shm.h>
#include <sys/timeb.h>
#include <sys/wait.h> 

#include <sys/shm.h>
#include <sys/ipc.h>

#include <sys/types.h>
#include <unistd.h>


// variaveis globais
long int * numerosAordenar;// array com os numeros a ordenar

long int Nr_processosAusar;// nr maximo de processos que vamos usar

long int *Nr_processosUsados;// nr de processos usados ate ao momento

float time_result; //resultado da diferença do tempo

struct timeb tempo_inicial, tempo_final; //variaveis que levam o tempo de inicio do quicksort e o de fim

//chaves para os segmentos de memoria a partilhar nomeadamente o array e o nr de processos ja utilizados
key_t keyArray = 50000;
key_t key_contador_processosUsados = 60000;

sem_t semaforo;//semaforo para controlo de concorrencia

long int shmid_array;

long int shmid_contador;


// declaraçao de funçoes usadas no main
void Ler_ficheiro(char * nomeFicheiro, long int quantidadeAordenar);
void escrever_ficheiro_ordenado(char* nomeFicheiro, long int quantidadeAordenar);
void QuickSort(long int * numeros, long int inicioVet, long int fimVet);
void Print_Array(long int * numerosAordenar, long int quantidadeAordenar);


void QuickSort(long int * numeros, long int inicioVet, long int fimVet)
{

	long int pivot, first, last, temporario;

	pid_t child_pid;

	if(inicioVet < fimVet)
	{

		pivot=numeros[inicioVet];
		first = inicioVet;
		last= fimVet;

		while(first<last)
		{
			while(numeros[first]<=pivot)
			{
				first++;
			}

			while(pivot<numeros[last])
			{
				last--;
			}

			if (first < last)
			{
				temporario=numeros[first];
				numeros[first]= numeros[last];
				numeros[last]=temporario;
				
			}
		}


		numeros[inicioVet] = numeros[last];
		numeros[last] = pivot;
		

		//controlar o acesso ao contador de processos
		sem_wait(&semaforo);
                
		if((*Nr_processosUsados) < Nr_processosAusar)
		{
			(*Nr_processosUsados)++;//incrementacao do nr de processos usados

			sem_post(&semaforo);
			if ((child_pid=fork())==0)//criacao de um processo filho
			{

				QuickSort(numeros,last+1,fimVet);

				exit(0);
			}
			else
			{
				//processo pai fica responsavel pela ordenacao da outra metade do array
				QuickSort(numeros, inicioVet,last-1);

				//espera pelo filho
				waitpid(child_pid,NULL,0);

			}

			

		}
		else
		{
			//se o numero de processos maximo ja foi atingido a ordenacao tem de ser feita toda num unico processo
			sem_post(&semaforo);
			QuickSort(numeros,inicioVet,last-1);
			QuickSort(numeros,last+1,fimVet);	
		}

	}

}

void escrever_ficheiro_ordenado(char* nomeFicheiro, long int quantidadeAordenar)
{

	FILE *in;

	strcat(nomeFicheiro, "-ordenado-processos"); //adiciona a string ao array de caracteres

	in = fopen(nomeFicheiro,"w");

	if(in == NULL)
	{
		printf("O ficheiro nao foi aberto\n");
		exit(1);

	}

	for (int i = 0; i < quantidadeAordenar; i++)
	{
		fprintf(in, "%li\n",numerosAordenar[i]);
	}
	fclose(in);

}

void Ler_ficheiro(char * nomeFicheiro, long int quantidadeAordenar)
{
	FILE *out;

	//printf("%s\n\n", nomeFicheiro); //imprime o nome do ficheiro


	out = fopen(nomeFicheiro,"r"); //abertura do ficheiro para ler

	for (int i = 0; i < quantidadeAordenar; i++)
	{
		fscanf(out, "%li\n", &numerosAordenar[i]);
		//printf("o numero lido foi: %li\n", numerosAordenar[i]);
	}
}
void Print_Array(long int * numerosAordenar, long int quantidadeAordenar)
{
	printf("\n\n\tArray Ordenado: \n\n");

	for (int i = 0; i < quantidadeAordenar; i++)
	{
		printf("%li\n", numerosAordenar[i]);
	}

	printf("Tempo de corrida: %.3f milisegundos\n", time_result);
}


int main(int argc, char *argv[])
{
	//alocacao de memoria para guardar a variavel com o nome do ficheiro
	char* nomeFicheiro = malloc(100 * sizeof(char));

	//copia do valor de argv[1] para a variavel nome do ficheiro pois vamos mais a frente manipular argv[1] para saber a quantidade a ordenar
	strcpy(nomeFicheiro,argv[1]);

	//manipulacao de argv[1] para obter a quantidade a ordenar
	char* String_daQuantidade = strtok(argv[1],"-"); //copia so da quantidade dos numeros a ordenar ainda com valor de string

	long int quantidadeAordenar = atol(String_daQuantidade); //quantidade de numeros a ordenar com valor long int

	//depois de sabermos a quantidade que vamos ordenar alocamos a memoria necessaria
	numerosAordenar = (long int*) malloc(quantidadeAordenar* sizeof(long int));

	//quantidade de processos a usar
	Nr_processosAusar = (long int) atol(argv[2]);//quantidade de processos a usar

	//printf("%li\n",Nr_processosAusar);

	//cria-se os segmentos de memória
	if ((shmid_array = shmget(keyArray,sizeof(long int)*quantidadeAordenar, IPC_CREAT | 0666)) < 0){ 
		perror("shmget");
		exit(1); 
        }
    
	if ((shmid_contador = shmget(key_contador_processosUsados,sizeof(long int), IPC_CREAT | 0666)) < 0){ 
		perror("shmget"); 
		exit(1); 
	}        
   
	//aloca-se o array a ordenar e o contador aos respectivos segmentos de memoria 
	if ((numerosAordenar = shmat(shmid_array, NULL, 0)) == (long int *) -1){ 
		perror("shmat"); 
		exit(1); 
	}

	if ((Nr_processosUsados = shmat(shmid_contador, NULL, 0)) == (long int *) -1) { 
		perror("shmat"); 
		exit(1); 
	}

	Ler_ficheiro(nomeFicheiro, quantidadeAordenar);

	*Nr_processosUsados =1;

	/*-----------------------------------QuickSort---------------------------------*/
	ftime(&tempo_inicial);

	sem_init(&semaforo, 1, 1);  

	QuickSort(numerosAordenar,0,quantidadeAordenar-1);


	ftime(&tempo_final);

	time_result= ((float) tempo_final.time + ((float) tempo_final.millitm * 0.001)) - ((float) tempo_inicial.time + ((float) tempo_inicial.millitm * 0.0001));

	escrever_ficheiro_ordenado(nomeFicheiro, quantidadeAordenar);
	

	//Print_Array(numerosAordenar, quantidadeAordenar);

	printf("Tempo de Corrida: %.3f segundos\nNumero de processos: %li\nNumero de inteiros: %li\n",time_result,Nr_processosAusar, quantidadeAordenar);
	
	//fecha os segmentos de memoria
	shmctl (shmid_array, IPC_RMID, 0);
	shmctl (shmid_contador, IPC_RMID, 0);
}