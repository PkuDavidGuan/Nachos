// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}
Directory::Directory(int size, int self, int father)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	    table[i].inUse = FALSE;
    
    table[0].inUse = true;
    table[0].isFirst = true;
    strcpy(table[0].name, ".");
    table[0].sector = self;
    table[1].inUse = true;
    table[1].isFirst = true;
    strcpy(table[1].name, "..");
    table[1].sector = father;
}
//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int nameBegin = -1;

int
Directory::FindIndex(char *name)
{
    int nameLength = strlen(name)+1;
    int copyBytes = 0;
    int nextEntry = -1;
    char *tmp = name;
    bool flag = false;
    for(int i = 0; i < tableSize; ++i)
    {
        name = tmp;        
        nameLength = strlen(name) + 1;
        flag = false;
        if(nameLength > FileNameMaxLen + 1)
            copyBytes = FileNameMaxLen + 1;
        else
            copyBytes = nameLength;
        for(int j = i; j < tableSize; ++j)
        {
            if(table[j].inUse && table[j].isFirst &&!strncmp(table[j].name, name, copyBytes))
            {
                nameLength -= copyBytes;
                nameBegin = j;
                if(nameLength == 0)
                    return j;
                nextEntry = ((table[j].sector < 0) ? -table[j].sector: table[j].sector);
                name += copyBytes;
                flag = true;
                break;
            }
        }
        if(flag == false)
            break;
        while(nameLength > 0)
        {
            if(nameLength > FileNameMaxLen + 1)
                copyBytes = FileNameMaxLen + 1;
            else
                copyBytes = nameLength;
            if(table[nextEntry].inUse && !strncmp(table[nextEntry].name, name, copyBytes))
            {
                nameLength -= copyBytes;
                if(nameLength == 0)
                    return nextEntry;
                nextEntry = (table[nextEntry].sector < 0) ? -table[nextEntry].sector: table[nextEntry].sector;
                name += copyBytes;
            }
            else
                break;
        }
    }
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector)
{ 
    if (FindIndex(name) != -1)
	return FALSE;

    int nameLength = strlen(name) + 1;
    int copyBytes = 0;
    int preEntry = -1;
    bool firstEntry = false;
    while(nameLength > 0)
    {
        if(nameLength > FileNameMaxLen+1)
            copyBytes = FileNameMaxLen+1;
        else
            copyBytes = nameLength;
        for (int i = 0; i < tableSize; i++)
        {
            if (!table[i].inUse) 
            {
                if(!firstEntry)
                {
                    table[i].isFirst = true;
                    firstEntry = true;
                }
                table[i].inUse = TRUE;
                strncpy(table[i].name, name, copyBytes);
                name = name + copyBytes;
                if(preEntry >= 0)
                    table[preEntry].sector = -i;
                preEntry = i;
                break;
	        }
        }
        nameLength -= copyBytes;
    }
    if(nameLength == 0)
    {
        ASSERT(preEntry >= 0);
        table[preEntry].sector = newSector;
        return true;
    }
    else
        return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    if (i == -1)
	return FALSE; 		// name not in directory
    int nextEntry = nameBegin;
    while(true)
    {
        ASSERT(nextEntry >= 0);
        table[nextEntry].inUse = false;
        table[nextEntry].isFirst = false;
        nextEntry = table[nextEntry].sector;
        if(nextEntry < 0)
            nextEntry = -nextEntry;
        else
            break;
    }
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
    bool notebook[tableSize];
    memset(notebook, 0, sizeof(notebook));
    int beginEntry;
    FileHeader *hdr;
    OpenFile *fd;
    Directory *subdir;
    for (int i = 0; i < tableSize; i++)
    {
        if (table[i].inUse && !notebook[i] && table[i].isFirst)
        {
            for(int j = 0; j < FileNameMaxLen+1; ++j)
            {
                if(table[i].name[j] != '\0')
                    printf("%c", table[i].name[j]);
                else
                    break;
            }
            notebook[i] = true;
            beginEntry = table[i].sector;
            while(beginEntry < 0)
            {
                for(int j = 0; j < FileNameMaxLen+1; ++j)
                {
                    if(table[-beginEntry].name[j] != '\0')
                        printf("%c", table[-beginEntry].name[j]);
                    else
                        break;
                }
                notebook[-beginEntry] = true;
                beginEntry = table[-beginEntry].sector;
            }
            printf("\n");
            hdr = new FileHeader;
            hdr->FetchFrom(beginEntry);
            if(i!=0 && i!=1 && hdr->GetFileType() == TYPE_DIR)
            {
                fd = new OpenFile(beginEntry);
                subdir = new Directory(10);
                subdir->FetchFrom(fd);
                printf("========sub dir:=========\n");
                subdir->List();
                printf("=======end sub dir=======\n");
                delete subdir;
                delete fd;
            }
            delete hdr;
        }      
    }       
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    int nextEntry;
    for (int i = 0; i < tableSize; i++)
	    if (table[i].inUse && table[i].isFirst) 
        {
            printf("--------------------------------------\n");
	        printf("Name: ");
            nextEntry = table[i].sector;
            for(int j = 0; j < FileNameMaxLen+1; ++j)
            {
                if(table[i].name[j] != '\0')
                    printf("%c", table[i].name[j]);
                else
                    break;
            }
            while(nextEntry < 0)
            {
                for(int j = 0; j < FileNameMaxLen+1; ++j)
                {
                    if(table[-nextEntry].name[j] != '\0')
                        printf("%c", table[-nextEntry].name[j]);
                    else
                        break;
                }
                nextEntry = table[-nextEntry].sector;
            }
            printf(", File header: %d\n", nextEntry);
	        hdr->FetchFrom(nextEntry);
	        hdr->Print();
	    }
    printf("\n");
    delete hdr;
}
