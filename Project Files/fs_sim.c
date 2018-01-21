// FS_SIM.C
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>

#include "fs.h"
#include "disk.h"

char fileName[256]= {};
pthread_t newThread;

void sigintHandler(int sig_num)
{
	//signal handling for unexpected break
    signal(SIGINT, sigintHandler);

    printf("\n Force close was called. Log file is being generated. \n");
    printf("./fs_sim was killed with exit status -1 \n");

    fs_umount(fileName);
    generate_log("filesystem was force closed",'\0','\0','\0','\0');
    pthread_create(&newThread,NULL,autosave_log,NULL);
    pthread_join(newThread,NULL);
    fflush(stdout);
    exit(-1);
}

bool command(char * comm, char * comm2)
{
    if( strlen( comm ) == strlen( comm2 ) && strncmp( comm, comm2, strlen( comm )) == 0 )
    {
        return true;
    }

    return false;
}

int main(int argc, char ** argv)
{
    time(&currTime);
    loggingFile = fopen( "logfile.txt", "ab+" );

    if( loggingFile == NULL )
    {
        fprintf( stderr, "error: failed to open logfile.txt!\n");

        return -1;
    }

    if( argc < 2 )
    {
        fprintf( stderr, "usage: ./fs disk_name\n" );

        return -1;
    }
    char comm[64];
    char arg1[16];
    char arg2[16];
    char arg3[16];
    char arg4[LARGE_FILE];
    char input[64 + 16 + 16 + 16 + LARGE_FILE];

    srand( time( NULL ));

    signal(SIGINT, sigintHandler);

    char *x=strcpy(fileName,argv[1]);
    srand( time( NULL ));

    //Call to fs.c file - which will pass file to disk.c to mount
    fs_mount( argv[1] );

    //Prints the standard prompt for input
    printf( "%% " );

    //The main while loop that will keep the simulator running until
    //  quit or exit called
    while( fgets( input, ( MAX_FILE_NAME + SMALL_FILE ), stdin ))
    {
        bzero( comm, 64 );
        bzero( arg1, 16 );
        bzero( arg2, 16 );
        bzero( arg3, 16 );
        bzero( arg4, LARGE_FILE );

        int numArg = sscanf( input, "%s %s %s %s %s", comm, arg1, arg2, arg3, arg4 );		// to change the format of displaying

        if( command( comm, "quit" ) || command( comm, "exit" )) break;
        else
        {
            execute_command(comm, arg1, arg2, arg3, arg4, numArg - 1);
            //generate_log(comm,arg1,arg2,arg3,arg4);
        }

        pthread_create(&newThread,NULL,autosave_log,NULL);
        pthread_join(newThread,NULL);

        printf("%% ");
    }

    //Call to fs.c file - which will pass file to disk.c to unmount
    generate_log("filesystem succesfully closed",NULL,NULL,NULL,NULL);

    //fclose(loggingfile);
    fs_umount(argv[1]);
}
