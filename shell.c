#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

int countPipes(char *input) {
	int pipes = 0;
	if(input == NULL || strcmp(input, "") == 0) {
		return -1;
	}
	else {
		for(int i = 0; input[i] != '\0'; i++) {
			if(input[i] == '|') {
				pipes++;
			}
		}
	}
	return pipes;
}

char **getCommand(char *input) {
	char **command = malloc(8 * sizeof(char *));
	char *separator = " ";
	char *parsed;
	int index = 0;
	int count = 8;

	parsed = strtok(input, separator);
	while(parsed != NULL) {
		command[index] = parsed;
		index++;
		parsed = strtok(NULL, separator);

		if(index >= count)
		{
			count *= 2;
			command = realloc(command, count * sizeof(char *));
		}
	}
	command[index] = NULL;
	return command;
}

int cd(char *path) {
	return chdir(path);
}

char **getPipes(char *input) {
	char **command = malloc(8 * sizeof(char *));
	char *separator = "|";
	char *parsed;
	int index = 0;
	int count = 8;

	parsed = strtok(input, separator);
	while(parsed != NULL) {
		command[index] = parsed;
		index++;
		parsed = strtok(NULL, separator);
		if(index >= count)
		{
			count *= 2;
			command = realloc(command, count * sizeof(char *));
		}
	}
	command[index] = NULL;
	return command;
}

int execPipes(char **piped_command, int pipein[2], int pipeout[2], int first, int last) {
	pid_t pid1;
	pid1 = fork();
	int stat_loc = 0;
	if(pid1 < 0) {
		perror("Piped fork failed");
		exit(1);
	}
	else if(pid1 == 0) {
	
		if(first == 1) {

			if(dup2(pipeout[1], STDOUT_FILENO) < 0) 
				perror("Dup2 out first failed");
			close(pipeout[0]);
			close(pipein[1]);
		}
		
		else if(last == 1) {

			if(dup2(pipein[0], STDIN_FILENO) < 0) 
				perror("Dup2 in last failed");
			close(pipein[1]);
			close(pipeout[0]);
		}
		
		else {

			if(dup2(pipein[0], STDIN_FILENO) < 0) 
				perror("Dup2 in failed");
			close(pipein[1]);

			if(dup2(pipeout[1], STDOUT_FILENO) < 0) 
				perror("Dup2 out failed");
			close(pipeout[0]);
		}
		close(pipein[0]);
		close(pipeout[1]);
		execvp(piped_command[0], piped_command);
		if(execvp(piped_command[0], piped_command) < 0){
			perror(piped_command[0]);
			exit(1);
		}
		else {
			waitpid(pid1, &stat_loc, WUNTRACED); // report after child stopped
		}
	}

	close(pipein[0]);
	close(pipeout[1]);

	return 0; 
}

int main() {
	int pipeCount;

	while(1) {
		char *input;
		input = readline(">$ ");
		if(input == NULL) {
			printf(" EOF\n");
			exit(1);
		}
		
		pipeCount = countPipes(input);
		switch(pipeCount)
		{
			case -1:
				continue;
				break;
			case 0:{
				char **command;
				command = getCommand(input);
				
				int stat_loc;

				if(strcmp(command[0], "exit") == 0) {
					exit(0);
				}

				if(strcmp(command[0], "cd") == 0) {
					if(cd(command[1]) < 0) {
						perror(command[1]);
					}
					break;
				}

				pid_t child_pid;
				child_pid = fork();
				
				if(child_pid < 0) {
					perror("Fork failed");
					exit(1);
				}
				else if(child_pid == 0) {
					execvp(command[0], command);
					if (execvp(command[0], command) < 0) { // bin/ls
						perror(command[0]);
						exit(1);
					}
				} 
				else {
					waitpid(child_pid, &stat_loc, WUNTRACED);
				}	

				free(command);
				break;
			}
			default:{
				char **pipeInput = malloc((pipeCount+1)* sizeof(char));
				int pipes[pipeCount+1][2];
				for(int i = 0; i < pipeCount+1; i++) {
					if(pipe(pipes[i]) < 0) 
						perror("Piping failed\n");
				}

				pipeInput = getPipes(input);

				for(int i = 0; i < pipeCount+1; i++){
					if(i == 0){
						execPipes(getCommand(pipeInput[i]), pipes[i-1], pipes[i], 1, 0);  // first
					}
					else if(i == pipeCount){
						execPipes(getCommand(pipeInput[i]), pipes[i-1], pipes[i], 0, 1);  // last
					}
					else{
						execPipes(getCommand(pipeInput[i]), pipes[i-1], pipes[i], 0, 0);  // neither
					}
				}

				free(pipeInput);
				for(int i = 0; i < pipeCount+1; i++) {
						close(pipes[i][0]);
						close(pipes[i][1]);
						wait(NULL);
				}				
				break;
			}
		}
		free(input);
	}
	return 0;
}
