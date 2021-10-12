# Syscall List

---
#### Sleep (id: ???) 
Arguments: **sleepInMs: uint64**

Causes the calling process to sleep for the specified amount of milliseconds.
  

---
#### HeapAlloc (id: ???)
Arguments: **requestedInc: int64**

Increase/decrease the size of the process heap.  


---
#### KernelLog (id: ???)
Arguments: **str: string**

Logs a message to the kernel debug log.  

---
#### ProcessGetPid (id: ???)
Arguments: **-**

Returns the PID of the process of the currently running thread. 

