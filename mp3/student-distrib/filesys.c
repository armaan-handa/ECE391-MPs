#include "filesys.h"
#include "lib.h"
#include "terminal.h"
#include "rtc.h"
#include "pcb.h"

// Addresses of the file system module
static uint32_t module_start; 
static uint32_t module_end; 
static boot_block_t boot_block;

// Operations table
static operations_t file_operations[3] = {
	// type 0 is RTC
	{.open_op = open_rtc, .read_op = read_rtc, .write_op = write_rtc, .close_op = close_rtc},
	// type 1 is directory
	{.open_op = fdir_open, .read_op = fdir_read, .write_op = fdir_write, .close_op = fdir_close},
	// type 2 is file
	{.open_op = file_open, .read_op = file_read, .write_op = file_write, .close_op = file_close}
};

/*
 * read_dentry_by_name
 *   Reads a directory entry based on filename.
 *   Inputs: fname - pointer to filename string
 *			 dentry - data entry struct to write to
 *   Outputs: -1 on failure, 0 on success.
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
	int i;
	char desired[33];
	char actual[33];
	
	if (dentry == NULL || fname == NULL) {
		return -1;
	}
	
	if (*fname == '\0') {
		return -1;
	}
	
	for (i = 0; i < DENTRY_COUNT; i++) {
		memcpy (desired, fname, FNAME_MAX_LEN + 1);
		memcpy (actual, boot_block.entries[i].file_name, FNAME_MAX_LEN);
		actual[32] = '\0';
		
		if (strncmp(actual, desired, FNAME_MAX_LEN + 1) == 0) { // if all 32 characters + terminator match
			memcpy(dentry, &(boot_block.entries[i]), sizeof(dentry_t));
			return 0;
		}
	}
	
	return -1;
}

/*
 * read_dentry_by_index
 *   Reads a directory entry based on index.
 *   Inputs: index of the data entry
 *			 dentry - data entry struct to write to
 *   Outputs: -1 on failure, 0 on success.
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
	if (index < 0 || index >= DENTRY_COUNT || dentry == NULL) {
		return -1;
	}
	
	// copy into dentry
	memcpy(dentry, &(boot_block.entries[index]), sizeof(dentry_t));
	return 0;
}

/*
 * read_data
 *   Reads data from file system based on inode and offset
 *   Inputs: inode index corresponding to the data entry
 *			 offset from the beginning of the inode
 *		     buf - buffer to write data to
 *           length - number of bytes to write
 *   Outputs: -1 on failure, returns the number of bytes read
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
	unsigned int block_index; // which block, within the inode structure, we are accessing
	unsigned int data_offset; // offset of the data within the block
	unsigned int copied; // bytes that have been copied
	unsigned int memory_index; // memory location of actual data
	inode_t* node; // inode containing file information
	
	// Check inode bounds and arguments
	if (inode < 0 || inode >= boot_block.num_inodes || buf == NULL) {
		return -1;
	}
	
	node = (inode_t *) (module_start + FS_BLOCK_SIZE * (inode + 1)); // get address of inode
	
	if (offset >= node->length + 1) { // offset is outside file
		return -1;
	}
	
	copied = 0;
	block_index = offset / FS_BLOCK_SIZE;
	data_offset = offset % FS_BLOCK_SIZE;
	memory_index = ((node->data_blocks[block_index] + boot_block.num_inodes + 1) * FS_BLOCK_SIZE);
	
	if (length + offset > node->length) {  // if desired bytes go past EOF
		length = node->length - offset; // truncate desired bytes to end of file
	}
	
	// if we start off in the middle of a block
	if (data_offset > 0) {
		// bad block index
		if (node->data_blocks[block_index] >= boot_block.num_datablocks) {
			return -1;
		}
		
		if (FS_BLOCK_SIZE - data_offset >= length) { // if we can copy everything from 1 block
			memcpy(buf, (void *)(memory_index + data_offset + module_start), length); 
			return length;
		} else { // pull from the rest of this block
			memcpy(buf, (void *)(memory_index + data_offset + module_start), FS_BLOCK_SIZE - data_offset); // copy the bytes until we're aligned
			block_index++; // go to start of next block
			copied = FS_BLOCK_SIZE - data_offset;
			data_offset = 0;
		}
	}
	
	// copy the rest of the data block by block
	while (copied < length) {
		memory_index = ((node->data_blocks[block_index] + boot_block.num_inodes + 1) * FS_BLOCK_SIZE);

		// bad block index
		if (node->data_blocks[block_index] >= boot_block.num_datablocks) {
			return -1;
		}
		
		if (length - copied > FS_BLOCK_SIZE) { // if remaining bytes more than 1 block
			memcpy(buf + copied, (void *)(memory_index + module_start), FS_BLOCK_SIZE); // copy one block of data
			block_index++;
		}  else { // remaining bytes get copied
			memcpy(buf + copied, (void *)(memory_index + module_start), length - copied);
			return length;
		}
		
		copied += FS_BLOCK_SIZE;
	}
	
	return copied;
}

/*
 * filesys_init
 *   Sets up function pointers and static variables for the rest of the file system driver.
 *   Inputs: module_ptr - Pointer to the file system module installed at boot.
 *   Outputs: none
 */
void filesys_init (module_t * module_ptr) {	
	if (module_ptr == NULL) {
	    return;
	}
	
	module_start = module_ptr->mod_start;
	module_end = module_ptr->mod_end;
	
	// pull boot block info
	memcpy((void *) (&boot_block), (void *) module_start, sizeof(boot_block_t));
	
	return;
}

/*
 * fdir_open
 *   Opens the directory. Should not get called
 *   Inputs: filename - does nothing
 *   Outputs: -1 on failure, 0 on success.
 */
