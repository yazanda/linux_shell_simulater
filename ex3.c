/*
 * Shell Simulator
 * Authored by Yazan Daefi
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <fcntl.h>
//defining const strings.
#define STOP "done"
#define CD "cd"
#define ERROR "error"
#define HISTORY "history"
#define FROM_HISTORY "dontPlay"
#define PIPE "pipe"
#define NOHUP "nohup"
#define MAX_LENGTH 514
//global variables to use in the program.
int commandsNum = 0, commandLength = 0,totalWords = 0, pipesNum = 0, totalPipes = 0, marks = 0;
int backGround = 0;
char status[MAX_LENGTH] = "";
char parsedCmd[MAX_LENGTH] = "";

//defining functions.
void shellPlayer();
void getStatus(char *);
void parsing(const char *);
void commandPlay(char *);
void saveToHistory(char *);
void getHistory(long n, char *);
void makeArray(char **, char *);
void pipePlayer(char *);
void onePipePlayer(char **, char **);
void twoPipesPlayer(char **, char **, char **);
void freeAllocated(char **);
void sig_handler(int);

/*main function of the program.
 * it uses a loop to receive an input from the user, prints the path of the shell,
 * and calls other functions to use.
 */
void shellPlayer() {
    char input[MAX_LENGTH]; //to save the input of the user.
    char myPath[PATH_MAX]; //to save the path of the program.
    while (1) {
        //printing the path.
        if (getcwd(myPath, sizeof(myPath)) != NULL) {
            printf("%s>", myPath);
        } else {
            perror("getcwd() error");
            return;
        }
        backGround = 0; commandLength = 0;
        fgets(input, MAX_LENGTH, stdin);
        input[strlen(input) - 1] = '\0'; //replace the '\n' by '\0'.
        if(strlen(input) == 0) continue; //enter case.
        parsing(input);
        getStatus(parsedCmd);
        printf("Parsed Command: %s\n", parsedCmd);
        if(backGround) puts("runs at the background!");
        //stop if the user input is done.
        if (strcmp(status, STOP) == 0) {
            totalWords--;
            break;
        }
            //pipe case.
        else if (strcmp(status, PIPE) == 0 || (strcmp(status,NOHUP) == 0 && pipesNum > 0)) {
            if(pipesNum < 3)
                pipePlayer(parsedCmd);
            else fprintf(stderr,"Can't run more than 2 pipes!\n");
        }
            //nohup case.
        else if (strcmp(status, NOHUP) == 0) {
            printf("nohup: ignoring input and redirecting stderr to stdout!\n");
            getStatus(parsedCmd);
            commandPlay(parsedCmd);
        }
        //saving the commands to history (not cd and not the '!' command).
        if (strcmp(status, CD) != 0 && strcmp(status, FROM_HISTORY) != 0 && commandLength != 0) {
            saveToHistory(input); commandsNum++;
        }
        //running commands from history.
        if (strcmp(status, FROM_HISTORY) == 0) {
            strcpy(input,parsedCmd);
            parsing(input);
            getStatus(parsedCmd);
            if(strlen(parsedCmd) != 0) {
                saveToHistory(parsedCmd); commandsNum++;
                if (pipesNum == 0)commandPlay(parsedCmd);
                else if(pipesNum < 3) pipePlayer(parsedCmd);
                else fprintf(stderr,"Can't run more than 2 pipes!\n");
            }
            strcpy(status, "");
        }
            //simple commands.
        else if (strcmp(status, FROM_HISTORY) != 0 && strcmp(status, PIPE) != 0 && strcmp(status,NOHUP) != 0 ) {
            commandPlay(parsedCmd);
        }
        strcpy(status, "");
    }
    //printing the total number of the commands and the pipes(when the program ends).
    printf("Num of commands: %d\n", commandsNum);
    printf("Num of words in all commands: %d\n", totalWords);
    printf("Total number of pipes: %d!\n", totalPipes);
    printf("The Program ended!\n");
}
/*
 * A function that receives a char pointer to parse.
 * it gets a new parsedCmd having the format "cmd | cmd | cmd" and equal formats according to the case.
 * in addition, it parses commands that called from history.
 */
