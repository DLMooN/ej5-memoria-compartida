#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define SIZE 1000

typedef struct{
	char nick[30];
	char texto[SIZE];
}msj_t;

typedef struct{
	msj_t *msj_ptr;
	sem_t *sem_ptr;
	char cliente[30];

}escucha_param_t;

void *escucha(void *param){
	escucha_param_t *parametros =(escucha_param_t *)param;
	sem_t *sem_lectura = parametros->sem_ptr;
	msj_t *mensaje = parametros->msj_ptr;
	char cliente[30];
	strcpy( cliente , parametros->cliente );	
	while(1){
		sem_wait( sem_lectura );
		if( strcmp( mensaje->nick , "Server") == 0 && strcmp(mensaje->texto , ":end") == 0){
			printf("Sala de chat finalizada.\n");
			sem_close( sem_lectura );
			sem_unlink( cliente );
			exit(1);
		}
		printf("%s: %s\n" , mensaje->nick , mensaje->texto );
	}
}

//--------------------------------------------------------------

int main( int argc, char *argv[] ){

	if(argc == 1){
		printf("Debe especificar nick del usuario.");
		exit(1);
	}

	if( strcmp( argv[1] , "-h" ) == 0 ){
		puts( "Debe ejecutar el cliente con el nick que usará como parametro.");
		puts( "Además, debe ejecutar el server primero." );
		exit(1);
	}

	char cliente[30];
	strcpy( cliente , argv[1] );
	//cliente[ sizeof( argv[ 1 ] + 1 ) ] = '\0';
	int shmid;
	sem_t *sem_escritura;
	sem_t *sem_lectura;
	sem_t *sem_aviso_msj_nuevo;
	msj_t *mensaje;


	shmid = shmget( (key_t)9876 , sizeof( msj_t ) , 0666 );
	if( shmid < 0 ){
		perror("shmget");
		exit(1);
	}
	mensaje = (msj_t *)shmat( shmid , 0 , 0 );
	if( !mensaje ){
		perror("shmat");
		exit(1);
	}


	sem_escritura = sem_open( "/sem_escritura" , 0 );

	
	sem_wait(sem_escritura);

	strcpy( mensaje->texto , cliente );

	printf("Conectado, esperando a otros clientes...\n");

	while(strcmp( mensaje->texto, "SALA DE CHAT INICIADA") != 0){
		usleep(1000);
	}
	sem_aviso_msj_nuevo = sem_open( "/sem_aviso_msj_nuevo" , 0 );
	sem_lectura = sem_open( cliente , 0 );
	printf( "%s\n" , mensaje->texto );

	pthread_t hilo;

	// Crea thread para leer mensajes de otros clientes.
	escucha_param_t escucha_parametros;
	escucha_parametros.msj_ptr = mensaje;
	escucha_parametros.sem_ptr = sem_lectura;
	strcpy( escucha_parametros.cliente , cliente );
	pthread_create( &hilo , NULL , escucha , (void *) &escucha_parametros );


	// Mensajes normales, deja la sala con ":quit"
	char buffer[300];

	while(1){
		fgets( buffer , 300 , stdin );
		if( ( strlen(buffer) > 0 ) && ( buffer[ strlen( buffer ) -1 ] == '\n' ))
			buffer[strlen(buffer) -1] = '\0';
		sem_wait(sem_escritura);
		strcpy( mensaje->texto , buffer );
		strcpy( mensaje->nick , cliente );
		sem_post(sem_aviso_msj_nuevo);
		if( strcmp( buffer , ":quit" ) == 0 )
			break;
	}

	sem_close( sem_lectura );
	sem_unlink( cliente );
	return 0;
}

