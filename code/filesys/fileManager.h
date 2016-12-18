#ifndef FILEMANAGE_H
#define FILEMANAGE_H

#include "copyright.h"
#include "synch.h"
#include "system.h"
#include "filesys.h"

typedef struct fileEntry_
{
    char name[100];
    int count;
    Condition *c;
} FileEntry;
class FileManager
{
public:
    FileManager();
    ~FileManager();
    bool Create(char *name, int initialSize, int type)
    {
        return fileSystem->Create(name, initialSize, type);
    }
    OpenFile* Open(char *name); 	// Open a file (UNIX open)

    bool Remove(char *name);  		// Delete a file (UNIX unlink)
    void Close(char *name);
private:
    Lock *l;
    FileEntry *fileTable;
};
#endif 