void parsing(const char *input) {
    int k = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == ' ') {
            while (input[i + 1] == ' ') i++;
        }
        if(input[i] == '|') { pipesNum++; totalPipes++;} //counting.
        if((input[i] == '|' || input[i] == '&') && input[i-1] != ' ') parsedCmd[k++] = ' ';//put space if not exist.
        if(input[i] != '"') parsedCmd[k++] = input[i];
        if(input[i] == '|' && input[i+1] != ' ') parsedCmd[k++] = ' ';
        if(input[i] != ' ' && input[i] != '|' && (input[i+1] == ' ' || input[i+1] == '\0' || input[i+1] == '|')) commandLength++;
        if(input[i] == '!') marks++;
    }
    totalWords += commandLength;
    parsedCmd[k] = '\0';
    char *cmd = parsedCmd;
    if(marks > 0){
        char cmd1[100] = ""; char cmd2[100] = ""; char cmd3[100] = "";
        char *token;
        int count = 0;
        //parsing the command.
        if(pipesNum == 0){
            getHistory(strtol(cmd + 1, (char **) NULL, 10),cmd1);
            strcpy(parsedCmd,cmd1);
        }
        else if(pipesNum == 1) {
            //cutting the command.
            while((token = strtok_r(cmd,"|",&cmd)) && count < 2){
                if(count == 0) strncpy(cmd1, token, strlen(token)-1);
                else strncpy(cmd2, token+1, strlen(token)-1);
                count++;
            }
            //calling from history.
            if(cmd1[0] == '!') getHistory(strtol(cmd1 + 1, (char **) NULL, 10),cmd1);
            if(cmd2[0] == '!') getHistory(strtol(cmd2 + 1, (char **) NULL, 10),cmd2);
            sprintf(parsedCmd,"%s | %s", cmd1, cmd2);
        }
        else if (pipesNum == 2) {
            count = 0;
            //cutting the command.
            while((token = strtok_r(cmd,"|",&cmd)) && count < 3){
                if(count == 0) strncpy(cmd1, token, strlen(token)-1);
                else if(count == 1) strncpy(cmd2, token+1, strlen(token)-2);
                else strncpy(cmd3, token+1, strlen(token)-1);
                count++;
            }
            //calling from history.
            if(cmd1[0] == '!')getHistory(strtol(cmd1 + 1, (char **) NULL, 10),cmd1);
            if(cmd2[0] == '!')getHistory(strtol(cmd2 + 1, (char **) NULL, 10),cmd2);
            if(cmd3[0] == '!')getHistory(strtol(cmd3 + 1, (char **) NULL, 10),cmd3);
            sprintf(parsedCmd, "%s | %s | %s", cmd1, cmd2, cmd3);
        }
    }
}
/*
 * function that checks the command status, we have many cases such as error, pipe, nohup, history, get commands from history and cd.
 */
void getStatus(char *cmd) {
    char tmp[6];
    //saving the first 5 chars to check if it's a nohup command.
    strncpy(tmp, cmd, 5); tmp[5] = '\0';
    //check if contains & to run in the background.
    if(cmd[strlen(cmd)-1] == '&' && marks == 0){
        backGround = 1;
        strncpy(parsedCmd,cmd, strlen(cmd)-2);
        parsedCmd[strlen(cmd)-2] = '\0';
    }
    //spaces before or after the command.
    if(cmd[0] == ' ' || cmd[strlen(cmd) - 1] == ' ') {
        strcpy(status, ERROR);
        return;
    }
    else if(strcmp(cmd, HISTORY) == 0){
        strcpy(status,HISTORY);
    }
    else if(strcmp(cmd, STOP) == 0 || strcmp(cmd, CD) == 0)
        strcpy(status,cmd);
        //contains a read to commands from history.
    else if(marks > 0){
        strcpy(status, FROM_HISTORY);
        totalPipes -= pipesNum;
        totalWords -= commandLength;
        commandLength = 0;
        marks = 0;
    }
        //nohup case.
    else if(strcmp(tmp, NOHUP) == 0){
        strcpy(parsedCmd,cmd+6);
        strcpy(status,NOHUP);
    }
    else{ //pipe case.
        pipesNum = 0;
        for(int i = 0; cmd[i] != '\0'; i++){
            if(cmd[i] == '|'){ pipesNum++;}
        }
        if(pipesNum > 0) strcpy(status, PIPE);
    }
}
/*a main function of the program.
 * it runs the command according to the status(error,cd,history or a shell command).
 * opens another child process in order to run the command.
 */
