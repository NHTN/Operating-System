#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

//child process runs concurently: & --> fork(), execvp(), wait() 
//history: !!
//redirection: >, < --> dup2()
//pipe: | --> pipe(), dup2()

#define MAX_LENGTH_COMMAND 100
#define MAX_LENGTH_ARGS 100

int inputCommand(char* input)
{
	int i = 0;
	char c;
	while ((c = getchar()) != '\n')
	{
		input[i] = c;
		i++;
	}
	return i;
}

int parseCommand(char* input, char* args[MAX_LENGTH_ARGS])
{
	int i = 0;
	args[i] = strtok(input, " ");
	while (args[i] != NULL)
		args[++i] = strtok(NULL, " ");
	args[i] = NULL; 
	i++;
	return i;
}

void printError(char* error)
{
	printf("osh: %s\n", error);
}

int main()
{
	char command[MAX_LENGTH_COMMAND];
	char command_copy[MAX_LENGTH_COMMAND];

	char history[MAX_LENGTH_COMMAND];
	memset(history, 0, sizeof(history));
	char history_copy[MAX_LENGTH_COMMAND];
	memset(history_copy, 0, sizeof(history_copy));

	char* args[MAX_LENGTH_ARGS];

	int history_count = 0;
	int should_run = 1;
	
	while (should_run)
	{
		printf("osh>");
		fflush(stdout);

		memset(command, 0, sizeof(command));
		memset(command_copy, 0, sizeof(command_copy));

		int length = inputCommand(command);

		strcpy(command_copy, command);
		int num_args = parseCommand(command_copy, args);

		if (length == 0) // no input
			continue;
		if (strcmp(command, "exit") == 0) // exit
		{
			should_run = 0;
			continue;
		}
		if (strcmp(command, "!!") == 0) // history: !!
		{
			if (history_count > 0)
			{
				// print in user's screen
				// copy in args
				printf("%s\n", history);
				strcpy(history_copy, history);
				num_args = parseCommand(history_copy, args);
			}
			else
			{
				printError("No commands in history");
				continue;
			}
		}
		else // save in history
		{
			if (history_count == 0)
			{
				strcpy(history, command);
				history_count = 1;
			}
			else if (history_count == 1)
			{
				memset(history, 0, sizeof(history));
				strcpy(history, command);
			}
		}

		// to this point, we have args[][]
		/* cout << "ARGS: ";
		for (int i = 0; i < num_args - 1; i++)
			cout << args[i] << " ";
		cout << endl;*/

		int concurrent = 0;
		if (strcmp(args[num_args - 2], "&") == 0) // concurently: &
		{
			concurrent = 1;
			num_args--;
			args[num_args - 1] = NULL;
		}

		int task = 0; // no redirection nor pipe
		int pos;
		for (pos = 0; pos < num_args - 1; pos++)
		{
			if (strcmp(args[pos], ">") == 0)
			{
				task = 1; // redirection >
				break;
			}
			if (strcmp(args[pos], "<") == 0)
			{
				task = 2; // redirection <
				break;
			}
			if (strcmp(args[pos], "|") == 0)
			{
				task = 3; // pipe |
				break;
			}
		}

		if (task == 0)
		{
			int status;
			pid_t pid = fork();
			if (pid < 0) // fail
			{
				printError("Error creating child process");
				return 1;
			}
			if (pid == 0) // in child process
			{
				status = execvp(args[0], args);
				if (status == -1)
					printError("Error executing the command");
				return 0;
			}
			else // child process id - in parent process
			{
				if (concurrent == 1)
					printf("Child process #%d runs concurrently", pid);
				else
					wait(&status);
			}
		} 
		else if (task == 1)
		{
			int status;
			pid_t pid = fork();
			if (pid < 0) // fail
			{
				printError("Error creating child process");
				return 1;
			}
			if (pid == 0) // in child process
			{
				int fd = open(args[pos + 1], O_CREAT | O_WRONLY | O_APPEND, 0777);
				if (fd < 0)
				{
					printError("Error opening file\n");
					continue;
				}
				dup2(fd, STDOUT_FILENO); 
				num_args = num_args - 2;
				args[num_args - 1] = NULL;

				status = execvp(args[0], args);
				if (status == -1)
					printError("Error executing the command");
				close(fd);
				return 0;
			}
			else // child process id - in parent process
			{
				if (concurrent == 1)
					printf("Child process #%d runs concurrently", pid);
				else
					wait(&status);
			}
		}
		else if (task == 2)
		{
			int status;
			pid_t pid = fork();
			if (pid < 0) // fail
			{
				printError("Error creating child process");
				return 1;
			}
			if (pid == 0) // in child process
			{
				int fd = open(args[pos + 1], O_RDONLY | O_APPEND);
				if (fd < 0)
				{
					printError("Error opening file\n");
					continue;
				}
				dup2(fd, STDIN_FILENO);

				num_args = num_args - 2;
				args[num_args - 1] = NULL;

				status = execvp(args[0], args);
				if (status == -1)
					printError("Error executing the command");
				close(fd);
				return 0;
			}
			else // child process id - in parent process
			{
				if (concurrent == 1)
					printf("Child process #%d runs concurrently", pid);
				else
					wait(&status);
			}
		}
 		else if (task == 3) // pipe |
		{			
			// seperate commands into 2 args[]
			char* args_1[MAX_LENGTH_ARGS];
			for (int i = 0; i < pos; i++)
				args_1[i] = args[i]; 
			args_1[pos] = NULL;

			char* args_2[MAX_LENGTH_ARGS];
			for (int i = 0; i < num_args - pos - 2; i++)
				args_2[i] = args[i + pos + 1]; 
			args_2[num_args - pos - 2] = NULL;

			int status;
			pid_t pid = fork();
			if (pid < 0) // fail
			{
				printError("Error creating child process");
				return 1;
			}
			if (pid == 0) // in child process
			{
				int fd[2];
				if (pipe(fd) < 0)
				{
					printError("Error creating pipe\n");
					continue;
				}

				int status2;
				pid_t pid2 = fork();
				if (pid2 < 0)
				{
					printError("Error creating child child process");
					return 1;
				}
				if (pid2 == 0) // in child child process
				{
					dup2(fd[0], STDIN_FILENO);   

					close(fd[0]);
					close(fd[1]);
					
					// char buffer[32];
        			// scanf("%s", buffer);   /* scanf now reads from pipe */
        			// printf("child said: %s\n", buffer);

					status2 = execvp(args_2[0], args_2);
					if (status2 == -1)
						printError("Error executing the command");

					return 0;
				}
				else // in parent child process
				{					
					dup2(fd[1], STDOUT_FILENO);
				
					close(fd[0]);
					close(fd[1]);

					//printf("hello from the other side\n");

					status = execvp(args_1[0], args_1);
					if (status == -1)
						printError("Error executing the command");

					wait(&status2); 
				}
				return 0;
			}
			else // child process id - in parent process
			{
				if (concurrent == 1)
					printf("Child process #%d runs concurrently", pid);
				else
					wait(&status);
			}
		}
    }
	return 0;
}