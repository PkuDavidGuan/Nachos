/* fileTest.c 
 *    Test the syscall related to the file system.
 *
*/

#include "syscall.h"

int main()
{
    int fd, i;
    int tmp = 0xdeadbeef;

    GYS();
    /*Create a file*/
    //Create("../filesys/test/lab6");
    /*open a file*/
    fd = Open("../filesys/test/lab6");
    Write((char *)&tmp, 4, fd);
    Close(fd);
    fd = Open("../filesys/test/lab6");
    tmp = 0;
    Read((char *)&tmp, 4, fd);
    Close(fd);
    Exit(0);
}
