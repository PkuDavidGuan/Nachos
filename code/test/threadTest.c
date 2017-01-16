/* threadTest.c 
 *    Test the syscall related to the thread.
 *
*/

#include "syscall.h"
void testFork()
{
    int a = 2;
    a = 3+5;
}
int main()
{
    GYS();
    Fork(testFork);
    GYS();
    Exit(0);
}
