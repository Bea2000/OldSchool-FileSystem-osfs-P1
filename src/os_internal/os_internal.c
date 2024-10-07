#include "./os_internal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

FILE* fp = NULL;

void internal_set_file(char* disk_name) {
  fp = fopen(disk_name, "rb+");
}

void internal_unset_file() {
  fclose(fp);
  fp = NULL;
}

void internal_read_disk(void* ptr, int from_block, int size, int amount) {
  fseek(fp, (1 << 10) * (long)from_block, SEEK_SET);
  fread(ptr, size, amount, fp);
}

void internal_write_disk(void* ptr, int to_block, int size, int amount) {
  fseek(fp, (1 << 10) * (long)to_block, SEEK_SET);
  fwrite(ptr, size, amount, fp);
}

void internal_clear_block(int block_to_empty) {
  char buf[BLOCK_SIZE];
  memset(buf, 0, BLOCK_SIZE);
  fseek(fp, (1 << 10) * (long)block_to_empty, SEEK_SET);
  fwrite(buf, sizeof(char), BLOCK_SIZE, fp);
}

int32_t internal_get_file_size(int block) {
  int32_t size;
  internal_read_disk(&size, block, sizeof(int32_t), 1);
  return size;
}

void internal_print_dir_item(osInternalBlockDirectoryItem item, int padding) {
  if (item.valid == DIRECTORY_ITEM_DIR) {
    printf("%*c%s - %d\n", 2 * padding, ' ', item.name, item.block);
  } else if (item.valid == DIRECTORY_ITEM_FILE) {
    printf("%*c%s - %d - %d\n", 2 * padding, ' ', item.name, item.block, internal_get_file_size(item.block));
  }
}

void internal_recursive_tree(int block, int depth) {
  osInternalBlockDirectoryItem block_mem[DIRECTORY_SIZE];
  internal_read_disk(block_mem, block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);

  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    internal_print_dir_item(block_mem[i], depth);
    if (block_mem[i].valid == DIRECTORY_ITEM_DIR) internal_recursive_tree(block_mem[i].block, depth + 1);
  }
}

int32_t internal_find_matching_dir_item_index_by_name(osInternalBlockDirectoryItem* dir_items, char* name, int name_size) {
  for (int block_i = 0; block_i < DIRECTORY_SIZE; block_i++) {
    if (dir_items[block_i].valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) continue;
    if (strncmp(name, dir_items[block_i].name, name_size) != 0) continue;
    return block_i;
  }

  return -1;
}

osInternalBlockDirectoryItem internal_get_block_from_path(char* path) {
  return internal_get_block_from_path_with_limit(path, -1);
}

osInternalBlockDirectoryItem internal_get_block_from_path_with_limit(char* path, int limit) {
  int name_size = 0;
  char name_mem[PATHNAME_SIZE] = "";

  osInternalBlockDirectoryItem current_block = (osInternalBlockDirectoryItem){.valid = DIRECTORY_ITEM_DIR, .name = "(root)", .block = 256};
  int to = limit == -1 ? strlen(path) : limit;  // limitamos por int o el largo del path

  for (int i = 1; i <= to; i++) {
    // Si es el termino del pedazo y tenemos al menos un caracter
    if ((path[i] == '/' || path[i] == '\0') && name_size > 0) {
      // No podemos seguir si el actual es un archivo
      if (current_block.valid == DIRECTORY_ITEM_FILE) return (osInternalBlockDirectoryItem){.valid = DIRECTORY_ITEM_EMPTY_OR_INVALID, .name = "", .block = 0};

      // Buscamos el nombre en el bloque (directorio) actual
      osInternalBlockDirectoryItem directory_items[DIRECTORY_SIZE];
      internal_read_disk(directory_items, current_block.block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);
      int32_t item_index = internal_find_matching_dir_item_index_by_name(directory_items, name_mem, name_size);

      // Entregara un bloque invalido si no lo encontré
      if (item_index == -1) return (osInternalBlockDirectoryItem){.valid = DIRECTORY_ITEM_EMPTY_OR_INVALID, .name = "", .block = 0};

      // Siguiente bloque
      current_block = directory_items[item_index];
      name_size = 0;
    } else {
      name_mem[name_size] = path[i];
      name_size++;
    }
  }

  return current_block;
}

