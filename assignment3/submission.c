/*
Author: Gopikrishnan Rajeev
ID: 110085458
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int invertSave(int fd, char *file){
char buf, rowNum[100], colNum[100], greyc[100], *fileData, imageHeader[100];
int col, row, grey, len, i=0, j=0, k=0, l=0, headerSize=0, metaCount=3, spacing=0, dataSize;

while(1){ //Fetch the header
	if(metaCount==0)	//Used to make sure we are reading only header part by for counting whitespaces
		break;	//Exit the loop
	read(fd,&buf,1);	//Reading one byte
	if(buf=='\r' || buf == '\n'|| buf == '\t') //Whitespace chars
		metaCount--;	//Reduce metaCount as we encounter whitespaces
	imageHeader[headerSize]=buf;	//Store header into imageHeader
	headerSize++;	//Increment headerSize to track size of header
}

if(imageHeader[0]!='P' && imageHeader[1]!='5') //Validate the magic number
	return 2;

for(i=2;i<headerSize;i++){ //Parse the header and fetch the rows, columns and grey value
	if(imageHeader[i]=='\n')	//Parsing the header for \n and incrementing metaCount to identify which line is being parsed
		metaCount++;	//Incrementing etacount
	if(metaCount==1){	//If the line is the line containing details of rows and cols
		if(imageHeader[i]==' ')	//Detecting spaces to identify if row detail has finished being iterated and parsed
			spacing=1;	//If space has already been iterated make spacing true so that column can be parsed
		if(spacing==0){	//If spacing is 0 means the row is being parsed
			rowNum[j]=imageHeader[i];	//Store the row info
			j++;	//Increment to next rowNum cell
		}else if(imageHeader[i]!=' '){	//Check if the current char is not ' ' (space)
			colNum[k]=imageHeader[i];	//Store the col info
			k++;	//Increment to next rowNum cell
		}
	}
	if(metaCount==2){	//If the line is the line containing details grey value
		greyc[l]=imageHeader[i];	//Store the grey info
		l++;	//Increment to next greyc cell
	}	
}

col = atoi(colNum);	//Convert to int
row = atoi(rowNum);	//Convert to int
grey = atoi(greyc);	//Convert to int

if(row==0||col==0||grey==0) //Validate the rows, columns and grey values
	return 3;

if((len = lseek(fd,0,SEEK_END))==-1) //Find the size of the contents
        return 4;

if(row*col!=(len-headerSize)){ //Validate size of the data part with row and column info
	return 5;
}

if(lseek(fd,headerSize,SEEK_SET)==-1) //Move to the data part
        return 6;

fileData = malloc(sizeof(char)*len);	//Create a buffer of size = len

i=0;
while((read(fd,&buf,1))>0){ //Read the data of the file
	fileData[i]=buf;	//Store the chars into fileData
	i++;	//Increment the iterator
}

if(i!=((len-headerSize))) //Check if all the data was read
	return 7;

close(fd);	//Close the file to reopen in write mode
fd = open(file,O_WRONLY); //Open the file in write mode
if(fd == -1)	//Check if file was opened
	return 8;
	
if(lseek(fd,headerSize,SEEK_SET)==-1) //Move the cursor to the beginning of data part
        return 9;

for(j=0;j<col;j++)
	for(k=i-row*(j+1);k<(i-row*(j+1))+row;k++)	//Go through each column and row to write into file
		write(fd, &fileData[k],1);	//Write to the file

free(fileData);	//Free the buffer
close(fd);	//Close the file
return 0;
}

int main(int argc, char **argv){

int fd = open(argv[1],O_RDONLY);	//Open the input file in read mode
if(fd == -1)	//check if the file was opened
	return 1;
else
	if(invertSave(fd, argv[1])!=0)	//If readFile does not return 0 display "Processing error."
		printf("Processing error.");
return 0;
}