void commandPlay(char *input) {
    int info;
    waitpid(-1,&info,0);
    //invalid command case.
    if (strcmp(status, ERROR) == 0)
        fprintf(stderr, "error: command with spaces!\n");
        //cd command case.
    else if (strcmp(status, CD) == 0)
        fprintf(stderr, "Command not supported (Yet)\n");
        //print the history command case.
    else if (strcmp(status, HISTORY) == 0)
        getHistory(-1, NULL);
    else {
        pid_t child;
        char **command = (char **) calloc(commandLength + 1, sizeof(char *));
        if(command == NULL){
            freeAllocated(command);
            fprintf(stderr,"alloc() error");
            exit(1);
        }
        makeArray(command, input);
        //nohup case (continue running after ends the program).
        if(strcmp(status,NOHUP) == 0) {
            //opening file to redirect the output in.
            int fd = open("nohup.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            //receiving signal when the program exit, then ignoring it to continue running the command.
            signal(SIGHUP, SIG_IGN);
            //opening a child process.
            child = fork();
            //failing fork case.
            if (child < 0) {
                perror("fork() error");
                exit(1);
            } else if (child == 0) { // the child's process.
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup() failed");
                    exit(1);
                }
                if (execvp(command[0], command) < 0)
                    perror("execvp() error");
                close(fd);
                exit(0);
            }
        }else{
            child = fork();
            //failing fork case.
            if (child < 0) {
                perror("fork() error");
                exit(1);
            } else if (child == 0) { // the child's process.
                if(backGround) {
                    //prctl(PR_SET_PDEATHSIG,SIGHUP); //when the process exits send sighup.
                    raise(SIGCHLD);
                }
                if (execvp(command[0], command) < 0) {
                    perror("execvp() error");
                }
                exit(0);
            }
        }
        if(!backGround) wait(&info); // waiting to the child to end his process.
        //free all the memory after using.
        freeAllocated(command);
        commandLength = 0;
    }
}
void pipePlayer(char *input) {
    //initialize a memory arrays to save the command.
    char **command1;
    char **command2;
    char **command3;
    int length = 0, secondCo = 0, thirdCo = 0, begin = 0;
    //parsing the commands into the arrays.
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] != ' ' && input[i] != '|' &&
            (input[i + 1] == ' ' || input[i + 1] == '|' || input[i + 1] == '\0')) {
            length++;
        }
        if(input[i] == '|'){
            if(input[i+1] != ' ') begin = i + 1;
            else begin = i + 2;
        }
        if (input[i] != ' ' && (input[i+2] == '|') && !secondCo && !thirdCo) {
            command1 = (char **) calloc(length + 1, sizeof(char *));
            char tmp[i-begin+1]; tmp[i-begin+1] = '\0';
            makeArray(command1,strncpy(tmp,input+begin,i-begin+1));
            secondCo = 1; length = 0;
        } else if (secondCo && ((pipesNum == 1 && input[i+1] == '\0')||(pipesNum == 2 && input[i+2] == '|'))) {
            if (((input[i+1] == '|' || input[i+1] == ' ') && pipesNum == 2) || (input[i + 1] == '\0' && pipesNum == 1)) {
                command2 = (char **) calloc(length + 1, sizeof(char *));
                char tmp[i-begin+2]; tmp[i-begin+1] = '\0';
                makeArray(command2, strncpy(tmp,input+begin,i - begin + 1));
                thirdCo = 1; secondCo = 0; length = 0;
            }
        } else if (thirdCo && pipesNum == 2 && input[i + 1] == '\0') {
            command3 = (char **) calloc(length + 1, sizeof(char *));
            char tmp[i-begin+2]; tmp[i-begin+1] = '\0';
            makeArray(command3, strncpy(tmp,input+begin,i - begin + 1));
            thirdCo = 0; length = 0;
        }
    }
    if (command1 == NULL || command2 == NULL || (pipesNum == 2 && command3 == NULL)) { //failing allocate case.
        freeAllocated(command1);
        freeAllocated(command2);
        if(pipesNum == 2) freeAllocated(command3);
        fprintf(stderr, "alloc() error");
        exit(1);
    }
    if(pipesNum == 2)
        twoPipesPlayer(command1, command2, command3);
    else if(pipesNum == 1)
        onePipePlayer(command1, command2);
}
/*
 * function that plays command contains one pipe.
 * receives two dynamic arrays that contains two commands.
 */