int fdir_open (const uint8_t* filename) {
	return 0;
}

/*
 * fdir_close
 *   Closes the directory. 
 *   Inputs: fd - does nothing
 *   Outputs: none
 */
int fdir_close (uint32_t fd) {
	return file_close(fd);
}

/*
 * fdir_write
 *   Does nothing.
 *   Inputs: fd, buf, nbytes - Do nothing.
 *   Outputs: none
 */
int fdir_write (uint32_t fd, const void * buf, uint32_t nbytes) {
	return -1;
}


/*
 * fdir_read
 *   Read information on all the files in the directory and writes as a string
 *   Inputs: fd - file descriptor, ignored 
 *			buf - buffer to read into
 *			nbytes - number of bytes to read
 *   Outputs: none
 *   Side effects: Prints out directory contents
 */
int fdir_read (uint32_t fd, void * buf, uint32_t nbytes) {
	int i;
	int8_t string_to_send[nbytes + FNAME_MAX_LEN + 1]; // buffer with sufficient space
	pcb_t * pcb;
	int len;
	
	pcb = get_pcb();
	
	// check file descriptor and buffer
	if (fd < 0 || fd >= FARRAY_SIZE || buf == NULL) {
		return -1;
	}
	
	// if no file opened
	if (pcb->file_array[fd].flags == 0) {
		return -1;
	}

	// if nothing to read
	if (pcb->file_array[fd].file_pos >= boot_block.num_entries) {
		return 0;
	}
	
	i = pcb->file_array[fd].file_pos;

	strncpy(string_to_send, boot_block.entries[i].file_name, FNAME_MAX_LEN); // copy filename
		
	// move offset
	if (strlen(boot_block.entries[i].file_name) < FNAME_MAX_LEN) {
		len = strlen(boot_block.entries[i].file_name);  
	} else {
		len = FNAME_MAX_LEN;
	}
	
	string_to_send[len] = '\0';

	memcpy(buf, (void* ) string_to_send, len);

	pcb->file_array[fd].file_pos++;
	
	return len;
}

/*
 * file_open
 *   Opens a file in the file system given a file name
 *   Inputs: filename - string containing a file's name
 *   Outputs: Returns -1 on failure, otherwise returns a file descriptor.
 *   Side effects: Sets up an opened file in the file array
 */
int file_open (const uint8_t * filename) {
	int i;
	dentry_t file_dentry;
	pcb_t * pcb;
	
	pcb = get_pcb();
	
	// Get index of first available index entry
	i = 0;
	while (i < FARRAY_SIZE) {
		if (pcb->file_array[i].flags == 0) {
			break;
		}
		i++;
	}
	
	// no available entries or bad filename
	if (i >= FARRAY_SIZE || filename == NULL) {
		return -1;
	}
	
	if (-1 == read_dentry_by_name((uint8_t*) filename, &file_dentry)) {
		// return on failure
		return -1;
	}
	
	pcb->file_array[i].operations_pointer = (operations_t *) (&(file_operations[file_dentry.file_type])); // set up operations table
	
	if (file_dentry.file_type == 2) { // entry is a normal file, type 2
		pcb->file_array[i].inode_num = file_dentry.inode_num;
	} else { // entry is directory or rtc
		pcb->file_array[i].inode_num = 0;
		pcb->file_array[i].operations_pointer->open_op(filename);
	}
	
	pcb->file_array[i].flags |= 0x1; // mark as in use
	
	return i;
}

/*
 * file_read
 *   Reads data from a given file and writes to a buffer.
 *   Inputs: fd - A file descriptor int.
 *           buf - pointer to buffer to write to
 *           num - number of bytes to read
 *   Outputs: Returns -1 on failure. Otherwise returns the number of bytes read.
 *   Side effects: Writes to buf
 */
int file_read (uint32_t fd, void * buf, uint32_t num) {
	int read;
	pcb_t * pcb;
	
	pcb = get_pcb();
	
	// check file descriptor and buffer
	if (fd < 0 || fd >= FARRAY_SIZE || buf == NULL) {
		return -1;
	}
	
	// if no file opened
	if (pcb->file_array[fd].flags == 0) {
		return -1;
	}
	
	read = read_data(pcb->file_array[fd].inode_num, pcb->file_array[fd].file_pos, (uint8_t*) buf, num);
	if (read == -1) {
		return -1;
	}
	
	// move file position
	pcb->file_array[fd].file_pos += read;
	
	return read;
}

/*
 * file_close
 *   Closes a file in the file system given a file descriptor
 *   Inputs: fd - file descriptor/index in file array
 *   Outputs: Returns 0
 *   Side effects: Removes file from file array.
 */
int file_close (uint32_t fd) {
	return 0;
}


/*
 * file_write
 *   Does nothing.
 *   Inputs: fd, buf, nbytes - do nothing
 *   Outputs: none
 */
int file_write (uint32_t fd, const void * buf, uint32_t nbytes) {
	return -1;
}

/*
 * program_imgcpy
 *   Copy program image into a contiguious section of memory.
 *   Inputs: prgm - string name of program image to write
 *			 memloc - memory address to write program image to
 *   Outputs: Returns -1 on failure, otherwise returens file size
 */
int program_imgcpy(uint8_t * prgm, void * memloc) {
	int read;
	dentry_t file_dentry;
	
	if (prgm == NULL || memloc == NULL) {
	    return -1;
	}
	
	if (-1 == read_dentry_by_name(prgm, &file_dentry)) {
		// return on failure
		return -1;
	}
	
	read = read_data(file_dentry.inode_num, 0, memloc, 0xFFFFFFFF); // read as many bytes as possible to memloc
	
	// read failed
	if (read == -1) {
		return -1;
	}
	
	return read;
}
