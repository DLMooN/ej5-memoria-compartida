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

	int shmid;
	char *mensaje;
	sem_t *sem_escritura;
	char default_m = '*';
	int i;

	shmid = shmget( (key_t)9876 , SIZE * sizeof(char *), IPC_CREAT | IPC_EXCL | 0666 );
	if( shmid < 0 ){
		perror("shmget");
		exit(1);
	}

	mensaje = (char *)shmat( shmid , NULL , 0 );
	if( !mensaje ){
		perror("shmat");
		exit(1);
	}

	sem_escritura = sem_open( "/sem_escritura" , O_CREAT , 0666 , 1 );

	mensaje[0] = '*';
	
	while(cant_clientes_actuales < cant_clientes){
		if( mensaje[0] != '*'){
			printf("Cliente conectado");
			cant_clientes_actuales++;
			printf("Cant Clientes Conectados: %d" , cant_clientes_actuales);
			printf("Cant Clientes faltantes: %d" , cant_clientes_actuales - cant_clientes );
			memcpy( mensaje , &default_m , sizeof(default_m) );
			sem_post(sem_escritura);
		}
	}


	return 0;
}
