#include <stdio.h>   // FILE, fopen, fclose, etc.
#include <stdlib.h>  // malloc, calloc, free, etc
#include <string.h>  // memcpy, memset, etc.
#include <math.h>

#include "../os_API/os_API.h"

int test_simple_directory(char* parent_dir_path, char* name) {
  printf("INICIO TEST creación y eliminación de directorio %s en %s\n", name, parent_dir_path);

  char full_path[256] = "\0";
  strcat(full_path, parent_dir_path);
  strcat(full_path, name);

  os_mkdir(name, parent_dir_path);
  int ok_test_dir = os_exists(full_path);

  if (!ok_test_dir) {
    printf("\033[0;31mNo se ha creado el directorio\n\033[0m");
    return 0;
  }

  printf("\033[0;32mEl directorio existe\n\033[0m");
  int initial_used_memory = used_memory();
  os_rmdir(full_path);

  if (os_exists(full_path)) {
    printf("\033[0;31mNo se ha borrado el directorio\n\033[0m");
    return 0;
  }

  printf("\033[0;32mEl directorio se ha borrado\n\033[0m");

  if (used_memory() + 1 != initial_used_memory) {
    printf("\033[0;31mLa memoria no se ha liberado\n\033[0m");
    return 0;
  }

  printf("\033[0;32mLa memoria se ha liberado\n\033[0m");
  return 1;
}

int test_recursive_dir_delete() {
  int initial_used_memory = used_memory();
  printf("INICIO TEST creación y eliminación de directorio recursivo\n");

  int error_1 = os_mkdir("a", "/grand_parent_dir/parent_dir");
  if (error_1) {
    printf("\033[0;31mNo se ha creado el directorio 1 \n\033[0m");
    return 0;
  }
  int error_2 = os_mkdir("b", "/grand_parent_dir/parent_dir/a");
  if (error_2) {
    printf("\033[0;31mNo se ha creado el directorio 2 \n\033[0m");
    return 0;
  }
  int error_3 = os_mkdir("c", "/grand_parent_dir/parent_dir/a/b");
  if (error_3) {
    printf("\033[0;31mNo se ha creado el directorio 3 \n\033[0m");
    return 0;
  }
  int error_4 = os_mkdir("d", "/grand_parent_dir/parent_dir/a/b/c");
  if (error_4) {
    printf("\033[0;31mNo se ha creado el directorio 4 \n\033[0m");
    return 0;
  }

  if (used_memory() != initial_used_memory + 4) {
    printf("\033[0;31mLa memoria no se ha reservado correctamente\n\033[0m");
    return 0;
  }

  printf("\033[0;32mLa memoria se ha reservado correctamente\n\033[0m");

  os_rmdir("/grand_parent_dir/parent_dir/a");

  if (used_memory() != initial_used_memory) {
    printf("\033[0;31mLa memoria no se ha liberado correctamente\n\033[0m");
    return 0;
  }

  printf("\033[0;32mLa memoria se ha liberado correctamente\n\033[0m");

  return 1;
}

int test_mkdir_and_rm_dir() {
  int ok_mkdir_in_root = test_simple_directory("/", "test_dir");
  if (!ok_mkdir_in_root) {
    printf("\033[0;31m!! No funciona la creación de directorios\n\033[0m");
    return 0;
  }

  // Setup
  os_mkdir("grand_parent_dir", "/");
  os_mkdir("parent_dir", "/grand_parent_dir");

  // Test dentro de varios diretcorios
  int ok_mkdir = test_simple_directory("/grand_parent_dir/parent_dir/", "test_dir");
  if (ok_mkdir)
    printf("\033[0;32m!! Funcióna la creación de directorios\n\033[0m");
  else
    printf("\033[0;31m!! No funciona la creación de directorios\n\033[0m");

  // Test de directorios anidados
  int ok_recursive_dir_delete = test_recursive_dir_delete();
  if (ok_recursive_dir_delete)
    printf("\033[0;32m!! Funcióna la eliminación de directorios recursivos\n\033[0m");
  else
    printf("\033[0;31m!! No funciona la eliminación de directorios recursivos\n\033[0m");

  // Cleanup
  os_rmdir("/grand_parent_dir/parent_dir");
  os_rmdir("/grand_parent_dir");

  return 1;
}

int test_file_write_read_and_delete(char* real_input_path, char* real_output_path, char* disk_path) {
  printf("\nINICIO TEST escritura y lectura de archivos\n");

  // Obtenemos el tamaño del archivo 1
  FILE* file1 = fopen(real_input_path, "r");
  fseek(file1, 0L, SEEK_END);
  long int file1_size = ftell(file1);

  // Vemos la escritura del archivo
  int initial_memoty = used_memory();
  osFile* file_desc_w = os_open(disk_path, 'w');
  if (used_memory() != initial_memoty + 1) {
    os_close(file_desc_w);
    printf("\033[0;31m!! No se ha reservado bien la memoria para el archivo al iniciar\n\033[0m");
    return 0;
  }

  os_write(file_desc_w, real_input_path);
  os_close(file_desc_w);

  // Vemos si la cantidad de bloques reservados es la correcta
  int blocks_that_should_be_used = round((float)file1_size / BLOCK_SIZE) + round((float)file1_size / (BLOCK_SIZE * FILE_INDIRECT_BLOCKS)) + 1;
  if (used_memory() != initial_memoty + blocks_that_should_be_used) {
    printf("\033[0;31m!! No se ha reservado bien la memoria para el archivo al escribir\n\033[0m");
    return 0;
  }

  osFile* file_desc_r = os_open(disk_path, 'r');
  os_read(file_desc_r, real_output_path);
  os_close(file_desc_r);


  // Comparar el archivo original con el archivo leído
  FILE* file2 = fopen(real_output_path, "r");
  // Comparamos el tamaño
  fseek(file2, 0L, SEEK_END);
  long int file2_size = ftell(file2);
  if (file1_size != file2_size) {
    printf("\033[0;31m!! El archivo leído no tiene el mismo tamaño que el original\n\033[0m");
    return 0;
  }
  // Comparamos el contenido
  fseek(file1, 0L, SEEK_SET);
  fseek(file2, 0L, SEEK_SET);
  char file1_buffer[1024];
  char file2_buffer[1024];
  while (fgets(file1_buffer, 1024, file1) != NULL && fgets(file2_buffer, 1024, file2) != NULL) {
    if (strcmp(file1_buffer, file2_buffer) != 0) {
      printf("\033[0;31m!! El archivo leído no tiene el mismo contenido que el original\n\033[0m");
      return 0;
    }
  }

  os_rm(disk_path);

  if (used_memory() != initial_memoty) {
    printf("\033[0;31m!! No se ha liberado bien la memoria para el archivo\n\033[0m");
    return 0;
  }

  printf("\033[0;32m!! Funcióna la escritura y lectura de archivos\n\033[0m");
  return 1;
}

int main(int argc, char* argv[]) {
  os_mount(argv[1]);
  cleanup();

  used_memory();
  os_tree();
  test_mkdir_and_rm_dir();
  test_recursive_dir_delete();
  os_tree();

  os_unmount();
}
