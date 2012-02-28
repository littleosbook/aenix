# Scheduling
How do you make multiple processes appear to run at the same time? Today, this
question has two answers:

- With the availability of multi-core processors, or on system with multiple
  processors, two processes actually can run at the same time by running two
  processes on different cores or processors
- Fake it. That is, switch rapidly (faster than a human can experience) between
  the processes. At any given moment, there is only process executing, but the
  rapid switching gives the impression that they are running "at the same
  time".

Since the operating system created in this book does not support multi-core
processors or multiple processors, our only option is to fake it. The part of
the operating system responsible for rapidly switching between the processes is
called the _scheduler_.

## Creating new processes
Creating new processes is usually done with two different system calls: `fork`
and `exec`. `fork` creates an exact copy of the currently running process,
while `exec` replaces the current process with another, often specified by a
path to the programs location in the file system. Of these two, we recommend
that you start implementing `exec`, since this system call will do almost
exactly the same steps as described in ["Setting up for user
mode"](#setting-up-for-user-mode).

## Cooperative scheduling with yielding
The easiest way to achieve the rapid switching between processes is if the
processes themselves are responsible for the switching. That is, the processes
runs for a while and then tells the OS (via a syscall) that it can now switch
to another process. Giving up the control of CPU to another process is called
_yielding_ and when the processes themselves are responsible for the
scheduling, it's called _cooperative scheduling_, since all the processes must
cooperate with each other.

In order to implement scheduling, you must keep a list of which processes are
running. The system call "yield" should then run the next process in the list.
New processes are loaded the same way as user mode was entered, via `iret`.
This was explained in the section ["Entering user mode"](#entering-user-mode).

We __strongly__ recommend that you start to implement support for multiple
processes by implementing cooperative scheduling. We further recommend that you
have a working solution for both `exec`, `fork` and `yield` before implementing
preemptive scheduling. The reason for this is because it is much easier to
debug cooperative scheduling, since it is deterministic.

## Preemptive scheduling with the timer
Instead of letting the processes themselves manage when it is time to change to
another process, the OS can switch processes automatically after a short period
of time. The OS can set up the programmable interval timer (PIT) to raise an
interrupt after a short period of time, for example 20 ms. In the interrupt
handler for the PIT interrupt, the OS will change the running processes to a
new one. This way, the processes themselves don't need to think about
scheduling.

### Programmable interval timer
TODO: write about how to implement a driver for the timer

### Separate kernel stacks for processes
When the yield system calls is used, a privilege level change will occur, since
the processes runs in user mode and the process switching will run in kernel
mode. If all processes uses the same kernel stack (the stack exposed by the
TSS), there will be trouble if a process in interrupted while still in kernel
mode.

The process that is being switched to will now use the same kernel stack, and
will then overwrite what the previous have written on the stack (remember that
TSS data structure points to the _beginning_ of the stack).

To solve this problem, every process should have it's own kernel stack, the
same way that each process have their own user mode stack. When switching
process, the TSS must then be updated to point to the new process kernel stack.

### Difficulties with preemptive scheduling
TODO: write about switching from kernel mode, user mode etc

## Further reading
