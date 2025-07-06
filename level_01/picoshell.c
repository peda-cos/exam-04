#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define STDIN_FD 0
#define STDOUT_FD 1

/* Helper function to safely close file descriptors */
static void	safe_close(int fd)
{
	if (fd > 0)
		close(fd);
}

/* Helper function to handle child process setup */
static void	setup_child_process(int input_fd, int output_fd)
{
	if (input_fd != STDIN_FD)
	{
		if (dup2(input_fd, STDIN_FD) == -1)
			exit(EXIT_FAILURE);
		close(input_fd);
	}
	if (output_fd != -1)
	{
		if (dup2(output_fd, STDOUT_FD) == -1)
			exit(EXIT_FAILURE);
		close(output_fd);
	}
}

/* Helper function to clean up resources on error */
static void	cleanup_on_error(int pipe_read, int pipe_write, int input_fd)
{
	safe_close(pipe_read);
	safe_close(pipe_write);
	safe_close(input_fd);
}

/* Check if any child process failed */
static int	has_process_failed(int status)
{
	return ((WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
		|| !WIFEXITED(status));
}

int	picoshell(char **commands[])
{
	int		cmd_index;
	int		pipe_fds[2];
	int		previous_input_fd;
	int		exit_status;
	pid_t	child_pid;
	int		wait_status;
	int		is_last_command;
	int		output_fd;

	cmd_index = 0;
	previous_input_fd = STDIN_FD;
	exit_status = EXIT_SUCCESS;
	/* Process each command in the pipeline */
	while (commands[cmd_index])
	{
		is_last_command = (commands[cmd_index + 1] == NULL);
		/* Create pipe for inter-process communication if not the last command */
		if (!is_last_command)
		{
			if (pipe(pipe_fds) == -1)
				return (EXIT_FAILURE);
		}
		child_pid = fork();
		if (child_pid < 0)
		{
			if (!is_last_command)
				cleanup_on_error(pipe_fds[0], pipe_fds[1], previous_input_fd);
			return (EXIT_FAILURE);
		}
		/* Child process: execute the command */
		if (child_pid == 0)
		{
			output_fd = is_last_command ? -1 : pipe_fds[1];
			setup_child_process(previous_input_fd, output_fd);
			/* Close unused pipe end in child */
			if (!is_last_command)
				close(pipe_fds[0]);
			execvp(commands[cmd_index][0], commands[cmd_index]);
			exit(EXIT_FAILURE);
		}
		/* Parent process: manage file descriptors and continue */
		safe_close(previous_input_fd);
		if (!is_last_command)
		{
			close(pipe_fds[1]);
			previous_input_fd = pipe_fds[0];
		}
		cmd_index++;
	}
	/* Wait for all child processes to complete */
	while (wait(&wait_status) > 0)
	{
		if (has_process_failed(wait_status))
			exit_status = EXIT_FAILURE;
	}
	return (exit_status);
}
