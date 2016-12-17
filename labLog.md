#David's log
##Lab4 part 1
###exercise2
>     the flow of pagefault: 
oneinstruction->readMem->Translate(return false)->raiseExecption(badaddr=addr)->*exceptionHandler*->readMem(return false)->oneinstruction(return directly)

###exercise3
use LFU and LRU, add count, frequency and recent into tlbentry

##Lab4 part 1 log
###2016/11/21 13:54
I finished the exercise 2 and 3, and delete a threadtest in threadTest.cc. However, I have no time to compile and there may be  some bugs... Now, I will have a lunch break.lalala^_^