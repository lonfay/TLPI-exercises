#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "sys/errno.h"

#ifndef BUF_SIZE
#define BUF_SIZE 40960
#endif

void printerr(int err){
    if(err == EBADF)
        printf("fd is not an open file descriptor.\n");
    else if(err == EINVAL)
        printf("whence  is  not  one  of  SEEK_SET,  SEEK_CUR,  SEEK_END; or the\
   resulting file offset would be negative, or beyond the end of a  seekable device.\n");
    else if(err == EOVERFLOW )
        printf(" The resulting file offset cannot be represented in an off_t.\n");
    else if(err == ESPIPE)
        printf("fd is associated with a pipe, socket, or FIFO.\n");
}


/*计算行数，并且查找定位*/
int locate(int fd, int nline, char* buf){

    off_t offseek;
    int len = 0;
    int offset, whence=SEEK_END,
            i = 0,
            numline = 0,
            line = 1, //如果行数多于buffer，重复往上增大offset
            off2end = 0; //距文件结尾的偏移量
    char* ptr = NULL;

    /*检查文件*/
    struct stat pstat;
    if(fstat(fd, &pstat) < 0){
        perror("fstat");
        return -1;
    }

    if(pstat.st_size <= BUF_SIZE){
        offset = 0;
        whence = SEEK_CUR;
    } else {
        offset = 0-BUF_SIZE; // 文件过大，直接跳到文件底部
        whence = SEEK_END;
    }


    while((offseek = lseek(fd, line * offset, whence)) != -1){
        line++;
        if((len = read(fd, buf, BUF_SIZE)) > 0){
            for(i = len-1; i > 0; i--){ //倒着循环
                if(buf[i] == '\n'){
                    if(++numline == nline + 1){
                        ptr = &buf[i];
                        break;
                    }
                }
            }

            if(numline != nline+1){
                off2end += offseek;
            }
            else{
                off2end += &buf[len-1] - ptr;
                break;
            }
        }
        memset(buf, 0, BUF_SIZE);
    }

    if(offseek == -1){
        printerr(errno);
        return -1;
    }

    return off2end;

}

int printd(int fd, int off2end, char* buf){
    int len = 0;

    if(off2end == -1 || lseek(fd, off2end, SEEK_END) == -1)
        return -1;
    memset(buf,0,BUF_SIZE);

    while((len = read(fd, buf, BUF_SIZE)) > 0){
        printf("%s", buf);
        memset(buf,0,BUF_SIZE);
    }
    return 0;

}



int main(int argc, char* argv[]){
    int c, lineNum=10, filed;
    char buf[BUF_SIZE];

    if((c = getopt(argc, argv, "n:")) != -1){
        switch (c){
            case 'n':
                lineNum = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-n lines]\n", argv[0]);
                return -1;
        }

    }
    if((filed = open(argv[optind], O_RDONLY)) == -1){
        printf("open %s failed!\n", argv[optind]);
        return -1;
    }
    printd(filed, (-locate(filed, lineNum, buf)),buf);

    if((close(filed) == 1)){
        printf("close failed.\n");
    }
    return(EXIT_SUCCESS);
}
