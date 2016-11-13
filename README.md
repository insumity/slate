# scheduler

**Why we changed the pthread library?**

When MCTOP was introduced in the `wc` application of METIS, the `set_mempolicy` functionality was used.
We do not know of any way to call `set_mempolicy` for another process. To solve this problem, we change the pthread libary (nptl) of glibc and then run applications by linking them to our own library. By doing so, we can inject code in the `pthread_create` function that the application calls without its knowledge. To be exact, when an application calls `pthread_create(..., function, ...)`, we call `injected_function` that runs some extra code before the actual `function` gets executed. This extra code calls `set_mempolicy` and `set_schedaffinity` for the corresponding thread.

**How does the pthread library communicate with the scheduler?**
The idea is the following. Both the pthread and the scheduler issue read and writes to the same file (`/tmp/scheduler`) that is memory mapped by both. This files contain some slots (`communication_slot`) and their access is controlled by semaphores.
The protocol is the following:

1. The scheduler initializes its slots to `NONE` (meaning noone is using them)
2. When the pthread library creates a new thread, it uses an emtpy slot and changes is it to `PTHREADS`
3. The scheduler keeps on checking the slots: When it reads a `PTHREADS` slot, it changes its `core` and `node` values and changes the slot to `SCHEDULER`
4. The pthread library sees that its slots got changed to `SCHEDULER`, so it gets the data (`core` and `node`) and changes its affinity and memory policy. Afterwards it erases the slot by making it as a `NONE`
