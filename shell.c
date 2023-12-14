#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFSIZE 4096

/* Retrieve the hostname and make sure that this program is not being run on the main odin server.
 * It must be run on one of the vcf cluster nodes (vcf0 - vcf3).
 */
void check()
{
        char hostname[10];
		gethostname(hostname, 9);
        hostname[9] = '\0';
        if (strcmp(hostname, "csci-odin") == 0) {
                fprintf(stderr, "WARNING: TO MINIMIZE THE RISK OF FORK BOMBING THE ODIN SERVER,\nYOU MUST RUN THIS PROGRAM ON ONE OF THE VCF CLUSTER NODES!!!\n");
                exit(EXIT_FAILURE);
        } // if
} // check

int main()
{
	check();
	setbuf(stdout, NULL); // makes printf() unbuffered
	int n;
	char cmd[BUFFSIZE];

	// Project 3 TODO: set the current working directory to the user home directory upon initial launch of the shell
	// You may use getenv("HOME") to retrive the user home directory

	char* home_dir = getenv("HOME");

	// inifite loop that repeated prompts the user to enter a command
	while (1) {
		// Project 3 TODO: display the current working directory as part of the prompt

		//gets the current working directory and stores it in cmd
		getcwd(cmd, sizeof(cmd));

		//if checks if the current working directory contains the user's home directory
		if (strncmp(home_dir, cmd, strlen(home_dir)) == 0) {
			//prints only the home directory
			printf("1730sh:~%s$ ", cmd + strlen(home_dir));
		} //if
		//checks if the current working directory doesn't contain the user's home director
		else if (strncmp(home_dir, cmd, strlen(home_dir)) != 0) {
			printf("1730sh:%s$ ", cmd);
		}

		n = read(STDIN_FILENO, cmd, BUFFSIZE);


		// if user enters a non-empty command
		if (n > 1) {
			cmd[n-1] = '\0'; // replaces the final '\n' character with '\0' to make a proper C string


			// Lab 06 TODO: parse/tokenize cmd by space to prepare the
			// command line argument array that is required by execvp().
			// For example, if cmd is "head -n 1 file.txt", then the
			// command line argument array needs to be
			// ["head", "-n", "1", "file.txt", NULL].
			char* parse = strtok(cmd, " ");
			char* args[BUFFSIZE];
			int args_count = 0; //count the number of args

			while(parse != NULL) {
				args[args_count] = parse;
				args_count++;
				parse = strtok(NULL, " ");
			} //while

			args[args_count] = NULL;


            // -Lab 07 TODO: if the command contains input/output direction operators
			// such as "head -n 1 < input.txt > output.txt", then the command
			// line argument array required by execvp() needs to be
			// ["head", "-n", "1", NULL], while the "< input.txt > output.txt" portion
			// needs to be parsed properly to be used with dup2(2) inside the child process

			//in order to work it has to go inside of the child process. If the code is put here
			//it will be wrong and it will throw an infinite loop.

			// -Lab 06 TODO: if the command is "exit", quit the program
			if (strcmp(args[0], "exit") == 0) {
				exit(EXIT_SUCCESS);
			} //if


			// Project 3 TODO: else if the command is "cd", then use chdir(2) to
			// to support change directory functionalities

			else if(strcmp(args[0], "cd") == 0) {
                if (args[1] == NULL ||(args[1][0] == '~' && args[1][1] =='\0' && args[1][2] == '\0')) {

					if( home_dir != NULL) {
						if(chdir(home_dir) != 0) {
							perror("chdir");
						} // if
					} else {
						fprintf(stderr, "home environment was not set\n");
					}
				} else if(args[1][0] == '/') {
                    // if absolute path is given
                    if(chdir(args[1]) != 0) {
						perror("chdir");
					} // if
				} else if (args[1][0] == '~') {
                    // if ~ mydir is given switch to that directory from another directory.
					strcpy(cmd, getenv("HOME"));
                    strcat(cmd, args[1] + 1);
                    if (chdir(cmd) != 0) {
						perror("chdir");
					} //if
                } else if (strcmp(args[1], ". .") == 0) {
					// if the command to move back one directory
					if(chdir(". .") != 0) {
                        perror("chdir");
					} // if
				}   else {
					if (chdir(args[1]) != 0) {
						perror("chdir");
					} // if
				} // else
            } // else-if

			// -Lab 06 TODO: for all other commands, fork a child process and let
			// the child process execute user-specified command with its options/arguments.
			// NOTE: fork() only needs to be called once. DO NOT CALL fork() more than one time.
			else {
				pid_t pid;

				if ((pid = fork()) == -1) {
					perror("fork");
					exit(EXIT_FAILURE);
				} //if
				else if (pid == 0) { //inside child process

				    // -Lab 07 TODO: inside the child process, use dup2(2) to redirect
				    // standard input and output as specified by the user command

					for (int i = 0; i < args_count; i++) {
						if (strcmp(args[i] , "<") == 0 ) { //input
							//open file
							int fd = open(args[i+1], O_RDONLY);
							if (fd == - 1) {
								perror("open");
								exit(EXIT_FAILURE);
							}
							dup2(fd, STDIN_FILENO); //redirect to keyboard (to user)
							close(fd);
							args[i] = NULL;
							args[i + 1] = NULL;
							i++;
						}
						else if (strcmp(args[i] , ">") == 0 ) { //output
							int oflags = O_WRONLY | O_CREAT | O_TRUNC;
							//open file
							int fd = open(args[i+1], oflags , 0666);
							if (fd == -1) {
								perror("open");
								exit(EXIT_FAILURE);
							}
							dup2(fd, STDOUT_FILENO); //redirect to screen
							close(fd);
							args[i] = NULL;
							args[i + 1] = NULL;
							i++;
						}
						else if (strcmp(args[i] , ">>") == 0 ) { //append
							int oflags = O_WRONLY | O_CREAT | O_APPEND;
							//open file
							int fd = open(args[i+1], oflags, 0666);
							if (fd == -1) {
								perror("open");
								exit(EXIT_FAILURE);
							}
							dup2(fd, STDOUT_FILENO); //redirect to screen
							close(fd);
							args[i] = NULL;
							args[i + 1] = NULL;
							i++;
						} //if
					} // for

				    // -Lab 06 TODO: inside the child process, invoke execvp().
				    // if execvp() returns -1, be sure to use exit(EXIT_FAILURE);
				    // to terminate the child process
					if(execvp(args[0], args) == -1) { //execute commands
						perror("execvp");
						exit(EXIT_FAILURE);
					} //if
				} //else-if

				else { //inside parent process

				    //- Lab 06 TODO: inside the parent process, wait for the child process
				    // You are not required to do anything special with the child's
				    // termination status
					int status;
					wait(&status);

				} //else
			} //else
		} // if
	} // while

} // main
