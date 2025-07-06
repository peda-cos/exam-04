#include <sys/wait.h>
#include <unistd.h>

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1
#define POPEN_READ 'r'
#define POPEN_WRITE 'w'

static int	setup_child_read_mode(int pipe_fds[2])
{
	close(pipe_fds[PIPE_READ_END]);
	if (dup2(pipe_fds[PIPE_WRITE_END], STDOUT_FILENO) == -1)
		return (-1);
	close(pipe_fds[PIPE_WRITE_END]);
	return (0);
}

static int	setup_child_write_mode(int pipe_fds[2])
{
	close(pipe_fds[PIPE_WRITE_END]);
	if (dup2(pipe_fds[PIPE_READ_END], STDIN_FILENO) == -1)
		return (-1);
	close(pipe_fds[PIPE_READ_END]);
	return (0);
}

static void	cleanup_pipe_on_error(int pipe_fds[2])
{
	close(pipe_fds[PIPE_READ_END]);
	close(pipe_fds[PIPE_WRITE_END]);
}

static int	handle_child_process(const char *executable, const char *argv[],
		char io_type, int pipe_fds[2])
{
	if (io_type == POPEN_READ)
	{
		if (setup_child_read_mode(pipe_fds) == -1)
			_exit(1);
	}
	else
	{
		if (setup_child_write_mode(pipe_fds) == -1)
			_exit(1);
	}
	execvp(executable, (char *const *)argv);
	_exit(1);
}

static int	handle_parent_process(char io_type, int pipe_fds[2])
{
	if (io_type == POPEN_READ)
	{
		close(pipe_fds[PIPE_WRITE_END]);
		return (pipe_fds[PIPE_READ_END]);
	}
	else
	{
		close(pipe_fds[PIPE_READ_END]);
		return (pipe_fds[PIPE_WRITE_END]);
	}
}

static int	is_valid_parameters(const char *executable, const char *argv[],
		char io_type)
{
	return (executable && argv && (io_type == POPEN_READ
			|| io_type == POPEN_WRITE));
}

int	ft_popen(const char *executable, const char *argv[], char io_type)
{
	int		pipe_fds[2];
	pid_t	child_pid;

	if (!is_valid_parameters(executable, argv, io_type))
		return (-1);
	if (pipe(pipe_fds) == -1)
		return (-1);
	child_pid = fork();
	if (child_pid == -1)
	{
		cleanup_pipe_on_error(pipe_fds);
		return (-1);
	}
	if (child_pid == 0)
		handle_child_process(executable, argv, io_type, pipe_fds);
	return (handle_parent_process(io_type, pipe_fds));
}
