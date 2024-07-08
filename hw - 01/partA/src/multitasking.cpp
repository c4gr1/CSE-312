#include <multitasking.h>
#include <memorymanagement.h>

using namespace myos;
using namespace myos::common;

extern void sysprintf(char* str);
void sprintf(char* buffer, const char* format, ...);

Task::Task(GlobalDescriptorTable *gdt, void (*entrypoint)())
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;
    
    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    cpustate->eflags = 0x202;
    cpustate->esp = (uint32_t)stack + 4096;
    cpustate->ss = gdt->DataSegmentSelector();
    taskState = READY;
}

Task::Task() {}

Task::~Task() {}

TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager() {}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    return true;
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;

    if(currentTask >= 0)
        tasks[currentTask]->cpustate = cpustate;

    currentTask++;
    if(currentTask >= numTasks)
        currentTask = 0;

    while (tasks[currentTask]->taskState == FINISHED)
    {
        // Remove finished task from the task list
        delete tasks[currentTask];
        for (int i = currentTask; i < numTasks - 1; i++)
        {
            tasks[i] = tasks[i + 1];
        }
        numTasks--;
        if (numTasks == 0)
        {
            return cpustate;
        }
        if (currentTask >= numTasks)
        {
            currentTask = 0;
        }
    }

    // Debugging: Print task information
    sysprintf("Switching to task ");
    char buffer[16];
    sprintf(buffer, "%d", currentTask);
    sysprintf(buffer);
    sysprintf("\n");

    return tasks[currentTask]->cpustate;
}

void TaskManager::Yield()
{
    asm("int $0x20"); // Trigger the timer interrupt manually
}

common::uint32_t TaskManager::ForkTask(CPUState* cpustate)
{
    if(numTasks >= 256)
    {
        sysprintf("Fork failed: too many tasks\n");
        return -1;
    }

    Task* parentTask = tasks[currentTask];
    Task* newTask = new Task();
    
    // Copy the CPU state without using memcpy
    newTask->cpustate->eax = parentTask->cpustate->eax;
    newTask->cpustate->ebx = parentTask->cpustate->ebx;
    newTask->cpustate->ecx = parentTask->cpustate->ecx;
    newTask->cpustate->edx = parentTask->cpustate->edx;
    newTask->cpustate->esi = parentTask->cpustate->esi;
    newTask->cpustate->edi = parentTask->cpustate->edi;
    newTask->cpustate->ebp = parentTask->cpustate->ebp;
    newTask->cpustate->eip = parentTask->cpustate->eip;
    newTask->cpustate->cs = parentTask->cpustate->cs;
    newTask->cpustate->eflags = parentTask->cpustate->eflags;
    newTask->cpustate->esp = parentTask->cpustate->esp;
    newTask->cpustate->ss = parentTask->cpustate->ss;

    newTask->cpustate->eax = 0; // Child process returns 0

    tasks[numTasks++] = newTask;
    sysprintf("Fork successful: child task created\n");
    return newTask->cpustate->eax;
}

common::uint32_t TaskManager::ExecTask(void* entrypoint)
{
    Task* task = tasks[currentTask];
    task->cpustate->eip = (uint32_t)entrypoint;
    return task->cpustate->eax;
}

bool TaskManager::WaitTask(common::uint32_t pid)
{
    int taskIndex = getIndex(pid);
    if (taskIndex == -1)
    {
        sysprintf("Wait failed: task not found\n");
        return false; // Task not found
    }

    sysprintf("Waiting for task to finish\n");
    tasks[currentTask]->taskState = WAITING;
    tasks[currentTask]->waitpid = pid;

    while (tasks[taskIndex]->taskState != FINISHED)
    {
        sysprintf("Task not finished yet, scheduling...\n");
        Schedule(tasks[currentTask]->cpustate); // Yield CPU until the target task finishes
    }

    sysprintf("Task finished\n");
    tasks[currentTask]->taskState = READY;
    return true;
}

int TaskManager::getIndex(common::uint32_t pid)
{
    for (int i = 0; i < numTasks; i++)
    {
        if (tasks[i]->pId == pid)
        {
            return i;
        }
    }
    return -1;
}

void TaskManager::ExitTask()
{
    sysprintf("Task exiting\n");
    tasks[currentTask]->taskState = FINISHED; // Mark the task as finished
    
    // Reschedule to the next task
    CPUState* cpustate = tasks[currentTask]->cpustate;
    currentTask++;
    if(currentTask >= numTasks)
        currentTask = 0;
    cpustate = tasks[currentTask]->cpustate;
    asm volatile("mov %0, %%esp" : : "g"(cpustate));
}

void operator delete(void* p, unsigned int size) {
    MemoryManager::activeMemoryManager->free(p);
}