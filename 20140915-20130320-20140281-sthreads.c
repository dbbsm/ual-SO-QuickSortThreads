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
long int * numerosAordenar; // array com os numeros a ordenar

pthread_t * arrayThreads; //array com as threads que vamos usar

long int Nr_threadsAusar; // nr maximo de threads que vamos usar

long int Nr_threadsUsadas=1; // nr de threads ja usadas começa a 1 pois consideramos que o programa a correr e a thread principal

float time_result; //resultado da diferença do tempo

struct timeb tempo_inicial, tempo_final; //variaveis que levam o tempo de inicio do quicksort e o de fim

sem_t semaforo;

// struct que e utilizada para passar os argumentos como o array o inicio e o fim do mesmo para a thread quando esta e criada
struct Args_QuickSort
{
	long int * numeros;

	long int inicioVet;

	long int fimVet;

};

// declaraçao de funçoes usadas no main

void Ler_ficheiro(char * nomeFicheiro, long int quantidadeAordenar);
void escrever_ficheiro_ordenado(char* nomeFicheiro,  long int quantidadeAordenar);
void QuickSort(long int * numeros, long int inicioVet, long int fimVet);
void Print_Array(long int * numerosAordenar, long int quantidadeAordenar);

// funçao responsavel por instanciar a funçao quicksort na thread
void* QuickSort_Thread(void * args)
{
	struct Args_QuickSort * parametros = (struct Args_QuickSort*) args;

	QuickSort(parametros->numeros,parametros->inicioVet,parametros->fimVet);

	pthread_exit(0);
}

void QuickSort(long int * numeros, long int inicioVet, long int fimVet)
{

	long int pivot, first, last, temporario;

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
		
		//controlar o acesso ao contador de threads
		sem_wait(&semaforo);
		if(Nr_threadsUsadas < Nr_threadsAusar)
		{
			
			long int Nr_threadCriada = Nr_threadsUsadas;
           
            Nr_threadsUsadas++; //incrementa o consumo de threads

            sem_post(&semaforo);
            //passagem dos argumentos do quicksort para a struct
			struct Args_QuickSort args ={numeros,last+1,fimVet};

			// envia a struct e o metodo que chama o quicksort com os parametros enviados na struct
			pthread_create(&arrayThreads[Nr_threadCriada], NULL, &QuickSort_Thread, &args);

			QuickSort(numeros,inicioVet,last-1);

			//espera pelo termino da thread criada como o segundo argumento esta a null quer dizer que nao vaireceber nenhum parametro de retorno
			pthread_join(arrayThreads[Nr_threadCriada],NULL);

		}
		else
		{
			sem_post(&semaforo);
			QuickSort(numeros,inicioVet,last-1);
			QuickSort(numeros,last+1,fimVet);	
		}

	}

}

//funcao que escreve o novo ficheiro mas ordenado
void escrever_ficheiro_ordenado(char* nomeFicheiro,  long int quantidadeAordenar)
{

	FILE *in;

	strcat(nomeFicheiro, "-ordenado-threads"); //adiciona a string ao array de caracteres

	in = fopen(nomeFicheiro,"w"); // abertura do ficheiro para escrita

	if(in == NULL)
	{
		printf("O ficheiro nao foi aberto\n");
		exit(1);

	}

	for (int i = 0; i < quantidadeAordenar; ++i)
	{
		fprintf(in, "%li\n",numerosAordenar[i] );
	}
	fclose(in);

}

//funcao responsavel por ler o ficheiro que esta desordenado
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

//funcao responsavel por escrever o array ordenado no ecra( e necessario activa la no main para que ela seja executada)
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

	//leitura do ficheiro a ordenar
	Ler_ficheiro(nomeFicheiro,quantidadeAordenar);


	long int threads = atol(argv[2]);
	Nr_threadsAusar = threads;//quantidade de threads a usar

	//depois de saber o numero de threads a usar temos que alocar memoria para o array das threads
	arrayThreads = malloc( Nr_threadsAusar * sizeof(pthread_t));


	

	/*-----------------------------------QuickSort---------------------------------*/
	ftime(&tempo_inicial);//obtencao do tempo inicial

	sem_init(&semaforo, 1, 1);  

	QuickSort(numerosAordenar,0,quantidadeAordenar-1);//chamada a primeira vez o quicksort


	ftime(&tempo_final);//obtencao do tempo final

	//resultado da diferenca do tempo
	time_result= ((float) tempo_final.time + ((float) tempo_final.millitm * 0.001)) - ((float) tempo_inicial.time + ((float) tempo_inicial.millitm * 0.0001));

	//escreve o ficheiro com o array ja ordenado
	escrever_ficheiro_ordenado(nomeFicheiro, quantidadeAordenar);
	

	//Print_Array(numerosAordenar,quantidadeAordenar);

	printf("Tempo de Corrida: %.3f segundos\nNumero de Threads: %li\nNumero de inteiros: %li\n",time_result,Nr_threadsAusar, quantidadeAordenar);
	
}