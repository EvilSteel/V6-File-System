//Adam Nguyen
//Arun Palaniappan
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct superBlock {
	unsigned short isize;
	unsigned short fsize;
	unsigned short nfree;
	unsigned short free[100];
	unsigned short ninode;
	unsigned short inode[100];
	char flock;
	char ilock;
	char fmod;
	unsigned short time[2];
};
struct inode {
	unsigned short flag;	//Stores inode flag.  Later converted into type
	char nlinks;
	char uid;
	char gid;
	char fsize1;	//Size of size0
	unsigned short fsize2;	//Size of size1
	unsigned short addr[8];
	unsigned short actime[2];
	unsigned short modtime[2];
};

struct directory {
	unsigned short inodeNumber;
	char name[14];
};

int fd;
struct superBlock sb;
unsigned int tempArray[512];

unsigned int freeBlock(int n);
unsigned int allocFreeBlock();
int freeINode(int fd, int n);

int main(){

int i;
int index = 1024;
struct inode s2;
int isMounted = 0;
int count1;
char buffer1[512];
struct directory dir;
struct inode tempNode;
struct inode tempNode1;
struct inode cpinNode;
while (1)
{
char command[64];
char *args[64];
int count = 0;
printf("Enter command: ");
fgets(command, sizeof(command), stdin);
char *temp;
//Removes the '\n' character from the input and allocates memory to args (to avoid core dump error)
command[strlen(command) - 1] = 0;
for (i = 0; i < 10; i++)
	args[i] = (char*)malloc(20 * sizeof(char));

temp = strtok(command, " ");
strcpy(args[count++], temp);
while (temp != NULL)
{
	temp = strtok(NULL, " ");
	if (temp == NULL)
		break;
	strcpy(args[count++], temp);
}
args[count] = NULL;
//Quit the program.
if(strcmp("q", command) == 0)
{
   exit(0);
}
if (strcmp(args[0], "mount") == 0)
{
	//make sure test.data exists. If not, use touch command to create an empty file called test.data
	int fd = open(args[1], 2);
	int j;
	//start next read from 1024th byte of the file. Not from the beginning.
	printf("File successfully mounted.\n");
	isMounted = 1;
}
else if (strcmp(args[0], "initfs") == 0)
{
	if(isMounted == 0)
	{
		printf("Please mount a data file before you initiate a file system.\n");
	}
	else
	{
		int j;
		int numBlocks = atoi(args[1]);
		int numNodes = atoi(args[2]);
		sb.fsize = numBlocks;
		if((numNodes % 16) == 0)
			sb.isize = numNodes / 16;
		else
			sb.isize = (numNodes / 16) + 1;
		sb.ninode = 100;
		sb.nfree = 0;
		int nodeStart = (numBlocks + 2) * 512;
		lseek(fd, nodeStart, SEEK_SET);
		for(i = 0; i < 100; i++)
		{
			sb.free[i] = 0;		//Clear free[]
			sb.inode[i] = i;	//Initialize inode[]
		}
		lseek(fd, 512, SEEK_SET);
		write(fd, &sb, 512);
		
		unsigned int buffer[512];
		unsigned int numChunks = numBlocks / 100;
		unsigned int extraBlocks = numBlocks % 100;
		//Clear buffer and tempArray
		for(i = 0; i < 512; i++)
		{
			buffer[i] = 0;
			tempArray[i] = 0;
		}
		for(i = 0; i < numChunks; i++)
		{
			tempArray[0] = 100;
			
			for(j = 0; j < 100; j++)
			{
				if(i == (numChunks - 1) && extraBlocks == 0 && j == 0)
				{
					tempArray[j + 1] = 0;
				}
				else
				{
					tempArray[j + 1] = 2 + sb.isize + j + 100 * (i + 1);
				}
			}
			lseek(fd, (2 + sb.isize + 100 * i) * 512, SEEK_SET);
			write(fd, tempArray, 512);
			
			for(i = 1; i <= 100; i++)
			{
				lseek(fd, (2 + sb.isize + (100 * i)) * 512, SEEK_SET);
				write(fd, buffer, 512);
			}
		}
		
		tempArray[0] = extraBlocks;
		tempArray[1] = 0;
		for(j = 1; j <= extraBlocks; j++)
		{
			tempArray[j + 1] = 2 + sb.isize + j + (100 * i);
		}
		lseek(fd, (2 + sb.isize + (100 * i)) * 512, SEEK_SET);
		write(fd, tempArray, 512);
		
		for(j = 1; j <= extraBlocks; j++)
		{
			lseek(fd, (2 + sb.isize + 1 + j + (100 * i)) * 512, SEEK_SET);
			write(fd, tempArray, 512);
		}
		
		for(i = 0; i < 512; i++)
		{
			tempArray[i] = 0;
		}
		
		//Fill free array
		for(i = 0; i < 100; i++)
		{
			freeBlock(i + 2 + sb.isize);
		}
		
		//Create root directory
		struct directory rootDir;
		struct inode rootNode;
		unsigned short rootBlock = allocFreeBlock();
		for(i = 0; i < 14; i++)
		{
			rootDir.name[i] = 0;
		}
		rootDir.name[0] = '.';
		rootDir.name[1] = '\0';
		rootDir.inode = 1;
		rootNode.flags = 140077;
		rootNode.nlinks = 2;
		rootNode.uid = '0';
		rootNode.gid = '0';
		rootNode.size0 = '0';
		rootNode.size1 = 32;
		rootNode.addr[0] = rootBlock;
		for(i = 1; i < 8; i++)
			rootNode.addr[i] = 0;
		rootNode.actime[0] = 0;
		rootNode.modtime[0] = 0;
		rootNode.modtime[1] = 0;
		lseek(fd, 2 * 512 + 0 * 32, SEEK_SET);
		write(fd, &rootNode, 32);
		lseek(fd, rootBlock * 512, SEEK_SET);
		write(fd, &rootDir, 16);
		rootDir.name[1] = '.';
		rootDir.name[2] = '\0';
		write(fd, &rootDir, 16);
		
		printf("File system initiated.\n");
	}
}
else if (strcmp(args[0], "mkdir") == 0)
{
	char* dname = args[1];
	unsigned int bNum = allocFreeBlock();
	unsigned int nNum = allocINode();
	strncpy(dir.name, dname, 14);
	dir.inode = nNum;
	lseek(fd, 2 * 512, SEEK_SET);
	//Create directory inode
	tempNode1.flag = 140077;
	tempNode1.nlinks = 2;
	tempNode1.uid = '0';
	tempNode1.gid = '0';
	tempNode1.fsize1 = '0';
	tempNode1.fsize2 = 32;
	tempNode1.addr[0] = dNum;
	
	for(i = 1; i < 8; i++)
		tempNode1.addr[i] = 0;
	tempNode1.actime[0] = 0;
	tempNode1.modtime[0] = 0;
	tempNode1.modtime[1] = 0;
	
	lseek(fd, 2 * 512 + (nNum * sb.isize), SEEK_SET);
	write(fd, &tempNode1, sb.isize);
	lseek(fd, 2 * 512, SEEK_SET);
	int blck = read(fd, &tempNode1, sizeof(tempNode1));
	tempNode1.nlinks++;
	
	printf("Directory created.\n");
}
else if (strcmp(args[0], "cpin") == 0)
{
	char* f1 = args[1]
	char* f2 = args[2]
	int infd;
	int infdSize = 0;
	int reader;
	char buff[512];
	int readSize;
	off_t cpinsize;
	if((infd = open(f1, O_RDONLY)) == -1)
	{
		printf("File opened unsuccessfully.\n");
		break;
	}
	cpinsize = lseek(infd, 0, SEEK_END);
	unsigned int cpinInum = allocINode();
	if(cpinInum < 0)
	{
		printf("There are no more available inodes.\n");
		break;
	}
	unsigned int tempBlock = allocFreeBlock();
	
	rootDir.inode = cpinInum;
	memcpy(rootDir.name, f2, strlen(f2));
	cpinNode.flag = 100077;
	cpinNode.nlinks = 1;
	cpinNode.uid = '0';
	cpinNode.gid = '0';
	cpinNode.size0 = '0';
	cpinNode.size1 = cpinsize;
	cpinNode.actime[0] = 0;
	cpinNode.modtime[0] = 0;
	cpinNode.modtime[1] = 0;
	
	while(1)
	{
		if((readSize = read(infd, buff, 512)) != 0)
		{
			lseek(fd, tempBlock * 512, SEEK_SET);
			write(fd, buff, 512);
			cpinNode.addr[i] = tempBlock;
			if(readSize < 512)
			{
				
			}
		}
	}
}
}
}

unsigned int freeBlock(int n)
{
	sb.free[sb.nfree] = n;
	sb.nfree++;
}

unsigned int allocFreeBlock()
{
	sb.nfree--;
	int block = sb.free[sb.nfree];
	sb.free[sb.nfree] = 0;
	
	if(sb.nfree == 0)
	{
		int i;
		lseek(fd, block * 512, SEEK_SET);
		read(fd, tempArray, SEEK_SET);
		sb.nfree = tempArray[0];
		for(i = 0; i < 100; i++)
		{
			sb.free[i] = tempArray[i + 1];
		}
	}
	return block;
}

unsigned short allocINode()
{
	unsigned short iNum;
	sb.ninode--;
	iNum = sb.inode[sb.ninode];
	return iNum;
}

int freeINode(int fd, int n)
{
	int index = (n + 512) + 1025;
	lseek(fd, index, SEEK_SET);
	write(fd, "00", 2);
	return n;
}