# OldSchool FileSystem (osfs)

## Overview

This project is part of the **Operating Systems course** (2023-2). It involves the implementation of a simplified filesystem called **OldSchool FileSystem (osfs)**, designed to handle file and directory management on a virtual disk. The project mimics a real filesystem using index-based allocation and various block types such as bitmaps, directories, index blocks, indirect addressing, and data blocks.

## Key Features

- **File and Directory Management**: The system supports creating, deleting, and navigating files and directories.
- **Indexed Allocation**: Files are stored using an index block, which references the data blocks and supports indirect addressing.
- **Virtual Disk**: The filesystem operates on a simulated disk stored in a binary file, divided into blocks of 1 KB.
- **Bitmap Management**: The system uses a bitmap to track used and free blocks in the virtual disk.
- **Little Endian**: All numeric data is stored in little-endian format.

## Prerequisites

To run the project, you need:

- A C compiler (such as `gcc`)
- A Linux-based or Unix-based environment is recommended for execution.

## Compilation

To compile the project, run the following command in the terminal:

```bash
make
```

This will generate the executable `osfs`.

## Running the Program

The virtual disk must be mounted before using the filesystem that you need to unzip You can execute the program as follows:

```bash
./osfs simdiskfilled.bin
```

## Example Usage

Here is an example of using the filesystem:

```c
  os_mount(argv[1]);
  cleanup();

  used_memory();
  os_tree();
  test_mkdir_and_rm_dir();
  test_recursive_dir_delete();
  os_tree();

  os_unmount();
```

## API Documentation

The following functions are part of the osfs API:

### General Functions

`void os_mount(char* diskname)`
Mounts the virtual disk and sets it for further operations.

`void os_tree()`
Prints the entire directory tree starting from the root.

`void os_bitmap(unsigned int num)`
Displays the state of the specified bitmap block, or the entire bitmap if num = -1.

`void os_ls(char* path)`
Lists the valid entries in the directory at the given path.

`int os_exists(char* path)`
Checks whether a file or directory exists at the given path.

`int os_mkdir(char *foldername, char* path)`
Creates a new directory in the specified path.

`void os_unmount()`
Unmounts the virtual disk and saves any changes.

### File Handling Functions

`osFile* os_open(char* path, char mode)`
Opens a file. Use mode 'r' for reading or 'w' for writing.

`void os_read(osFile* file_desc, char* dest)`
Reads the file and stores it at the specified dest path on your local system.

`int os_write(osFile* file_desc, char* path)`
Writes a file from the local system at the specified path into the virtual disk.

`void os_close(osFile* file_desc)`
Closes the file and updates it on the virtual disk.

`void os_rm(char* path)`
Deletes the specified file from the disk.

`void os_rmdir(char* path)`
Deletes the specified directory and all its contents.

`void cleanup()`
Identifies and reclaims unused blocks that are marked as occupied in the bitmap but are not referenced by any files or directories.

## Assumptions

- The first function always called is os_mount(), and the last is os_unmount().
- Paths provided to functions are always valid unless checking with os_exists().
- File and directory names are unique.
- The system uses little-endian format for multi-byte numbers.

## Conclusion

This project provides a simplified yet robust implementation of a virtual filesystem with basic file and directory operations. By using the osfs API, you can simulate a fully functional filesystem on a virtual disk.
