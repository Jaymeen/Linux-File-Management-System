//---IMPORT(S)---
#include <sys/time.h>
#include "disk.h"

//---DEFINITION(S)---
#define MAX_BLOCK 4096
#define MAX_INODE 512
#define SMALL_FILE 5120
#define LARGE_FILE 70656
#define MAX_FILE_NAME 16
#define MAX_DIR_ENTRY BLOCK_SIZE / sizeof( DirectoryEntry )


typedef enum {file, directory} TYPE;

//Super Block data structure
typedef struct
{
    int freeBlockCount;
    int freeInodeCount;
    char padding[504];
} SuperBlock;

//iNode Information
typedef struct
{
    TYPE type;
    struct timeval lastAccess;
    struct timeval created;
    int owner;
    int group;
    int size;
    int blockCount;
    int indirectBlock;
    int directBlock[10];
    char padding[24];
} Inode; // 128 bytes

//Each directory entry
typedef struct
{
    int inode;
    char name[MAX_FILE_NAME];
} DirectoryEntry;

//Entire dicrectory - One data block
typedef struct
{
    DirectoryEntry dentry[MAX_DIR_ENTRY];
    int numEntry;
    char padding[8];
} Dentry;

//---GLOBAL VARIABLE(S)---
//CHAR(S)
extern char inodeMap[MAX_INODE / 8];
extern char blockMap[MAX_BLOCK / 8];
extern int logCounter;
extern FILE* loggingFile;
extern time_t currTime;
extern struct tm* logTime;
//STRUCT(S)
extern SuperBlock superBlock;

//---METHOD INSTANTIATION(S)---
int fs_mount( char * name );
int fs_umount( char * name );
int dir_change( char * name );
int execute_command( char * comm, char * arg1, char * arg2, char * arg3, char * arg4, int numArg );
void generate_log(char* message,char* arg1,char *arg2,char*arg3,char*arg4);
void *autosave_log(void *vargp);
