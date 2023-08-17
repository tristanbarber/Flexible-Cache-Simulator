/*
 * Tristan Barber
 * EEL 4768 Computer Architecture
 * Spring 2023
 * Flexible Cache Simulator
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <stdbool.h>

//Global variables (allows for less function parameters)
double hit = 0, miss = 0, writes = 0, reads = 0;
int cache_size = 0, assoc = 0, lru = 0, write_b = 0, num_sets = 0;


//Function declarations
int get_next_nonblank_line(char *buf, int max_length, FILE *ifp);
void remove_crlf(char *s);
void update_fifo(char op, long long int add, long long int tag_array[][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc]);
void update_lru(long long int add, long long int tag_array[][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc], int i);
void write_through( char op,long long int add, long long int tag_array[][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc]);
void write_back( char op,long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc]);
void read( char op,long long int add, long long int tag_array[][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc]);
void simulate_access(char op, long long int add, long long int tag_array[][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc]);


int main(int argc, char* argv[])
{
    if(argc != 6)
    {
        return 0;
    }

    //Setup argument variables and initialize them
    cache_size = atoi(argv[1]);
    assoc = atoi(argv[2]);
    FILE *inp = fopen(argv[5], "r");

    if(inp == NULL)
    {
        return 0;
    }

    if(!(atoi(argv[3])))
        lru = 1;

    if(atoi(argv[4]))
        write_b = 1;

    //Determine num_sets
    num_sets = cache_size/(assoc*64);

    //Initialize dependent variables
    long long int tag_array[num_sets][assoc];
    long long int data_array[num_sets][assoc];
    int dirty[num_sets][assoc];

    //Begin the simulation loop
    char op;
    long long int add = 0;
    char buf[4096];


    while(!feof(inp))
    {
        get_next_nonblank_line(buf, 4095, inp);
        sscanf(buf, "%c %llx", &op, &add);

        // Simulation begins
        simulate_access(op, add, tag_array, data_array, dirty);
        // simulation ends
    }

    //Print out the statistics
    printf("\nMiss Ratio: %f\n", (miss/(hit + miss)));
    printf("Writes: %f\n", writes);
    printf("Reads: %f\n\n", reads);

    return 0;
}

void simulate_access(char op, long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc])
{
    if(op == 'R')
        read(op, add, tag_array, data_array, dirty);
    else if(write_b == 0)
        write_through(op, add, tag_array, data_array, dirty);
    else
        write_back(op, add, tag_array, data_array, dirty);
}

void read(char op, long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc])
{
    int set = (int)((add/64)%num_sets);
    long long int tag = (long long int)(add/64);

    int i = 0;
    int found = 0;
	for(i = 0 ; i < assoc; i++)
    {
		if(tag_array[set][i] == tag)
        {
			found = 1;
			hit++;
			break;
		}
	}
    //Do nothing if found and FIFO
	if(found == 1 && lru == 0)
    {
		;
	}
    //Update LRU if found and LRU
    else if(found == 1 && lru == 1)
    {
        update_lru(add, tag_array, data_array, dirty, i);
    }
    //read from memory since we had a miss
    else
    {
		miss++;
		reads++;

        update_fifo(op, add, tag_array, data_array, dirty);
	}
}

void write_through(char op, long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc])
{
    int set = (int)((add/64)%num_sets);
    long long int tag = (long long int)(add/64);

    int found = 0, i = 0;
    for(i = 0; i < assoc; i++)
    {
	    if(tag_array[set][i] == tag)
        {
		    found = 1;
		    hit++;
            writes++;
		    break;
        }
    }
    //Do nothing if found and FIFO
	if(found == 1 && lru == 0)
    {
		;
	}
    //Update LRU if found and LRU
    else if(found == 1 && lru == 1)
    {
        update_lru(add, tag_array, data_array, dirty, i);
    }
    //Read from memory since we had a miss
    else
    {
	    read(op, add, tag_array, data_array, dirty);
	    writes++;
    }
}

void write_back(char op, long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc])
{
    int set = (int)((add/64)%num_sets);
    long long int tag = (long long int)(add/64);

    int found = 0, i = 0;
    for(i = 0; i < assoc; i++)
    {
	    if(tag_array[set][i] == tag)
        {
		    found = 1;
		    hit++;

            dirty[set][i] = 1;
		    break;
        }
    }
    //Do nothing if found and FIFO
	if(found == 1 && lru == 0)
    {
		;
	}
    //Update LRU if found and LRU
    else if(found == 1 && lru == 1)
        update_lru(add, tag_array, data_array, dirty, i);
    //Read from memory since we had a miss
    else
	    read(op, add, tag_array, data_array, dirty);
}


void update_lru(long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc], int i)
{
    int set = (int)((add/64)%num_sets);
    long long int tag_temp = tag_array[set][i];
    long long int data_temp = data_array[set][i];
    bool dirty_temp = dirty[set][i];

    //Update tag array
    for(int j = i; j > 0; j--)
        tag_array[set][j] = tag_array[set][j - 1];

    tag_array[set][0] = tag_temp;

    //Update data array
    for(int j = i; j > 0; j--)
        data_array[set][j] = data_array[set][j - 1];

    data_array[set][0] = data_temp;

    //Update dirty array
    for(int j = i; j > 0; j--)
        dirty[set][j] = dirty[set][j - 1];

    dirty[set][0] = dirty_temp;

}

void update_fifo(char op, long long int add, long long int tag_array[num_sets][assoc], long long int data_array[num_sets][assoc], int dirty[num_sets][assoc])
{
    int set = (int)((add/64)%num_sets);
    long long int tag = (long long int)(add/64);

    //Write if dirty bit is about to get evicted (and if write_back is true)
    if(write_b == 1 && dirty[set][assoc - 1] == 1)
        writes++;


    //Update tag array
    for(int i = assoc - 1; i > 0; i--)
        tag_array[set][i] = tag_array[set][i - 1];

    tag_array[set][0] = tag;

    //Update data array
    for(int i = assoc - 1; i > 0; i--)
        data_array[set][i] = data_array[set][i - 1];

    data_array[set][0] = add;

    //Update dirty array
    for(int i = assoc - 1; i > 0; i--)
        dirty[set][i] = dirty[set][i - 1];

    if(op == 'R')
        dirty[set][0] = 0;
    else
        dirty[set][0] = 1;
}









// Class library functions developed in CSI for FileIO

/* remove_crlf: traverse s and clobber any and all newline characters on its right
with nulls. s must be a properly allocated string. */

