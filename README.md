# uLipeRtosPico
Simple preemptive-cooperative, realtime, multitask kernel made just for fun.
uLipeRtosPico is a subset of uLipeRTOS, but is small and powerful real time kernel, currently implemented for arm cortex M and AVR 8 bits processors.
The kernel uses a fully preemptive-cooperative schedule policy, and supports up to 32 priority levels for maximum flexibility.

# Low memory footprint:
  - 2.0KB of code with full modules enabled*; 
  - 80B of RAM data used by the kernel;	
  - fully static allocation of kernel objects less than 40 bytes per one(user controls ram usage during compile time);

  *built on GCC_ARM 5.2 with -Os option
  
# Main Features

- Real time, all functions has predictable execution time;
- Fast and predictable execution time context switching;
- Tickless optional operation;
- Dynamic threads creation and destruction;
- Supports up to 32 priority levels (0 - 31);
- Preemtpive kernel policy;
- Cooperative kernel policy with same priority threads;
- Thread signals with set, clear, any and match capabilities;
- Counting semaphores;
- Binary semaphores;
- Mutual exclusion semaphores with priority ceilling protocol;
- Message queues;
- Soft timers with tickless feature (hardware timer provided by user);
- Constant time, low overhead dynamic memory allocator based on TLSF strategy, tuned to have a low overhead of 480bytes;
- Unlimited kernel objects (limited by processor memory);
- Port file formed by a single file in C simple to port;
- Glue header kernel, just put master file in your application and enjoy.

# Recommended processor resources

- 3KB of Code targeted memory(ROM);
- 1024B of Data memory (RAM);

# Basic Usage

- uLipeRtosPico was built to be simple in its essence;
- You can navigate and use samples provided in sample/ directory;
- Add the folders to the include paths: uLipeRtosPico;
- Add the only picokernel/**k_mk.c** file to source path;
- Build using your preferred IDE;
