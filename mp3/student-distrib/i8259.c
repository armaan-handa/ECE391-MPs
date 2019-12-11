/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
/*
 * i8259_init()
 *      Initializes PIC
 *   Inputs: none
 *   Outputs: none
 *   Side effects: initializes PIC 
 */
void i8259_init(void) {
    cli();
    // init masks
    master_mask = 0xFF;
    slave_mask = 0xFF;
	
	// mask all interrupts on master and slave
    outb(master_mask, MASTER_DATA);
    outb(slave_mask, SLAVE_DATA);
    
	// Send ICW1 to master and slave ports 
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    // Send ICW2 to master and slave ports
    outb(ICW2_MASTER, MASTER_DATA);
    outb(ICW2_SLAVE, SLAVE_DATA);

    // Send ICW3 to master and slave ports
    outb(ICW3_MASTER, MASTER_DATA);
    outb(ICW3_SLAVE, SLAVE_DATA);

    // Send ICW4 to master and slave ports
    outb(ICW4, MASTER_DATA);
    outb(ICW4, SLAVE_DATA);
    
    enable_irq(SLAVE_IRQ); // enable slave

    printf("Init PIC\n");

}

/*
 * enable_irq()
 *      enables interrupts on given irq number
 *   Inputs: irq_num -- which irq to enable interrupts on
 *   Outputs: none
 *   Side effects: enables interrupts on given device
 */
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    //master_mask = inb(MASTER_DATA);
    //slave_mask = inb(SLAVE_DATA);
    if(irq_num > MAX_IRQ || irq_num < 0) // check if irq_num is valid
    {
        return;
    }
    if(irq_num >= SLAVE_OFFSET) // irq_num is on slave PIC
    {
        slave_mask &= ~(0x1 << (irq_num - SLAVE_OFFSET));
        outb(slave_mask, SLAVE_DATA);
        return;
    }
    else // irq_num is on master PIC
    {
        master_mask &= ~(0x1 << irq_num);
        outb(master_mask, MASTER_DATA);
        return;
    }
}

/*
 * disables_irq()
 *      disables interrupts on given irq number
 *   Inputs: irq_num -- which irq to disables interrupts on
 *   Outputs: none
 *   Side effects: disables interrupts on given device
 */
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    //master_mask = inb(MASTER_DATA);
    //slave_mask = inb(SLAVE_DATA);
    if(irq_num > MAX_IRQ || irq_num < 0) // check if irq_num is valid
    {
        return;
    }
    if(irq_num >= SLAVE_OFFSET) // irq_num is on slave PIC
    {
        slave_mask |= (0x1 << (irq_num - SLAVE_OFFSET));
        outb(slave_mask, SLAVE_DATA);
        return;
    }
    else // irq_num is on master PIC
    {
        master_mask |= (0x1 << irq_num);
        outb(master_mask, MASTER_DATA);
        return;
    }
}

/*
 * send_eoi()
 *      sends eoi to PIC for which device has been handles
 *   Inputs: irq_num -- which irq has been handles
 *   Outputs: none
 *   Side effects: sends eoi to PIC
 */
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    if(irq_num > MAX_IRQ || irq_num < 0) // check if irq_num is valid
    {
        return;
    }
    if(irq_num >= SLAVE_OFFSET) // irq_num is on slave PIC
    {
        outb(EOI | (irq_num - SLAVE_OFFSET), SLAVE_8259_PORT);
        outb(EOI | SLAVE_IRQ, MASTER_8259_PORT);
        return;
    }
    else // irq_num is on master PIC
    {
        outb(EOI | irq_num, MASTER_8259_PORT);
        return;
    }
}