// Usado para pedir bloques (alocar)
int32_t internal_mark_and_get_first_empty_block() {
  uint8_t block_bitmap[BLOCK_SIZE];
  int32_t block_position = 0;
  for (int i = 0; i < BITMAP_BLOCKS; i++) {
    internal_read_disk(block_bitmap, i, sizeof(uint8_t), BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE; j++) {
      for (int k = 7; k >= 0; k--) {
        if ((block_bitmap[j] & (1 << k)) == 0) {
          block_bitmap[j] |= (1 << k);
          internal_write_disk(block_bitmap, i, sizeof(uint8_t), BLOCK_SIZE);
          return block_position;
        }
        block_position++;
      }
    }
  }
  return -1;
}

int bits_per_block = 8 * 1024;

BlockBitPosition internal_get_block_bit_position(int block) {
  BlockBitPosition position;
  position.block = block / bits_per_block;
  int bits_within_block = block % bits_per_block;
  position.byte = bits_within_block / 8;
  position.bit = 7 - bits_within_block % 8;
  return position;
}

// Usado para eliminar cosas (desalocar)
void internal_mark_block_as_empty(int block) {
  uint8_t block_bitmap[1024];
  BlockBitPosition bitmap_position = internal_get_block_bit_position(block);
  internal_read_disk(block_bitmap, bitmap_position.block, sizeof(uint8_t), 1024);

  // Realiza un AND con el complemento del bit correspondiente
  block_bitmap[bitmap_position.byte] &= ~(1 << bitmap_position.bit);

  internal_write_disk(block_bitmap, bitmap_position.block, sizeof(uint8_t), BLOCK_SIZE);
}

void internal_mark_block_as_used(int block) {
  uint8_t block_bitmap[BLOCK_SIZE];
  BlockBitPosition bitmap_position = internal_get_block_bit_position(block);
  internal_read_disk(block_bitmap, bitmap_position.block, sizeof(uint8_t), BLOCK_SIZE);

  // Realiza un OR con el bit correspondiente
  block_bitmap[bitmap_position.byte] |= (1 << bitmap_position.bit);

  internal_write_disk(block_bitmap, bitmap_position.block, sizeof(uint8_t), BLOCK_SIZE);
}

void internal_recursive_remove(int block) {
  osInternalBlockDirectoryItem block_mem[DIRECTORY_SIZE];

  // leemos el directorio
  internal_read_disk(block_mem, block, sizeof(osInternalBlockDirectoryItem), 32);

  // Recorremos el directorio (sus 32 entradas)
  for (int i = 0; i < 32; i++) {
    // si es un directorio, llamamos recursivamente
    if (block_mem[i].valid == 0x01) {
      // marcamos directorio como vacío (inválido)
      block_mem[i].valid = 0x00;
      // borramos directorio de bitmap
      internal_mark_block_as_empty(block_mem[i].block);
      internal_recursive_remove(block_mem[i].block);
    }
    // si es un archivo, lo marcamos como vacío (inválido)
    if (block_mem[i].valid == 0x02) {
      block_mem[i].valid = 0x00;
      internal_mark_block_as_empty(block_mem[i].block);
    }
  }

  internal_mark_block_as_empty(block);
}

int32_t internal_get_disk_used_memory() {
  uint8_t block_bitmap[BLOCK_SIZE];
  int32_t used_memory = 0;
  for (int i = 0; i < BITMAP_BLOCKS; i++) {
    internal_read_disk(block_bitmap, i, sizeof(uint8_t), BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE; j++) {
      for (int k = 7; k >= 0; k--) {
        if ((block_bitmap[j] & (1 << k)) != 0) {
          used_memory++;
        }
      }
    }
  }
  return used_memory;
}

// Encuentra el último / en el path
int internal_find_last_slash(char* path) {
  for (int i = strlen(path); i >= 0; i--)
    if (path[i] == '/') return i;

  return -1;
}

