#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int	put_error(char *str)
{
	int	i;

	i = 0;
	while (str[i])
	{
		write(2, &str[i], 1);
		i++;
	}
	return (1);
}

static int	validate_cd(int argc)
{
	if (argc != 2)
		return (put_error("error: cd: bad arguments\n"));
	return (0);
}

static int	cd(char *path)
{
	if (chdir(path) == -1)
		return (put_error("error: cd: cannot change directory to "),
			put_error(path), put_error("\n"));
	return (0);
}

static int	cd(char **argv, int i)
{
	if (validate_cd(i))
		return (1);
	return (cd(argv[1]));
}

static int	is_builtin_command(char *cmd)
{
	return (!strcmp(cmd, "cd"));
}

static int	has_pipe(char **argv, int i)
{
	return (argv[i] && !strcmp(argv[i], "|"));
}

static int	create_pipe(int fd[2])
{
	if (pipe(fd) == -1)
		return (put_error("error: fatal\n"));
	return (0);
}

static int	setup_child_pipe(int fd[2])
{
	if (dup2(fd[1], 1) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
		return (put_error("error: fatal\n"));
	return (0);
}

static int	setup_parent_pipe(int fd[2])
{
	if (dup2(fd[0], 0) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
		return (put_error("error: fatal\n"));
	return (0);
}

static int	execute_command(char **argv, char **envp)
{
	if (is_builtin_command(*argv))
		return (put_error("error: cd: bad arguments\n"));
	execve(*argv, argv, envp);
	return (put_error("error: cannot execute"), put_error(*argv),
		put_error("\n"));
}

static int	handle_child_process(char **argv, int i, int fd[2], char **envp,
		int pipe_flag)
{
	argv[i] = 0;
	if (pipe_flag && setup_child_pipe(fd))
		return (1);
	return (execute_command(argv, envp));
}

static int	wait_for_child(int pid)
{
	int	status;

	waitpid(pid, &status, 0);
	return (WIFEXITED(status) && WEXITSTATUS(status));
}

static int	execute_simple_command(char **argv, int i, char **envp)
{
	int	pid;

	if (is_builtin_command(*argv))
		return (cd(argv, i));
	pid = fork();
	if (!pid)
		return (handle_child_process(argv, i, NULL, envp, 0));
	return (wait_for_child(pid));
}

static int	execute_piped_command(char **argv, int i, char **envp)
{
	int	fd[2];
	int	pid;

	if (create_pipe(fd))
		return (1);
	pid = fork();
	if (!pid)
		return (handle_child_process(argv, i, fd, envp, 1));
	if (wait_for_child(pid) || setup_parent_pipe(fd))
		return (1);
	return (0);
}

static int	exec(char **argv, int i, char **envp)
{
	if (has_pipe(argv, i))
		return (execute_piped_command(argv, i, envp));
	else
		return (execute_simple_command(argv, i, envp));
}

static int	find_command_end(char **argv)
{
	int	i;

	i = 0;
	while (argv[i] && strcmp(argv[i], "|") && strcmp(argv[i], ";"))
		i++;
	return (i);
}

static int	process_commands(char **argv, char **envp)
{
	int	i;
	int	status;

	status = 0;
	while (*argv)
	{
		i = find_command_end(argv);
		if (i > 0)
			status = exec(argv, i, envp);
		if (!argv[i])
			break ;
		argv += i + 1;
	}
	return (status);
}

int	main(int argc, char **argv, char **envp)
{
	if (argc > 1)
		return (process_commands(argv + 1, envp));
	return (0);
}
