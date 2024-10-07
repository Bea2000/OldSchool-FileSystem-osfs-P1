// Import the header file of this module
#include "./os_API.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void os_mount(char* disk_name) {
  internal_set_file(disk_name);
  internal_mark_reserved_blocks_in_bitmap();
}

void os_tree() {
  printf("/ - 256\n");
  internal_recursive_tree(256, 0);
}

void os_bitmap(int num) {
  uint8_t buffer[BLOCK_SIZE];
  if (num > -1) {
    internal_print_bitmap_block(buffer, num);
    int used_blocks = internal_get_disk_used_memory();
    printf("Bloques usados: %d\nBloques libres %d\n", used_blocks, AMOUNT_OF_BLOCKS - used_blocks); 
  } else if (num == -1) {
    int used_blocks = 0;
    for (int i = 0; i < BITMAP_BLOCKS; i++) used_blocks += internal_print_bitmap_block(buffer, i);
    printf("Bloques usados: %d\nBloques libres %d\n", used_blocks, AMOUNT_OF_BLOCKS - used_blocks); 
  } else {
    printf("Argumento invalido\n");
  }
}

void os_ls(char* path) {
  osInternalBlockDirectoryItem item = internal_get_block_from_path(path);
  if (item.valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) {
    printf("%s not found\n", path);
    return;
  }

  if (item.valid == DIRECTORY_ITEM_FILE) {
    printf("%s - %d\n", item.name, internal_get_file_size(item.block));
    return;
  }

  osInternalBlockDirectoryItem block_mem[DIRECTORY_SIZE];
  internal_read_disk(block_mem, item.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);
  for (int i = 0; i < DIRECTORY_SIZE; i++) internal_print_dir_item(block_mem[i], 0);
}

int os_exists(char* path) {
  osInternalBlockDirectoryItem item = internal_get_block_from_path(path);
  if (item.valid != DIRECTORY_ITEM_EMPTY_OR_INVALID) {
    printf("%s found\n", path);
    return 1;
  }
  printf("%s not found\n", path);
  return 0;
}

int os_mkdir(char* foldername, char* path) {
  // Obtenemos el bloque del directorio padre
  osInternalBlockDirectoryItem actual = internal_get_block_from_path(path);

  if (actual.valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) {
    printf("Invalid parent directory\n");
    return 1;
  }

  osInternalBlockDirectoryItem block_mem[DIRECTORY_SIZE];
  internal_read_disk(block_mem, actual.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);

  // Obtenemos el primer bloque vacío del directorio padre
  int empty_block = get_first_empty_block_for_path(block_mem, foldername);

  if (empty_block == -1) {
    printf("No more space in directory\n");
    return 1;
  }

  if (empty_block == -2) {
    printf("File or directory with the same name exists\n");
    return 1;
  }

  // Creamos el bloque de directorio
  int dir_block = internal_mark_and_get_first_empty_block();

  if (dir_block == -1) {
    printf("No more space in memory\n");
    return 1;
  }

  // Dejamos todo el bloque de directorio como 0
  internal_clear_block(dir_block);

  // Guardamos la información del directorio
  block_mem[empty_block].valid = DIRECTORY_ITEM_DIR;
  block_mem[empty_block].block = dir_block;
  strncpy(block_mem[empty_block].name, foldername, PATHNAME_SIZE);
  internal_write_disk(block_mem, actual.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);

  return 0;
}

void os_unmount() {
  internal_unset_file();
}

