#include <stdint.h>
#include <stdio.h>

#define PATHNAME_SIZE 27
#define BLOCK_SIZE 1024

#define BITMAP_BLOCKS 256
#define FILE_INDEX_SIZE 255
#define FILE_INDIRECT_BLOCKS 256

#define DIRECTORY_SIZE (BLOCK_SIZE / sizeof(osInternalBlockDirectoryItem))
#define AMOUNT_OF_BLOCKS_OF_BITMAP_BLOCK (8 * BLOCK_SIZE)
#define AMOUNT_OF_BLOCKS (BITMAP_BLOCKS * AMOUNT_OF_BLOCKS_OF_BITMAP_BLOCK)

#define DIRECTORY_ITEM_EMPTY_OR_INVALID 0x00
#define DIRECTORY_ITEM_DIR 0x01
#define DIRECTORY_ITEM_FILE 0x02

#define COLS_IN_BITMAP_REPR 16  // potencia de 2

typedef struct {
  int8_t valid;
  char name[PATHNAME_SIZE];
  int32_t block;
} osInternalBlockDirectoryItem;

typedef struct {
  int32_t size;
  int32_t blocks[FILE_INDEX_SIZE];
} osInternalBlockFileIndex;

typedef struct {
  /** Menos a más significativo */
  int32_t block;
  /** Menos a más significativo */
  int16_t byte;
  /** Más a menos significativo  */
  int8_t bit;
} BlockBitPosition;

void internal_set_file(char* disk_name);
void internal_unset_file();
void internal_read_disk(void* ptr, int from_block, int size, int amount);
void internal_write_disk(void* ptr, int to_block, int size, int amount);
void internal_clear_block(int block_to_empty);
void internal_recursive_tree(int block, int depth);
int32_t internal_get_file_size(int block);
void internal_print_dir_item(osInternalBlockDirectoryItem item, int padding);
osInternalBlockDirectoryItem internal_get_block_from_path(char* path);
osInternalBlockDirectoryItem internal_get_block_from_path_with_limit(char* path, int limit);
int32_t internal_mark_and_get_first_empty_block();
void internal_mark_block_as_empty(int block);
void internal_recursive_remove(int block);
int32_t internal_get_disk_used_memory();
BlockBitPosition internal_get_block_bit_position(int block);
int internal_find_last_slash(char* path);
int get_first_empty_block_for_path(osInternalBlockDirectoryItem* items, char* name);
int internal_print_bitmap_block(uint8_t* buffer, int block);
void internal_mark_reserved_blocks_in_bitmap();
void internal_mark_used_bits_in_bitmap_for_dir(int32_t block);
int32_t internal_find_matching_dir_item_index_by_name(osInternalBlockDirectoryItem* dir_items, char* name, int name_size);