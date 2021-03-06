#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define DEBUG 0

int capacidad_taller = 10;
int numero_macanicos_chapa = 2;
int numero_macanicos_motor = 2;

pthread_mutex_t fichero;//semaforo del log
pthread_mutex_t acceso_lista_vehiculos;

struct vehiculo {
	int matricula;
	char* averia;
	int atendido;
	int posicion_en_taller;
	pthread_mutex_t acceso;
	pthread_mutex_t espera_cliente;
};

struct mecanico {
	int identificador;
	char* puesto;
	int ocupado;
	pthread_t thread;
	int contador_cafe;
	int thread_terminated;
};

struct vehiculo** lista_vehiculos;
struct mecanico** lista_mecanicos;

int contador_vehiculos;

FILE *logFile;

//• Fichero de log (FILE * logFile);
const char *logFileName="registroTaller.log";

void writeLogMessage(char *id, char *msg,int m,int v) {
	pthread_mutex_lock(&fichero);
	// Calculamos la hora actual
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);
	// Escribimos en el log

#if DEBUG == 1
	printf("[%s] %s: %s, %d\n", stnow, id, msg, n);
#else	
	if(m !=-1 && v != -1){
		logFile = fopen(logFileName, "a");
	fprintf(logFile, "[%s] %s_%d: %s_%d\n", stnow, id, m, msg, v);
	fclose(logFile);
	}
	if(m != -1 && v == -1){
		logFile = fopen(logFileName, "a");
	fprintf(logFile, "[%s] %s_%d: %s\n", stnow, id, m, msg);
	fclose(logFile);

	}
	if(m == -1 && v != -1){
		logFile = fopen(logFileName, "a");
	fprintf(logFile, "[%s] %s: %s_%d\n", stnow, id, msg, v);
	fclose(logFile);

	}
	if(m == -1 && v == -1) {
		logFile = fopen(logFileName, "a");
		fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
		fclose(logFile);
	}
#endif

	pthread_mutex_unlock(&fichero);
}

int
calcularAleatorios(int min, int max){
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
	m->contador_cafe = 0;
	m->thread_terminated = 0;

	return m;
}

void
destruir_mecanico(struct mecanico* m) {
	free(m->puesto);
	free(m);
}

void
liberar_recursos_mecanicos(int numero){
	int i = 0;
	for(; i<numero; i++) {
		if(lista_mecanicos[i] != NULL){
			destruir_mecanico(lista_mecanicos[i]);
		}
	}
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

	v->posicion_en_taller = -1;
	pthread_mutex_init(&v->acceso,NULL);
	pthread_mutex_init(&v->espera_cliente,NULL);
	
	return v;
} 

void
destruir_vehiculo(struct vehiculo* v) {
	free(v->averia);
	free(v);
}

void
liberar_recursos_vehiculos(int numero){
	int i = 0;
	for(; i<numero; i++) {
		if(lista_vehiculos[i] != NULL){
			destruir_vehiculo(lista_vehiculos[i]);
		}
	}
}

int
esta_siendo_atendido(struct vehiculo* v){
	pthread_mutex_lock(&v->acceso);
	int atendido = v->atendido;
	pthread_mutex_unlock(&v->acceso);
	return atendido;
}

void
atendiendo(struct vehiculo* v,int atendido) {
	pthread_mutex_lock(&v->acceso);
	v->atendido = atendido;
	pthread_mutex_unlock(&v->acceso);
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
	pthread_mutex_lock(&acceso_lista_vehiculos);
	int i = 0;
	for(i = 0; i<tamanyo; i++) {
		if(lista_vehiculos[i] == NULL) {
			lista_vehiculos[i] = v;
			v->posicion_en_taller = i;
			pthread_mutex_unlock(&acceso_lista_vehiculos);
			return i;
		}
	}
	pthread_mutex_unlock(&acceso_lista_vehiculos);
	return -1;
}

