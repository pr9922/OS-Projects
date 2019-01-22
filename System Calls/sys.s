	#
	# user-side system calls
	#
	# System calls use a special convention:
        #     %eax  -  system call number
        #
        #

	# void exit(int status)
	.global exit
exit:
	mov $0,%eax
	int $48
	ret

	# ssize_t write(int fd, void* buf, size_t nbyte)
	.global write
write:
	mov $1,%eax
	int $48
	ret

	# int fork()
	.global fork
fork:
##############
# incomplete #
##############
	mov $2,%eax
	int $48
	ret

	# int sem(uint32_t init)
	.global sem
sem:
	mov $3,%eax
	int $48
	ret

	# int up(int s)
	.global up
up:
	mov $4,%eax
	int $48
	ret

	# int down(int s)
	.global down
down:
	mov $5,%eax
	int $48
	ret

	# int close(int id)
	.global close
close:
	mov $6,%eax
	int $48
	ret

	# int shutdown(void)
	.global shutdown
shutdown:
	mov $7,%eax
	int $48
	ret

	# int wait(int id, uint32_t *ptr)
	.global wait
wait:
	mov $8,%eax
	int $48
	ret

