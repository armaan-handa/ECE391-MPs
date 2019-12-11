#include "keyboard.h"
#include "types.h"
#include "i8259.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal.h"
#include "syscall.h"
#include "pcb.h"


extern int rtc_flag;
extern uint8_t active_terminal;
extern int screen_y;
extern int screen_x;
extern int screen_x_cache[3];
extern int screen_y_cache[3];



uint8_t caps_flag = RELEASED;
uint8_t shift_flag = RELEASED;
uint8_t ctrl_flag = RELEASED;
uint8_t alt_flag = RELEASED;
volatile uint8_t enter_flag[3] = {0,0,0};

char key_buf[BUF_LIMIT];
uint8_t key_buf_idx;
char term_buf[3][BUF_LIMIT];
uint8_t buf_size;
// \0le containing scancode values
static char scancodes[NUM_KEYS] = {'\0','\0','1','2','3','4','5','6','7','8','9','0','-','=','\0',
                     '\0','q','w','e','r','t','y','u','i','o','p','[',']','\0',
                     '\0','a','s','d','f','g','h','j','k','l',';','\'','`',
                     '\0', '\\','z','x','c','v','b','n','m',',','.','/','\0','\0',
                     '\0',' ','\0','\0','\0','\0'};

static char scancodes_caps[NUM_KEYS] = {'\0','\0','1','2','3','4','5','6','7','8','9','0','-','=','\0',
                     '\0','Q','W','E','R','T','Y','U','I','O','P','[',']','\0',
                     '\0','A','S','D','F','G','H','J','K','L',';','\'','`',
                     '\0', '\\','Z','X','C','V','B','N','M',',','.','/','\0','\0',
                     '\0',' ','\0','\0','\0','\0'};

static char scancodes_shift[NUM_KEYS] = {'\0','\0','!','@','#','$','%','^','&','*','(',')','_','+','\0',
                     '\0','Q','W','E','R','T','Y','U','I','O','P','{','}','\0',
                     '\0','A','S','D','F','G','H','J','K','L',':','\"','~',
                     '\0', '|','Z','X','C','V','B','N','M','<','>','?','\0','\0',
                     '\0',' ','\0','\0','\0','\0'};

static char scancodes_caps_shift[NUM_KEYS] = {'\0','\0','!','@','#','$','%','^','&','*','(',')','_','+','\0',
                     '\0','q','w','e','r','t','y','u','i','o','p','{','}','\0',
                     '\0','a','s','d','f','g','h','j','k','l',';','\"','`',
                     '\0', '|','z','x','c','v','b','n','m','<','>','?','\0','\0',
                     '\0',' ','\0','\0','\0','\0'};
/*
 *
 *  FUNCTIONS
 *
 */

/*
 * init_keyboard()
 *      Initializes keyboard interrupts
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen, enables keyboard interrupts
 */
void init_keyboard()
{
    enable_irq(KEYBOARD_IRQ_NUM);
    printf("Init keyboard\n");
}

/*
 * cp1_keyboard_handler()
 *      Placeholder handler for checkpoint one
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen, enables/disables rtc test if F1/F2 are pressed
 */

void cp1_keyboard_handler ()
{
    cli();
    uint8_t c_in = 0;

    while(1)
    {
        if(inb(KEYBOARD_PORT) != 0)
        {
            c_in = inb(KEYBOARD_PORT); // get scan code from keyboard
            break;
        }
    }
    if(c_in != 0 && c_in < NUM_KEYS) // if scancode is part of 'main keyboard'
    {
        clear();
        printf("%s", scancodes[c_in]); // print out what key is pressed
        if(c_in == F1)
        {
            rtc_flag = 1; // enable rtc test
        }
        if(c_in == F2)
        {
            rtc_flag = 0; // disable rtc test
        }
    }
    send_eoi(KEYBOARD_IRQ_NUM);
    sti();
}

/*
 * keyboard_handler()
 *      Handles keyboard inputs
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen, fills terminal buffer when enter is pressed (to be used for read_terminal)
 */
