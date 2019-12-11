#include "lib.h"
#include "x86_desc.h"
#include "exception.h"
#include "syscall.h"
#include "types.h"


/*
 *  init_exception_idt
 *  Inputs: none
 *  Outputs: Generated IDT
 *  Desc: The function generates the IDT array. The information for each 
 *       block of the IDT array is as follows
 *       -------------------------------------------
 *        idt_desc_t eh_idt_desc - name for building IDT entry
 *        eh_idt_desc.seg_selector - IDT handler exists in kernel code space
 *        eh_idt_desc.reserved4 = 0x0 - reserved bits, default to 0
 *        eh_idt_desc.reserved3 = 0, indicate that this is an interrupt gate
 *        eh_idt_desc.reserved2 = 1, indicate that this is an interrupt gate
 *        eh_idt_desc.reserved1 = 1, indicate that this is an interrupt gate
 *        eh_idt_desc.size = size of the interrupt 1 = 32 bits 0 = 16 bits
 *        eh_idt_desc.reserved0 = 0, indicate that this is an interrupt gate
 *        eh_idt_desc.dpl = level of allowed access, 0 for exceptions
 *        eh_idt_desc.present = 1 if the flag is present
 *        Flag is present for all except for entires with eh_unused, which are not
 *        
 *        SET_IDT_ENTRY(eh_idt_desc, handler_function) ; 
 *        idt[IDT ENTRY INDEX] = eh_idt_desc;
*/
void init_exception_idt () {
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_divide_by_zero) ; 
        idt[0x00] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_debug) ; 
        idt[0x01] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_nmi) ; 
        idt[0x02] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_breakpoint) ; 
        idt[0x03] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_overflow) ; 
        idt[0x04] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_bound_range) ; 
        idt[0x05] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_opcode) ; 
        idt[0x06] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_device_not_available) ; 
        idt[0x07] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_double_fault) ; 
        idt[0x08] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 0;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_unused) ; 
        idt[0x09] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_invalid_tss) ; 
        idt[0x0A] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_segment_not_present) ; 
        idt[0x0B] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_stack_segment_fault) ; 
        idt[0x0C] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_protection_fault) ; 
        idt[0x0D] = eh_idt_desc;
    }
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_page_fault) ; 
        idt[0x0E] = eh_idt_desc;
    }
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_x87_floating_point) ; 
        idt[0x10] = eh_idt_desc;
    }
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_alignment_check) ; 
        idt[0x11] = eh_idt_desc;
    }
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_machine_check) ; 
        idt[0x12] = eh_idt_desc;
    }
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_simd_floating_point) ; 
        idt[0x13] = eh_idt_desc;
    }
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_virtualization) ; 
        idt[0x14] = eh_idt_desc;
    }
    
    {
        idt_desc_t eh_idt_desc;
        eh_idt_desc.seg_selector = KERNEL_CS;
        eh_idt_desc.reserved4 = 0x0;
        eh_idt_desc.reserved3 = 0;
        eh_idt_desc.reserved2 = 1;
        eh_idt_desc.reserved1 = 1;
        eh_idt_desc.size = 1;
        eh_idt_desc.reserved0 = 0;
        eh_idt_desc.dpl = 0;
        eh_idt_desc.present = 1;
        
        SET_IDT_ENTRY(eh_idt_desc, eh_security) ; 
        idt[0x1E] = eh_idt_desc;
    }
    
}

void eh_divide_by_zero () {
    cli();
    clear();
    printf("EXCEPTION 0x00: Divide by zero\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_debug () {
    cli();
    clear();
    printf("EXCEPTION 0x01: Debug\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_nmi () {
    cli();
    clear();
    printf("EXCEPTION 0x02: Non-maskable interrupt\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
    
}

void eh_breakpoint () {
    cli();
    clear();
    printf("EXCEPTION 0x03: Breakpoint\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
    
}

void eh_overflow () {
    cli();
    clear();
    printf("EXCEPTION 0x04: Overflow\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
    
}

void eh_bound_range () {
    cli();
    clear();
    printf("EXCEPTION 0x05: Bound range exceeded\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
    
}

void eh_opcode () {
    cli();
    clear();
    printf("EXCEPTION 0x06: Invalid opcode\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
    
}

void eh_device_not_available () {
    cli();
    clear();
    printf("EXCEPTION 0x07: Device not available\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
    
}

void eh_double_fault () {
    cli();
    clear();
    printf("EXCEPTION 0x08: Double fault\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_invalid_tss () {
    cli();
    clear();
    printf("EXCEPTION 0x0A: Invalid TSS\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_segment_not_present () {
    cli();
    clear();
    printf("EXCEPTION 0x0B: Segment not present\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_stack_segment_fault () {
    cli();
    clear();
    printf("EXCEPTION 0x0C: Stack segment fault\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_protection_fault () {
    cli();
    clear();
    printf("EXCEPTION 0x0D: General protection fault\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_page_fault () {
    cli();
    clear();
    printf("EXCEPTION 0x0E: Page fault\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_x87_floating_point () {
    cli();
    clear();
    printf("EXCEPTION 0x10: x87 Floating Point Exception\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_alignment_check () {
    cli();
    clear();
    printf("EXCEPTION 0x11: Alignment check\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_machine_check () {
    cli();
    clear();
    printf("EXCEPTION 0x12: Machine check\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_simd_floating_point () {
    cli();
    clear();
    printf("EXCEPTION 0x13: SIMD Floating Point Exception\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_virtualization () {
    cli();
    clear();
    printf("EXCEPTION 0x14: Virtualization Exception\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_security () {
    cli();
    clear();
    printf("EXCEPTION 0x1E: Security Exception\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

void eh_unused () {
    cli();
    clear();
    printf("EXCEPTION: Unused exception handler. Something went wrong.\n");
	halt_syscall(0);
    sti();
    execute_syscall((uint8_t*)"shell");
}

