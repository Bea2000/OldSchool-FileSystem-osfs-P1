// Tells the compiler to compile this file once
#pragma once

#include <stdint.h>

#include "../os_internal/os_internal.h"

typedef struct {
  char name[PATHNAME_SIZE];
  int32_t block;
  int32_t size;
  char mode;
} osFile;

// Funciones generales

/* Monta el disco, guardando globalmente su ruta */
void os_mount(char* disk_name);

/* Imprime el arbol de archivos, con nombre - n_bloque [- tamaño archivo] */
void os_tree();

/**
 * Imprime el binario del bloque num, si num es -1, se imprime todo el bitmap.
 * Luego se debe imprimir en lineas diferentes la cantidad de bloques libres y desocupados.
 */
void os_bitmap(int num);

/* Imprime los archivos que hay en un directorio */
void os_ls(char* path);

/* Retorna 1 si el archivo existe, 0 si no. Hay que comprobar directorios previos */
int os_exists(char* path);

/* Crea un directorio en el primer bloque vacío del disco */
int os_mkdir(char* foldername, char* path);

/* Desmonta el disco, reiniciando variables globales y liberando el heap */
void os_unmount();

// Manejo de archivos

/**
 * Abre o crea un archivo. Si es que lee `r`:
 * 1. Busca el archivo path en el disco
 * 2. Retorna osFile
 *
 * Si es que escribe `w`:
 * 1. Se verifica si el archivo existe
 * 2. El primer bloque vacío del disco será el bloque indice
 * 3. Crea la entrada en el directorio en el que fue creado
 * 4. Retorna osFile
 */
osFile* os_open(char* filename, char mode);

/* Copia un archivo del disco virtual hacia el disco real  */
void os_read(osFile* file_desc, char* dest);

/**
 * Escribe el archivo del disco real hacia el disco virtual.
 * Retorna cantidad de bytes escritos.
 */
int os_write(osFile* file_desc, char* src);

/* Cierra el archivo, garantizando que todo fue escrito */
void os_close(osFile* file_desc);

/* Borra un archivo, modificando el bitmap y la entrada del directorio */
void os_rm(char* path);

/* Borra un directorio y todo su contenido, modificando el bitmap y el directorio padre */
void os_rmdir(char* path);

// Bonus

/**
 * Libera bloques muertos, buscando bloques del bitmap marcados como utilizados
 * pero que no son referenciados en ningún bloque.
 */
void cleanup();

/** Obtiene la cantidad de memoria usada en el disco */
int used_memory();
