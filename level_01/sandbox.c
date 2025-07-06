#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Return codes for sandbox function */
#define SANDBOX_ERROR -1
#define SANDBOX_FUNCTION_FAILED 0
#define SANDBOX_FUNCTION_SUCCESS 1

/* Child process exit status */
#define CHILD_SUCCESS_EXIT 0

/* Signal handler that does nothing
	- used to handle SIGALRM without terminating */
static void	ignore_alarm_signal(int signal_number)
{
	(void)signal_number;
}

/* Setup signal handler for SIGALRM to prevent parent termination on timeout */
static int	setup_alarm_signal_handler(void)
{
	struct sigaction	signal_action;

	signal_action = (struct sigaction){0};
	signal_action.sa_handler = ignore_alarm_signal;
	sigemptyset(&signal_action.sa_mask);
	signal_action.sa_flags = 0;
	if (sigaction(SIGALRM, &signal_action, NULL) < 0)
		return (SANDBOX_ERROR);
	return (SANDBOX_FUNCTION_SUCCESS);
}

/* Handle timeout scenario by killing child and printing message */
static int	handle_timeout(pid_t child_pid, unsigned int timeout_seconds,
		bool verbose)
{
	kill(child_pid, SIGKILL);
	waitpid(child_pid, NULL, 0);
	if (verbose)
		printf("Bad function: timed out after %u seconds\n", timeout_seconds);
	return (SANDBOX_FUNCTION_FAILED);
}

/* Evaluate child process exit status and return appropriate result */
static int	evaluate_exit_status(int process_status, bool verbose)
{
	int	exit_code;

	exit_code = WEXITSTATUS(process_status);
	if (exit_code == CHILD_SUCCESS_EXIT)
	{
		if (verbose)
			printf("Nice function!\n");
		return (SANDBOX_FUNCTION_SUCCESS);
	}
	if (verbose)
		printf("Bad function: exited with code %d\n", exit_code);
	return (SANDBOX_FUNCTION_FAILED);
}

/* Evaluate child process signal termination and return appropriate result */
static int	evaluate_signal_status(int process_status,
		unsigned int timeout_seconds, bool verbose)
{
	int	termination_signal;

	termination_signal = WTERMSIG(process_status);
	if (termination_signal == SIGALRM)
	{
		if (verbose)
			printf("Bad function: timed out after %u seconds\n",
				timeout_seconds);
	}
	else
	{
		if (verbose)
			printf("Bad function: %s\n", strsignal(termination_signal));
	}
	return (SANDBOX_FUNCTION_FAILED);
}

/* Create child process to execute the sandboxed function */
static pid_t	create_sandboxed_child(void (*target_function)(void),
		unsigned int timeout_seconds)
{
	pid_t	child_pid;

	child_pid = fork();
	if (child_pid < 0)
		return (SANDBOX_ERROR);
	if (child_pid == 0)
	{
		/* Child process: set alarm and execute function */
		alarm(timeout_seconds);
		target_function();
		_exit(CHILD_SUCCESS_EXIT);
	}
	return (child_pid);
}

int	sandbox(void (*target_function)(void), unsigned int timeout_seconds,
		bool verbose)
{
	pid_t	child_pid;
	int		process_status;
	pid_t	wait_result;

	/* Create child process to run the target function */
	child_pid = create_sandboxed_child(target_function, timeout_seconds);
	if (child_pid == SANDBOX_ERROR)
		return (SANDBOX_ERROR);
	/* Setup signal handler to handle SIGALRM in parent process */
	if (setup_alarm_signal_handler() == SANDBOX_ERROR)
		return (SANDBOX_ERROR);
	/* Set parent alarm and wait for child completion */
	alarm(timeout_seconds);
	wait_result = waitpid(child_pid, &process_status, 0);
	/* Handle waitpid errors */
	if (wait_result < 0)
	{
		if (errno == EINTR)
			return (handle_timeout(child_pid, timeout_seconds, verbose));
		return (SANDBOX_ERROR);
	}
	/* Cancel alarm since child completed normally */
	alarm(0);
	/* Evaluate child process completion status */
	if (WIFEXITED(process_status))
		return (evaluate_exit_status(process_status, verbose));
	if (WIFSIGNALED(process_status))
		return (evaluate_signal_status(process_status, timeout_seconds,
				verbose));
	return (SANDBOX_ERROR);
}
