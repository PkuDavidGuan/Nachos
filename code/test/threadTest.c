/* threadTest.c 
 *    Test the syscall related to the thread.
 *
*/

#include "syscall.h"

int main()
{
    GYS();
    Yield();
    Exit(0);
}