void onePipePlayer(char **command1, char **command2) {
    int pipe_fd[2];
    pid_t child1, child2;
    int info;
    if (pipe(pipe_fd) < 0) {
        perror("pipe() failed");
        exit(1);
    }
    else { //starting two childes processes.
        if ((child1 = fork()) == 0) {
            if(backGround) {
                prctl(PR_SET_PDEATHSIG,SIGHUP);//when the process exits send sighup.
                raise(SIGCHLD);
            }
            if(dup2(pipe_fd[1], STDOUT_FILENO) < 0){
                perror("dup2() failed");
                freeAllocated(command1); freeAllocated(command2);
                exit(1);
            }
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            if (execvp(command1[0], command1) < 0)
                perror("execvp() error");
            exit(0);
        } else if (child1 < 0) {
            perror("fork() failed");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            freeAllocated(command1); freeAllocated(command2);
            exit(1);
        }
        if ((child2 = fork()) == 0) {
            if(backGround) {
                prctl(PR_SET_PDEATHSIG,SIGHUP);//when the process exits send sighup.
                raise(SIGCHLD);
            }
            if(dup2(pipe_fd[0], STDIN_FILENO) < 0){
                perror("dup2() failed");
                freeAllocated(command1); freeAllocated(command2);
                exit(1);
            }
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            if (execvp(command2[0], command2) < 0)
                perror("execvp() error");
            exit(0);
        } else if (child2 < 0) {
            perror("fork() failed");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            freeAllocated(command1); freeAllocated(command2);
            exit(1);
        } else {
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            if(!backGround) {
                wait(&info);
                wait(&info);
            }
        }
    }
    freeAllocated(command1); freeAllocated(command2);
    pipesNum = 0;
}\
/*
 * function that plays command contains two pipes.
 * receives two dynamic arrays that contains two commands.
 */
void twoPipesPlayer(char **command1, char **command2, char **command3){
    int pipe_fd[4];
    int info;
    pid_t child1, child2, child3;
    if(pipe(pipe_fd) < 0 || pipe(pipe_fd+2) < 0) {
        perror("pipe() failed");
        exit(1);
    }
    else{ //starting 3 childes processes.
        if((child1 = fork()) == 0){
            if(backGround) {
                prctl(PR_SET_PDEATHSIG,SIGHUP);//when the process exits send sighup.
                raise(SIGCHLD);
            }
            if(dup2(pipe_fd[1], STDOUT_FILENO) < 0){
                perror("dup2() failed");
                freeAllocated(command1); freeAllocated(command2);
            }
            for(int i = 0; i < 4; i++) {close(pipe_fd[i]);}
            if (execvp(command1[0], command1) < 0)
                perror("execvp() error");
            exit(0);
        }else if (child1 < 0) {
            perror("fork() failed");
            freeAllocated(command1); freeAllocated(command2); freeAllocated(command3);
            exit(1);
        }if ((child2 = fork()) == 0) {
            if(backGround) {
                prctl(PR_SET_PDEATHSIG,SIGHUP);//when the process exits send sighup.
                raise(SIGCHLD);
            }
            if(dup2(pipe_fd[0], STDIN_FILENO) < 0 || dup2(pipe_fd[3],STDOUT_FILENO) < 0) {
                perror("dup2() failed");
                freeAllocated(command1); freeAllocated(command2); freeAllocated(command3);
            }
            for(int i = 0; i < 4; i++) close(pipe_fd[i]);
            if (execvp(command2[0], command2) < 0)
                perror("execvp() error");
            exit(0);
        } else if (child2 < 0) {
            perror("fork() failed");
            for(int i = 0; i < 4; i++) {close(pipe_fd[i]);}
            freeAllocated(command1); freeAllocated(command2); freeAllocated(command3);
            exit(1);
        }if((child3 = fork()) == 0){
            if(backGround) {
                prctl(PR_SET_PDEATHSIG,SIGHUP);//when the process exits send sighup.
                raise(SIGCHLD);
            }
            if(dup2(pipe_fd[2], STDIN_FILENO) < 0){
                perror("dup2() failed");
                freeAllocated(command1); freeAllocated(command2); freeAllocated(command3);
            }
            for(int i = 0; i < 4; i++) {close(pipe_fd[i]);}
            if (execvp(command3[0], command3) < 0)
                perror("execvp() error");
            exit(0);
        }else if (child3 < 0) {
            perror("fork() failed");
            for(int i = 0; i < 4; i++) {close(pipe_fd[i]);}
            freeAllocated(command1); freeAllocated(command2); freeAllocated(command3);
            exit(1);
        }else{
            for(int i = 0; i < 4; i++){ close(pipe_fd[i]);}
            if(!backGround) {
                wait(&info);
                wait(&info);
                wait(&info);
            }
        }
    }
    freeAllocated(command1); freeAllocated(command2); freeAllocated(command3);
    pipesNum = 0;
}
/*
 * function that parsing the command into a dynamic array.
 * receives the dynamic array and the command.
 */