osFile* os_open(char* filename, char mode) {
  int32_t file_block;
  int32_t file_size;
  char file_name[PATHNAME_SIZE];

  if (mode == 'r') {
    // Obtenemos el bloque del archivo y validamos
    osInternalBlockDirectoryItem block = internal_get_block_from_path(filename);

    if (block.valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) {
      printf("%s not found\n", filename);
      return NULL;
    }
    if (block.valid == DIRECTORY_ITEM_DIR) {
      printf("%s is a directory\n", filename);
      return NULL;
    }

    // Guardamos la info del file descriptor
    file_block = block.block;
    strncpy(file_name, block.name, PATHNAME_SIZE);
    file_size = internal_get_file_size(block.block);

  } else if (mode == 'w') {
    // Dividimos el directorio y archivo para validar el directorio
    int slash_index = internal_find_last_slash(filename);

    if (slash_index == -1) {
      printf("Invalid path\n");
      return NULL;
    }

    osInternalBlockDirectoryItem parent_directory = internal_get_block_from_path_with_limit(filename, slash_index);

    if (parent_directory.valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) {
      printf("Directory not found\n");
      return NULL;
    }

    if (parent_directory.valid == DIRECTORY_ITEM_FILE) {
      printf("Directory is a file\n");
      return NULL;
    }

    // Copiamos temporalmente el nombre de archivo
    stpncpy(file_name, filename + slash_index + 1, PATHNAME_SIZE);

    // Obtenemos la información del directorio
    osInternalBlockDirectoryItem directory_items[DIRECTORY_SIZE];
    internal_read_disk(directory_items, parent_directory.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);

    // Obtenemos el primer bloque vacío y verificamos que no exista un archivo con el mismo nombre
    int empty_block = get_first_empty_block_for_path(directory_items, file_name);

    if (empty_block == -1) {
      printf("No more space in directory\n");
      return NULL;
    }

    if (empty_block == -2) {
      printf("File or directory with the same name exists\n");
      return NULL;
    }

    // Reservamos un bloque para el índice de archivo
    int32_t new_file_index_block = internal_mark_and_get_first_empty_block();

    if (new_file_index_block == -1) {
      printf("No more space in memory\n");
      return NULL;
    }

    // Inicializamos ese bloque con un índice vacío
    internal_clear_block(new_file_index_block);

    // Guardamos la info del file descriptor
    file_block = new_file_index_block;
    file_size = 0;

    // Añadimos el archivo al directorio
    directory_items[empty_block].valid = DIRECTORY_ITEM_FILE;
    strncpy(directory_items[empty_block].name, file_name, PATHNAME_SIZE);
    printf("new file index block: %d\n", new_file_index_block);
    directory_items[empty_block].block = new_file_index_block;
    internal_write_disk(directory_items, parent_directory.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);
  } else {
    printf("Invalid mode\n");
    return NULL;
  }

  // Creamos un file descriptor si está todo ok
  osFile* file_desc = malloc(sizeof(osFile));
  file_desc->mode = mode;
  file_desc->size = file_size;
  strncpy(file_desc->name, file_name, PATHNAME_SIZE);
  file_desc->block = file_block;

  return file_desc;
}

// Función para descargar un archivo del disco. Lee un archivo desde descrito por file desc y crea una copia del archivo en la ruta indicada por dest dentro de su computador.
void os_read(osFile* file_desc, char* dest) {
  FILE* fp = fopen(dest, "wb");

  if (!fp) {
    printf("Error opening file\n");
    return;
  }

  // Bloque de índice
  osInternalBlockFileIndex block_index;
  internal_read_disk(&block_index, file_desc->block, sizeof(osInternalBlockFileIndex), 1);
  int32_t remaining_size = block_index.size;

  char data_buffer[BLOCK_SIZE];

  for (int i = 0; i < FILE_INDEX_SIZE && 0 < remaining_size; i++) {
    // Bloque de direcionamiento indirecto
    int32_t indirect_block[256];
    internal_read_disk(&indirect_block, block_index.blocks[i], sizeof(int32_t), 256);

    for (int j = 0; j < 256 && 0 < remaining_size; j++) {
      // Bloque de datos
      int32_t amount_to_read = remaining_size < BLOCK_SIZE ? remaining_size : BLOCK_SIZE;
      internal_read_disk(data_buffer, indirect_block[j], sizeof(uint8_t), amount_to_read);
      fwrite(data_buffer, sizeof(char), amount_to_read, fp);

      remaining_size -= amount_to_read;
    }
  }

  fclose(fp);
}

int os_write(osFile* file_desc, char* src) {
  
  // Archivo Local
  FILE* fp = fopen(src, "rb");
  fseek(fp, 0L, SEEK_END);
  int32_t file_size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  float blocks_needed = ceil((float)file_size / BLOCK_SIZE);
  float indirect_blocks_needed = ceil(blocks_needed / 256);
  
  // Bloque de indice
  osInternalBlockFileIndex block_index;
  internal_read_disk(&block_index, file_desc->block, sizeof(osInternalBlockFileIndex), 1);
  int32_t remaining_size = file_size;
  int number_of_blocks_written = 0;
  int number_of_bytes_written = 0;
  char data_buffer[BLOCK_SIZE];

  for (int i = 0; i < (int)indirect_blocks_needed && number_of_blocks_written < (int)blocks_needed; i++){
    // Bloque de direcionamiento indirecto
    int32_t indirect_block[256];
    block_index.blocks[i] = internal_mark_and_get_first_empty_block();
    internal_clear_block(block_index.blocks[i]);
    
    for (int j = 0; j < 256 && number_of_blocks_written < (int)blocks_needed; j++) {
      // Bloque de datos
      int32_t data_block = internal_mark_and_get_first_empty_block();
      int32_t amount_to_read = remaining_size < BLOCK_SIZE ? remaining_size : BLOCK_SIZE;
      fread(data_buffer, sizeof(char), amount_to_read, fp);
      internal_write_disk(data_buffer, data_block, sizeof(uint8_t), amount_to_read);
      indirect_block[j] = data_block;
      
      remaining_size -= amount_to_read;
      number_of_bytes_written += amount_to_read;
      number_of_blocks_written++;
    }
    internal_write_disk(indirect_block, block_index.blocks[i], sizeof(int32_t), 256);
  }

  block_index.size = file_size;
  internal_write_disk(&block_index, file_desc->block, sizeof(osInternalBlockFileIndex), 1);
  fclose(fp);

  return number_of_bytes_written;
}

