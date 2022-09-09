/*
AUTHOR: Gopikrishnan Rajeev
ID: 110085458
*/
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <linux/limits.h>


short p;	//Used for storing the inode of files which are symbolic links
int cycles = 0;	//Handles the number of cycles tracked
char lock = 0;		//lock the execution of codeblock

int symbolicCounter(const char *fpath, const struct stat *sb,
                   int typeflag, struct FTW *ftwbuf){

	if(typeflag == FTW_SL){		// Check if file pointed by filepath is a symbolic link
		char flg = 1;
		short inode = sb->st_ino;		//Fetch inode of the current file
		if(p==inode){		//Check if p has the current file's inode
			cycles++;	//Increment if not visited before
			flg = 0;	//set flg to 0 so that next set of statements wont be executed
		}

		if(flg && lock==0){
			char *buf = malloc(PATH_MAX);	//Create buffer to hold realpath with size of maximum path possible
			p = inode;	//Add the inode of current file to p
			realpath(fpath,buf);		//Fetch realpath of symbolic link and put into buffer
			lock++;	//prevent this block of code from being executed as we want to perform search in the recursive call, and not detect another symbolic link
			nftw(buf, symbolicCounter, 60, FTW_F | FTW_D);		//Call nftw() on the real path to perform search, set flags to FTW_F | FTW_D to include both directory and file traversals
			lock--;	//unlock after checking
			free(buf);	//Free the buffer
		}
	}
	return 0;
}

char main(int argc, char *argv[]){
	nftw((argc<2)?"./":argv[1], symbolicCounter, 60, FTW_F | FTW_D);
	printf("Found %d cycles",cycles);	//Print the number of cycles
	return 0;
}
