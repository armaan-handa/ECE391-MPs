#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "keyboard.h"
#include "paging.h"
#include "filesys.h"
#include "rtc.h"
#include "terminal.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER     \
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)   \
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
    /* Use exception #15 for assertions, otherwise
       reserved by Intel */
    asm volatile("int $15");
}

extern char buf[BUF_LIMIT];
extern uint8_t buf_idx;

/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 10; ++i){
        if ((idt[i].offset_15_00 == NULL) &&
            (idt[i].offset_31_16 == NULL)){
            assertion_failure();
            result = FAIL;
        }
    }

    return result;
}

// add more tests here
/*
 * idt_size_test()
 *   Asserts: IDT entires are of the correct size
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen
 */
int idt_size_test()
{
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 10; i++)
    {
        if(sizeof(idt[i]) != sizeof(idt_desc_t))
        {
            printf("size: %zu\n", sizeof(idt[i]));
            assertion_failure();
            result = FAIL;
        }
    }
    return result;
}

/*
 * idt_interrupt_test()
 *   Asserts: Generates a system call
 *   Inputs: none
 *   Outputs: none
 *   Side effects: System calls handler in IDT
 */
void idt_interrupt_test()
{
    asm volatile (
        //"movl $2, %eax \n"
        "int $0x80"
    );
}

/*
 * page_access_test()
 *   Asserts: Paging allows access to correct memory
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen
 */
int page_access_test(){
	//TEST_HEADER;
    int pointer = 0;
	int temp = 0;

	// access first 10 elements in second page directory
    for (pointer = FOUR_MI_B; pointer < FOUR_MI_B + 10; pointer++) {
		temp = *((int*) pointer);
    }

    return PASS;
}

/*
 * page_fault_test()
 *   Asserts: page fault is invoked correctly
 *   Inputs: none
 *   Outputs: none
 *   Side effects: causes page fault exception
 */

int page_fault_test()
{
	TEST_HEADER;
	int* pointer = (int *)(FOUR_KI_B * PAGE_TABLE_SIZE * 2);
	int temp;

	// tries to access third page directory, which does not exist. This is to invoke a page fault
	for (; (int) pointer < FOUR_KI_B * PAGE_TABLE_SIZE * 3; pointer++) {
        temp = *pointer;
    }
	return FAIL;
}

/*
 * null_page()
 *   Asserts: Passing NULL pointer into allow_paging fails
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen
 */

int null_page()
{
	TEST_HEADER;
	// clear();
	allow_paging(NULL);
	return PASS;
}

/*
 * divide_by_zero()
 *   Asserts: exception handler for divide by 0 is working
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen, causes divide by zero exception
 */
int divide_by_zero(){
    TEST_HEADER;

    int j = 0;
    printf("%d", 1/j);
    return FAIL;
}

int dereference_null_test() {
	int* i = NULL;
	int j = 0;
	j += *i;
	return FAIL;
}


/* Checkpoint 2 tests */

/*
 * directory_test()
 *   Asserts: Tests if the directory has been properly accessed
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints files in directory to screen
 */
 int directory_test() {
 	unsigned char buf[1002];
	unsigned int read;
	clear();
	buf[1000] = '\n';
	buf[1001] = '\0';
	
	read = fdir_read(0, buf, 1000);
	
	if (read == -1) {
		printf("FAILED OPEN");
		return FAIL;
	}
	
	printf((char*)buf);
	return PASS;
}

/*
 * file_read_test()
 *   Asserts: Tests if a basic file can be read
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints file contents to screen
 */
int file_read_test() {
	int fd;
	unsigned int read;
	unsigned char buf[277];
	clear();
	buf[276] = '\n';
	buf[277] = '\0';

	fd = file_open((uint8_t*) "frame0.txt");
	if (fd == -1) {
		printf("FAILED OPEN");
		return FAIL;
	}

	read = file_read(fd, buf, 275);
	if (read == -1) {
		printf("FAILED READ");
		return FAIL;
	}

	buf[read] = '\0';

	printf((char*)buf);

	file_close(fd);


	return PASS;
}

/*
 * file_read_test_large()
 *   Asserts: Tests if a file with more than two blocks can be read
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints file contents to screen
 */
int file_read_test_large() {
	int fd;
	unsigned int read;
	unsigned char buf[4098];
	clear();
	buf[4096] = '\n';
	buf[4097] = '\0';
	fd = file_open((uint8_t*) "verylargetextwithverylongname.txt");
	if (fd == -1) {
		printf("FAILED OPEN");
		return FAIL;
	}

	read = file_read(fd, buf, 4096);
	if (read == -1) {
		printf("FAILED READ");
		return FAIL;
	}

	buf[read] = '\0';

	printf((char*)buf);

	clear();

	read = file_read(fd, buf, 4096);
	if (read == -1) {
		printf("FAILED READ");
		return FAIL;
	}

	buf[read] = '\0';

	printf((char*)buf);

	file_close(fd);


	return PASS;
}

/*
 * file_program_image_test()
 *   Asserts: Tests if a program image can be copied into memory
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints the length and last bytes of file
 */
int file_program_image_test(char * prgm){
	char buf[4096 * 40]; // max 40kb
	int read;

	read = program_imgcpy((uint8_t *) prgm, (void *) buf);
	if (read == -1) {
		return FAIL;
	}

	clear();
	printf("%d\n", read);
	write_terminal(0, buf + read - 26, 26);
	return PASS;
}

/*
 * file_read_test_offset()
 *   Asserts: Tests if a file can be read from the middle of a block
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints file contents to screen
 */
