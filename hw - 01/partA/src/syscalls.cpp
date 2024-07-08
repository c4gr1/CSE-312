#include <syscalls.h>
#include <multitasking.h>

using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber, TaskManager* taskManager)
: InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset()), taskManager(taskManager)
{
}

SyscallHandler::~SyscallHandler()
{
}

void printf(char*);

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;

    switch(cpu->eax)
    {
        case 4: // sys_write
            printf((char*)cpu->ebx);
            break;
        case 1: // sys_fork
            cpu->eax = taskManager->ForkTask(cpu);
            break;
        case 2: // sys_waitpid
            cpu->eax = taskManager->WaitTask(cpu->ebx);
            break;
        case 3: // sys_execve
            cpu->eax = taskManager->ExecTask((void*)cpu->ebx);
            break;
        case 5: // sys_exit
            taskManager->ExitTask();
            break;
        default:
            break;
    }

    return esp;
}

namespace myos {
    extern "C" int syscall_fork() {
        int pid;
        asm("int $0x80" : "=a"(pid) : "a"(1));
        return pid;
    }

    extern "C" void syscall_exit() {
        asm("int $0x80" : : "a"(5));
        //while (true);
    }

    extern "C" void syscall_waitpid(int pid) {
        asm("int $0x80" : : "a"(2), "b"(pid));
    }
}
