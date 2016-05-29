#include <unistd.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <error.h>
#include <stdlib.h>
#include <stdio.h>

/* here we define the time offset and the pathname of the real program */

#define RTIME 882391810
#define REALEXE "original.real"

void main(int argc, char* argv[]) {
  pid_t pid;		   /* pid of the real program */
  time_t dt;		   /* time interval to be subtracted */
  int stat;		   /* return value of the wait call */
  int now;		   /* current time (real!) */
  int eax,ebx;		   /* where to keep registers */
  char* targv[50];	   /* copy of the parameters passed to our program */
  int i;

/* prepare parameters for the real program. please note that also argv[0] is
copied, so the real name is preserved. */
  for(i=0;i<argc;i++) targv[i]=argv[i];
  targv[argc]=NULL;

  if ((pid=fork())) {
/* here is the controlling process */
/* calculate the time to be subtracted */
    dt=time(NULL)-RTIME;
/* wait for real program process to become ready and run it until next system
   call */
    waitpid(pid,&stat,0);
    if (WIFEXITED(stat)) exit(0);
    ptrace(PTRACE_SYSCALL,pid,1,0);
    while (1) {
/* wait until we got system call. note that we get here twice: before and
   after a system call except for fork. this is not a problem and so we ignore
   this, otherwise we would have a bad time handling forks (strace has
   problems either). */
      waitpid(pid,&stat,0);
/* test if program isn't finished. if it happens exit */
      if (WIFEXITED(stat)) exit(0);
      errno=0;
      eax=ptrace(PTRACE_PEEKUSER,pid,8*ORIG_RAX,0);
/* read eax and test for errors */
      if (eax == -1 && errno) {
	printf("error: ptrace(PTRACE_PEEKUSR, ... )\n");
      }
      else {
	if (eax==13) {
/* 13 is the syscall code for time. have a look at /usr/include/asm/unistd.h
   and keep in mind the calling conventions for the linux system calls */
	  eax=ptrace(PTRACE_PEEKUSER,pid,8*RAX,0);
/* the prototype for the syscall is time_t time(time_t *t), so ebx is the
   address of t */
	  ebx=ptrace(PTRACE_PEEKUSER,pid,8*RBX,0);
	  if (ebx) {
/* read the returned time, apply correction and change both the return value
   and the parameter value (if it isn't NULL) */
	    now=ptrace(PTRACE_PEEKDATA,pid,ebx,0);
	    ptrace(PTRACE_POKEUSER,pid,4*EAX,now-dt);
	    ptrace(PTRACE_POKEDATA,pid,ebx,now-dt);
	  }
	  else {
/* here we need to change only the return value */
	    now=eax;
	    ptrace(PTRACE_POKEUSER,pid,4*EAX,now-dt);
	  }
	}
      }
/* next syscall */
      ptrace(PTRACE_SYSCALL,pid,1,0);
    }
  } else {
/* waiting for the control process */
    ptrace(PTRACE_TRACEME, 0, 1, 0);
/* exec the real program */
    execv(REALEXE,targv);
  }
}
