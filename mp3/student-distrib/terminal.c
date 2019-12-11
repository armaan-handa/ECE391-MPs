#include "terminal.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "x86_desc.h"
#include "keyboard.h"
#include "pcb.h"

extern char term_buf[3][BUF_LIMIT];
extern uint8_t buf_size;
extern volatile uint8_t enter_flag[3];

/*
 * open_terminal
 *      does nothing
 *      INPUTS: ignore all inputs
 *      OUTPUTS: none
 *      RETURNS: 0
 */
int32_t open_terminal(const uint8_t* filename)
{
    return 0;
}

/*
 * close_terminal
 *      does nothing
 *      INPUTS: ignore all inputs
 *      OUTPUTS: none
 *      RETURNS: 0
 */
int32_t close_terminal(uint32_t fd)
{
    return 0;
}

/*
 * read_terminal
 *      reads from keyboard buffer to given buffer after enter is pressed
 *      INPUTS: fd -- ignored
 *              buf -- the buf into which we write the keyboard buffer
 *              nbytes -- number of characters to read
 *      OUTPUTS: buf -- filled in buffer
 *      RETURNS: number of byts read
 */
int32_t read_terminal(uint32_t fd, void* buf, uint32_t nbytes)
{
    int i, len;
    pcb_t * pcb = get_pcb();

    while (enter_flag[pcb->terminal] == 0); // wait for enter to be pressed
    enter_flag[pcb->terminal] = 0;
    len = (nbytes > buf_size ? buf_size : nbytes); // if nbytes is greater than the number of chars in buffer, set length to number of chars in buffer
    i = 0;
    // clear buffer
    while(((char*)buf)[i] != '\0')
    {
        ((char*)buf)[i] = '\0';
        i++;
    }
     // copy into buffer
    for (i = 0; (i < len - 2) && (i < BUF_LIMIT); i++)
    {
        ((char*)buf)[i] = term_buf[pcb->terminal][i];
    }
    //((char*)buf)[i++] = '\n';
    return len;
}

/*
 * write_terminal
 *      writes given buffer onto screen
 *      INPUTS: fd -- ignored
 *              buf -- the buf from which we write to screen
 *              nbytes -- number of characters to write
 *      OUTPUTS: buf -- filled in buffer
 *      RETURNS: number of byts printed
 */
int32_t write_terminal(uint32_t fd, const void* buf, uint32_t nbytes)
{
    int i;
    for (i = 0; i < nbytes; i++)
    {
        putc(((char*)buf)[i]);
    }
    return nbytes;
}
