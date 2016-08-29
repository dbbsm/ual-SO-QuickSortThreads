#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	long num = atol(argv[1]); // converte string em long

	long* numeros = malloc(num * sizeof(long int));//array para guardaros numeros gerados

	long r;

	long i=0;


	while(i<num)
	{
		r = rand();

		if(r<RAND_MAX)
		{
			numeros[i] = r;
			i++;
		}
	}

	for(long j=0;j<num;j++)
	{
		printf("valor %li = %li\n",j,numeros[j]);
	}

// criacao do ficheiro

	char* nomeficheiro = malloc(100* sizeof(char));

	FILE *in;

	strcpy(nomeficheiro, argv[1]); //copia a string para o array
	strcat(nomeficheiro, "-numeros-aleatorios"); //adiciona a string ao array

	printf("o nome do ficheiro vai ser: %s\n", nomeficheiro);

	in = fopen(nomeficheiro,"w");

	if(in == NULL)
	{
		printf("O ficheiro nao foi aberto\n");
		exit(1);

	}

	for (int i = 0; i < num; ++i)
	{
		fprintf(in, "%li\n",numeros[i] );
	}
	fclose(in);

}