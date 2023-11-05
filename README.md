# SmartLoader

SmartLoader is a program that loads and executes 32-bit ELF executables, handling segmentation faults and memory allocation dynamically. It works by lazily loading program segments when needed, similar to the behavior of the Linux operating system. This allows the program to execute without allocating memory for all segments upfront, thus conserving memory and resources.

## Features

- Dynamic memory allocation for program segments.
- Handling segmentation faults by allocating memory for specific segments.
- Tracking the total number of page faults, page allocations, and internal fragmentation.
- Printing the output of the loaded program after execution.

## Usage

To use SmartLoader, follow these steps:

1. Compile the program using `make`:

   ```bash
   make
Execute SmartLoader with the target ELF executable as the command-line argument:

bash
Copy code
./smartloader <target_executable>
Replace <target_executable> with the name of the ELF executable you want to run.

SmartLoader will execute the target executable, handling segmentation faults and dynamically allocating memory for program segments.

After execution, SmartLoader will display the following information:

Total page faults: The number of page faults encountered.
Total page allocations: The number of page allocations performed.
Total internal fragmentation (KB): The internal fragmentation in kilobytes.
Additionally, the program's output will be printed to the console.

Example
Let's say you want to execute an ELF executable named "fib":

bash
Copy code
./smartloader fib
SmartLoader will run the "fib" program, handle segmentation faults, and display the page fault statistics along with the program's output.

Compatibility
SmartLoader is designed for 32-bit ELF executables without dependencies on glibc APIs.
