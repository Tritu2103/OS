/* ----------------------------------------------------------------------------------------
	File System.
	Course: MAT3501 - Principles of Operating Systems, MIM - HUS
	Summary: Provide basic functions to access FS file system. Public FS functions start 
		with fs_ prefix.
		To start using FS, one must mount the disk file to load neccessary FS meta data 
		onto memory. 
		When finish using FS, one must unmount the disk file to update FS changes back to disk.
------------------------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include "fs.h"

#define B2b_t "%c%c%c%c%c%c%c%c"
#define B2b(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


extern char disk_name[128];   
extern int  disk_size;        
extern int  disk_fd;          
extern int  disk_blockCount;  

static superBlock_t superBlock;		
static inode_t inodeTable[BLOCKSIZE/sizeof(inode_t)];	
static fileDescriptor_t fileDescriptors[MAXOPENFILES]; 
static int fileCount;					
static int rootFd;						
static int numInodes;					
static int currentDirFd;				


int wildCmp(char *pPattern, char *pString);
static int getFreeInode();
static int getBlockIndex(int fd, uint16_t blockSeq) ;
static void addFreeBlock(uint16_t blockNo);
static int getFreeBlock();
static int openFileByInode(uint16_t inodeNo);
static int searchBlockByName(uint16_t blockNo, char *pFileName, dirRecord_t *pDirRecord);
static int path2inode(char *pPath, dirRecord_t *pDirRecord, uint16_t *pParentInodeNo);

// for the mini project
#include "checkdisk.c"


/* format disk
	pDiskName: disk name
	diskSize: disk size in bytes
	return: 0 on success, <0 on faults
*/
int fs_format(char *pDiskName, int diskSize)
{	int i, ret, *pVal;
	char buf[BLOCKSIZE];
	dirRecord_t dirRecord;

    strcpy (disk_name, pDiskName);
    disk_size = diskSize;
    disk_blockCount = disk_size / BLOCKSIZE;

    disk_fd = open (disk_name, O_RDWR);
    if (disk_fd == -1) return (-1);		
	
    
    superBlock.inodeBlock = 2;
    superBlock.rootDirBlock = 3;
	superBlock.rootInodeNo = 0;
	superBlock.dataBlock = 3;
	superBlock.freeBlockList = superBlock.dataBlock + 1;
	superBlock.blockSize=BLOCKSIZE;
	superBlock.freeBlockCount=disk_size / BLOCKSIZE - 4;
	ret = writeBlock(1, &superBlock);
	if (ret<0) return(-2);
	
	
	memset(&buf, 0, BLOCKSIZE);
	strcpy(dirRecord.fileName,".");
	dirRecord.inodeNo = superBlock.rootInodeNo;
	dirRecord.deleted = 0;
	memcpy(buf, &dirRecord, sizeof(dirRecord_t));
	strcpy(dirRecord.fileName,"..");
	dirRecord.inodeNo = superBlock.rootInodeNo;
	dirRecord.deleted = 0;
	memcpy(buf + sizeof(dirRecord_t), &dirRecord, sizeof(dirRecord_t));
	ret = writeBlock(superBlock.rootDirBlock, buf);
	if (ret<0) return(-2);
	

	
	memset(inodeTable, 0, sizeof(inodeTable));
	inodeTable[superBlock.rootInodeNo].mode |= DIRECTORY << 12;
	inodeTable[superBlock.rootInodeNo].linkCount = 2;
	inodeTable[superBlock.rootInodeNo].fileSize = 2*sizeof(dirRecord_t);
	inodeTable[superBlock.rootInodeNo].direct[0] = superBlock.rootDirBlock;
	inodeTable[superBlock.rootInodeNo].singleIndirect = 0;
	ret = writeBlock(superBlock.inodeBlock, inodeTable);
	if (ret<0) return(-2);
	

	
	memset(&buf, 0, BLOCKSIZE);
	ret = writeBlock(disk_blockCount-1, &buf);	
	if (ret<0) return(-2);
	pVal = (int*) buf;
	for (i=superBlock.freeBlockList; i<disk_blockCount-1; i++) {
		*pVal = i+1;		
		ret = writeBlock(i, &buf);
		if (ret<0) return(-2);
	}
	
    fsync (disk_fd);
    close (disk_fd);
	
    return (0);
}

