/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"
#include "keyboard.h"
#include "exception.h"
#include "rtc.h"
#include "paging.h"
#include "linkage.h"
#include "filesys.h"
#include "syscall_linkage.h"
#include "syscall.h"
#include "types.h"
#include "scheduling.h"

#define RUN_TESTS 1

/* Macros. */

/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;

    /* Clear the screen. */
    clear();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
			if (mod_count == 0) { // file system is first module
				printf("Loading file system module\n");
				printf("Module string: ");
				printf((char*) mod->string);
				printf("\n");
				filesys_init(mod);
			}
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char*)(mod->mod_start+i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
                (unsigned)elf_sec->num, (unsigned)elf_sec->size,
                (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
                (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (memory_map_t *)mbi->mmap_addr;
                (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
            printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
                    (unsigned)mmap->size,
                    (unsigned)mmap->base_addr_high,
                    (unsigned)mmap->base_addr_low,
                    (unsigned)mmap->type,
                    (unsigned)mmap->length_high,
                    (unsigned)mmap->length_low);
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }

    {
        idt_desc_t syscall_idt_desc;
        syscall_idt_desc.seg_selector = KERNEL_CS;
        syscall_idt_desc.reserved4 = 0x0;
        syscall_idt_desc.reserved3 = 1;
        syscall_idt_desc.reserved2 = 1;
        syscall_idt_desc.reserved1 = 1;
        syscall_idt_desc.size = 1;
        syscall_idt_desc.reserved0 = 0;
        syscall_idt_desc.dpl = 3; // DPL is 3 to allow user level access
        syscall_idt_desc.present = 1;

        SET_IDT_ENTRY(syscall_idt_desc, system_call_handler) ;
        idt[0x80] = syscall_idt_desc; // 0x80 is defined IDT for system calls
    }

    {
        idt_desc_t keyboard_idt_desc;
        keyboard_idt_desc.seg_selector = KERNEL_CS;
        keyboard_idt_desc.reserved4 = 0x0;
        keyboard_idt_desc.reserved3 = 0;
        keyboard_idt_desc.reserved2 = 1;
        keyboard_idt_desc.reserved1 = 1;
        keyboard_idt_desc.size = 1;
        keyboard_idt_desc.reserved0 = 0;
        keyboard_idt_desc.dpl = 0;
        keyboard_idt_desc.present = 1;

        //printf("keyboard interrupt occured\n");
        SET_IDT_ENTRY(keyboard_idt_desc, keyboard_linker) ;
        idt[0x21] = keyboard_idt_desc; // 0x21 is IDT entry for keyboards
    }

    {
        idt_desc_t rtc_idt_desc;
        rtc_idt_desc.seg_selector = KERNEL_CS;
        rtc_idt_desc.reserved4 = 0x0;
        rtc_idt_desc.reserved3 = 0;
        rtc_idt_desc.reserved2 = 1;
        rtc_idt_desc.reserved1 = 1;
        rtc_idt_desc.size = 1;
        rtc_idt_desc.reserved0 = 0;
        rtc_idt_desc.dpl = 0;
        rtc_idt_desc.present = 1;

        SET_IDT_ENTRY(rtc_idt_desc, rtc_linker) ;
        idt[0x28] = rtc_idt_desc; // 0x28 is defined IDT for RTC interrupts
    }

    {
        idt_desc_t pit_idt_desc;
        pit_idt_desc.seg_selector = KERNEL_CS;
        pit_idt_desc.reserved4 = 0x0;
        pit_idt_desc.reserved3 = 0;
        pit_idt_desc.reserved2 = 1;
        pit_idt_desc.reserved1 = 1;
        pit_idt_desc.size = 1;
        pit_idt_desc.reserved0 = 0;
        pit_idt_desc.dpl = 0;
        pit_idt_desc.present = 1;

        SET_IDT_ENTRY(pit_idt_desc, pit_linker) ;
        idt[0x20] = pit_idt_desc; // 0x20 is defined IDT for PIT interrupts
    }

    init_exception_idt();
    //clear();
    /* Init the PIC */
    i8259_init();
    init_keyboard();
    init_rtc(); //This tests open_rtc() because all that open_rtc() does is call this func and return 0;
    init_pit();
    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */
    init_pages();


    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    printf("Enabling Interrupts\n");
    sti();

#ifdef RUN_TESTS
    /* Run tests */
    // launch_tests();
#endif
    /* Execute the first program ("shell") ... */
    clear();
	execute_syscall((uint8_t*) "shell");
    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
