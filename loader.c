#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <signal.h>
#include <errno.h>
#include <ucontext.h>
#include <setjmp.h>

Elf32_Ehdr *EH;
Elf32_Phdr *PH;
int file_descriptor;
void *loaded_program_memory;
size_t total_page_faults = 0;
size_t total_page_allocations = 0;
size_t total_internal_fragmentation = 0;

// Jump buffer for resuming execution after segmentation fault
static sigjmp_buf jump_buffer;

// Signal handler for handling segmentation faults
void handle_segmentation_fault(int signum, siginfo_t *info, void *context) {
    void* fault_address = (void *)info->si_addr;

    // Find the corresponding segment based on fault_address
    Elf32_Phdr *faulting_segment = NULL;
    for (int i = 0; i < EH->e_phnum; i++) {
        if (fault_address >= (void*)PH[i].p_vaddr && fault_address < (void*)(PH[i].p_vaddr + PH[i].p_memsz)) {
            faulting_segment = &PH[i];
            break;
        }
    }

    if (faulting_segment == NULL) {
        // Handle the case where the segment is not found for the fault address
        fprintf(stderr, "Segment not found for fault address: %p\n", fault_address);
        exit(1);
    }

    // Allocate memory for the page and copy segment content
    size_t offset = (size_t)fault_address - faulting_segment->p_vaddr;
    size_t page_size = getpagesize();
    void* page_start = (void*)((size_t)fault_address & ~(page_size - 1));
    void* page_memory = mmap(page_start, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, file_descriptor, faulting_segment->p_offset);

    if (page_memory == MAP_FAILED) {
        perror("Failed to map page memory");
        exit(1);
    }

    total_internal_fragmentation += page_size - faulting_segment->p_memsz; // Calculate internal fragmentation
    total_page_allocations++;
    total_page_faults++;

    // Resume execution by returning to the main function
    siglongjmp(jump_buffer, 1);
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_sigaction = handle_segmentation_fault;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
}

void cleanup_resources() {
    if (file_descriptor != -1) {
        close(file_descriptor);
    }

    if (EH != NULL) {
        free(EH);
    }

    if (PH != NULL) {
        free(PH);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <executable>\n", argv[0]);
        exit(1);
    }

    setup_signal_handler();

    EH = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!EH) {
        perror("Failed to allocate memory for ELF header");
        exit(1);
    }

    file_descriptor = open(argv[1], O_RDONLY);
    if (file_descriptor < 0) {
        perror("Error opening ELF file");
        exit(1);
    }

    if (read(file_descriptor, EH, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("Failed to read ELF header");
        close(file_descriptor); // Close the file descriptor before exiting
        exit(1);
    }

    PH = (Elf32_Phdr *)malloc(EH->e_phentsize * EH->e_phnum);
    if (!PH) {
        perror("Failed to allocate memory for program headers");
        exit(1);
    }

    if (lseek(file_descriptor, EH->e_phoff, SEEK_SET) != EH->e_phoff) {
        perror("Failed to seek to program header table");
        exit(1);
    }

    if (read(file_descriptor, PH, EH->e_phentsize * EH->e_phnum) != (EH->e_phentsize * EH->e_phnum)) {
        perror("Failed to read program headers");
        exit(1);
    }

    size_t total_memory_size = 0;
    for (int i = 0; i < EH->e_phnum; i++) {
        total_memory_size += PH[i].p_memsz;
    }

    loaded_program_memory = malloc(total_memory_size);
    if (loaded_program_memory == NULL) {
        perror("Failed to allocate memory for the program");
        exit(1);
    }

    total_page_faults = 0;
    total_page_allocations = 0;
    total_internal_fragmentation = 0;

    uintptr_t entry_point = EH->e_entry;

    // Set up the jump buffer for resuming after a segmentation fault
    if (sigsetjmp(jump_buffer, 1) == 0) {
        // Call the entry point to start the execution of the loaded program
        typedef void (*EntryPointFunction)();
        EntryPointFunction entry_point_function = (EntryPointFunction)(entry_point);
        entry_point_function();
    }

    printf("Total page faults: %zu\n", total_page_faults);
    printf("Total page allocations: %zu\n", total_page_allocations);
    printf("Total internal fragmentation (KB): %zu\n", total_internal_fragmentation / 1024);

    cleanup_resources();
    close(file_descriptor);

    return 0;
}