void os_close(osFile* file_desc) {
  free(file_desc);
}

void os_rm(char* path) {
  // buscamos el bloque donde se almacena el indice del archivo
  int slash_index = internal_find_last_slash(path);

  if (slash_index == -1) {
    printf("Invalid path\n");
    return;
  }

  osInternalBlockDirectoryItem parent_directory = internal_get_block_from_path_with_limit(path, slash_index);

  if (parent_directory.valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) {
    printf("Directory not found\n");
    return;
  }

  if (parent_directory.valid == DIRECTORY_ITEM_FILE) {
    printf("Directory is a file\n");
    return;
  }

  osInternalBlockDirectoryItem parent_directory_items[DIRECTORY_SIZE];
  internal_read_disk(parent_directory_items, parent_directory.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);
  int32_t item_index = internal_find_matching_dir_item_index_by_name(parent_directory_items, path + slash_index + 1, PATHNAME_SIZE);

  if (item_index == -1) {
    printf("Directory not found\n");
    return;
  }

  osInternalBlockDirectoryItem item = internal_get_block_from_path(path);
  printf("[][] element %s is in block %d of type %d\n", path, item.block, item.valid);
  if (item.valid != DIRECTORY_ITEM_FILE) {
    printf("%s not found\n", path);
    return;
  }

  // marcamos el bloque como vacio (inválido)
  parent_directory_items[item_index].valid = DIRECTORY_ITEM_EMPTY_OR_INVALID;
  internal_write_disk(parent_directory_items, parent_directory.block, sizeof(osInternalBlockDirectoryItem), 32);

  printf("file index is in block %d\n", item.block);
  internal_mark_block_as_empty(item.block);

  osInternalBlockFileIndex block_adressing;
  // buscamos el bloque donde se almacenan los direccionamiento indirectos simple
  internal_read_disk(&block_adressing, item.block, sizeof(osInternalBlockFileIndex), 1);
  for (int i = 0; i < 255 && block_adressing.blocks[i] != 0; i++) {
    printf("file adressing on block %d\n", block_adressing.blocks[i]);
    internal_mark_block_as_empty(block_adressing.blocks[i]);
    int32_t data_block[256];
    // buscamos el bloque donde se almacenan los datos
    internal_read_disk(data_block, block_adressing.blocks[i], sizeof(int32_t), 256);
    for (int j = 0; j < 256 && data_block[j] != 0; j++) {
      printf("data block %d\n", data_block[j]);
      internal_mark_block_as_empty(data_block[j]);
    }
  }
}

void os_rmdir(char* path) {
  int slash_index = internal_find_last_slash(path);

  if (slash_index == -1) {
    printf("Invalid path\n");
    return;
  }

  osInternalBlockDirectoryItem parent_directory = internal_get_block_from_path_with_limit(path, slash_index);

  if (parent_directory.valid != DIRECTORY_ITEM_DIR) {
    printf("Directory not found\n");
    return;
  }

  osInternalBlockDirectoryItem parent_directory_items[DIRECTORY_SIZE];
  internal_read_disk(parent_directory_items, parent_directory.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);
  int32_t item_index = internal_find_matching_dir_item_index_by_name(parent_directory_items, path + slash_index + 1, PATHNAME_SIZE);

  if (item_index == -1) {
    printf("Directory not found\n");
    return;
  }

  // Marcamos el bloque como vacio (inválido)
  parent_directory_items[item_index].valid = DIRECTORY_ITEM_EMPTY_OR_INVALID;

  // Marcamos desocupados los bloques hijos del directorio
  internal_recursive_remove(parent_directory_items[item_index].block);

  // Removemos el directorio del directorio padre
  internal_write_disk(parent_directory_items, parent_directory.block, sizeof(osInternalBlockDirectoryItem), 32);
}

void cleanup() {
  for (int i = 0; i < BITMAP_BLOCKS; i++) internal_mark_block_as_empty(i);
  internal_mark_reserved_blocks_in_bitmap();
  internal_mark_used_bits_in_bitmap_for_dir(256);
}

int used_memory() {
  int used = internal_get_disk_used_memory();
  printf("Used memory: %d of 2097152 (%f%%)\n", used, (float)used / 2097152 * 100);
  return used;
}
