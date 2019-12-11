#ifndef _FILESYS_H
#define _FILESYS_H

#include "multiboot.h"
#include "types.h"

#define FARRAY_SIZE 8
#define DENTRY_COUNT 63
#define FS_BLOCK_SIZE 4096
#define FNAME_MAX_LEN 32

// Type for a file directory entry
typedef struct directory_entry {
    char file_name[FNAME_MAX_LEN]; // Name with up to 32 characters. Does not include EOS
	uint32_t file_type; // Type of file. 0 giving user-level access to RTC, 1 for directory, 2 for normal file
	uint32_t inode_num; // Index node number. Ignore for types 0 and 1
	uint8_t reserved[24]; // 24 reserved bytes
} dentry_t;

typedef struct boot_struct {
	uint32_t num_entries;
	uint32_t num_inodes;
	uint32_t num_datablocks; // number of data blocks in whole system
	uint8_t reserved[52]; // 52 reserved byte
	dentry_t entries[DENTRY_COUNT];
} boot_block_t;

typedef struct inode_struct {
	uint32_t length;
	uint32_t data_blocks[1023]; // data block numbers, up to 1023 blocks
} inode_t;

typedef struct operations_table_entry {
	int32_t (*open_op)(const uint8_t*);
	int32_t (*read_op)(uint32_t, void*, uint32_t);
	int32_t (*write_op)(uint32_t, const void*, uint32_t);
	int32_t (*close_op)(uint32_t);
} operations_t;

typedef struct file_array_entry {
    operations_t* operations_pointer; // pointer to correct operations jump table for file type
	uint32_t inode_num; // Should be 0 for directory and RTC file
	uint32_t file_pos; // Tracks current position of user in file
	uint32_t flags; // Multiple uses, marks entry as in use
} farray_t;

extern void filesys_init (module_t *);
extern int32_t read_dentry_by_name(const uint8_t*, dentry_t*);
extern int32_t read_data(uint32_t, uint32_t, uint8_t*, uint32_t);
extern int fdir_open(const uint8_t* filename);
extern int fdir_close(uint32_t fd);
extern int fdir_write(uint32_t, const void *, uint32_t);
extern int fdir_read(uint32_t, void *, uint32_t);
extern int file_open(const uint8_t *);
extern int file_close(uint32_t);
extern int file_read(uint32_t, void *, uint32_t);
extern int file_write(uint32_t, const void *, uint32_t);
extern int program_imgcpy(uint8_t *, void *);

#endif