void remove_crlf(char *s)
{
/* Remember, in C, a string is an array of characters ending with a null - a '\
    '.  We're given the pointer to the FRONT of the string - the first printable character of
    the string. If we take n to be the length of the string, and move forward n characters from
    that, we find the null sentinel.  So let's use pointer arithmetic, combined with strlen(), to
    do exactly that.*/
    char *end = s + strlen(s);
/* end now points at the null sentinel.  But we don't actually want the null
    sentinel - we want the last substantive character of the string.  But since that lives one spot
    to the left of the null sentinel... */
    end--;
/* end now points to the last character of s.  We haven't changed the string
    yet, and we also haven't changed its pointer. */
/* Make sure end hasn't slipped off the left of the string, and check that we
    actually have a newline to remove.  Then, use the indirection operator to examine the
    actual chracter pointed to by end.  We take advantage of C's short circuit evaluation here -
    if end is to the left of s, we'll never try to (disastrously) read the end we can't
    read. */

/* Repeat all that until we either reach the left edge of the string or find
something that isn't a newline character.  The final effect is that we clobber everything
at the end of our string that's a newline character, and nothing else. */
    while((end >= s) && (*end == '\n' || *end == '\r')) {
        *end = '\0'; // clobber the character at end by overwriting it with null
        end--;
    }
}

/* get_next_nonblank_line(): reads the next line from ifp that isn't blank into
    buf, removing any newline characters.  Reads an empty string at EOF, and only then.
    Returns true if it read and false if it didn't. buf must be an allocated
    buffer large enough to hold max_length characters. ifp must be an open file that can be read. */

int get_next_nonblank_line(char *buf, int max_length, FILE *ifp)
{
    buf[0] = '\0'; // Start with an empty string
    while(!feof(ifp) && (buf[0] == '\0'))
    {
/* If we're not at the end of the file, and we haven't read a nonblank line
    yet, read a line and pull the CRLFs off the end. */
        fgets(buf, max_length, ifp);
        remove_crlf(buf);
    }
    return 1;
}
