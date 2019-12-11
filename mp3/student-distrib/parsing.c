#include "parsing.h"
#include "types.h"
#include "pcb.h"

/*
 * parseString()
 *      Description: takes an input string and parses it using space as divider
 *      Inputs:     input -- the string to parse.
 *      Outputs:    command -- the string in which to place the first word.
 *                  args -- string to put the rest of the words in.
 *      Returns: none
 */
void parseString(const char* input, char* command, uint8_t args[ARG_LIMIT])
{
    int i = 0; // loop counter
    int wordPos = 0; // position of the end of the first word

    // get command
    while(1)
    {
        if(input[i] == SPACE || input[i] == NULL_CHAR) // on first space break loop and start filling args
        {
            command[i] = NULL_CHAR; // terminate command with NULL
            i++;
			wordPos = i;
            break;
        }
        command[i] = input[i];
        i++;
    }

    // get args
    while(input[i] != NULL_CHAR && i - wordPos < ARG_LIMIT) // if arg is too large or the string is terminated by NULL, we're done
    {   
        args[i - wordPos] = (uint8_t)input[i]; 
        
        i++;
    }
	
	// terminate argument string
	args[i - wordPos] = NULL_CHAR;
}
