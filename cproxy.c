#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <errno.h>

const int long_size = sizeof(unsigned long long);

union long_chars{
	unsigned long long val;
	char chars[8];
}long_chars;

void getdata(pid_t child, unsigned long long addr, char *str, int len){
	char *laddr;
    int i, j;
    union u {
            unsigned long long val;
            char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        data.val = ptrace(PTRACE_PEEKDATA,
                          child, addr + i * 8,
                          NULL);
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        data.val = ptrace(PTRACE_PEEKDATA,
                          child, addr + i * 8,
                          NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}

int main()
{   pid_t child;
	struct user_regs_struct regs;
	int status;
	int insyscall = 0;
	char *str;

    child = fork();
    if(child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl("/bin/ls", "ls", NULL);
    }
    else {
		while (1) {
			wait(&status);
			if (WIFEXITED(status)) break;
			ptrace(PTRACE_GETREGS, child, NULL, &regs);
	        //printf("The child made a system call %ld\n", orig_rax);
			if(regs.orig_rax == SYS_write) {
             if(insyscall == 0) {
                /* Syscall entry */
                insyscall = 1;
                printf("Write called with %ld, %ld, %ld\n", regs.rdi, regs.rsi, regs.rdx);
				long_chars.val = ptrace(PTRACE_PEEKDATA, child, regs.rsi, NULL);
				long_chars.chars[0] = 'h';
				ptrace(PTRACE_POKEDATA, child, regs.rsi, long_chars);
				printf("It tried printing %s\n", long_chars.chars);
                } else { /* Syscall exit */
                	printf("Write returned with %ld\n", regs.rax);
                    insyscall = 0;
                }
            }
            ptrace(PTRACE_SYSCALL,child, NULL, NULL);
		}
    }
    return 0;
}
