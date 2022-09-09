/*
Author: Gopikrishnan Rajeev
ID: 110085458
*/

#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>

void parseNrun(char cmd[]){

    int **fd = NULL,*subCommandLength = malloc(sizeof(int)*strlen(cmd)),*instructionIndex = NULL, *index, i, j, cmdLen=0, pipeCount = 0;
    char ***commands = malloc(sizeof(char *)*strlen(cmd)), ***args, *command = NULL, *parsedCommand = NULL, *executableName = NULL;
    for(i=0;i<strlen(cmd);i++)   
        commands[i] = malloc(sizeof(char *)*strlen(cmd));   //Initializing command array

    for(i=0;i<strlen(cmd);i++)  //Counting the number of pipes in the entire command
        if(cmd[i]=='|')
            pipeCount++;
    
    char* rest = cmd;
    while ((command = strtok_r(rest, "|", &rest))){ //Seperating each command having pipes
        char *rest1 = command;
        char *subcommand;
        i=0;
        while ((subcommand = strtok_r(rest1, " ", &rest1))){    //Remove all spaces in the subcommand
            commands[cmdLen][i]=subcommand; //Save the individual subcommand and args in the 3d array
            i++;
        }
        subCommandLength[cmdLen]=i; //Save the subcommand lengths inside the subCommandLength array
        cmdLen++;
    }

    args = malloc(sizeof(char *)*cmdLen);   //Array to store args for all execv processes

    for(i=0;i<cmdLen;i++)
        args[i]= malloc(sizeof(char*)*cmdLen);  //Initialize for each subcommand

    instructionIndex = malloc(sizeof(int));   //instructionIndex will be used to iterate through the commandList
    index = malloc(sizeof(int));    //Index to iterate through each args for each subcommand
    *index = 0;    

    if(pipeCount>0){    //If we find even 1 pipe we are sure that there are either more pipes or the command will end with the next one

        fd = malloc(sizeof(int *)*pipeCount);   //Create fds dynamically using pipecount
        
        for(i=0;i<cmdLen;i++){  //Initialize the fds with dynamic memory
            fd[i] = malloc(sizeof(int)*2);
            pipe(fd[i]);    //Open pipes with the fds
        }
        
        for(*instructionIndex=0;*instructionIndex<cmdLen;(*instructionIndex)++){    //Iterate through each process
            pid_t pid = fork();
            if(pid){    //Parent
                int status;
                if(*instructionIndex<cmdLen-1)  //Close write fd of the parent provided its not the last subcommand
                    close(fd[*instructionIndex][1]);
                wait(&status);  //Wait for child to exit
            }
            else{
                if(*instructionIndex==0){   //First subcommand needs to only write to pipe, so redirect STDOUT to corresponding fd[1]
                    dup2(fd[*instructionIndex][1], STDOUT_FILENO); //Redirect STDOUT to fd[1]
                    close(fd[*instructionIndex][0]);    //Close fd[0]
                    close(fd[*instructionIndex][1]);    //Close fd[1]
                }
                else if(*instructionIndex==pipeCount){  //Last subcommand needs to only read from pipe, so redirect STDIN to corresponding fd[0]
                    dup2(fd[(*instructionIndex)-1][0], STDIN_FILENO); //Redirect STDIN to fd[0]
                    close(fd[(*instructionIndex)-1][1]);    //Close fd[1]   
                    close(fd[(*instructionIndex)-1][0]);    //Close fd[0]
                }
                else{   //In between suncommands need to read from previous pipe and write to next pipe, so redirect STDIN and STDOUT to corresponding fds of pipes
                    dup2(fd[(*instructionIndex)-1][0], STDIN_FILENO);// 0
                    dup2(fd[*instructionIndex][1], STDOUT_FILENO); // 1
                    close(fd[(*instructionIndex)-1][1]);    //Close fd[1] of previous pipe
                    close(fd[(*instructionIndex)][0]);  //Close fd[0] of next pipe
                    close(fd[(*instructionIndex)-1][0]);    //Close fd[0] of previous pipe
                    close(fd[*instructionIndex][1]);    //Close fd[1] of next pipe
                }

                if(commands[*instructionIndex][0][0]=='.'&&commands[*instructionIndex][0][1]=='/'){ //If command is to execute a binary in the current working dir
                    char path[PATH_MAX];
                    char executableName[PATH_MAX];
                    char *slash = "/";
                    args[*index] = malloc(sizeof(char *)*(subCommandLength[*instructionIndex]+1));   //Create args list to be used by execv with extra space for NULL

                    getcwd(path, PATH_MAX); //Get current working dir name and add to array
                    strcpy(executableName,commands[*instructionIndex][0]+2);    //Get binary name after removing "./" and copy to executable name
                    strcat(path,slash); //Concatenate a slash to the end of the path
                    strcat(path,executableName);    //Concatenate the name of the executable to 'path' to obtain the path of the binary as execv requires pathname of the binary to be executed

                    args[*index][0]=path;   //Add path of binary to the arglist as required by execv command
                    for(i=1;i<subCommandLength[*instructionIndex];i++)
                        args[*index][i] = commands[*instructionIndex][i];   //Add remaining args to the list
                    args[*index][i] = NULL; //Add NULL to the end of arglist as required by execv command

                    (*index)++;
                    execv(path, args[*index-1]);  //Run execv

                    exit(0);    //Exit to signal parent
                }
                else{   //If command is to execute a binary in /usr/bin dir
                    char path[] = "/usr/bin";
                    args[*index] = malloc(sizeof(char *)*(subCommandLength[*instructionIndex]+1));   //Create args list to be used by execv with extra space for NULL
                    char executableName[PATH_MAX];
                    char *slash = "/";

                    strcpy(executableName,commands[*instructionIndex][0]);  //Get binary name and copy to executable name
                    strcat(path,slash); //Concatenate a slash to the end of the path
                    strcat(path, executableName);   //Concatenate the name of the executable to 'path' to obtain the path of the binary as execv requires pathname of the binary to be executed
                    
                    args[*index][0]=commands[*instructionIndex][0]; //Add path of binary to the arglist as required by execv command
                    for(i=1;i<subCommandLength[*instructionIndex];i++)
                        args[*index][i] = commands[*instructionIndex][i];   //Add remaining args to the list
                    args[*index][i] = NULL; //Add NULL to the end of arglist as required by execv command

                    (*index)++;
                    execv(path, args[*index-1]);  //Run execv

                    exit(0);    //Exit to signal parent
                }
            }
        }
    }
    else{   //If the command to be executed does not contain '|'
        for(*instructionIndex=0;*instructionIndex<cmdLen;(*instructionIndex)++){
            
            if(strcmp(commands[*instructionIndex][0],"cd")==0){ //If the command is 'cd'
                char s[PATH_MAX];
                char *login, *dir, home[] = "/home/";

                if(commands[*instructionIndex][1]==NULL||strcmp(commands[*instructionIndex][1], "~")==0||strcmp(commands[*instructionIndex][1], "HOME")==0){    //To handle 'cd', 'cd ~' and 'cd HOME'
                    login = getlogin(); //Fetch the name of the current user
                    strcat(home, login);    //Append to the home variable to fetch the current user's home directory
                    dir = home; //Change directory to be CD'd into 'home'
                }
                else 
                    dir = commands[*instructionIndex][1];   //To handle all other 'cd' command arguments 
                
                if (chdir(dir) == 0)    //Attempt to change the directory
                        printf("%s\n", getcwd(s, PATH_MAX));    //If successful print it
                else
                        printf("failed %s\n", getcwd(s, PATH_MAX)); //If failed print it
            
            }

            if(commands[*instructionIndex][0][0]=='.'&&commands[*instructionIndex][0][1]=='/'){ //If command is to execute a binary in the current working dir
                    pid_t pid = fork();
                    if(pid){
                        int status;
                        wait(&status);  //Wait for child to exit
                    }
                    else{
                        char path[PATH_MAX];
                        char executableName[PATH_MAX];
                        char *slash = "/";
                        args[*index] = malloc(sizeof(char *)*(subCommandLength[*instructionIndex]+1));   //Create args list to be used by execv with extra space for NULL

                        getcwd(path, PATH_MAX); //Get current working dir name and add to array
                        strcpy(executableName,commands[*instructionIndex][0]+2);    //Get binary name after removing "./" and copy to executable name
                        strcat(path,slash); //Concatenate a slash to the end of the path
                        strcat(path,executableName);    //Concatenate the name of the executable to 'path' to obtain the path of the binary as execv requires pathname of the binary to be executed

                        args[*index][0]=path;   //Add path of binary to the arglist as required by execv command
                        for(i=1;i<subCommandLength[*instructionIndex];i++)
                            args[*index][i] = commands[*instructionIndex][i];   //Add remaining args to the list
                        args[*index][i] = NULL; //Add NULL to the end of arglist as required by execv command

                        (*index)++;
                        execv(path, args[*index-1]);  //Run execv

                        exit(0);    //Exit to signal parent
                    }
               
             }
            else{   //If command is to execute a binary in /usr/bin dir
                    pid_t pid = fork();
                    if(pid){
                        int status;
                        wait(&status);  //Wait for child to exit
                    }
                    else{
                        char path[] = "/usr/bin";
                        args[*index] = malloc(sizeof(char *)*(subCommandLength[*instructionIndex]+1));   //Create args list to be used by execv with extra space for NULL
                        char executableName[PATH_MAX];
                        char *slash = "/";

                        strcpy(executableName,commands[*instructionIndex][0]);  //Get binary name and copy to executable name
                        strcat(path,slash); //Concatenate a slash to the end of the path
                        strcat(path, executableName);   //Concatenate the name of the executable to 'path' to obtain the path of the binary as execv requires pathname of the binary to be executed
                        
                        args[*index][0]=commands[*instructionIndex][0]; //Add path of binary to the arglist as required by execv command
                        for(i=1;i<subCommandLength[*instructionIndex];i++)
                            args[*index][i] = commands[*instructionIndex][i];   //Add remaining args to the list
                        args[*index][i] = NULL; //Add NULL to the end of arglist as required by execv command

                        (*index)++;
                        execv(path, args[*index-1]);  //Run execv

                        exit(0);    //Exit to signal parent
                    }      
                }
            }
        
        }

    free(index);    //Free all the pointers  which are dynamically allocated
    free(args);
    free(instructionIndex); 
    free(fd);
    free(subCommandLength);
    free(commands);    

}

int main(int argc, char **argv){
    char *command = NULL, *rest = NULL, commandBuf[10000];  
    while(1){
        printf("%s","(Cute Cherry Shell)$");
        fgets(commandBuf, 64, stdin);   //Fetch user input
        if(strcmp(commandBuf,"exit\n")==0)  //Upon typing exit to exit shell
            break;
        rest = commandBuf;
        while ((command = strtok_r(rest, ";", &rest))){ //Fetch individual commands from multiple commands on single line (';')
                command[strcspn(command, "\n")] = 0;    //Removing the \n at the end of command
                parseNrun(command); //Run the command
        }
    }
    return 0;
}