int file_read_test_offset() {
	int fd;
	unsigned int read;
	unsigned char buf[4098];
	clear();
	buf[4096] = '\n';
	buf[4097] = '\0';
	fd = file_open((uint8_t*) "verylargetextwithverylongname.txt");
	if (fd == -1) {
		printf("FAILED OPEN");
		return FAIL;
	}

	read = file_read(fd, buf, 13); // read first 13 bytes
	if (read == -1) {
		printf("FAILED READ");
		return FAIL;
	}

	buf[read] = '\0';

	printf((char*)buf);

	clear();

	read = file_read(fd, buf, 20); // read next 20 bytes
	if (read == -1) {
		printf("FAILED READ");
		return FAIL;
	}

	buf[read] = '\0';

	printf((char*)buf);

	read = file_read(fd, buf, 4096); // read rest of file
	if (read == -1) {
		printf("FAILED READ");
		return FAIL;
	}

	buf[read] = '\0';

	printf((char*)buf);

	file_close(fd);

	return PASS;
}

/*
 * test_read_rtc()
 *   Asserts: Tests if you can wait/iterate a program with use of rtc_read()
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Should spam screen
 */
int test_read_rtc(){
    TEST_HEADER;

    int i = 0;
    for (i = 0; i < 10; i++) {
    read_rtc(1, (void*)1, 1);
    printf("%d ", i);
    }
    if (i == 10) {
        return PASS;
    }
    else {
        return FAIL;
    }
}

/*
 * test_write_rtc()
 *   Asserts: Tests if you can wait/iterate a program with use of rtc_read(),
 *   and also change the frequency to all values
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Should spam screen
 */
int test_write_rtc(){
    TEST_HEADER;

    clear();
    reset_cursor();
    int i = 0;
    int arg = 2;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 5; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

	  clear();
    reset_cursor();
	  arg = 4; 
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 10; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 8;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 20; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 16;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 40; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 32;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 80; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 64;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 75; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 128;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 75; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 256;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 100; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 512;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 150; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 1024;
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 300; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear(); //Will reset to 2Hz
    reset_cursor();
	  open_rtc((uint8_t*)1);
    for (i = 0; i < 5; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
	  arg = 3; 	//Will default to 1024Hz, showing the default cast AND the max speed case
    write_rtc(1, (void*)arg, sizeof(arg));
    for (i = 0; i < 300; i++) {
    read_rtc(1, (void*)1, 1);
    putc(49);
    }

    clear();
    reset_cursor();
    if (i == 300) {
        return PASS;
    }
    else {
        return FAIL;
    }
}

/*
 * test_open_close_rtc()
 *   Asserts: Tests open and close functions for rtc, shoudld be working if no crash
 *   Inputs: none
 *   Outputs: none
 *   Side effects: resets RTC to 2Hz
 */
int test_open_close_rtc(){
    TEST_HEADER;

    int i = 0;
	i += close_rtc(1);
	i += open_rtc((uint8_t*)1);

    if (i == 0) {
        return PASS;
    }
    else {
        return FAIL;
    }
}

/*
 * test_write_rtc_trash()
 *   Asserts: Tests wether or not write_rtc rejects trash inputs
 *   Inputs: none
 *   Outputs: none
 *   Side effects: nothing
 */
int test_write_rtc_trash(){
    TEST_HEADER;

    int i = 0;
	//Testing if fail on arg != 4bytes
	i += write_rtc(1, (void*)1, 3);
	//Testing whether fail on null pointer
	i += write_rtc(1, NULL, sizeof(i));

    if (i == -2) {
        return PASS;
    }
    else {
        return FAIL;
    }
}


/*
 * test_read_write_terminal()
 *   Asserts: functionality of read_terminal and write_terminal
 *   Inputs: none
 *   Outputs: none
 *   Side effects: takes input from user, write to screen.
 */
int test_read_write_terminal()
{
	TEST_HEADER;

	int length;
	int ret = PASS;
    while(1)
    {
        void* temp[BUF_LIMIT];
	    printf("Type something:\n");
	    length = read_terminal(0, temp, BUF_LIMIT);
	    printf("You typed: ");
	    write_terminal(0, temp, length);
        printf("\n");
    }
	
	return ret;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	//clear();
/* CHECKPOINT 1 */
    // TEST_OUTPUT("idt_test", idt_test());
    // launch your tests here
    // TEST_OUTPUT("idt_size_test", idt_size_test());
   //  TEST_OUTPUT("divide_by_zero", divide_by_zero());
	// TEST_OUTPUT("dereference_null_test", dereference_null_test());
	// TEST_OUTPUT("null_page", null_page());
	// TEST_OUTPUT("page_fault_test", page_fault_test());
	// idt_interrupt_test();
/* CHECKPOINT 2 */
	 //TEST_OUTPUT("directory_test",directory_test());
    // TEST_OUTPUT("file_read_test",file_read_test());
    // TEST_OUTPUT("file_read_test_large",file_read_test_large());
	//TEST_OUTPUT("file_program_image_test", file_program_image_test("hello"));
	// TEST_OUTPUT("file_program_image_test", file_program_image_test("counter"));
	// TEST_OUTPUT("file_program_image_test", file_program_image_test("this_is_bad_input")); //Should fail
    // TEST_OUTPUT("file_read_test_offset",file_read_test_offset());
    // TEST_OUTPUT("test_read_rtc", test_read_rtc());
    // TEST_OUTPUT("test_write_rtc", test_write_rtc());
	// TEST_OUTPUT("test_open_close_rtc", test_open_close_rtc());
	// TEST_OUTPUT("test_write_rtc_trash", test_write_rtc_trash())
	// TEST_OUTPUT("test_read_terminal", test_read_terminal());
	// TEST_OUTPUT("test_write_terminal", test_write_terminal());
	// TEST_OUTPUT("test_read_write_terminal", test_read_write_terminal());
    assertion_failure();
/* CHECKPOINT 3 */
}
