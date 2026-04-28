```mermaid
flowchart TD
    Start([Scheduler Invoked]) --> isQueueEmpty{Is Ready Queue\nEmpty?}
    isQueueEmpty -- Yes --> ReturnNull([Return NULL / Idle])
    isQueueEmpty -- No --> AlgoCheck{Which Algorithm?}
    
    AlgoCheck -- FCFS --> FCFS[Pop Front of FIFO Queue]
    AlgoCheck -- Round Robin --> RR[Pop Front of Queue\nRecord Start Tick]
    AlgoCheck -- Priority --> Prio[Sort Queue by Priority\nPop Highest Priority\n(Lowest Priority Number)]
    
    FCFS --> ReturnProc([Return PCB])
    RR --> ReturnProc
    Prio --> ReturnProc
```
