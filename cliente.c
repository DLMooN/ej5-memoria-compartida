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

	if(argc == 1){
		printf("Debe especificar nick del usuario.");
		exit(1);
	}


	char cliente[30];
	strcpy( cliente , argv[1] );
	int shmid;
	sem_t *sem_escritura;
	char *mensaje;


	shmid = shmget( (key_t)9876 , sizeof(char *) * SIZE , IPC_CREAT | 0666 );
	if( shmid < 0 ){
		perror("shmget");
		exit(1);
	}

	mensaje = (char *)shmat( shmid , NULL , 0 );
	if( !mensaje ){
		perror("shmat");
		exit(1);
	}

	

	sem_escritura = sem_open( "/sem_escritura" , 0 );

	printf("Semaforo de escritura en: %d\n" , sem_getvalue(sem_escritura, NULL));
	sem_wait(sem_escritura);

	mensaje[0] = 'a';
	printf("Conectado\n");

	return 0;
}
