#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int	write_error_message(char *error_message)
{
	while (*error_message)
		write(2, error_message++, 1);
	return (1);
}

int	execute_cd_command(char **command_args, int arg_count)
{
	if (arg_count != 2)
		return (write_error_message("error: cd: bad arguments\n"));
	if (chdir(command_args[1]) == -1)
		return (write_error_message("error: cd: cannot change directory to "),
			write_error_message(command_args[1]), write_error_message("\n"));
	return (0);
}

int	setup_pipe_output(int pipe_fds[2])
{
	if (dup2(pipe_fds[1], 1) == -1 || close(pipe_fds[0]) == -1
		|| close(pipe_fds[1]) == -1)
		return (write_error_message("error: fatal\n"));
	return (0);
}

int	setup_pipe_input(int pipe_fds[2])
{
	if (dup2(pipe_fds[0], 0) == -1 || close(pipe_fds[0]) == -1
		|| close(pipe_fds[1]) == -1)
		return (write_error_message("error: fatal\n"));
	return (0);
}

int	execute_external_command(char **command_args, char **environment_vars)
{
	execve(*command_args, command_args, environment_vars);
	return (write_error_message("error: cannot execute "),
		write_error_message(*command_args), write_error_message("\n"));
}

int	is_builtin_command(char *command)
{
	return (!strcmp(command, "cd"));
}

int	has_pipe_operator(char **command_args, int arg_count)
{
	return (command_args[arg_count] && !strcmp(command_args[arg_count], "|"));
}

int	execute_command_in_child(char **command_args, int arg_count,
		char **environment_vars, int pipe_fds[2], int has_pipe)
{
	command_args[arg_count] = 0;
	if (has_pipe && setup_pipe_output(pipe_fds))
		return (1);
	if (is_builtin_command(*command_args))
		return (execute_cd_command(command_args, arg_count));
	return (execute_external_command(command_args, environment_vars));
}

int	execute_single_command(char **command_args, int arg_count,
		char **environment_vars)
{
	int	pipe_fds[2];
	int	child_status;
	int	has_pipe;
	int	child_pid;

	has_pipe = has_pipe_operator(command_args, arg_count);
	if (!has_pipe && is_builtin_command(*command_args))
		return (execute_cd_command(command_args, arg_count));
	if (has_pipe && pipe(pipe_fds) == -1)
		return (write_error_message("error: fatal\n"));
	child_pid = fork();
	if (!child_pid)
		return (execute_command_in_child(command_args, arg_count,
				environment_vars, pipe_fds, has_pipe));
	waitpid(child_pid, &child_status, 0);
	if (has_pipe && setup_pipe_input(pipe_fds))
		return (1);
	return (WIFEXITED(child_status) && WEXITSTATUS(child_status));
}

int	find_command_end(char **command_tokens, int start_index)
{
	int	token_index;

	token_index = start_index;
	while (command_tokens[token_index] && strcmp(command_tokens[token_index],
			"|") && strcmp(command_tokens[token_index], ";"))
		token_index++;
	return (token_index - start_index);
}

int	main(int argc, char **argv, char **environment_vars)
{
	int	current_index;
	int	exit_status;
	int	command_length;

	current_index = 1;
	exit_status = 0;
	if (argc > 1)
	{
		while (current_index < argc)
		{
			command_length = find_command_end(argv, current_index);
			if (command_length > 0)
				exit_status = execute_single_command(&argv[current_index],
						command_length, environment_vars);
			current_index += command_length;
			if (current_index < argc && (!strcmp(argv[current_index], "|")
					|| !strcmp(argv[current_index], ";")))
				current_index++;
		}
	}
	return (exit_status);
}