/* format disk with predefined errors for the purpose of disk checking
	pDiskName: disk name
	diskSize: disk size in bytes
	return: 0 on success, <0 on faults
*/
int fs_badFormat(char *pDiskName, int diskSize)
{	int i, ret, *pVal;
	char buf[BLOCKSIZE];
	dirRecord_t dirRecord;
	
	int fs_fd, size, freeSize, n, len; 
	FILE *fp;
	char fileName[256];

    strcpy (disk_name, pDiskName);
    disk_size = diskSize;
    disk_blockCount = disk_size / BLOCKSIZE;

    disk_fd = open (disk_name, O_RDWR);
    if (disk_fd == -1) return (-1);		
	
    
    superBlock.inodeBlock = 2;
    superBlock.rootDirBlock = 3;
	superBlock.rootInodeNo = 0;
	superBlock.dataBlock = 3;
	superBlock.freeBlockList = superBlock.dataBlock + 1;
	superBlock.blockSize=BLOCKSIZE;
	superBlock.freeBlockCount=disk_size / BLOCKSIZE - 4;
	ret = writeBlock(1, &superBlock);
	if (ret<0) return(-2);
	
	
	memset(&buf, 0, BLOCKSIZE);
	strcpy(dirRecord.fileName,".");
	dirRecord.inodeNo = superBlock.rootInodeNo;
	dirRecord.deleted = 0;
	memcpy(buf, &dirRecord, sizeof(dirRecord_t));
	strcpy(dirRecord.fileName,"..");
	dirRecord.inodeNo = superBlock.rootInodeNo;
	dirRecord.deleted = 0;
	memcpy(buf + sizeof(dirRecord_t), &dirRecord, sizeof(dirRecord_t));
	ret = writeBlock(superBlock.rootDirBlock, buf);
	if (ret<0) return(-2);
	

	
	memset(inodeTable, 0, sizeof(inodeTable));
	inodeTable[superBlock.rootInodeNo].mode |= DIRECTORY << 12;
	inodeTable[superBlock.rootInodeNo].linkCount = 2;
	inodeTable[superBlock.rootInodeNo].fileSize = 2*sizeof(dirRecord_t);
	inodeTable[superBlock.rootInodeNo].direct[0] = superBlock.rootDirBlock;
	inodeTable[superBlock.rootInodeNo].singleIndirect = 0;
	ret = writeBlock(superBlock.inodeBlock, inodeTable);
	if (ret<0) return(-2);
	

	
	memset(&buf, 0, BLOCKSIZE);
	ret = writeBlock(disk_blockCount-1, &buf);	
	if (ret<0) return(-2);
	pVal = (int*) buf;
	for (i=superBlock.freeBlockList; i<disk_blockCount-1; i++) {
		*pVal = i+1;		
		ret = writeBlock(i, &buf);
		if (ret<0) return(-2);
	}
	
	
	
	*pVal = disk_blockCount-3; 
	ret = writeBlock(disk_blockCount-2, &buf);	
	if (ret<0) return(-2);
	
	fsync (disk_fd);
    close (disk_fd);
	
	ret = fs_mount (DISKNAME);
	if (ret != 0) {
		if (ret==-2) printf ("Could not mount '%s'. Please reformat the disk.\n", DISKNAME); 
		else printf ("Virtual disk '%s' not found. Run makedisk first.\n", DISKNAME); 
		exit(-1); 
	}
		
	
	
	
	strcpy(fileName, "story1.txt");
	fp = fopen(fileName, "r");	
	if (fp==NULL) { 
		fprintf(stderr,"Source file does not exist\n"); 
		return(-2);
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	freeSize = fs_diskFreeBlocks()*BLOCKSIZE;
	if (freeSize < size) {
		printf("Not enough disk space! Free %d, request %d\n", freeSize, size);
		fclose(fp);
		return (-1);
	} 
	fseek(fp, 0L, SEEK_SET);
		
	fs_fd = fs_create(fileName);
	if (fs_fd<0) {
		printf("Could not create file. Error = %d\n", fs_fd);
		return(-1);
	}
	fs_fd = fs_open(fileName);
	if (fs_fd<0) { 
		fprintf(stderr,"Cannot open file %s in FS. Error = %d\n",fileName, fs_fd); 
		return(-2);
	}
	
	while (!feof(fp)) {
		n = fread(buf, BLOCKSIZE, 1, fp);
		if (n==1) len = BLOCKSIZE;
		else len = size % BLOCKSIZE;
		fs_write(fs_fd, buf, len);
	}
	
	if (fs_close(fs_fd)<0) printf("Fail to close fsf file\n");
	fclose(fp);
	
	
	strcpy(fileName, "story2.txt");
	fp = fopen(fileName, "r");	
	if (fp==NULL) { 
		fprintf(stderr,"Source file does not exist\n"); 
		return(-2);
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	freeSize = fs_diskFreeBlocks()*BLOCKSIZE;
	if (freeSize < size) {
		printf("Not enough disk space! Free %d, request %d\n", freeSize, size);
		fclose(fp);
		return (-1);
	} 
	fseek(fp, 0L, SEEK_SET);
		
	fs_fd = fs_create(fileName);
	if (fs_fd<0) {
		printf("Could not create file. Error = %d\n", fs_fd);
		return(-1);
	}
	fs_fd = fs_open(fileName);
	if (fs_fd<0) { 
		fprintf(stderr,"Cannot open file %s in FS. Error = %d\n",fileName, fs_fd); 
		return(-2);
	}
	
	while (!feof(fp)) {
		n = fread(buf, BLOCKSIZE, 1, fp);
		if (n==1) len = BLOCKSIZE;
		else len = size % BLOCKSIZE;
		fs_write(fs_fd, buf, len);
	}
	
	if (fs_close(fs_fd)<0) printf("Fail to close fsf file\n");
	fclose(fp);
	
	
	strcpy(fileName, "story3.txt");
	fp = fopen(fileName, "r");	
	if (fp==NULL) { 
		fprintf(stderr,"Source file does not exist\n"); 
		return(-2);
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	freeSize = fs_diskFreeBlocks()*BLOCKSIZE;
	if (freeSize < size) {
		printf("Not enough disk space! Free %d, request %d\n", freeSize, size);
		fclose(fp);
		return (-1);
	} 
	fseek(fp, 0L, SEEK_SET);
		
	fs_fd = fs_create(fileName);
	if (fs_fd<0) {
		printf("Could not create file. Error = %d\n", fs_fd);
		return(-1);
	}
	fs_fd = fs_open(fileName);
	if (fs_fd<0) { 
		fprintf(stderr,"Cannot open file %s in FS. Error = %d\n",fileName, fs_fd); 
		return(-2);
	}
	
	while (!feof(fp)) {
		n = fread(buf, BLOCKSIZE, 1, fp);
		if (n==1) len = BLOCKSIZE;
		else len = size % BLOCKSIZE;
		fs_write(fs_fd, buf, len);
	}
	
	if (fs_close(fs_fd)<0) printf("Fail to close fsf file\n");
	fclose(fp);
	
	
	
	inodeTable[2].direct[0] = inodeTable[1].direct[0];
	
	
	addFreeBlock(inodeTable[3].direct[0]);
	
	return (0);
}


/*
   Mount the file system and make it ready: load super block, inode table into RAM,
   open root directory.
   return: 0 on success, <0 on faults
*/
int fs_mount (char *pDiskName)
{
    struct stat finfo;

    strcpy (disk_name, pDiskName);
    disk_fd = open (disk_name, O_RDWR);
    if (disk_fd == -1) return(-1);

    fstat (disk_fd, &finfo);
    disk_size = (int) finfo.st_size;
    disk_blockCount = disk_size / BLOCKSIZE;

    
	readBlock(1, &superBlock);				
	if (superBlock.freeBlockCount==0 || superBlock.blockSize!=BLOCKSIZE) {		
		
		close(disk_fd);
		return(-2); 
	}
	readBlock(superBlock.inodeBlock, &inodeTable);	
	
	
	rootFd = 0;
	fileDescriptors[rootFd].state = 1;			
	fileDescriptors[rootFd].fp = 0;
	fileDescriptors[rootFd].inodeNo = superBlock.rootInodeNo;
	fileCount++;
	currentDirFd = rootFd;	
	
	numInodes = BLOCKSIZE/sizeof(inode_t);
	
	/*printf ("fs_mount: mounting %s, size=%dKB, free=%dKB\n", disk_name,
        (int) finfo.st_size/1024, fs_diskFreeBlocks()*BLOCKSIZE/1024);
	*/
	
    return (0);
}


/* unmount the file system: write changes in superblock, inode table back to disk
*/
int fs_umount()
{
	for (int i=0; i<MAXOPENFILES; i++)	
		if (fileDescriptors[i].state>0) fs_close(i);
	
    writeBlock(1, &superBlock);
	writeBlock(superBlock.inodeBlock, &inodeTable);	

    fsync (disk_fd);
    close (disk_fd);
    return (0);
}


/* create a file in current directory 
	return: 0 success, <0 faults
*/
int fs_create(char *pPathName)
{	dirRecord_t dirRecord;
	int inodeNo, ret;
	uint16_t parentInodeNo, mode;
	char *pFileName, pathName[MAXPATHNAME];
	int dirFd;
	
	if (strlen(pPathName)>=MAXPATHNAME) return(-5);
	strcpy(pathName, pPathName);
		
    
    ret = path2inode(pathName, &dirRecord, &parentInodeNo);
    if (ret==0) return(-1);			
	if (ret==-1) return(-2);		

	pFileName = splitPathAndName(pathName);

	
    if (strlen(pathName)==0 || pathName==pFileName) {
		if (strlen(pathName)==0) dirFd = rootFd;
		else dirFd = currentDirFd;
	} 
	else {
		dirFd = openFileByInode(parentInodeNo);
		if (dirFd<0) return(-3);	
	}

	
	strcpy(dirRecord.fileName, pFileName);
	dirRecord.deleted = 0;
	inodeNo = getFreeInode();
	if (inodeNo<0) {
		fprintf(stderr, "Inode table full\n");
		return(-4);	
	}
	dirRecord.inodeNo = inodeNo;

	
	fs_seek(dirFd, FILESIZE(dirFd));
	fs_write(dirFd, &dirRecord, sizeof(dirRecord_t));
	if (dirFd != currentDirFd && dirFd!=rootFd) fs_close(dirFd);

	
	inodeTable[parentInodeNo].linkCount++;	
	mode = inodeTable[inodeNo].mode;
	mode = (mode << 4)>>4;
	mode |= (REGULARFILE<<12);
	inodeTable[inodeNo].mode = mode;	

	return (0);
 }


/* open file filename 
	return: 0 success, <0 faults
*/
int fs_open(char *pPathName)
{	dirRecord_t dirRecord;
	uint16_t parentInodeNo;
	char buf[BLOCKSIZE], pathName[MAXPATHNAME], linkName[MAXPATHNAME];
	int ret, fd = -1;

	strcpy(pathName, pPathName); 
	ret = path2inode(pathName, &dirRecord, &parentInodeNo);
	if (ret<0) return(-1);	

	
	if (inodeTable[dirRecord.inodeNo].mode>>12 == SYMBOLICLINK) {
		readBlock(inodeTable[dirRecord.inodeNo].direct[0], buf);
		sscanf(buf, "%s", linkName);
		
		ret = path2inode(linkName, &dirRecord, &parentInodeNo);
		if (ret<0) return (fd);	
	}
		
	fd = openFileByInode(dirRecord.inodeNo);
	return (fd);
}

/* close an open file */
int fs_close (int fd)
{	uint16_t indBlock;

    
	indBlock = inodeTable[fileDescriptors[fd].inodeNo].singleIndirect;
	if ((indBlock>0)&&(fileDescriptors[fd].state==2))
		
		writeBlock(indBlock, fileDescriptors[fd].indirectTable);
	
	fileDescriptors[fd].state = 0; 
	
    return (0);
}

/* check for end of file
	return: 1 on yes, 0 on no
*/
int fs_eof(int fd)
{
    return (fileDescriptors[fd].fp>=inodeTable[fileDescriptors[fd].inodeNo].fileSize);
}

/* seek to position offset in an open file */
int fs_seek(int fd, int offset)
{
    int position = -1;

	if (offset <= FILESIZE(fd)) {
		fileDescriptors[fd].fp = offset;
		position = offset;
	}

    return (position);
}

/* get size of an open file */
int fs_fileSize (int fd)
{   int size = -1;
    
	size = FILESIZE(fd);
    return (size);
}

/* Delete a file name 
   Free direct blocks, indirect blocks.
   The file entry in directory is marked deleted, instead of being completely removed.
*/
int fs_delete(char *pPathName)
{	dirRecord_t dirRecord;
	int i, j, numBlocks, dirFd, found=0;
	uint16_t indirectBlock[BLOCKSIZE/sizeof(uint16_t)];
	inode_t *pInode;
	char *pFileName, pathName[MAXPATHNAME];

	if (strlen(pPathName)>=MAXPATHNAME) return(-5);
	strcpy(pathName, pPathName);
		
	
	pFileName = splitPathAndName(pathName);
	if (strcmp(pFileName,".")==0 || strcmp(pFileName,"..")==0) 
		return (-1);	

	if (strlen(pathName)==0 || pathName==pFileName) {
		if (strlen(pathName)==0) dirFd = rootFd;
		else dirFd = currentDirFd;
	} 
	else dirFd = fs_open(pathName);	
	if (dirFd<0) return (-2);			

	while (fs_searchDirByName(dirFd, pFileName, &dirRecord, found)) {	
		found = 1;	
		if (inodeTable[dirRecord.inodeNo].mode>>12 == DIRECTORY &&
			inodeTable[dirRecord.inodeNo].linkCount>2) continue;
		
		dirRecord.deleted=1;
		fs_seek(dirFd, fileDescriptors[dirFd].fp - sizeof(dirRecord_t));
		fs_write(dirFd, &dirRecord, sizeof(dirRecord_t));
		inodeTable[fileDescriptors[dirFd].inodeNo].linkCount--;
		
		
		pInode = &inodeTable[dirRecord.inodeNo];
		if (pInode->mode>>12 == DIRECTORY) pInode->linkCount = 0; 
		else pInode->linkCount--;	
		if (pInode->linkCount>0) return 0;  
		
		
		if (pInode->fileSize==0) numBlocks=0;
		else numBlocks = (pInode->fileSize-1)/BLOCKSIZE + 1;
		
		for (i=0; (i<DIRECTBLOCKS)&&(i<numBlocks); i++) addFreeBlock(pInode->direct[i]);
		
		if (numBlocks>DIRECTBLOCKS) { 
			readBlock(pInode->singleIndirect, &indirectBlock);
			j = numBlocks - DIRECTBLOCKS;
			for (i=0; i<j; i++) addFreeBlock(indirectBlock[i]);
			addFreeBlock(pInode->singleIndirect);
		}
		
		
		
	}
	if (dirFd != currentDirFd && dirFd!=rootFd) fs_close(dirFd);

    if (found) return 0;
	else return (-3);	
}

/* read from an open file 
	fd - file descriptor
	buf - buffer to store data
	n - number of bytes to read
	return: number of bytes read, -1 on faults
*/
int fs_read(int fd, void *buf, int n)
{	uint16_t beginBlock, endBlock, noOfBlocks, blockIndex, i, tmp;
	unsigned char blockBuf[BLOCKSIZE];
    int bytes_read = -1;

    
	if ((fileDescriptors[fd].state==0) || (FILESIZE(fd)==0))
		return bytes_read;	
	
	
	beginBlock = fileDescriptors[fd].fp / BLOCKSIZE; 
	endBlock = (fileDescriptors[fd].fp + n - 1) / BLOCKSIZE; 
	noOfBlocks = (FILESIZE(fd)-1) / BLOCKSIZE + 1;
	
	
	if (endBlock > noOfBlocks - 1) endBlock = noOfBlocks - 1;  
	
	
	blockIndex = getBlockIndex(fd, beginBlock);	
	readBlock(blockIndex, blockBuf);
	tmp = fileDescriptors[fd].fp % BLOCKSIZE; 
	if (n<=BLOCKSIZE - tmp) {
		memcpy(buf, blockBuf + tmp, n);
		bytes_read = n;
		fileDescriptors[fd].fp += n;
		return (bytes_read);
	}
	memcpy(buf, blockBuf + tmp, BLOCKSIZE - tmp);
	buf += BLOCKSIZE - tmp;
	bytes_read = BLOCKSIZE - tmp;
	
	
	for (i=beginBlock+1; i<endBlock; i++) {
		blockIndex = getBlockIndex(fd, i);	
		readBlock(blockIndex, blockBuf);
		memcpy(buf, blockBuf, BLOCKSIZE);
		buf += BLOCKSIZE;
		bytes_read += BLOCKSIZE;
	}
	
	
	if (endBlock > beginBlock) {
		blockIndex = getBlockIndex(fd, endBlock);	
		readBlock(blockIndex, blockBuf);
		memcpy(buf, blockBuf, (fileDescriptors[fd].fp + n) % BLOCKSIZE);
		bytes_read += (fileDescriptors[fd].fp + n) % BLOCKSIZE;
	}
	if (fileDescriptors[fd].fp + n <= inodeTable[fileDescriptors[fd].inodeNo].fileSize)
		fileDescriptors[fd].fp += n;
	else fileDescriptors[fd].fp = inodeTable[fileDescriptors[fd].inodeNo].fileSize;

    return (bytes_read);

}

/* write to an open file 
	fd - file descriptor
	buf - data to write
	n - number of bytes to write
	return: number of bytes written, <0 on faults
*/
int fs_write(int fd, void *buf, int n)
{	uint16_t beginBlock, endBlock, noOfBlocks, blockIndex; 
	int i, j, tmp, blockNo;
	unsigned char blockBuf[BLOCKSIZE];
    int bytes_written = -1;

    
	if (fileDescriptors[fd].state) { 
		beginBlock = fileDescriptors[fd].fp / BLOCKSIZE; 
		endBlock = (fileDescriptors[fd].fp + n - 1) / BLOCKSIZE; 

		if (FILESIZE(fd) == 0) noOfBlocks = 0;
		else noOfBlocks = (FILESIZE(fd) - 1) / BLOCKSIZE + 1;
		
		
		if (fs_diskFreeBlocks() < (endBlock + 1 - noOfBlocks)) {
			fprintf(stderr, "Out of disk space\n");
			return (-1); 
		}
		
		if (endBlock+1 > DIRECTBLOCKS+MAXINDIRECTBLOCKS) {
			fprintf(stderr, "File exceeds max size\n");
			return (-2); 
		}
		
		
		for (i=noOfBlocks; i<endBlock+1; i++) {
			blockNo = getFreeBlock();
			if (blockNo < 0) {
				fprintf(stderr, "Out of disk space.\n");
				return(-1);
			}
			
			if (i<DIRECTBLOCKS) inodeTable[fileDescriptors[fd].inodeNo].direct[i] = blockNo;
			else { 
				j = i - DIRECTBLOCKS;
				if (j==0) { 
					tmp = getFreeBlock(); 
					if (blockNo < 0) {
						fprintf(stderr, "Out of disk space.\n");
						return(-1);
					}
					inodeTable[fileDescriptors[fd].inodeNo].singleIndirect = tmp;
				}
				fileDescriptors[fd].indirectTable[j] = blockNo;
			}
		}
		
		
		blockIndex = getBlockIndex(fd, beginBlock);	
		readBlock(blockIndex, blockBuf);			
		tmp = fileDescriptors[fd].fp % BLOCKSIZE; 	
		
		if (BLOCKSIZE - tmp >= n) { 
			memcpy(blockBuf + tmp, buf, n);
			buf += n;	
			bytes_written = n;
		}
		else {
			memcpy(blockBuf + tmp, buf, BLOCKSIZE - tmp);
			buf += BLOCKSIZE - tmp;
			bytes_written = BLOCKSIZE - tmp;
		}
		writeBlock(blockIndex, blockBuf); 			
		
		
		for (i=beginBlock+1; i<endBlock; i++) {
			memcpy(blockBuf, buf, BLOCKSIZE);
			blockIndex = getBlockIndex(fd, i);	
			writeBlock(blockIndex, blockBuf);
			buf += BLOCKSIZE;
			bytes_written += BLOCKSIZE;
		}
		
		
		if (endBlock > beginBlock) {
			blockIndex = getBlockIndex(fd, endBlock);	
			readBlock(blockIndex, blockBuf);		
			memcpy(blockBuf, buf, (fileDescriptors[fd].fp + n) % BLOCKSIZE); 
			writeBlock(blockIndex, blockBuf);	
			bytes_written += (fileDescriptors[fd].fp + n) % BLOCKSIZE;
		}
	}
	if (fileDescriptors[fd].fp + n > inodeTable[fileDescriptors[fd].inodeNo].fileSize) {
		inodeTable[fileDescriptors[fd].inodeNo].fileSize = fileDescriptors[fd].fp + n;
		fileDescriptors[fd].state = 2;	
	}
	fileDescriptors[fd].fp += n;

    return (bytes_written);
}


/* create a hard link 
	pSourceName - original file
	pTargetName - name of hard-linked file
	return: 0 on success, <0 on faults
*/
int fs_createHardLink(char *pSourceName, char *pTargetName)
{	dirRecord_t dirRec;
	int ret, dirFd;
	uint16_t parentInodeNo;
	char pathName[MAXPATHNAME], *pFileName;
	
	strcpy(pathName, pSourceName); 
	if (fs_isDir(pathName)) return(-2);	
	
	ret = path2inode(pathName, &dirRec, &parentInodeNo);

	if (ret<0) return(-1);					
	inodeTable[dirRec.inodeNo].linkCount++;	
	
	strcpy(pathName, pTargetName);
	pFileName = splitPathAndName(pathName);
	strcpy(dirRec.fileName, pFileName);	
	if (strlen(pathName)==0 || pathName==pFileName) {
		if (strlen(pathName)==0) {dirFd = rootFd; printf("xxxx\n");}
		else dirFd = currentDirFd;
	}
	else dirFd = fs_open(pathName); 	
	fs_seek(dirFd, fs_fileSize(dirFd)); 			
	fs_write(dirFd, &dirRec, sizeof(dirRecord_t));   
	inodeTable[fileDescriptors[dirFd].inodeNo].linkCount++;
	if (dirFd != currentDirFd && dirFd!=rootFd) fs_close(dirFd);
	
	return 0;
}

/* create a symbolic link 
	pSourceName - original file	
	pTargetName - name of symbolic link
	return: 0 on success, <0 on faults
*/
int fs_createSymLink(char *pSourceName, char *pTargetName)
{	dirRecord_t dirRec;
	int ret, tfd;
	uint16_t parentInodeNo, mode;
	char pathName[MAXPATHNAME];

	strcpy(pathName, pSourceName); 
	
	ret = path2inode(pathName, &dirRec, &parentInodeNo);
	if (ret<0) return(-1);	
	
	ret = fs_create(pTargetName);
	if (ret<0) return(-2);	
	tfd = fs_open(pTargetName);
	if (tfd<0) return(-3);	
	ret = fs_write(tfd, pSourceName, strlen(pSourceName));
	
	mode = inodeTable[fileDescriptors[tfd].inodeNo].mode;
	mode = (mode << 4)>>4;
	mode |= (SYMBOLICLINK<<12);
	inodeTable[fileDescriptors[tfd].inodeNo].mode = mode;
	fs_close(tfd);
	
	return 0;
}


/* 	search current directory for a given file name 
	pDirRecord points to the file entry found
	option: 0 search from the beginning / 1 search next
*/
int fs_searchDirByName(int dirFd, char *pFileName, dirRecord_t *pDirRecord, int option)
{	static int pos = 0;		
	int i, numEntries, ret=0;
	
	if (option==0) pos = 0;	
	numEntries = inodeTable[fileDescriptors[dirFd].inodeNo].fileSize / sizeof(dirRecord_t);
	fs_seek(dirFd, pos*sizeof(dirRecord_t));
	for (i=pos; i<numEntries; i++) {
		fs_read(dirFd, pDirRecord, sizeof(dirRecord_t));
		if ((pDirRecord->deleted!=1)&& wildCmp(pFileName, pDirRecord->fileName)) {
			ret = 1; 
			break;
		}
	}
	if (i<numEntries) pos = i+1;
	else pos = i;
	return(ret);	
}

/* set current dir to a path name, starting from current dir
	return 0: no fault, -1 path invalid, -2 max open file limit
*/
int fs_setCurrentDir(char *pPath)
{	dirRecord_t dirRecord;
	uint16_t parentInodeNo;
	char pathName[MAXPATHNAME], linkName[MAXPATHNAME];
	int ret, dirFd;
	char buf[BLOCKSIZE];
	
	if (strlen(pPath)>=MAXPATHNAME) return(-5);		
	strcpy(pathName, pPath);
	
	dirFd = currentDirFd;		

	if (strlen(pathName)==0) { 
		currentDirFd = rootFd; 
		return 0;
	}
	if (pathName[0]=='/') currentDirFd = rootFd;	
	ret = path2inode(pathName, &dirRecord, &parentInodeNo);
	if (ret<0) {

		currentDirFd = dirFd;	
		return (-1);			
	}
	if (inodeTable[dirRecord.inodeNo].mode >> 12 != DIRECTORY) {
		if (inodeTable[dirRecord.inodeNo].mode >> 12 == SYMBOLICLINK) {
			readBlock(inodeTable[dirRecord.inodeNo].direct[0], buf);
			sscanf(buf, "%s", linkName);
			ret = path2inode(linkName, &dirRecord, &parentInodeNo);
			if (ret<0) return (-3);	
			if (inodeTable[dirRecord.inodeNo].mode >> 12 != DIRECTORY)
				return(-2); 
		}
		else return(-2); 
	}
	
	if (dirRecord.inodeNo != superBlock.rootInodeNo)
		dirFd = openFileByInode(dirRecord.inodeNo);
	else dirFd = rootFd;
	if (currentDirFd != rootFd) fs_close(currentDirFd);
	
	currentDirFd = dirFd;	

	return 0;
}


/* make a directory 
	retur: 0 on success, <0 on faults
*/
int fs_makeDir(char *pPathName)
{	dirRecord_t dirRecord;
	int inodeNo, ret;
	uint16_t parentInodeNo, mode;
	char *pFileName, pathName[MAXPATHNAME];
	int dirFd, newDirFd;
	
	if (strlen(pPathName)>=MAXPATHNAME) return(-5);
	strcpy(pathName, pPathName);
		
    
    ret = path2inode(pathName, &dirRecord, &parentInodeNo);
    if (ret==0) return(-1);			
	if (ret==-1) return(-2);		

	pFileName = splitPathAndName(pathName);

	
    if (strlen(pathName)==0 || pathName==pFileName) {
		if (strlen(pathName)==0) dirFd = rootFd;
		else dirFd = currentDirFd;
	}
	else {
		dirFd = openFileByInode(parentInodeNo);
		if (dirFd<0) return(-3);	
	}

	
	strcpy(dirRecord.fileName, pFileName);
	dirRecord.deleted = 0;
	inodeNo = getFreeInode();
	if (inodeNo<0) {
		fprintf(stderr, "Inode table full\n");
		return(-4);	
	}
	dirRecord.inodeNo = inodeNo;

	
	fs_seek(dirFd, FILESIZE(dirFd));
	fs_write(dirFd, &dirRecord, sizeof(dirRecord_t));
	if (dirFd != currentDirFd && dirFd!=rootFd) fs_close(dirFd);

	
	inodeTable[parentInodeNo].linkCount++;
	mode = inodeTable[inodeNo].mode;
	mode = (mode << 4)>>4;
	mode |= (DIRECTORY<<12);
	inodeTable[inodeNo].mode = mode;
	inodeTable[inodeNo].linkCount = 2;
	
	
	newDirFd = openFileByInode(dirRecord.inodeNo);
	
	strcpy(dirRecord.fileName,".");
	dirRecord.inodeNo = inodeNo;
	dirRecord.deleted = 0;
	fs_write(newDirFd, &dirRecord, sizeof(dirRecord_t));
	
	strcpy(dirRecord.fileName,"..");
	dirRecord.inodeNo = parentInodeNo;
	dirRecord.deleted = 0;
	fs_write(newDirFd, &dirRecord, sizeof(dirRecord_t));
	
	fs_close(newDirFd);
	
	return (0);
 }
 
 /* return number of files in the directory (>=2), 0 if not a dir */
 int fs_isDir(char *pPathName)
 {	int fd;
 
	fd = fs_open(pPathName);
	if (fd<0) return 0;
	if (inodeTable[fileDescriptors[fd].inodeNo].mode >> 12 == DIRECTORY &&
		inodeTable[fileDescriptors[fd].inodeNo].linkCount>0)
		return (inodeTable[fileDescriptors[fd].inodeNo].linkCount);
	return 0;
 }

/*	return number of free blocks */
int fs_diskFreeBlocks(void)
{		
	return superBlock.freeBlockCount;
}

/* print content of current directory with optional path and wildcards*/
int fs_printDir (char *pPathName)
{	int n, i, dirFd, count=0;
	dirRecord_t dirRecord;
	char *pPattern;
	char buf[BLOCKSIZE], pathName[MAXPATHNAME], linkName[MAXPATHNAME];
	
	memset(pathName, 0, MAXPATHNAME-1);
	if (pPathName !=NULL) strcpy(pathName, pPathName); 
	else strcat(pathName, "*");

	if (strstr(pathName,"*")==NULL && strstr(pathName,"?")==NULL) {
		if (pathName[strlen(pathName)-1]=='/') strcat(pathName,"*");
		else strcat(pathName,"/*");
	}

	pPattern = splitPathAndName(pathName);

	if (strlen(pathName)==0 || pathName==pPattern) {
		if (strlen(pathName)==0) dirFd = rootFd;
		else dirFd = currentDirFd;
	} 
	else {
		dirFd = fs_open(pathName);
		if (dirFd<0) return(-1);	
	}
	
	n = inodeTable[fileDescriptors[dirFd].inodeNo].fileSize / sizeof(dirRecord_t);
	fs_seek(dirFd, 0);
	printf("LC	Size	Inode	File name\n");
	printf("--	------	-----	----------------\n");
	for (i=0; i<n; i++) {
		fs_read(dirFd, &dirRecord, sizeof(dirRecord_t));
		if ( (dirRecord.deleted!=1)&& wildCmp(pPattern, dirRecord.fileName) ) {
			printf("%2d\t%6d\t%04Xh\t%s", inodeTable[dirRecord.inodeNo].linkCount,
			inodeTable[dirRecord.inodeNo].fileSize, dirRecord.inodeNo,
			dirRecord.fileName);
			if (inodeTable[dirRecord.inodeNo].mode>>12==SYMBOLICLINK) {
				readBlock(inodeTable[dirRecord.inodeNo].direct[0], buf);
				sscanf(buf, "%s", linkName);
				printf("->%s", linkName);
			} 
			else if (inodeTable[dirRecord.inodeNo].mode>>12==DIRECTORY)
				printf("\t<DIR>");
			printf("\n");
			count++;
		}
	}
	printf("----------------------------------------\n");
	if (superBlock.freeBlockCount*BLOCKSIZE < 1024)
		printf("Total %d.\tFree space %dB\n", count, superBlock.freeBlockCount*BLOCKSIZE);
	else
		printf("Total %d.\tFree space %dKB\n", count, superBlock.freeBlockCount*BLOCKSIZE/1024);
	if (dirFd != currentDirFd && dirFd!=rootFd) fs_close(dirFd);
	return 0;
}

/* print inode table, only non-zero inodes are displayed */
void fs_printInodeTable()
{	int i, j;

	printf("I-node Table:\n");
	printf("Inode  Mode  LC  Size    IndPtr  DirPtr\n");
	printf("-----  ----  --  ------  ------  ------\n");
	for (i=0; i< numInodes; i++) {
		if (inodeTable[i].linkCount>0) { 
			printf("%5d  %4d  %2d  %6d  %6d  ", i, inodeTable[i].mode>>12,
				inodeTable[i].linkCount, inodeTable[i].fileSize, 
				inodeTable[i].singleIndirect);
			for (j=0; j<DIRECTBLOCKS; j++) printf("%d, ", inodeTable[i].direct[j]);
			printf("\b\b \n");
		}
	} 
	return;
}


/* print content of an inode */
void fs_printInode(uint16_t inodeNo)
{	inode_t *pInode;
	uint16_t buf[BLOCKSIZE/sizeof(uint16_t)];
	int i;

	if (inodeNo>numInodes) {
		fprintf(stderr, "Inode %d is out of range\n", inodeNo);
		return;
	}
	pInode = &inodeTable[inodeNo];
	printf("I-node %d:\n", inodeNo);
	printf("Mode = "B2b_t" "B2b_t" (binary)\n", B2b(pInode->mode >> 8), B2b(pInode->mode));
	printf("Link count = %d\n", pInode->linkCount);
	printf("File size = %d bytes\n", pInode->fileSize);
	
	printf("Direct pointers = ");
	for (i=0; i<DIRECTBLOCKS; i++) printf("%d, ", pInode->direct[i]);
	printf("\b\b \n");
	
	printf("Indirect pointers = %d: ", pInode->singleIndirect);
	if (pInode->singleIndirect==0) return;
	readBlock(pInode->singleIndirect, (char*)buf);
	for (i=0; i<BLOCKSIZE/sizeof(uint16_t)-1; i++) 
		if (buf[i]>0) printf("%d, ", buf[i]);
	printf("\b\b \n");
	
	return;
}

/* print inode table */
void fs_printSuperBlock()
{	
	printf("Super block:\n");
	printf("Inode block = %d\n", superBlock.inodeBlock);
	printf("Start of data block = %d\n", superBlock.dataBlock);
	printf("First free block = %d\n", superBlock.freeBlockList);
	printf("Free blocks = %d\n", superBlock.freeBlockCount);
	printf("Block size = %d\n", superBlock.blockSize);
	printf("Root directory inode = %d\n", superBlock.rootInodeNo);
	return;
}


/* Supporting functions ---------------------------------------------------------------- */

/*	String compare with wildcards using recursive algo 
	Return 1 if found, 0 otherwise
*/

int wildCmp(char *pPattern, char *pString)
{
	if(*pPattern=='\0' && *pString=='\0')		
		return 1;
		
	if(*pPattern=='?' || *pPattern==*pString)	
		return wildCmp(pPattern+1,pString+1);
		
	if (*pPattern=='*') {
		if (strlen(pString)==0 && strlen(pPattern)>1) return 0;
		else return wildCmp(pPattern+1,pString) || wildCmp(pPattern,pString+1);	
	}
	return 0;
}

/*	String compare with wildcards using non-recursive algo (faster)
	Return 1 if found, 0 otherwise
*/
/* 
int wildCmp(char *pPattern,char *pString) 
{	char *cp = NULL, *mp = NULL;

	while ((*pString) && (*pPattern != '*')) {
		if ((*pPattern != *pString) && (*pPattern != '?')) {
		  return 0;
		}
		pPattern++;
		pString++;
	}

	while (*pString) {
		if (*pPattern == '*') {
			if (!*++pPattern) {
				return 1;
			}
			mp = pPattern;
			cp = pString+1;
		} 
		else if ((*pPattern == *pString) || (*pPattern == '?')) {
			pPattern++;
			pString++;
		} else {
			pPattern = mp;
			pString = cp++;
		}
	}

	while (*pPattern == '*') {
		pPattern++;
	}
	return !*pPattern;
}

 */


/* look in inode table for a free inode
	return inode number or <0 on faults
 */
int getFreeInode() 
{	
	for (int i=superBlock.rootInodeNo+1; i< numInodes; i++) {	
		if (inodeTable[i].linkCount==0) {
			memset(&inodeTable[i], 0, sizeof(inode_t));
			inodeTable[i].linkCount = 1;	
			return(i);
		}
	}
	return(-1);
}


/* convert a file block sequence to physical data block */
int getBlockIndex(int fd, uint16_t blockSeq) 
{		
	if (blockSeq<DIRECTBLOCKS) 
		return(inodeTable[fileDescriptors[fd].inodeNo].direct[blockSeq]);
	else 
		return(fileDescriptors[fd].indirectTable[blockSeq-DIRECTBLOCKS]);
}

/* add a block to the free block list */
void addFreeBlock(uint16_t blockNo)
{	char buf[BLOCKSIZE];
	uint16_t *pNo;
	
	
	memset(buf, 0, BLOCKSIZE);
	pNo = (uint16_t*) buf;
	*pNo = superBlock.freeBlockList;
	writeBlock(blockNo, buf);
	
	superBlock.freeBlockList = blockNo;	
	superBlock.freeBlockCount++;
}

/* allocate a free block, then remove from the free block list */
int getFreeBlock()
{	char buf[BLOCKSIZE];
	int ret=-1;
	
	if (superBlock.freeBlockList>0) {
		ret = superBlock.freeBlockList;					
		readBlock(ret, buf);
		superBlock.freeBlockList = *((uint16_t*)buf);	
		superBlock.freeBlockCount--;
	}
	
	return(ret);
	
}

/* 	search a data block of a directory by file name
	return inode number via pInodeNo
*/
int searchBlockByName(uint16_t blockNo, char *pFileName, dirRecord_t *pDirRecord)
{	char buf[BLOCKSIZE];
	dirRecord_t *pRecords;;
	int i;

	readBlock(blockNo, buf);
	pRecords = (dirRecord_t*) buf;
	for (i=0; i<BLOCKSIZE/sizeof(dirRecord_t); i++) {
		if (strcmp(pRecords[i].fileName, pFileName)==0 &&
			pRecords[i].deleted==0) break;
	}
	if (i<BLOCKSIZE/sizeof(dirRecord_t)) {
		memcpy(pDirRecord, &pRecords[i], sizeof(dirRecord_t));
		return 1;		
	}
	return 0;	
}

/*	convert from path name to inode.
	return: 0 no error, -1 path not found, -2 file not found
*/
int path2inode(char *pPath, dirRecord_t *pDirRecord, uint16_t *pParentInodeNo)
{	inode_t *pInode;
	uint16_t indirectBlock[MAXINDIRECTBLOCKS];
	char pathName[MAXPATHNAME];
	char* argv[MAXPATHNAME];
	int argc=0, i, j, inodeNo, parentInodeNo, found=0;
	
	
	strcpy(pathName, pPath);
	i=0;
	argv[i] = strtok(pathName, "/\r\n");
	while ((argv[i]!=NULL)&&(i<MAXPATHNAME-1)) {
		i++;
		argv[i] = strtok(NULL, "/\r\n");
	}
	argc = i;

	
	if (pPath[0]=='/') inodeNo = fileDescriptors[rootFd].inodeNo;
	else inodeNo = fileDescriptors[currentDirFd].inodeNo;
	parentInodeNo = inodeNo;	
	strcpy(pDirRecord->fileName,".");
	pDirRecord->inodeNo = inodeNo;
	pDirRecord->deleted = 0;

	for (i=0; i<argc; i++) {
		parentInodeNo = inodeNo;
		pInode = &inodeTable[inodeNo];
		for (j=0; j<DIRECTBLOCKS; j++) {	
			if (j*BLOCKSIZE > pInode->fileSize) break;
			if (searchBlockByName(pInode->direct[j], argv[i], pDirRecord)) {
				found = 1;
				break;
			} else found=0;
		}

		
		if (!found) {
			readBlock(pInode->singleIndirect, indirectBlock);
			for (j=0; j<MAXINDIRECTBLOCKS; j++) {
				if (j*BLOCKSIZE > pInode->fileSize) break;
				if (searchBlockByName(indirectBlock[j], argv[i], pDirRecord)) {
					found = 1;
					break;
				} else found=0;
			}
		}
		if (!found) {
			*pParentInodeNo = parentInodeNo;
			if (i<argc-1) return (-1);	
			else return(-2);			
		}
		else inodeNo = pDirRecord->inodeNo;
	}
	*pParentInodeNo = parentInodeNo;	

	return 0;
}

/* split a path and file name
	return pointer to filename. if no path is specified, then pPathName will 
		either be zero length or pPathName==pFileName
	note: the original string could be modified, i.e split into 2 strings
*/
char *splitPathAndName(char *pPathName)
{	int i;

	
	for (i=strlen(pPathName)-1; i>=0; i--) {
		if (pPathName[i]=='/') break;
	}
	if (i>=0) 
		pPathName[i] = 0;
	return(pPathName + i + 1);
}

/* open file by inode 
	return file descriptor on success, <0 on faults
*/
static int openFileByInode(uint16_t inodeNo)
{	int fd = -1;

	if (inodeNo>=numInodes) return(fd); 

	
	fileDescriptors[fileCount].state = 1;	
	fileDescriptors[fileCount].inodeNo = inodeNo;
	fileDescriptors[fileCount].fp = 0;
	
	
	if (inodeTable[inodeNo].singleIndirect>0) 
		readBlock(inodeTable[inodeNo].singleIndirect, fileDescriptors[fileCount].indirectTable);	
	
	fd = fileCount;
	
	do fileCount = (fileCount+1)%MAXOPENFILES;
	while ( (fileDescriptors[fileCount].state!=0)&&(fileCount!=fd));
	if (fileCount==fd) return (-2);	

    return (fd);
}

