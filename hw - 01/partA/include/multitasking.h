#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{
    namespace hardwarecommunication {
        class InterruptHandler;
    }
    
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;
    } __attribute__((packed));

    enum TaskState { READY, WAITING, FINISHED };
    
    class Task
    {
        friend class TaskManager;
    private:
        common::uint8_t stack[4096]; // 4 KiB
        common::uint32_t pId = 0;
        common::uint32_t pPid = 0;
        TaskState taskState;
        common::uint32_t waitpid;
        CPUState* cpustate;
    public:
        Task(GlobalDescriptorTable *gdt, void (*entrypoint)());
        Task();
        common::uint32_t getId();
        ~Task();
    };
    
    class TaskManager
    {
        friend class hardwarecommunication::InterruptHandler;
    private:
        Task* tasks[256];
        int numTasks;
        int currentTask;
        GlobalDescriptorTable *gdt = nullptr;
        int getIndex(common::uint32_t pid);
    protected:
        void PrintProcessTable();
    public:
        TaskManager();
        ~TaskManager();
        void Yield();
        bool AddTask(Task* task);
        int getCurrentTask() { return currentTask; }  // Getter for currentTask
        CPUState* Schedule(CPUState* cpustate);
        common::uint32_t AddTask(void (*entrypoint)());
        common::uint32_t ExecTask(void* entrypoint);
        common::uint32_t GetPid();
        common::uint32_t ForkTask(CPUState* cpustate);
        bool ExitCurrentTask();
        bool WaitTask(common::uint32_t pid);
        void ExitTask();  // Add this line

    };
}

void operator delete(void* p, unsigned int size);

#endif