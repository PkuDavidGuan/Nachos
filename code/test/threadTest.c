/* threadTest.c 
 *    Test the syscall related to the thread.
 *
*/

#include "syscall.h"
int main()
{
    int tid;
    tid = Exec("../test/sort");
    Join(tid);
    Exit(0);
}
