README for Building and Running the Operating System

Introduction

This README file provides instructions for compiling, building, and running the custom operating system using the provided Makefile. It also includes steps to clean up the build artifacts.

Prerequisites

Make sure you have the following packages installed on your system:

sudo apt-get install g++ binutils libc6-dev-i386
sudo apt-get install VirtualBox grub-legacy xorriso

Files and Directories

- src/: Directory containing the source code.
- include/: Directory containing the header files.
- obj/: Directory where the compiled object files will be stored.
- iso/: Temporary directory used for creating the bootable ISO image.
- linker.ld: Linker script used to link the object files into a binary.

Makefile Targets

1. Compile the Project

To compile the project and create the necessary object files and binary, run the following command:

make

2. Run the Operating System

To create a bootable ISO image and run the operating system in VirtualBox, use the following command:

make run

This command will:
- Compile the source files into object files.
- Link the object files into a binary (mykernel.bin).
- Create a bootable ISO image (mykernel.iso).
- Start VirtualBox and boot the operating system.

3. Install the Kernel

To install the kernel binary to /boot, run the following command:

make install

4. Clean the Build Artifacts

To clean up the compiled object files and binaries, run the following command:

make clean

This command will remove the obj/ directory, mykernel.bin, and mykernel.iso.

Makefile Details

Variables

- GCCPARAMS: Parameters for the GCC compiler.
- ASPARAMS: Parameters for the assembler.
- LDPARAMS: Parameters for the linker.
- objects: List of object files to be compiled and linked.

Targets

- run: Builds the ISO image and runs the OS in VirtualBox.
- obj/%.o: src/%.cpp: Compiles C++ source files into object files.
- obj/%.o: src/%.s: Assembles assembly source files into object files.
- mykernel.bin: Links the object files into a binary.
- mykernel.iso: Creates a bootable ISO image.
- install: Copies the kernel binary to /boot.
- clean: Removes the build artifacts.

Sample Commands

1. Compile the project:
   make

2. Run the operating system:
   make run

3. Install the kernel binary:
   make install

4. Clean the build artifacts:
   make clean

Conclusion

This README provides the necessary steps to build, run, and clean the custom operating system using the provided Makefile. Follow the instructions carefully to ensure a successful build and execution of the operating system.
