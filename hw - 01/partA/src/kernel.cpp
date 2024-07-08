#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <drivers/amd_am79c973.h>
#include <stdarg.h>
// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

void itoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;

    // Handle 0 explicitly
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Handle negative numbers for base 10
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // Append negative sign for base 10
    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0';  // Null-terminate the string

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void sprintf(char* str, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int i = 0;
    for (const char* p = format; *p != '\0'; p++) {
        if (*p == '%' && *(p + 1) == 'd') {
            int num = va_arg(args, int);
            char numStr[10];  // Buffer for number string
            itoa(num, numStr, 10);  // Convert integer to string
            for (char* np = numStr; *np != '\0'; np++) {
                str[i++] = *np;
            }
            p++;  // Skip 'd'
        } else {
            str[i++] = *p;
        }
    }
    str[i] = '\0';

    va_end(args);
}

void delay(int milliseconds)
{
    int count = milliseconds * 10000;  // Basit bir gecikme döngüsü
    while(count--) asm volatile("nop");
}


void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    static char* logBuffer = (char*)0x400000; // Bellekte 4MB'dan itibaren ayır.
    static uint32_t logIndex = 0;

    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
            case '\n':
                x = 0;
                y++;
                logBuffer[logIndex++] = '\n'; // Log buffer'a yaz
                break;
            default:
                VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
                logBuffer[logIndex++] = str[i]; // Log buffer'a yaz
                x++;
                break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}

void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}

void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}

void printInteger(int number)
{
    if (number == 0)
    {
        printf("0");
        return;
    }

    char buffer[12]; // Maximum length of an integer (including sign) + null terminator
    buffer[11] = '\0';
    int i = 10;
    bool isNegative = number < 0;
    if (isNegative)
        number = -number;

    while (number > 0 && i >= 0)
    {
        buffer[i--] = '0' + (number % 10);
        number /= 10;
    }

    if (isNegative)
        buffer[i--] = '-';

    printf(&buffer[i + 1]);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:

    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4
            | (VideoMemory[80 * y + x] & 0xF000) >> 4
            | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4
            | (VideoMemory[80 * y + x] & 0xF000) >> 4
            | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80) x = 79;
        if (x < 0) x = 0;
        y += yoffset;
        if (y >= 25) y = 24;
        if (y < 0) y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4
            | (VideoMemory[80 * y + x] & 0xF000) >> 4
            | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

void sysprintf(char* str)
{
    asm("int $0x80" : : "a"(4), "b"(str));
}

void collatz(int n)
{
    sysprintf("Collatz sequence for ");
    printInteger(n);
    sysprintf(": ");
    while (n != 1)
    {
        printInteger(n);
        sysprintf(", ");
        if (n % 2 == 0)
            n /= 2;
        else
            n = 3 * n + 1;
    }
    sysprintf("1\n");
}

void collatzTask()
{
    for (int i = 1; i < 4; i++)
    {

        sysprintf("Forking new task\n");
        int pid = syscall_fork();
        if (pid == 0)
        {
            sysprintf("Child task running\n");
            collatz(i);
            sysprintf("Child task exiting\n");
            syscall_exit();
        }
        else
        {
            sysprintf("Parent task waiting for child\n");
            syscall_waitpid(pid);
            sysprintf("Task finished\n");
        }
    }
    sysprintf("Collatz task exiting\n");
    syscall_exit();
}

void longRunningProgram() {
    int result = 0;
    int n = 1000; // Belirlenen n değeri
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            result += i * j;
        }
    }
    sysprintf("Long running program result: ");
    printInteger(result);
    sysprintf("\n");
}

void longRunningProgramTask() {
    longRunningProgram();
    syscall_exit();
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("cagriOS\n");

    GlobalDescriptorTable gdt;
    TaskManager taskManager;
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80, &taskManager);

    Task longRunningTask(&gdt, longRunningProgramTask);
    Task collatzTask2(&gdt, collatzTask);
    
        taskManager.AddTask(&longRunningTask);
    taskManager.AddTask(&collatzTask2);


    interrupts.Activate();

    printf("cagriOS22\n");

    while (1)
    {
        // Debugging: Print current task index
        char buffer[32];
        sprintf(buffer, "Current Task: %d\n", taskManager.getCurrentTask());
        printf(buffer);
        
#ifdef GRAPHICSMODE
        desktop.Draw(&vga);
#endif

        delay(1000);  // 1 saniyelik gecikme
    }
}