// Retorna el índice del primer bloque vacío en el directorio, -1 si no hay espacio, -2 si ya existe un archivo con el mismo nombre
int get_first_empty_block_for_path(osInternalBlockDirectoryItem* items, char* name) {
  int first_empty_block = -1;
  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    if (items[i].valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) {
      if (first_empty_block == -1) first_empty_block = i;
    } else {
      if (strncmp(items[i].name, name, PATHNAME_SIZE) == 0) return -2;
    }
  }
  return first_empty_block;
}

int internal_print_bitmap_block(uint8_t* buffer, int block) {
  int used_memoty = 0;

  internal_read_disk(buffer, block, sizeof(uint8_t), BLOCK_SIZE);
  printf("B%-3d ", block);
  for (int j = 0; j < COLS_IN_BITMAP_REPR; j++) {
    printf("%8d%c", j, j == COLS_IN_BITMAP_REPR - 1 ? '\n' : ' ');
  }
  for (int i = 0; i < BLOCK_SIZE / COLS_IN_BITMAP_REPR; i++) {
    printf("%4d ", i * COLS_IN_BITMAP_REPR);
    for (int j = 0; j < COLS_IN_BITMAP_REPR; j++) {
      for (int b = 7; b >= 0; b--) {
        int bit = buffer[i * COLS_IN_BITMAP_REPR + j] & (1 << b) ? 1 : 0;
        printf("%d", bit);
        used_memoty += bit;
      }
      printf(" ");
    }
    printf("\n");
  }

  return used_memoty;
}

void internal_mark_reserved_blocks_in_bitmap() {
  uint8_t block_bitmap[BLOCK_SIZE];
  internal_read_disk(block_bitmap, 0, sizeof(uint8_t), BLOCK_SIZE);
  // Primeros 257 bits son 1
  // Es decir, los primeros 32 bits del primer bloque son 1
  // Y el bit 33 tiene como bit más significativo un 1
  for (int i = 0; i < 32; i++) block_bitmap[i] = 0b11111111;
  block_bitmap[32] |= 0b10000000;  // OR para marcarlo si no lo está
  internal_write_disk(block_bitmap, 0, sizeof(uint8_t), BLOCK_SIZE);
}

void internal_mark_used_bits_in_bitmap_for_file(int32_t block) {
  osInternalBlockFileIndex file_index;
  internal_read_disk(&file_index, block, sizeof(osInternalBlockFileIndex), 1);

  int32_t blocks_needed = ceil((double)file_index.size / BLOCK_SIZE);
  uint32_t indirect_block[FILE_INDIRECT_BLOCKS];

  // Marcamos cada bloque de direcionamiento indirecto requerido
  for (int i_inderect_block = 0; blocks_needed != 0 && i_inderect_block < FILE_INDEX_SIZE; i_inderect_block++) {
    internal_mark_block_as_used(file_index.blocks[i_inderect_block]);
    // Marcamos cada bloque de datos requerido
    internal_read_disk(indirect_block, file_index.blocks[i_inderect_block], sizeof(uint32_t), FILE_INDIRECT_BLOCKS);
    for (int i_data_block = 0; blocks_needed != 0 && i_data_block < FILE_INDIRECT_BLOCKS; i_data_block++, blocks_needed--) {
      internal_mark_block_as_used(indirect_block[i_data_block]);
    }
  }
}

void internal_mark_used_bits_in_bitmap_for_dir(int32_t block) {
  osInternalBlockDirectoryItem directory_elements[DIRECTORY_SIZE];
  internal_read_disk(directory_elements, block, sizeof(osInternalBlockDirectoryItem), DIRECTORY_SIZE);

  for (int i = 0; i < DIRECTORY_SIZE; i++) {
    if (directory_elements[i].valid == DIRECTORY_ITEM_EMPTY_OR_INVALID) continue;

    internal_mark_block_as_used(directory_elements[i].block);

    if (directory_elements[i].valid == DIRECTORY_ITEM_DIR) {
      internal_mark_used_bits_in_bitmap_for_dir(directory_elements[i].block);
    } else if (directory_elements[i].valid == DIRECTORY_ITEM_FILE) {
      internal_mark_used_bits_in_bitmap_for_file(directory_elements[i].block);
    }
  }
}
