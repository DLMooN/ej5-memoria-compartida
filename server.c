#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 1000

typedef struct{
	char nick[30];
	char texto[SIZE];
}msj_t;


int main( int argc, char *argv[] ){


	if(argc == 1 ){
		printf("Debe especificar la cantidad de clientes.\n");
		exit(1);
	}

	if( strcmp( argv[1] , "-h" ) == 0){
		puts ( "Debe ejecutar con el parametro de cuantos clientes van a entrar a la sala de chat." );
		exit(1);
	}

	int sval_leido;
	int cant_clientes =  atoi(argv[1]);
	int cant_clientes_actuales = 0;
	char clientes[cant_clientes][30]; // Array para guardar los nombres de los clientes

	printf("Se creara un server con %d clientes\n" , cant_clientes);


	int shmid;
	msj_t *mensaje;
	sem_t *sem_escritura;
	sem_t *sem_lectura[cant_clientes];
	sem_t *sem_aviso_msj_nuevo;
	int i;
	char buffer[30];

	// Consigo un ID unico de memoria compartida.
	shmid = shmget( (key_t)9876 , sizeof( msj_t ) , IPC_CREAT | IPC_EXCL | 0666 );
	if( shmid < 0 ){
		perror("shmget");
		exit(1);
	}


	// Apunto mensaje a la memoria compartida
	mensaje = (msj_t *)shmat( shmid , 0 , 0 );
	if( !mensaje ){
		perror("shmat");
		exit(1);
	}


	// Abro semaforo de escritura en memoria compartida en 1
	sem_escritura = sem_open( "/sem_escritura" , O_CREAT | O_EXCL , 0666 , 1 );
	if( sem_escritura == SEM_FAILED ){
		perror("sem_open");
		exit(1);
	}

	sem_aviso_msj_nuevo = sem_open( "/sem_aviso_msj_nuevo" , O_CREAT | O_EXCL , 0666 , 0 );
	if( sem_aviso_msj_nuevo == SEM_FAILED ){
		perror("sem_open");
		exit(1);
	}

	// Primer caracter de memoria compartida en *, cuando un cliente se conecta cambia el valor
	
	msj_t *inicio = mensaje;
	strcpy( mensaje->texto , "Servidor Iniciandose" );

	//Registracion de los clientes
	while( cant_clientes_actuales < cant_clientes ){
		while( strcmp( mensaje->texto , "Servidor Iniciandose" ) == 0 ){
			printf("Esperando clientes...(faltan %d clientes)\n", cant_clientes - cant_clientes_actuales);
			sleep(3);
		}
		printf("Cliente conectado: %s\n" , mensaje->texto );
		//mensaje = inicio;
		strcpy( clientes[cant_clientes_actuales] , mensaje->texto );
		cant_clientes_actuales++;
		printf("Cant Clientes Conectados: %d\n" , cant_clientes_actuales);
		printf("Cant Clientes faltantes: %d\n" , cant_clientes - cant_clientes_actuales );
		strcpy( mensaje->texto , "Servidor Iniciandose");
		sem_post(sem_escritura); //permite otro cliente conectarse
	
	}


	// Semaforo de lectura para los clientes
	for( i = 0 ; i < cant_clientes ; i++ ){
		sem_lectura[i] = sem_open( clientes[i] , O_CREAT | O_EXCL , 0666 , 0);
	}


	// Termina la registracion y empieza la sala de chat
	strcpy( mensaje->texto , "SALA DE CHAT INICIADA" );
	printf("Clientes conectados:\n");
	for( i = 0 ; i < cant_clientes ; i++ ){
		printf( "%s, " , clientes[i]);
	}


	while(1){
		sem_wait(sem_aviso_msj_nuevo);

		// Mensaje de salida?

		if( strcmp( mensaje->texto , ":quit" ) == 0 ){
			// Busca el cliente
			for( i = 0 ; i < cant_clientes_actuales ; i ++ ){

				//Encuentra el cliente
				if( strcmp( mensaje->nick , clientes[i]) == 0 ){
					// Cierra el semaforo del cliente
					sem_close(sem_lectura[i]);
					sem_unlink(clientes[i]);
					printf("Cliente: %s desconectado." , clientes[i]);

					//Avisa que se desconecto un cliente
					strcpy( mensaje->nick , "Server");
					sprintf( buffer , "%s desconectado." , clientes[i] );
					strcpy( mensaje->texto , buffer );

					// Remueve el cliente de vector

					for( i ; i < cant_clientes_actuales -1 ; i++){
						
						strcpy( clientes[i] , clientes[ i + 1 ] );
						sem_lectura[i] = sem_lectura[ i + 1 ];
					}
					break;
				}
				
			}

			cant_clientes_actuales--;

			for( i = 0 ; i < cant_clientes_actuales ; i++ ){
				sem_post(sem_lectura[i]);
			}

			while(1){
				for( i = 0 ; i < cant_clientes_actuales ; i++ ){
					sem_getvalue( sem_lectura[i] , &sval_leido );
					if(sval_leido != 0 )
						break;
				}
				if(sval_leido == 0 )
					break;
			}

			sem_post( sem_escritura );

			if(cant_clientes_actuales == 1){

				strcpy( mensaje->nick , "Server" );
				strcpy( mensaje->texto , ":end" );
				sem_post( sem_lectura[0] );
				break;
			}



		}

		// Mensaje normal

		else{

			// Avisa que hay mensaje nuevo a los clientes que no mandaron el mensaje
			for( i = 0 ; i < cant_clientes_actuales ; i++ ){
				if( strcmp( clientes[i] , mensaje->nick ) != 0)
					sem_post(sem_lectura[i]);

			}

			// Espera a que todos lean el mensaje

			while(1){
				for( i = 0 ; i < cant_clientes_actuales ; i++ ){
					sem_getvalue( sem_lectura[i] , &sval_leido );
					if(sval_leido != 0 )
						break;
				}
				if(sval_leido == 0 )
					break;
			}
			// Abre semáforo para nueva escritura
			sem_post( sem_escritura );
		}
	}

	// Termina la sala de chat
	printf("Sala de chat terminada\n");

	shmdt( mensaje );
	shmctl( shmid , IPC_RMID , NULL );
	sem_close(sem_escritura);
	sem_unlink("/sem_escritura");
	sem_close(sem_aviso_msj_nuevo);
	sem_unlink("/sem_aviso_msj_nuevo");

	return 0;
}
