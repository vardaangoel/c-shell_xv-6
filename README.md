Vardaan Goel
Roll number: 2025117001

Two projects - cshell and xv6.

Repo structure:
    
    mini-project1/c-shell
    mini-project1/xv6
    mini-project1/AI-usage.pdf
    mini-project1/README.md

Requirements

For c-shell, a Linux system with gcc, make, and the standard POSIX libraries is needed.

For xv6, the RISC-V cross compiler and QEMU are needed. The important commands are:

	riscv64-unknown-elf-gcc
	riscv64-unknown-elf-ld
	riscv64-unknown-elf-objdump
	qemu-system-riscv64

Python 3 with pandas and matplotlib is needed only for generating the xv6 plot.

c-shell

How to build and run

From the root:

	cd c-shell
	make all
	./shell.out

To remove the compiled files:

	make clean

Project structure

	c-shell/include/   Header files
	c-shell/src/       Shell implementation files
	c-shell/Makefile   Build instructions

The shell is compiled as a normal Linux program. It uses the system's process, file, pipe, and terminal interfaces. The shell is expected to run on a POSIX-like system. The compiler treats warnings as errors, so the code must compile without warnings.

Assumptions

Each command word must fit in the buffer, which is 4096 bytes. 

Extremely large commands or pipelines in command line with more than 1000 tokens are not supported.

Quotes and backslash escaping must be properly closed. An unmatched quote or a trailing backslash is reported as invalid syntax.


xv6

How to build and run


	cd xv6

The default scheduler is round-robin(RR):

	make clean
	make qemu

To build and run the FCFS scheduler:

	make clean
	make SCHEDULER=FCFS qemu

To build and run the MLFQ scheduler:

	make clean
	make SCHEDULER=MLFQ qemu

Inside the xv6 shell, run the scheduler workload:

	schedulertest

For MLFQ, the workload prints queue events. Press Ctrl+P while it is running to print the process table and scheduler information. Exit QEMU with Ctrl+A followed by X.

To generate the MLFQ plot from the saved observations:

	python3 analysis/plot_scheduler.py analysis/events.csv analysis/mlfq-timeline.png

The plot is watermarked with vardaan.goel. The CSV contains tick, pid, and queue columns.

Project structure

	xv6/kernel/       xv6 kernel source and scheduler code
	xv6/user/         xv6 user programs, including schedulertest.c
	xv6/mkfs/         Filesystem image creation tool
	xv6/analysis/     Scheduler data, plotting code, and generated plot
	xv6/Makefile      Build configuration and scheduler selection
	xv6/report.md     Project report

Key xv6 changes

The Makefile selects one scheduler at compile time. If no scheduler is given, round-robin is used. FCFS and MLFQ can be selected with SCHEDULER=FCFS and SCHEDULER=MLFQ.

The process structure now stores queue information, slice usage, runnable ordering, arrival time, first-run time, CPU ticks, and runnable waiting ticks. New processes enter queue 0 for MLFQ.

The MLFQ scheduler has four queues. Their time slices are 1, 2, 8, and 16 ticks. A process that uses its complete slice moves to the next lower queue. A process that sleeps or voluntarily gives up the CPU keeps its current queue. Every 48 ticks, active processes are moved back to queue 0.


Assumptions

The comparison workload is the same for FCFS, RR, and MLFQ. Waiting time means time spent in the RUNNABLE state; time spent sleeping for I/O is not counted as ready-queue waiting time.

The xv6 build must be run from the xv6 directory because the QEMU command uses local paths for the kernel and fs.img. 