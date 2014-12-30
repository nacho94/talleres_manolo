#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPACIDAD_TALLER 10

struct vehiculo {
	int matricula;
	char* averia;
};

struct vehiculo* lista_vehiculos[CAPACIDAD_TALLER];

struct vehiculo*
crear_vehiculo(int matricula, char* averia) {
	struct vehiculo* v;
	//reservamos la memoria
	v = (struct vehiculo*)malloc(sizeof(struct vehiculo));
	v->matricula = matricula;

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
inicializar_lista_vehiculos() {
	int i = 0;
	for(i = 0; i<CAPACIDAD_TALLER; i++) {
		lista_vehiculos[i] = NULL;
	}
}

int
insertar_vehiculo(struct vehiculo* v) {
	int i = 0;
	for(i = 0; i<CAPACIDAD_TALLER; i++) {
		if(lista_vehiculos[i] == NULL) {
			lista_vehiculos[i] = v;
			return i;
		}
	}

	return -1;
}

int
main(void) {
	inicializar_lista_vehiculos();
	return 0;
}
