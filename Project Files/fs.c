#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "fs.h"
#include "fs_util.h"
#include "disk.h"

Inode inode[MAX_INODE];
SuperBlock superBlock;
Dentry curDir;

int hasRemovedBefore = 0;
int curDirBlock;
int currentDirectoryInode;
char inodeMap[MAX_INODE / 8];
char blockMap[MAX_BLOCK / 8];

int logCounter=1;
FILE* loggingFile=NULL;
time_t currTime;
struct tm* logTime=NULL;

void generate_log(char* message,char* arg1,char *arg2,char*arg3,char*arg4)
{
    logTime=localtime(&currTime);
    fprintf(loggingFile, "%d:%.24s:%s:%s:%s:%s\n",logCounter,asctime(logTime),message,arg1,arg2,arg3,arg4);
    ++logCounter;
    fflush(loggingFile);
    //fclose(loggingFile);
}

void *autosave_log(void *vargp)
{
    int TIME_TO_SLEEP=10;
    if(loggingFile!=NULL) fflush(loggingFile);
    if(loggingFile!=NULL) fclose(loggingFile);
    printf("Autosaving log....\n");
    //sleep(TIME_TO_SLEEP);
    loggingFile=fopen("logfile.txt","ab+");

}


int fs_mount(char * name) {
    int i = 0;
    int index = 0;
    int inode_index = 0;
    int numInodeBlock = (sizeof(Inode) * MAX_INODE) / BLOCK_SIZE;
    if (disk_mount(name) == 1) {
        disk_read(0, (char * ) & superBlock);
        disk_read(1, inodeMap);
        disk_read(2, blockMap);
        for (i = 0; i < numInodeBlock; i++) {
            index = i + 3;
            disk_read(index, (char * )(inode + inode_index));
            inode_index += (BLOCK_SIZE / sizeof(Inode));
        }
        curDirBlock = inode[0].directBlock[0];
        disk_read(curDirBlock, (char * ) & curDir);
    } else {
        superBlock.freeBlockCount = MAX_BLOCK - (1 + 1 + 1 + numInodeBlock);
        superBlock.freeInodeCount = MAX_INODE;
        for (i = 0; i < MAX_INODE / 8; i++) {
            set_bit(inodeMap, i, 0);
        }
        for (i = 0; i < MAX_BLOCK / 8; i++) {
            if (i < (1 + 1 + 1 + numInodeBlock)) {
                set_bit(blockMap, i, 1);
            } else {
                set_bit(blockMap, i, 0);
            }
        }
        int rootInode = get_free_inode();
        currentDirectoryInode = rootInode;
        curDirBlock = get_free_block();
        inode[rootInode].type = directory;
        inode[rootInode].owner = 0;
        inode[rootInode].group = 0;
        gettimeofday( & (inode[rootInode].created), NULL);
        gettimeofday( & (inode[rootInode].lastAccess), NULL);
        inode[rootInode].size = 1;
        inode[rootInode].blockCount = 1;
        inode[rootInode].directBlock[0] = curDirBlock;
        curDir.numEntry = 1;
        strncpy(curDir.dentry[0].name, ".", 1);
        curDir.dentry[0].name[1] = NULL;
        curDir.dentry[0].inode = rootInode;
        currentDirectoryInode = rootInode;
        disk_write(curDirBlock, (char * ) & curDir);
    }
    return 0;
}
int fs_umount(char * name) {
    int numInodeBlock = (sizeof(Inode) * MAX_INODE) / BLOCK_SIZE;
    int i, index, inode_index = 0;
    disk_write(0, (char * ) & superBlock);
    disk_write(1, inodeMap);
    disk_write(2, blockMap);
    for (i = 0; i < numInodeBlock; i++) {
        index = i + 3;
        disk_write(index, (char * )(inode + inode_index));
        inode_index += (BLOCK_SIZE / sizeof(Inode));
    }
    disk_write(curDirBlock, (char * ) & curDir);
    disk_umount(name);
    return 0;
}
int search_cur_dir(char * name) {
    int i;
    for (i = 0; i < MAX_DIR_ENTRY; i++) {
        if (command(name, curDir.dentry[i].name)) {
            return curDir.dentry[i].inode;
        }
    }
    return -1;
}
int file_create(char * name, int size) {
    int foundSpot = 0;
    int i;
    int inodeNum;
    int numBlock;
    if (size >= SMALL_FILE) {
        printf("File create error: Do not support files larger than %d bytes yet.\n", SMALL_FILE);
        return -1;
    }
    inodeNum = search_cur_dir(name);
    if (inodeNum >= 0) {
        printf("File create error:  %s exist.\n", name);
        return -1;
    }
    printf("the generated random string was: ");
    if (curDir.numEntry + 1 > (BLOCK_SIZE / sizeof(DirectoryEntry))) {
        printf("File create error: directory is full!\n");
        return -1;
    }
    numBlock = size / BLOCK_SIZE;
    if (size % BLOCK_SIZE > 0) {
        numBlock++;
    }
    if (numBlock > superBlock.freeBlockCount) {
        printf("File create error: not enough blocks\n");
        return -1;
    }
    if (superBlock.freeInodeCount < 1) {
        printf("File create error: not enough inodes\n");
        return -1;
    }
    char * tmp = (char * ) malloc(sizeof(int) * size + 1);
    rand_string(tmp, size);
    printf("%s\n", tmp);
    inodeNum = get_free_inode();
    if (inodeNum < 0) {
        printf("File create error: not enough inode.\n");
        free(tmp);
        return -1;
    }
    inode[inodeNum].type = file;
    inode[inodeNum].owner = 1;
    inode[inodeNum].group = 2;
    gettimeofday( & (inode[inodeNum].created), NULL);
    gettimeofday( & (inode[inodeNum].lastAccess), NULL);
    inode[inodeNum].size = size;
    inode[inodeNum].blockCount = numBlock;
    if (hasRemovedBefore == 1) {
        for (i = 0; i < MAX_DIR_ENTRY; i++) {
            if ((curDir.dentry[i].inode == 0) && strcmp(curDir.dentry[i].name, "") == 0) {
                strncpy(curDir.dentry[i].name, name, strlen(name));
                curDir.dentry[i].name[strlen(name)] = NULL;
                curDir.dentry[i].inode = inodeNum;
                curDir.numEntry++;
                for (i = 0; i < numBlock; i++) {
                    int block = get_free_block();
                    if (block == -1) {
                        printf("File create error: get_free_block failed\n");
                        free(tmp);
                        return -1;
                    }
                    inode[inodeNum].directBlock[i] = block;
                    disk_write(block, tmp + (i * BLOCK_SIZE));
                }
                printf("File created: %s, inode %d, size %d\n", name, inodeNum, size);
                free(tmp);
                return 0;
            }
        }
    } else {
        strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
        curDir.dentry[curDir.numEntry].name[strlen(name)] = NULL;
        curDir.dentry[curDir.numEntry].inode = inodeNum;
        curDir.numEntry++;
        for (i = 0; i < numBlock; i++) {
            int block = get_free_block();
            if (block == -1) {
                printf("File create error: get_free_block failed\n");
                free(tmp);
                return -1;
            }
            inode[inodeNum].directBlock[i] = block;
            disk_write(block, tmp + (i * BLOCK_SIZE));
        }
        printf("File created: %s, inode %d, size %d\n", name, inodeNum, size);
        free(tmp);
    }
    return 0;
}
int file_cat(char * name) {
    int i = 0;
    int inodeNum = 0;
    int blockNum = 0;
    char fileContents[MAX_BLOCK];
    inodeNum = search_cur_dir(name);
    if (inodeNum == -1) {
        printf("File cat error: file does not exist\n");
        memset(fileContents, 0, sizeof(fileContents));
        return -1;
    } else {
        blockNum = inode[inodeNum].blockCount;
        for (i = 0; i < blockNum; i++) {
            int block = inode[inodeNum].directBlock[i];
            disk_read(block, fileContents);
            printf("%s", fileContents);
        }
        memset(fileContents, 0, sizeof(fileContents));
        printf("\n");
    }
    gettimeofday( & (inode[inodeNum].lastAccess), NULL);
    return 0;
}
int file_read(char * name, int offset, int size) {
    int i = 0;
    int inodeNum = 0;
    int blockNum = 0;
    char fileContents[MAX_BLOCK + MAX_BLOCK];
    char tempContentsHolder[MAX_BLOCK + MAX_BLOCK];
    size_t len;
    if (offset < 0 || size < 0) {
        if (offset < 0 && size < 0) {
            printf("File read error: Can not have an offset & size less than 0\n");
        } else if (offset < 0) {
            printf("File read error: Can not have an offset less than 0\n");
        } else if (size < 0) {
            printf("File read error: Can not have a size less than 0\n");
        }
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
        return 0;
    }
    inodeNum = search_cur_dir(name);
    if (inodeNum == -1) {
        printf("File read error: file does not exist\n");
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
        return 0;
    } else {
        blockNum = inode[inodeNum].blockCount;
        for (i = 0; i < blockNum; i++) {
            int block = inode[inodeNum].directBlock[i];
            disk_read(block, fileContents);
            strcat(tempContentsHolder, fileContents);
        }
        len = strlen(tempContentsHolder);
        if (offset > len) {
            printf("File read error: The offset is greater than the size of the file contents\n");
            memset(fileContents, 0, sizeof(fileContents));
            memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
            return 0;
        } else {
            printf("%.*s\n", size, (tempContentsHolder + offset));
        }
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
    }
    gettimeofday( & (inode[inodeNum].lastAccess), NULL);
    return 0;
}
int file_write(char * name, int offset, int size, char * buf) {
    int i = 0;
    int inodeNum = 0;
    int blockNum = 0;
    char temp[MAX_BLOCK + MAX_BLOCK];
    char temp2[MAX_BLOCK + MAX_BLOCK];
    char temp3[MAX_BLOCK + MAX_BLOCK];
    char fileContents[MAX_BLOCK + MAX_BLOCK];
    char tempContentsHolder[MAX_BLOCK + MAX_BLOCK];
    size_t len;
    if (offset < 0 || size < 0) {
        if (offset < 0 && size < 0) {
            printf("File write error: Can not have an offset & size less than 0\n");
        } else if (offset < 0) {
            printf("File write error: Can not have an offset less than 0\n");
        } else {
            printf("File write error: Can not have a size less than 0\n");
        }
        memset(temp, 0, sizeof(temp));
        memset(temp2, 0, sizeof(temp2));
        memset(temp3, 0, sizeof(temp3));
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
        return 0;
    }
    if (size != strlen(buf)) {
        printf("File write error: The size you entered doesn't match the length of the buffer string you entered\n");
        memset(temp, 0, sizeof(temp));
        memset(temp2, 0, sizeof(temp2));
        memset(temp3, 0, sizeof(temp3));
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
        return 0;
    }
    inodeNum = search_cur_dir(name);
    if (inodeNum == -1) {
        printf("File write error: file does not exist\n");
        memset(temp, 0, sizeof(temp));
        memset(temp2, 0, sizeof(temp2));
        memset(temp3, 0, sizeof(temp3));
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
        return -1;
    } else {
        blockNum = inode[inodeNum].blockCount;
        for (i = 0; i < blockNum; i++) {
            int block = inode[inodeNum].directBlock[i];
            disk_read(block, fileContents);
            strcat(tempContentsHolder, fileContents);
        }
        len = strlen(tempContentsHolder);
        if (offset > len) {
            printf("The offset is greater than the size of the file contents\n");
            memset(temp, 0, sizeof(temp));
            memset(temp2, 0, sizeof(temp2));
            memset(temp3, 0, sizeof(temp3));
            memset(fileContents, 0, sizeof(fileContents));
            memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
            return 0;
        } else {
            sprintf(temp3, "%.*s", offset, (tempContentsHolder));
            sprintf(temp, "%s", buf);
            strcat(temp3, temp);
            sprintf(temp2, "%s", (tempContentsHolder + offset + size));
            strcat(temp3, temp2);
            printf("%s\n", temp3);
            if (inode[inodeNum].size == strlen(temp3)) {
                int k = 0;
                for (k = 0; k < blockNum; k++) {
                    int block = inode[inodeNum].directBlock[k];
                    disk_write(block, temp3 + (k * BLOCK_SIZE));
                }
            } else {
                if (strlen(temp3) >= SMALL_FILE) {
                    printf("File write error: Do not support files larger than %d bytes yet.\n", SMALL_FILE);
                    memset(temp, 0, sizeof(temp));
                    memset(temp2, 0, sizeof(temp2));
                    memset(temp3, 0, sizeof(temp3));
                    memset(fileContents, 0, sizeof(fileContents));
                    memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
                    return -1;
                }
                int newBlockNum = (size + strlen(tempContentsHolder)) / BLOCK_SIZE;
                if ((size + strlen(tempContentsHolder)) % BLOCK_SIZE > 0) {
                    newBlockNum++;
                }
                if (newBlockNum == blockNum) {
                    inode[inodeNum].size = strlen(temp3);
                    int k = 0;
                    for (k = 0; k < blockNum; k++) {
                        int block = inode[inodeNum].directBlock[k];
                        disk_write(block, temp3 + (k * BLOCK_SIZE));
                    }
                } else {
                    int count = 0;
                    int blockDifference = newBlockNum - blockNum;
                    if (blockDifference > superBlock.freeBlockCount) {
                        printf("File write error: File create failed: not enough space\n");
                        memset(temp, 0, sizeof(temp));
                        memset(temp2, 0, sizeof(temp2));
                        memset(temp3, 0, sizeof(temp3));
                        memset(fileContents, 0, sizeof(fileContents));
                        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
                        return -1;
                    }
                    inode[inodeNum].size = strlen(temp3);
                    inode[inodeNum].blockCount = newBlockNum;
                    int k = 0;
                    for (k = 0; k < newBlockNum; k++) {
                        if (count != blockDifference) {
                            int block = inode[inodeNum].directBlock[k];
                            disk_write(block, temp3 + (k * BLOCK_SIZE));
                            count++;
                        } else {
                            int block = get_free_block();
                            if (block == -1) {
                                printf("File write error: get_free_block failed\n");
                                memset(temp, 0, sizeof(temp));
                                memset(temp2, 0, sizeof(temp2));
                                memset(temp3, 0, sizeof(temp3));
                                memset(fileContents, 0, sizeof(fileContents));
                                memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
                                return -1;
                            }
                            inode[inodeNum].directBlock[k] = block;
                            disk_write(block, temp3 + (k * BLOCK_SIZE));
                        }
                    }
                }
            }
        }
        gettimeofday( & (inode[inodeNum].lastAccess), NULL);
        memset(temp, 0, sizeof(temp));
        memset(temp2, 0, sizeof(temp2));
        memset(temp3, 0, sizeof(temp3));
        memset(fileContents, 0, sizeof(fileContents));
        memset(tempContentsHolder, 0, sizeof(tempContentsHolder));
    }
    return 0;
}
int file_remove(char * name) {
    int inodeNum = 0;
    int i = 0;
    int counter = 0;
    inodeNum = search_cur_dir(name);
    if (inodeNum < 0) {
        printf("File remove failed:  %s file doesn't exist.\n", name);
        return -1;
    }
    if (inode[inodeNum].type == file) {
        for (i = 0; i < MAX_DIR_ENTRY; i++) {
            if (curDir.dentry[i].inode == inodeNum) {
                hasRemovedBefore = 1;
                strncpy(curDir.dentry[i].name, "", strlen(""));
                curDir.dentry[i].name[strlen("")] = NULL;
                curDir.dentry[i].inode = 0;
                set_bit(inodeMap, inodeNum, 0);
                superBlock.freeInodeCount++;
                curDir.numEntry--;
                int numBlock = inode[inodeNum].blockCount;
                for (i = 0; i < numBlock; i++) {
                    set_bit(blockMap, inode[inodeNum].directBlock[i], 0);
                }
                superBlock.freeBlockCount = numBlock + superBlock.freeBlockCount;
                gettimeofday( & (inode[inodeNum].lastAccess), NULL);
                break;
            }
        }
    } else {
        printf("File remove error: This is a directory, can't delete it\n");
        return -1;
    }
    return 0;
}
int file_stat(char * name) {
    int inodeNum = search_cur_dir(name);
    char timebuf[28];
    if (inodeNum < 0) {
        printf("File cat error: file is not exist.\n");
        return -1;
    }
    printf("Inode = %d\n", inodeNum);
    if (inode[inodeNum].type == file) {
        printf("type = file\n");
    } else {
        printf("type = directory\n");
    }
    printf("owner = %d\n", inode[inodeNum].owner);
    printf("group = %d\n", inode[inodeNum].group);
    printf("size = %d\n", inode[inodeNum].size);
    printf("num of block = %d\n", inode[inodeNum].blockCount);
    format_timeval( & (inode[inodeNum].created), timebuf, 28);
    printf("Created time = %s\n", timebuf);
    format_timeval( & (inode[inodeNum].lastAccess), timebuf, 28);
    printf("Last accessed time = %s\n", timebuf);
    return 0;
}
int dir_make(char * name) {
    int i;
    int parentDirInode;
    int currentDirInode;
    int oldCurDirBlock;
    int inodeNum = search_cur_dir(name);
    if (inodeNum >= 0) {
        printf("Directory create failed: %s exist.\n", name);
        return -1;
    }
    if (curDir.numEntry + 1 > (BLOCK_SIZE / sizeof(DirectoryEntry))) {
        printf("Directory create failed: directory is full!\n");
        return -1;
    }
    if (superBlock.freeInodeCount < 1) {
        printf("Directory create failed: not enough inodes\n");
        return -1;
    }
    int directoryInode = get_free_inode();
    if (directoryInode < 0) {
        printf("Directory create error: not enough inode.\n");
        return -1;
    }
    oldCurDirBlock = curDirBlock;
    curDirBlock = get_free_block();
    if (curDirBlock == -1) {
        printf("Directory create error: get_free_block failed\n");
        return -1;
    }
    inode[directoryInode].type = directory;
    inode[directoryInode].owner = 1;
    inode[directoryInode].group = 2;
    gettimeofday( & (inode[directoryInode].created), NULL);
    gettimeofday( & (inode[directoryInode].lastAccess), NULL);
    inode[directoryInode].size = 1;
    inode[directoryInode].blockCount = 1;
    inode[directoryInode].directBlock[0] = curDirBlock;
    if (hasRemovedBefore == 1) {
        for (i = 0; i < MAX_DIR_ENTRY; i++) {
            if ((curDir.dentry[i].inode == 0) && strcmp(curDir.dentry[i].name, "") == 0) {
                parentDirInode = currentDirectoryInode;
                strncpy(curDir.dentry[i].name, name, strlen(name));
                curDir.dentry[i].name[strlen(name)] = NULL;
                curDir.dentry[i].inode = directoryInode;
                currentDirInode = directoryInode;
                curDir.numEntry++;
                break;
            }
        }
    } else {
        parentDirInode = currentDirectoryInode;
        strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
        curDir.dentry[curDir.numEntry].name[strlen(name)] = NULL;
        curDir.dentry[curDir.numEntry].inode = directoryInode;
        currentDirInode = directoryInode;
        curDir.numEntry++;
    }
    disk_write(oldCurDirBlock, (char * ) & curDir);
    bzero( & curDir, sizeof(curDir));
    curDir.numEntry = 0;
    strncpy(curDir.dentry[curDir.numEntry].name, ".", 1);
    curDir.dentry[curDir.numEntry].name[1] = NULL;
    curDir.dentry[curDir.numEntry].inode = currentDirInode;
    curDir.numEntry++;
    strncpy(curDir.dentry[curDir.numEntry].name, "..", 2);
    curDir.dentry[curDir.numEntry].name[2] = NULL;
    curDir.dentry[curDir.numEntry].inode = parentDirInode;
    curDir.numEntry++;
    disk_write(inode[directoryInode].directBlock[0], (char * ) & curDir);
    disk_read(oldCurDirBlock, (char * ) & curDir);
    curDirBlock = oldCurDirBlock;
    printf("Directory created: %s, inode %d, size %d\n", name, directoryInode, inode[directoryInode].size);
    return 0;
}
int dir_remove(char * name) {
    int i;
    int j;
    if (strcmp(name, ".") == 0) {
        printf("Directory remove error: Can't remove the directory you are in.\n");
        return -1;
    } else if (strcmp(name, "..") == 0) {
        printf("Directory remove error: Can't remove parent directory because it contains files.\n");
        return -1;
    }
    int directoryInodeNum = search_cur_dir(name);
    if (directoryInodeNum < 0) {
        printf("Directory remove error: directory does not exist.\n");
        return -1;
    }
    if (inode[directoryInodeNum].type == directory) {
        dir_change(name);
        if (curDir.numEntry > 2) {
            printf("Directory remove error: The directory has files in it. Cannot remove\n");
            dir_change("..");
            return -1;
        }
        dir_change("..");
        hasRemovedBefore = 1;
        for (j = 0; j < MAX_DIR_ENTRY; j++) {
            if (curDir.dentry[j].inode == directoryInodeNum) {
                strncpy(curDir.dentry[j].name, "", strlen(""));
                curDir.dentry[j].name[strlen("")] = NULL;
                curDir.dentry[j].inode = 0;
                set_bit(inodeMap, directoryInodeNum, 0);
                superBlock.freeInodeCount++;
                set_bit(blockMap, inode[directoryInodeNum].directBlock[i], 0);
                superBlock.freeBlockCount++;
                curDir.numEntry--;
            }
        }
    } else {
        printf("Directory remove error: You are trying to remove a file. Please check the name of the directory and try again.\n");
        return -1;
    }
    return 0;
}
int dir_change(char * name) {
    if (strcmp(name, ".") == 0) {
        printf("You can't go where you already are!\n");
        return 0;
    } else if (strcmp(name, "..") == 0) {
        int changeFromDirectoryInode = search_cur_dir(".");
        int changeFromDirectoryCurDirBlock = inode[changeFromDirectoryInode].directBlock[0];
        int changeToParentDirectoryInode = search_cur_dir("..");
        int changeToParentDirectoryCurDirBlock = inode[changeToParentDirectoryInode].directBlock[0];
        if (changeFromDirectoryInode == 0) {
            printf("Change directory error: Currently in this directory\n");
            return 0;
        }
        disk_write(changeFromDirectoryCurDirBlock, (char * ) & curDir);
        currentDirectoryInode = changeToParentDirectoryInode;
        curDirBlock = changeToParentDirectoryCurDirBlock;
        gettimeofday( & (inode[currentDirectoryInode].lastAccess), NULL);
        disk_read(changeToParentDirectoryCurDirBlock, (char * ) & curDir);
    } else {
        int changeToDirectoryInode = search_cur_dir(name);
        if (changeToDirectoryInode < 0) {
            printf("Change directory error: file is not exist.\n");
            return -1;
        }
        currentDirectoryInode = changeToDirectoryInode;
        if (inode[changeToDirectoryInode].type == directory) {
            int changeToDirectoryCurDirBlock = inode[changeToDirectoryInode].directBlock[0];
            int changeFromDirectoryCurDirBlock = inode[search_cur_dir(".")].directBlock[0];
            disk_write(changeFromDirectoryCurDirBlock, (char * ) & curDir);
            curDirBlock = changeToDirectoryCurDirBlock;
            gettimeofday( & (inode[changeToDirectoryInode].lastAccess), NULL);
            disk_read(changeToDirectoryCurDirBlock, (char * ) & curDir);
        } else {
            printf("Change directory error: Type is file, not directory\n");
            return -1;
        }
    }
    return 0;
}
int ls(char* arg1) {
    int i;
    if(strcmp(arg1,"-i")==0)
        printf("name \t\ttype\t\tinode\tsize(bytes)\n");
    else
        printf("name \t\ttype\t\tsize(bytes)\n");
    for (i = 0; i < MAX_DIR_ENTRY; i++) {
        int n = curDir.dentry[i].inode;
        if ((inode[n].size != 1) || (strcmp(curDir.dentry[i].name, "") != 0)) {
            printf("%-16s", curDir.dentry[i].name);
            if (inode[n].type == file) printf("file\t\t");
            else printf("dir	  \t");
            if(strcmp(arg1,"-i")==0)
                printf("%3d \t", curDir.dentry[i].inode);
            printf("%4d \n", inode[n].size);
        }
    }
    return 0;
}
int fs_stat() {
    printf("File System Status: \n");
    printf("# of free blocks: %d (%d bytes), # of free inodes: %d\n", superBlock.freeBlockCount, superBlock.freeBlockCount * 512, superBlock.freeInodeCount);
    return 0;
}

