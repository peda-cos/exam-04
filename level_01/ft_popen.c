#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int	ft_popen(const char *file, const char *argv[], char type)
{
	int		pipefd[2];
	pid_t	pid;

	// Validate parameters
	if (!file || !argv || (type != 'r' && type != 'w'))
		return (-1);
	// Create pipe
	if (pipe(pipefd) == -1)
		return (-1);
	// Fork process
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (-1);
	}
	if (pid == 0) // Child process
	{
		if (type == 'r')
		{
			// Redirect stdout to pipe write end
			close(pipefd[0]); // Close read end
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]); // Close original write end
		}
		else // type == 'w'
		{
			// Redirect stdin to pipe read end
			close(pipefd[1]); // Close write end
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]); // Close original read end
		}
		// Execute the command
		execvp(file, (char *const *)argv);
		exit(1); // If execvp fails
	}
	else // Parent process
	{
		if (type == 'r')
		{
			// Return read end, close write end
			close(pipefd[1]);
			return (pipefd[0]);
		}
		else // type == 'w'
		{
			// Return write end, close read end
			close(pipefd[0]);
			return (pipefd[1]);
		}
	}
}
