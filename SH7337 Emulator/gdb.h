#pragma once

typedef enum {
	START,
	//Signal Hang Up
	SIGHUP,
	//CTRL + C
	SIGINT,
	//Quit signal
	SIGQUIT,
	//Illegal instruction
	SIGILL,
	//Breakpoint was hit
	SIGTRAP,
	//Abort
	SIGABRT,
	//Emulator trap
	SIGEMT,
	//Floating point or integer arithmetic exceptional condition
	SIGFPE,
	//Termininate imediately
	SIGKILL,
	//Bus error
	SIGBUS,
	//Segmentation fault
	SIGSEGV,
	//System call bad argument
	SIGSYS,
	//Pipe without process
	SIGPIPE,
	//Clock alarm
	SIGALRM,
	//Requesting termination of process
	SIGTERM,
	//User defined signal 1
	SIGUSR1,
	//User defined signal 2
	SIGUSR2,
	//The child has been terminated
	SIGCHLD,
	//Power failure
	SIGPWR,
	//Terminal size change signal
	SIGWINCH,
	//Urgent out-of-band data with the socket
	SIGURG,
	//File descriptor poll event
	SIGPOLL,
	//Stop the process
	SIGSTOP,
	//Terminal stop
	SIGTSTP,
	//Continue execution
	SIGCONT,
	//Background process wants to read
	SIGTTIN,
	//Background process wants to write
	SIGTTOU,
	//Virtual timer expire signal
	SIGVTALRM,
	//Time limit signal
	SIGPROF,
	//CPU time limit exceeded
	SIGXCPU,
	//File size limit exceeded
	SIGXFSZ,
	//Process. LWPs are blocked
	SIGWAITING,
	//Special signal used by thread library
	SIGLWP,
	//Special signal used by CPR
	SIGFREEZE,
	//Special signal used by CPR
	SIGTHAW,
	//Thread cancellation signal used by libthread
	SIGCANCEL
} posix_signal;

void gdb_start();
void gdb_loop();

void gdb_send_interrupt(posix_signal signal);