int execute_command(char * comm, char * arg1, char * arg2, char * arg3, char * arg4, int numArg)
{

    time(&currTime);

    loggingFile = fopen("logfile.txt","ab+");
    if(loggingFile==NULL) {
        printf("error opening logfile, exiting -1");
        exit(-1);
    }
    if (command(comm, "create")) {
        if (numArg < 2) {
            printf("Error: create <filename> <size>\n");
            return -1;
        }


        //generate_log();


//			fprintf(loggingFile, "%.24s : File %s created of size %s bytes\n",asctime(mytime),arg1,arg2);
        generate_log("There was an attempt to create a file ->",arg1,arg2,NULL,NULL);

        return file_create(arg1, atoi(arg2));
    } else if (command(comm, "cat")) {
        if (numArg < 1) {
            printf("Error: cat <filename>\n");
            return -1;
        }
        //mytime=localtime(&currTime);
        //fprintf(loggingFile, "%.24s : There was an attempt to access file \" %s\" file\n",asctime(mytime),arg1);
        generate_log("There was an attempt to access file ->",arg1,NULL,NULL,NULL);
        return file_cat(arg1);
    } else if (command(comm, "write")) {
        if (numArg < 4) {
            printf("Error: write <filename> <offset> <size> <buf>\n");
            return -1;
        }
        //mytime=localtime(&currTime);
        //fprintf(loggingFile, "%.24s : There was an attempt to modify file \" %s\"\n",asctime(mytime),arg1);
        generate_log("There was an attempt to write to a file named -> ",arg1,arg2,arg3,arg4);
        return file_write(arg1, atoi(arg2), atoi(arg3), arg4);
    } else if (command(comm, "read")) {
        if (numArg < 3) {
            printf("Error: read <filename> <offset> <size>\n");
            return -1;
        }
        generate_log("There was an attempt to read file ->",arg1,NULL,NULL,NULL);
        return file_read(arg1, atoi(arg2), atoi(arg3));
    } else if (command(comm, "rm")) {
        if (numArg < 1) {
            printf("Error: rm <filename>\n");
            return -1;
        }
        generate_log("There was an attempt to remove file->",arg1,NULL,NULL,NULL);
        return file_remove(arg1);
    } else if (command(comm, "mkdir")) {
        if (numArg < 1) {
            printf("Error: mkdir <dirname>\n");
            return -1;
        }
        generate_log("There was an attempt to make a directory ->",arg1,NULL,NULL,NULL);
        return dir_make(arg1);
    } else if (command(comm, "rmdir")) {
        if (numArg < 1) {
            printf("Error: rmdir <dirname>\n");
            return -1;
        }
        generate_log("There was an attempt to remove directory->",arg1,NULL,NULL,NULL);
        return dir_remove(arg1);
    } else if (command(comm, "cd")) {
        if (numArg < 1) {
            printf("Error: cd <dirname>\n");
            return -1;
        }
        generate_log("There was an attempt to change directory to ->",arg1,NULL,NULL,NULL);
        return dir_change(arg1);
    } else if (command(comm, "ls")) {
        return ls(arg1);
    } else if (command(comm, "stat")) {
        if (numArg < 1) {
            printf("Error: stat <filename>\n");
            return -1;
        }
        generate_log("There was an attempt to access statistics of file ->",arg1,NULL,NULL,NULL);
        return file_stat(arg1);
    } else if (command(comm, "df")) {
        generate_log("There was an attempt to access disk usage utility",NULL,NULL,NULL,NULL);
        return fs_stat();
    } else {
        fprintf(stderr, "%s: command not found.\n", comm);
        return -1;
    }
    return 0;
}