void keyboard_handler()
{
    //printf("INTERRUPT\n");
    uint8_t c_in = inb(KEYBOARD_PORT); // get scan code from keyboard
    send_eoi(KEYBOARD_IRQ_NUM);

    pcb_t* pcb = get_pcb();
    
    // set video adress to VGA memory
    page_on_4kb ((void*) (VIDEO), (void*) (map_loc));
	page_on_4kb ((void*) (VIDEO), (void*) (VIDEO));

    // set cursor to current terminals cursor
    screen_x_cache[pcb->terminal] = screen_x;
    screen_y_cache[pcb->terminal] = screen_y;
    screen_x = screen_x_cache[current_terminal];
    screen_y = screen_y_cache[current_terminal];

    switch (c_in)
    {
    case CAPS_LOCK:
        caps_flag = !caps_flag;
        break;
    case L_SHIFT:
        shift_flag = PRESSED;
        break;
    case L_SHIFT_REL:
        shift_flag = RELEASED;
        break;
    case R_SHIFT:
        shift_flag = PRESSED;
        break;
    case R_SHIFT_REL:
        shift_flag = RELEASED;
        break;
    case BACKSPACE:
        backspace();
        break;
    case ENTER:
        enter();
        break;
    case CTRL:
        ctrl_flag = PRESSED;
        break;
    case CTRL_REL:
        ctrl_flag = RELEASED;
        break;
    case ALT:
        alt_flag = PRESSED;
        break;
    case ALT_REL:
        alt_flag = RELEASED;
        break;
    // TODO: add cases for function keys
    default:
        keypress(c_in);
        break;
    }

    
    // restore VIDEO pointer to currect buffer/VGA memory
    if (pcb->terminal == current_terminal) {
		page_on_4kb ((void*) (VIDEO), (void*) (map_loc));
		page_on_4kb ((void*) (VIDEO), (void*) (VIDEO));
	} else {
		page_on_4kb ((void*) (vmem_buffers[pcb->terminal]), (void*) (map_loc));
		page_on_4kb ((void*) (vmem_buffers[pcb->terminal]), (void*) (VIDEO));
	}

    // reset cursor
    screen_x_cache[current_terminal] = screen_x;
    screen_y_cache[current_terminal] = screen_y;
    screen_x = screen_x_cache[pcb->terminal];
    screen_y = screen_y_cache[pcb->terminal];
}

/*
 * keypress()
 *      handles character presses and ctrl + l/c
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen, clears screen if control + l is pressed
 *                  returns if ctrl + c is pressed
 */
void keypress(uint8_t _scancode)
{

    if(_scancode >= NUM_KEYS) // not a valid keystroke
    {
        return;
    }

    char key;

    // select different key array depending on shift/caps
    if (!shift_flag && !caps_flag)
    {
        key = scancodes[_scancode];
    }
    if (shift_flag && !caps_flag)
    {
        key = scancodes_shift[_scancode];
    }
    if (!shift_flag && caps_flag)
    {
        key = scancodes_caps[_scancode];
    }
    if (shift_flag && caps_flag)
    {
        key = scancodes_caps_shift[_scancode];
    }

    if(ctrl_flag)
    {
        switch (key)
        {
        case 'l': // ctrl + l means clear screen
            clear();
            // TODO: reset screen pos
            reset_cursor();
			key_buf_idx = 0;
            break;

        case 'c': // ctrl+ c means return
            // asm volatile(
            //     "movl $1 , %%eax 
            //      movl $-1, %%ebx
            //      int $0x80 
            //      popl %%ebx
            //      ret"
            //      :
            //      :
            //      :"eax", "ebx"
            // );
            return;
            break;
        }
    }
    if(alt_flag)
    {
        // TODO: launch terminals
        switch(_scancode)
        {
            case F1:
                // launch terminal 1
				active_terminal = 0;
                break;
            case F2:
                // launch terminal 2
				active_terminal = 1;
                break;
            case F3:
                // launch terminal 3
				active_terminal = 2;
                break;
            default:
                // do nothing
                break;
        }
    }
    else
    {
        add_to_key_buf(key);
    }


}

/*
 * add_to_key_buf()
 *      appends  character to keybuffer
 *   Inputs: none
 *   Outputs: none
 *   Side effects: Prints to screen
 */
void add_to_key_buf(char key)
{
    if(key_buf_idx < BUF_LIMIT)
    {
        key_buf[key_buf_idx++] = key;
        putc(key);
        set_cursor();
    }
}

/*
 * enter()
 *      handles enter. clears key buffer and writes to terminal buffer
 *   Inputs: none
 *   Outputs: none
 *   Side effects: goes to next line, sets enter flag, clears buffer, sets terminal buffer
 */
void enter()
{
    int i;
    // TODO: process stuff
    key_buf[key_buf_idx++] = '\n'; // add new line to buffer
    // TODO: set screen position to next line
    //putc('\n');
    next_line();
    // clear terminal buffer
    for (i = 0; i < BUF_LIMIT; i++)
    {
        term_buf[current_terminal][i] = '\0';
    }
    //copy key buffer to terminal buffer
    for(i = 0; i < key_buf_idx; i++)
    {
        term_buf[current_terminal][i] = key_buf[i];
    }
    buf_size = ++i; // sets buffer size
    //clear key buffer
    for (i = 0; i < BUF_LIMIT; i++)
    {
        key_buf[i] = '\0';
    }
    enter_flag[current_terminal] = 1; // set enter flag
    key_buf_idx = 0; // set key buffer index
}

/*
 * backspace()
 *      handles backspace
 *   Inputs: none
 *   Outputs: none
 *   Side effects: removes previous character, changes key buffer
 */
void backspace()
{
    if (key_buf_idx > 0)
    {
		back_cursor();
        // TODO: set screen position to one character before current
        key_buf_idx--;
        key_buf[key_buf_idx] = '\0'; // set buffer val to NULL
    }
}