void makeArray(char **command, char *input) {
    int charsNum = 0;
    //a temporary string to save each word of the command and copy to the array.
    char tmp[strlen(input)];
    //failing allocate case.
    if (command == NULL) {
        freeAllocated(command);
        fprintf(stderr, "alloc() error");
        exit(1);
    }
    int k = 0;
    for (int i = 0, j = 0; input[i] != '\0'; i++) {
        if (input[i] != ' ') { //copy the word to tmp and count the chars.
            tmp[j++] = input[i];
            charsNum++;
        }
        if (input[i] != ' ' && (input[i + 1] == ' ' || input[i + 1] == '\0')) {
            tmp[j] = '\0';
            j = 0;
            //allocating a place for the word.
            command[k] = (char *) calloc(charsNum + 1, sizeof(char));
            if (command[k] == NULL) {
                freeAllocated(command);
                fprintf(stderr, "alloc() error");
                exit(1);
            }
            //copying the word to the array.
            strcpy(command[k++], tmp);
            strcpy(tmp, "");
            charsNum = 0;
        }
        command[k] = NULL;
    }
}
/*
 *function receives a string and print it to the file as a line.
 *inputs: pointer to char/char array.
*/
void saveToHistory(char *line) {
    int linesCounter = 1;
    FILE *his = fopen("file.txt", "a+");
    if (his != NULL) {
        char c;
        //counting the lines in the file in order to know the number of the next line that we want to print.
        while ((c = (char) getc(his)) != EOF)
            if (c == '\n') linesCounter++;
        //writing the next line to the file.
        fprintf(his, "%d: %s\n", linesCounter, line);
        fclose(his);
    } else {
        printf("Problem with opening the file!");
        exit(1);
    }
}
/*
 * function that read the history file to print it or get a command from the history according to the input.
 * input: a number n, the number of the command that the user want to run.
 * if n = -1 it prints the history.
 */
void getHistory(long n, char * cmd) {
    FILE *fr = fopen("file.txt", "r");
    if (fr != NULL) {
        /*
         *f seek in the file(move the pointer from the top of the file to the end).
         *inputs: pointer to the file, beginning index and where to go.
        */
        fseek(fr, 0, SEEK_END);
        //f tell(returns the current file position of the specified stream with respect to the starting of the file).
        long size = ftell(fr);
        fclose(fr);
        //if size = 0 so the file is empty.
        if (0 == size)
            printf("There's no history saved!\n");
        else {
            //opening the file another time to read the history.
            fr = fopen("file.txt", "r");
            if (fr != NULL) {
                if (n == -1) {
                    char c;
                    //reading from the file char by char and print to the screen.
                    while ((c = (char) fgetc(fr)) != EOF) {
                        printf("%c", c);
                    }
                } else { //find the n command.
                    char str[514];
                    int lineNumber = 1, linesCounter = 0;
                    while (fgets(str, 514, fr) != NULL && n > 0) {
                        linesCounter++;
                        if (lineNumber == n) {
                            str[strlen(str) - 1] = '\0';
                            int k = 0;
                            while (str[k] != ' ') k++;
                            //copying the command to the variable to run it.
                            strcpy(cmd, str + k + 1);
                            break;
                        }
                        lineNumber++;
                    }
                    //the case if the command is not in the history.
                    if (lineNumber > linesCounter || n < 1) {
                        strcpy(status, FROM_HISTORY);
                        fprintf(stderr, "NOT IN HISTORY\n");
                    }
                }
                fclose(fr);
            } else {
                printf("Problem with opening the file!");
                exit(1);
            }
        }
    } else {
        printf("Problem with opening the file!");
        exit(1);
    }
}
/*
 * function that free all the allocated memory by the pointer that it receives.
 */
void freeAllocated(char **commands) {
    if (commands != NULL) {
        int i = 0;
        while (commands[i] != NULL) {
            free(commands[i++]);
        }
        free(commands);
        commands = NULL;
    }
}
/*
 * signal handler function that free all the memory allocated by any child process when it runs in the background.
 * uses waitpid to free all the childes.
 */
void sig_handler(int sig){
    int wait_signal = 1;
    while(wait_signal > 0)
        wait_signal = waitpid(-1,&sig,WNOHANG);
}
int main() {
    //receiving a signal from any child process when it exits, then calling sig_handler function.
    signal(SIGCHLD,sig_handler);
    shellPlayer();
    return 0;
}
