#include "fileManager.h"
#include "string.h"
FileManager::FileManager()
{
    l = new Lock("file manager");
    fileTable = new FileEntry[100];
    for(int i = 0; i < 100; ++i)
    {
        memset(fileTable[i].name, 0, sizeof(fileTable[i].name));
        fileTable[i].count = 0;
        fileTable[i].c = new Condition("file manager");
    }
}
FileManager::~FileManager()
{
    for(int i = 0; i < 100; ++i)
    {
        delete fileTable[i].c;
    }
    delete [] fileTable;
    delete l;
}
OpenFile* FileManager::Open(char *name)
{
    //printf("Open %s\n", name);
    l->Acquire();
    bool already = false;
    for(int i = 0; i < 100; ++i)
    {
        if(!strcmp(fileTable[i].name, name))
        {
            already = true;
            fileTable[i].count++;
            break;
        }
    }
    if(!already)
    {
        for(int i = 0; i < 100; ++i)
        {
            if(fileTable[i].count == 0)
            {
                strcpy(fileTable[i].name, name);
                printf("%s\n", fileTable[i].name);
                fileTable[i].count++;
                already = true;
                break;
            }
        }
    }
    ASSERT(already);
    OpenFile *ret = fileSystem->Open(name);
    l->Release();
    return ret;
}
void FileManager::Close(char *name)
{
    //printf("Close %s\n", name);
    bool ret = false;
    l->Acquire();
    for(int i = 0; i < 100; ++i)
    {
        if(!strcmp(fileTable[i].name, name))
        {
            ASSERT(fileTable[i].count > 0);
            
            fileTable[i].count--;
            if(fileTable[i].count == 0)
            {
                printf("signal\n");
                fileTable[i].c->Signal(l);
            }
            ret = true;
            break;
        }
    }
    ASSERT(ret);
    l->Release();
}
bool FileManager::Remove(char *name)
{
    //printf("Remove %s\n", name);
    bool ret = false;
    l->Acquire();
    printf("%s\n",fileTable[0].name);
    for(int i = 0; i < 100; ++i)
    {
        if(!strcmp(fileTable[i].name, name))
        {
            printf("I came to remove\n");
            while(fileTable[i].count > 0)
            {
                printf("wait\n");
                fileTable[i].c->Wait(l);
            }
            ret = fileSystem->Remove(name);
            break;
        }
    }
    l->Release();
    return ret;
}