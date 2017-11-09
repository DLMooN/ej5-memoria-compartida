#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 100


int main( int argc, char *argv[] ){

	if(argc == 1 ){
		printf("Debe especificar la cantidad de clientes.\n");
		exit(1);
	}

	int cant_clientes =  atoi(argv[1]);
	int cant_clientes_actuales = 0;
	printf("Se creara un server con %d clientes" , cant_clientes);

	int shmid;
	char *mensaje;
	sem_t *sem_escritura;
	char default_m = '*';
	int i;

	// Consigo un ID unico de memoria compartida.
	shmid = shmget( (key_t)9876 , SIZE * sizeof(char *), IPC_CREAT | IPC_EXCL | 0666 );
	if( shmid < 0 ){
		perror("shmget");
		exit(1);
	}

	// Apunto mensaje a la memoria compartida
	mensaje = (char *)shmat( shmid , NULL , 0 );
	if( !mensaje ){
		perror("shmat");
		exit(1);
	}

	// Abro semaforo de escritura en memoria compartida en 1
	sem_escritura = sem_open( "/sem_escritura" , O_CREAT , 0666 , 1 );

	// Primer caracter de memoria compartida en *, cuando un cliente se conecta cambia el valor
	mensaje[0] = '*';
	

	while( cant_clientes_actuales < cant_clientes ){
		while( mensaje[0] == '*' ){
			printf("Esperando clientes...(faltan %d clientes\n", cant_clientes - cant_clientes_actuales);
			sleep(1);
		}
		printf("Cliente conectado");
		cant_clientes_actuales++;
		printf("Cant Clientes Conectados: %d\n" , cant_clientes_actuales);
		printf("Cant Clientes faltantes: %d\n" , cant_clientes - cant_clientes_actuales );
		mensaje[0] = '*';
		sem_post(sem_escritura); //permite otro cliente conectarse
	
	}


	return 0;
}