void 
retirar_vehiculo(int posicion) {
	pthread_mutex_lock(&acceso_lista_vehiculos);
	if(lista_vehiculos[posicion]!=NULL) {
		destruir_vehiculo(lista_vehiculos[posicion]);
		lista_vehiculos[posicion] = NULL;
	}else{
		fatal("no hay un vehiculo en la posicion indicada");
	}
	pthread_mutex_unlock(&acceso_lista_vehiculos);
}

struct vehiculo*
buscar_vehiculo_que_mas_lleva_esperando(char* averia) {
	pthread_mutex_lock(&acceso_lista_vehiculos);
	struct vehiculo* resultado = NULL;
	int i = 0;
	for(; i<capacidad_taller; i++) {
		if(lista_vehiculos[i] != NULL && esta_siendo_atendido(lista_vehiculos[i]) == 0 && strcmp(lista_vehiculos[i]->averia,averia) == 0) {
			if(resultado != NULL) {
				if(lista_vehiculos[i]->matricula < resultado->matricula) {
					resultado = lista_vehiculos[i];
				}
			} else {
				resultado = lista_vehiculos[i];
			} 
		}
	}

	pthread_mutex_unlock(&acceso_lista_vehiculos);
	return resultado;
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

int
me_quedo_esperando_otro_poco(){
	//0 me voy, el resto me quedo
	//esto cumple con el 20% de probabilidad de irme
	return calcularAleatorios(0,4);
}

void*
cliente(void* data) {

	struct vehiculo* v = (struct vehiculo*)data;
	writeLogMessage("Entrada vehiculo",v->averia,v->matricula,-1);
	sleep(10);

	espero_otro_poco://se que no se debe usar pero...

	if(esta_siendo_atendido(v) != 1 && esta_siendo_atendido(v) != 2) {
		if(me_quedo_esperando_otro_poco() != 0) {
			//espero
			sleep(5);
			goto espero_otro_poco;
		} else {
			writeLogMessage("Salida vehiculo","me canse de esperar",v->matricula,-1);
			retirar_vehiculo(v->posicion_en_taller);
			pthread_exit(NULL);
		}
	} else {
		pthread_mutex_lock(&v->espera_cliente);
		pthread_mutex_unlock(&v->espera_cliente);
		writeLogMessage("Salida cliente","vehiculo",-1,v->matricula); //TODO: escribir el mensaje
		retirar_vehiculo(v->posicion_en_taller);
		pthread_exit(NULL);
		//a partir de esta linea v no es valido
	}

	return 0;
}

void
poner_a_esperar_cliente(struct vehiculo* v) {
	pthread_t c;
	pthread_create(&c,NULL,cliente,v);
}

char*
puesto_contrario(char* puesto) {
	if(strcmp(puesto,"motor")==0){
		return "chapa";
	} else {
		return "motor";
	}
}

int 
tipo_atencion(){
	return calcularAleatorios(0,9);

}

char*
atender(int tipo) {
	char* result;

	switch(tipo){
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6: 
			result = "se le resuelve la incidencia";
			sleep(calcularAleatorios(17,22));
			break;
		case 7:
		case 8:
			result = "no se dispone de piezas necesarias";
			sleep(calcularAleatorios(12,15));
			break;
		case 9:
			result = "no se puede resolver y se pasa al almacen";
			sleep(calcularAleatorios(15,18));
			break;
		default: 
			break;	
	}
	return result;
}


void* 
accion_mecanico(void* data) {
	char* msj;

	struct mecanico* m = (struct mecanico*)data;

	while(!m->thread_terminated) {
		struct vehiculo* v = buscar_vehiculo_que_mas_lleva_esperando(m->puesto);
		if(v==NULL) {
			v = buscar_vehiculo_que_mas_lleva_esperando(puesto_contrario(m->puesto));
		}
		if(v==NULL) {
			sleep(1);
		} else {
			atendiendo(v,1);
			int ta = tipo_atencion();

			writeLogMessage("Mecanico","atendiendo Vehiculo",m->identificador+1,v->matricula);
			//TODO mostrar el id de vehiculo y mecanico en el mensaje
			pthread_mutex_lock(&v->espera_cliente);
			msj = atender(ta);
			pthread_mutex_unlock(&v->espera_cliente);

			writeLogMessage("Mecanico","ha atendido al Vehiculo",m->identificador+1,v->matricula);
			writeLogMessage("Incidencia del Vehiculo", msj,v->matricula,-1);

			atendiendo(v,2);

			m->contador_cafe++;

			if(m->contador_cafe != 0 && m->contador_cafe % 10 == 0) {
				writeLogMessage("Mecanico","se va a tomar cafe",m->identificador+1,-1);
				sleep(20);
				writeLogMessage("Mecanico","salio de tomar el cafe",m->identificador+1,-1);
			}
		}
	}
	return 0;
}

void 
indicar_a_los_mecanicos_que_terminen(int numero){
	writeLogMessage("TALLER","INDICANDO A LOS MECANICOS QUE VAYAN TERMINANDO",-1,-1);
	int i = 0;
	for(; i<numero; i++) {
		lista_mecanicos[i]->thread_terminated = 1;
	}
}

void
poner_a_trabajar_mecanicos(int numero) {
	int i = 0;
	for(; i<numero; i++) {
		pthread_create(&lista_mecanicos[i]->thread,NULL,accion_mecanico,lista_mecanicos[i]);
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
	struct vehiculo* v ;
	
	sprintf(msj,"cuyo numero es_%d",(int)sig);
	writeLogMessage("Recibida la señal",msj,-1,-1);
	contador_vehiculos++;

	switch(sig){
		case SIGUSR1:{
			v = crear_vehiculo(contador_vehiculos,"chapa",0);
			break;
		}
		case SIGUSR2:{
			v = crear_vehiculo(contador_vehiculos,"motor",0);
			break;
		}
		default: {
			writeLogMessage("señal","desconocida",-1,-1);
			break;
		}

	}

	if(insertar_vehiculo(v,capacidad_taller)!=-1){
		poner_a_esperar_cliente(v);
	} else { 
		destruir_vehiculo(v);
	}
}

void 
imprimir_estadisticas(){

	int i = 0;
	for(; i<numero_macanicos_motor + numero_macanicos_chapa; i++) {
		writeLogMessage("Mecanico","atendido a",lista_mecanicos[i]->identificador+1,lista_mecanicos[i]->contador_cafe);
	}

}

void 
manejador_salida(int sig) {
	indicar_a_los_mecanicos_que_terminen(numero_macanicos_motor + numero_macanicos_chapa);
	writeLogMessage("TALLER","esperando a que terminen los mecanicos",-1,-1);
	esperar_por_los_mecanicos(numero_macanicos_chapa + numero_macanicos_motor);
	imprimir_estadisticas();
	writeLogMessage("TALLER","CERRADO",-1,-1);
	liberar_recursos_vehiculos(capacidad_taller);
	liberar_recursos_mecanicos(numero_macanicos_chapa + numero_macanicos_motor);
	exit(0);
}

int
main(void) {
	signal(SIGUSR1,manejadora);
	signal(SIGUSR2,manejadora);
	signal(SIGINT,manejador_salida);

	pthread_mutex_init(&fichero,NULL);
	pthread_mutex_init(&acceso_lista_vehiculos,NULL);

	contador_vehiculos = 0;
	srand (time(NULL));
	crear_lista_vehiculos(capacidad_taller);
	inicializar_lista_vehiculos(capacidad_taller);
	crear_lista_mecanicos(numero_macanicos_motor + numero_macanicos_chapa);
	inicializar_lista_mecanicos(numero_macanicos_motor,"motor",0);
	inicializar_lista_mecanicos(numero_macanicos_chapa,"chapa",numero_macanicos_motor);
	
	writeLogMessage("Mecanicos", "empezando a trabajar",-1,-1);
	
	poner_a_trabajar_mecanicos(numero_macanicos_chapa + numero_macanicos_motor);
	
	while(1) {
		pause();
	}
	return 0;
}
