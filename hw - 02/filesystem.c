#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BLOCKS 4096
#define MAX_FILE_SYSTEM_SIZE (4 * 1024 * 1024) // 4MB
#define SUPER_BLOCK_SIZE 1024
#define DIRECTORY_ENTRIES 100

typedef struct {
    char filename[255];
    unsigned int size;
    unsigned char owner_permission; // bit 0: read, bit 1: write
    time_t last_modification;
    time_t creation_time;
    char password[255];
    unsigned short first_block;
} DirectoryEntry;

typedef struct {
    DirectoryEntry entries[DIRECTORY_ENTRIES];
    unsigned int entry_count;
} DirectoryTable;

typedef struct {
    unsigned short next_block; // 0xFFFF indicates end of file
} FATEntry;

typedef struct {
    char data[SUPER_BLOCK_SIZE];
} DataBlock;

typedef struct {
    float block_size; // Block size changed to float
    unsigned int total_blocks;
    unsigned int free_blocks;
} SuperBlock;

// File Allocation Table (FAT)
FATEntry FAT[MAX_BLOCKS];

// Data blocks
DataBlock data_blocks[MAX_BLOCKS];

// Directory Table
DirectoryTable directory_table;

// Super block
SuperBlock super_block;

void init_file_system(float block_size) { // block_size type changed to float
    // Check if block size is valid
    if (block_size != 0.5 && block_size != 1.0) {
        fprintf(stderr, "Invalid block size. Only 0.5KB and 1KB supported.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize super block
    super_block.block_size = block_size * 1024; // Convert KB to bytes
    super_block.total_blocks = MAX_FILE_SYSTEM_SIZE / super_block.block_size;
    super_block.free_blocks = super_block.total_blocks - 1; // 1 block for superblock

    // Initialize FAT and directory table
    for (int i = 0; i < super_block.total_blocks; i++) {
        FAT[i].next_block = 0xFFFF;
    }
    directory_table.entry_count = 0;

    printf("File system initialized with block size: %.1f KB\n", block_size);
}

unsigned short allocate_block() {
    for (int i = 0; i < super_block.total_blocks; i++) {
        if (FAT[i].next_block == 0xFFFF) {
            return i;
        }
    }
    return 0xFFFF; // No free block found
}

void create_file(const char *filename, const char *password) {
    if (directory_table.entry_count >= DIRECTORY_ENTRIES) {
        printf("Directory is full\n");
        return;
    }

    DirectoryEntry *entry = &directory_table.entries[directory_table.entry_count];
    strcpy(entry->filename, filename);
    entry->size = 0;
    entry->owner_permission = 0b11; // Read and write permissions
    entry->creation_time = time(NULL);
    entry->last_modification = entry->creation_time;
    strcpy(entry->password, password);
    entry->first_block = allocate_block();

    if (entry->first_block == 0xFFFF) {
        printf("No free blocks available\n");
        return;
    }

    directory_table.entry_count++;

    // Print file creation information
    printf("File created:\n");
    printf("Name: %s\n", entry->filename);
    printf("Size: %u bytes\n", entry->size);
    printf("Owner Permission: ");
    printf((entry->owner_permission & 0b01) ? "Read " : "");
    printf((entry->owner_permission & 0b10) ? "Write" : "");
    printf("\n");
    printf("Creation Time: %s", ctime(&entry->creation_time)); // Convert time to string
    printf("Last Modification Time: %s", ctime(&entry->last_modification)); // Convert time to string
    printf("First Block: %hu\n", entry->first_block);
    printf("Password: %s\n", entry->password);
}

void save_file_system(const char *file_name) {
    int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("Failed to create file system");
        exit(EXIT_FAILURE);
    }

    // Write super block
    write(fd, &super_block, sizeof(SuperBlock));

    // Write FAT
    write(fd, FAT, sizeof(FATEntry) * super_block.total_blocks);

    // Write Directory Table
    write(fd, &directory_table, sizeof(DirectoryTable));

    // Write Data Blocks
    for (int i = 0; i < super_block.total_blocks; i++) {
        write(fd, &data_blocks[i], sizeof(DataBlock));
    }

    close(fd);
    printf("File system saved: %s\n", file_name);
}

void load_file_system(const char *file_name) {
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        perror("Failed to load file system");
        exit(EXIT_FAILURE);
    }

    // Read super block
    read(fd, &super_block, sizeof(SuperBlock));

    // Read FAT
    read(fd, FAT, sizeof(FATEntry) * super_block.total_blocks);

    // Read Directory Table
    read(fd, &directory_table, sizeof(DirectoryTable));

    // Read Data Blocks
    for (int i = 0; i < super_block.total_blocks; i++) {
        read(fd, &data_blocks[i], sizeof(DataBlock));
    }

    close(fd);
    printf("File system loaded: %s\n", file_name);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <block_size> <file_system_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    float block_size = atof(argv[1]); // Convert command line argument to float
    const char *file_name = argv[2];

    init_file_system(block_size);
    create_file("example.txt", "password123");
    save_file_system(file_name);

    // Load file system to verify
    load_file_system(file_name);

    return 0;
}

