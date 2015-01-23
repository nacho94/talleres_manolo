#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

int capacidad_taller = 10;
int numero_macanicos_chapa = 2;
int numero_macanicos_motor = 2;

pthread_mutex_t fichero;//semaforo del log

struct vehiculo {
	int matricula;
	char* averia;
	int atendido;
};

struct mecanico {
	int identificador;
	char* puesto;
	int ocupado;
	pthread_t thread;
};

struct vehiculo** lista_vehiculos;
struct mecanico** lista_mecanicos;

int contador_vehiculos;

FILE *logFile;

//• Fichero de log (FILE * logFile);
const char *logFileName="registroTaller.log";

void writeLogMessage(char *id, char *msg) {

// Calculamos la hora actual
time_t now = time(0);
struct tm *tlocal = localtime(&now);
char stnow[19];
strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);
// Escribimos en el log

logFile = fopen(logFileName, "a");
fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
fclose(logFile);
}

int calcularAleatorios(int min, int max){
	srand(getpid());
	return rand() % (max-min+1) + min;
}

void
fatal(char* msj) {
	printf("FATAL: %s\n",msj);
	exit(1);
}

struct mecanico* 
crear_mecanico(int identificador, char* puesto) {
	struct mecanico* m;

	m = (struct mecanico*)malloc(sizeof(struct mecanico));
	m->identificador = identificador;
	m->ocupado = 0;

	m->puesto = (char*)malloc(strlen(puesto));
	strcpy(m->puesto,puesto);

	return m;
}

struct vehiculo*
crear_vehiculo(int matricula, char* averia, int atendido) {
	struct vehiculo* v;
	//reservamos la memoria
	v = (struct vehiculo*)malloc(sizeof(struct vehiculo));
	v->matricula = matricula;
	v->atendido = atendido;

	v->averia = (char*)malloc(strlen(averia));
	strcpy(v->averia,averia);
	
	return v;
} 

void
destruir_vehiculo(struct vehiculo* v) {
	free(v->averia);
	free(v);
}

void
crear_lista_vehiculos(int tamanyo) {
	lista_vehiculos = (struct vehiculo**)malloc(sizeof(struct vehiculo*) * tamanyo);
}

void
inicializar_lista_vehiculos(int tamanyo) {
	int i = 0;
	for(i = 0; i<tamanyo; i++) {
		lista_vehiculos[i] = NULL;
	}
}

int
insertar_vehiculo(struct vehiculo* v, int tamanyo) {
	int i = 0;
	for(i = 0; i<tamanyo; i++) {
		if(lista_vehiculos[i] == NULL) {
			lista_vehiculos[i] = v;
			return i;
		}
	}

	return -1;
}

void 
retirar_vehiculo(int posicion) {
	if(lista_vehiculos[posicion]!=NULL) {
		destruir_vehiculo(lista_vehiculos[posicion]);
		lista_vehiculos[posicion] = NULL;
	}else{
		fatal("no hay un vehiculo en la posicion indicada");
	}
}
void 
crear_lista_mecanicos(int tamanyo) {
	lista_mecanicos = (struct mecanico**)malloc(sizeof(struct mecanico*) * tamanyo);
}

void
inicializar_lista_mecanicos(int tamanyo, char* puesto,int offset) {
	int i = offset;
	for(; i<tamanyo + offset; i++) {
		lista_mecanicos[i] = crear_mecanico(i,puesto);
	}
}

void*
cliente(void* data) {
	return 0;
}

void* 
mecanico(void* data) {
	while(1) {

	}
	return 0;
}

void
poner_a_trabajar_mecanicos(int numero) {
	int i = 0;
	for(; i<numero; i++) {
		pthread_create(&lista_mecanicos[i]->thread,NULL,mecanico,lista_mecanicos[i]);
	}
}

void
esperar_por_los_mecanicos(int numero) {
	int i = 0;
	for(; i<numero; i++) {
		pthread_join(lista_mecanicos[i]->thread,NULL);
	}
}

void
manejadora(int sig) {
	char* msj;
	pthread_mutex_lock(&fichero);
	sprintf(msj,"cuyo numero es %d",(int)sig);
	writeLogMessage("Recibida la señal",msj);

	pthread_mutex_unlock(&fichero);
}

int
main(void) {
	signal(SIGUSR1,manejadora);
	pthread_mutex_init(&fichero,NULL);
	crear_lista_vehiculos(capacidad_taller);
	inicializar_lista_vehiculos(capacidad_taller);
	crear_lista_mecanicos(numero_macanicos_motor + numero_macanicos_chapa);
	inicializar_lista_mecanicos(numero_macanicos_motor,"motor",0);
	inicializar_lista_mecanicos(numero_macanicos_chapa,"chapa",numero_macanicos_motor);

	pthread_mutex_lock(&fichero);	
	writeLogMessage("Mecanicos empezando", "a trabajar");
	pthread_mutex_unlock(&fichero);

	poner_a_trabajar_mecanicos(numero_macanicos_chapa + numero_macanicos_motor);
	
	while(1) {
		pause();
	}
	return 0;
}
