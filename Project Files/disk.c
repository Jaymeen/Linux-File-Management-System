#include <stdio.h>
#include <string.h>
#include "disk.h"

//buffer to store the whole disk in array
char disk[MAX_BLOCK][BLOCK_SIZE];

//read from disk and store it in buffer
int disk_read( int block, char * buf )
{
	//check for errors
    if( block < 0 || block >= MAX_BLOCK )
    {
        printf( "error while reading the disk \n" );

        return -1;
    }
    //copy into buffer
    memcpy( buf, disk[block], BLOCK_SIZE );

    return 0;
}

//flush the buffer into the disk
int disk_write( int block, char * buf)
{
    //check for errors
    if( block < 0 || block >= MAX_BLOCK )
    {
        printf( "error while writing to the disk\n" );

        return -1;
    }
	//move from buffer to disk
    memmove( disk[block], buf, BLOCK_SIZE );

    return 0;
}
//mount the file system on disk
int disk_mount( char * name )
{
    // opening file in read mode
    FILE *fp = fopen( name, "r" );
  
    if( fp != NULL )
    {
        //read from disk 
        fread( disk, BLOCK_SIZE, MAX_BLOCK, fp );

        // closing file
        fclose( fp );

        return 1;
    }

    return 0;
}



int disk_umount( char * name )
{
    // open file in write mode
    FILE * fp = fopen( name, "w" );

    if( fp == NULL )
    {
        fprintf( stderr, "disk_umount: file open error! %s\n", name );

        return -1;
    }

	//flush data into file from buffer
    fwrite( disk, BLOCK_SIZE, MAX_BLOCK, fp );
    // closing file
    fclose( fp );

    return 1;
}
