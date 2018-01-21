#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fs.h"

// for generate the random string
int rand_string( char * str, size_t size)
{
    if(size < 1)
        return 0;

    int n, key;

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$()"; //assign charset as all useful character

    for (n = 0; n < size; n++)
    {
        key = rand() % (int) (sizeof charset - 1);
        str[n] = charset[key];
    }
    str[size] = '\0';

    return size + 1;
}

// toggle bit at given index
void toggle_bit( char * array, int index )
{
    array[index/8] ^= 1 << ( index % 8 ); //do array[index/8] because char(array element) size 8 bits for 32 bit system
}

//get bit at given index
char get_bit( char * array, int index )
{
    return 1 & ( array[index/8] >> ( index % 8 )); //do bits and with 1 which give us bit at index in 1 or 0
}

//set bit at index in array as value
void set_bit( char * array, int index, char value )
{
    //check value is 1 or 0
    if( value != 0 && value != 1 )
    {
        return;
    }

	
    array[index/8] ^= 1 << ( index % 8 ); //toggle bit because we initial bit as garbage at first time run

    // check array index and value are same or not if yes then no need to toggle the bit
    if( get_bit( array, index ) == value)
    {
        return;
    }

    // else toggle the bit
    toggle_bit( array, index );
}

//get free inode position in inodemap
int get_free_inode()
{
    int i = 0;

    // search for the free inode
    for( i = 0; i < MAX_INODE; i++ )
    {
        // if free inode is found
        if( get_bit( inodeMap, i ) == 0) //check inodemap ith bit is 0 or not
        {
            //if 0 then set it as used and it will be used to write the file
            set_bit( inodeMap, i, 1 );

            // decrease the the no. of free inode count
            superBlock.freeInodeCount--;

            // return the position of the inode in inodemap onto which file will be written
            return i;
        }
    }

    return -1;
}

// this function returns the free blocks from blockmap
int get_free_block()
{
    int i = 0;

    for( i = 0; i < MAX_BLOCK; i++ )
    {
        // search for the free block
        if( get_bit( blockMap, i ) == 0 ) //check blockmap ith bit is 0 or not
        {
            // if 0 then mark that block as used so that
            set_bit( blockMap, i, 1 );
            
			// decrease the number of the free block count 
            superBlock.freeBlockCount--;

            // return the block position
            return i;
        }
    }

    return -1;
}

int format_timeval( struct timeval * tv, char * buf, size_t sz )
{
    // size that can be written within single operation
    ssize_t written = -1;
    struct tm * gm;
    gm = gmtime( &tv -> tv_sec );		// gmtime is used to fill the tm structures

    if( gm )
    {
        // formatting the time format using
        // buf is the destination
        // sz is the size of th formatted time
        // gm contains the calender broken into the pieces
        written = ( ssize_t )strftime( buf, sz, "%Y-%m-%d %H:%M:%S", gm );


        if(( written > 0 ) && (( size_t )written < sz ))
        {
            int w = snprintf( buf + written, sz - ( size_t )written, ".%06dZ", tv -> tv_usec );		//sends the formetted output to str at first
            // write the final time to the variable
            written = (w > 0) ? written + w : -1;
        }
    }
    return written;
}
