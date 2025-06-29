#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int	put_error(char *str)
{
	while (*str)
		write(2, str++, 1);
	return (1);
}

static int	is_cd(char **argv)
{
	return (argv && !strcmp(argv[0], "cd"));
}

static int	valid_cd(int argc)
{
	return (argc == 2);
}

static int	cd(char **argv, int argc)
{
	if (!valid_cd(argc))
		return (put_error("error: cd: bad arguments\n"));
	if (chdir(argv[1]) == -1)
		return (put_error("error: cd: cannot change directory to "),
			put_error(argv[1]), put_error("\n"));
	return (0);
}
static int	pipe_output(int *fd)
{
	if (dup2(fd[1], 1) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
		return (put_error("error: fatal\n"));
	return (0);
}

static int	pipe_input(int *fd)
{
	if (dup2(fd[0], 0) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
		return (put_error("error: fatal\n"));
	return (0);
}

static int	exec_child(char **argv, int argc, int *fd, int is_piped,
		char **envp)
{
	argv[argc] = 0;
	if (is_piped && pipe_output(fd))
		return (1);
	if (is_cd(argv))
		return (cd(argv, argc));
	execve(argv[0], argv, envp);
	return (put_error("error: cannot execute "), put_error(argv[0]),
		put_error("\n"));
}

static int	exec(char **argv, int argc, char **envp)
{
	int	fd[2];
	int	status;
	int	is_piped;
	int	pid;

	is_piped = argv[argc] && !strcmp(argv[argc], "|");
	if (!is_piped && is_cd(argv))
		return (cd(argv, argc));
	if (is_piped && pipe(fd) == -1)
		return (put_error("error: fatal\n"));
	pid = fork();
	if (!pid)
		return (exec_child(argv, argc, fd, is_piped, envp));
	waitpid(pid, &status, 0);
	if (is_piped && pipe_input(fd))
		return (1);
	return (WIFEXITED(status) && WEXITSTATUS(status));
}
int	main(int argc, char **argv, char **envp)
{
	int	i;
	int	status;

	i = 0;
	status = 0;
	if (argc > 1)
	{
		while (argv[i] && argv[++i])
		{
			argv += i;
			i = 0;
			while (argv[i] && strcmp(argv[i], "|") && strcmp(argv[i], ";"))
				i++;
			if (i)
				status = exec(argv, i, envp);
		}
	}
	return (status);
}
