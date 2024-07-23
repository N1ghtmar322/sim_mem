// JE
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include "mem_sim.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <unistd.h>  // for lseek
#include <fcntl.h>   // for open
#include <stdlib.h>  // for exit

void reset_system(struct sim_database *mem_sim) {
    // Clear the system
    clear_system(mem_sim);

    // Reinitialize the system
    mem_sim = init_system("exec_file", "swap_file", 40, 40, 120);

    printf("System reset completed.\n");
}

void run_tests(struct sim_database *mem_sim) {
    char val;
    char output[1024];
    int test_passed = 1;

    printf("\nRunning tests:\n");


    // Test 1: Check initial memory state
    printf("Test 1: Check initial memory state... ");
    test_passed = 1;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (mem_sim->main_memory[i] != '0') {
            test_passed = 0;
            break;
        }
    }
    for (int i = 0; i < NUM_OF_PAGES; i++) {
        if (mem_sim->page_table[i].V != 0 || mem_sim->page_table[i].D != 0 ||
            mem_sim->page_table[i].P != (i < 5 ? 1 : 0) || mem_sim->page_table[i].frame_swap != -1) {
            test_passed = 0;
            break;
        }
    }
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 2: Check initial swap state
    printf("Test 2: Check initial swap state... ");
    test_passed = 1;
    char swap_content[SWAP_SIZE];
    lseek(mem_sim->swapfile_fd, 0, SEEK_SET);
    read(mem_sim->swapfile_fd, swap_content, SWAP_SIZE);
    for (int i = 0; i < SWAP_SIZE; i++) {
        if (swap_content[i] != '0') {
            test_passed = 0;
            break;
        }
    }
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 3: Initialize system
    printf("Test 3: Initialize system... ");
    printf(mem_sim != NULL ? "PASSED\n" : "FAILED\n");

    // Test 4: Load from text segment
    printf("Test 4: Load from text segment... ");
    val = load(mem_sim, 2);
    test_passed = (val == 'a' && mem_sim->page_table[0].V == 1 && mem_sim->page_table[0].frame_swap == 0);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 5: Load from data segment
    printf("Test 5: Load from data segment... ");
    val = load(mem_sim, 44);
    test_passed = (val == 'f' && mem_sim->page_table[5].V == 1 && mem_sim->page_table[5].frame_swap == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 6: Store in writable page
    printf("Test 6: Store in writable page... ");
    store(mem_sim, 50, 'X');
    val = load(mem_sim, 50);
    test_passed = (val == 'X' && mem_sim->page_table[6].V == 1 && mem_sim->page_table[6].D == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 7: Attempt to write in read-only page
    printf("Test 7: Attempt to write in read-only page... ");
    store(mem_sim, 2, 'Z');
    val = load(mem_sim, 2);
    test_passed = (val == 'a');
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 8: Load from uninitialized page
    printf("Test 8: Load from uninitialized page... ");
    val = load(mem_sim, 80);
    test_passed = (val == '0' && mem_sim->page_table[10].V == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 9: Store and load from heap
    printf("Test 9: Store and load from heap... ");
    store(mem_sim, 70, 'A');
    val = load(mem_sim, 70);
    test_passed = (val == 'A' && mem_sim->page_table[8].V == 1 && mem_sim->page_table[8].D == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Test 10: Check page table after operations
    printf("Test 10: Check page table after operations... ");
    test_passed = (mem_sim->page_table[0].V == 1 && mem_sim->page_table[5].V == 1 && mem_sim->page_table[8].V == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    // Reset system between tests
    reset_system(mem_sim);

    // Test 11: Load from boundary of two pages
    printf("Test 11: Load from boundary of two pages... ");
    val = load(mem_sim, 7);
    test_passed = (val == 'a');
    val = load(mem_sim, 8);
    test_passed = test_passed && (val == 'b');
    test_passed = test_passed && (mem_sim->page_table[0].V == 1) && (mem_sim->page_table[1].V == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 12: Attempt to access out of bounds address
    printf("Test 12: Attempt to access out of bounds address... ");
    val = load(mem_sim, 1000);
    test_passed = (val == '\0');;
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 13: Load multiple pages
    printf("Test 13: Load multiple pages... ");
    load(mem_sim, 0);
    load(mem_sim, 8);
    load(mem_sim, 16);
    test_passed = (mem_sim->page_table[0].V == 1) && (mem_sim->page_table[1].V == 1) && (mem_sim->page_table[2].V == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 14: Check page replacement
    printf("Test 14: Check page replacement... ");
    for (int i = 0; i < 6; i++) {
        load(mem_sim, i * 8);
    }
    int swapped_out_page_found = 0;
    for (int i = 0; i < 6; i++) {
        if (mem_sim->page_table[i].V == 0) {
            swapped_out_page_found = 1;
            break;
        }
    }
    test_passed = swapped_out_page_found;
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 15: Store in multiple pages
    printf("Test 15: Store in multiple pages... ");
    store(mem_sim, 42, 'X');
    store(mem_sim, 50, 'Y');
    store(mem_sim, 58, 'Z');
    val = load(mem_sim, 42);
    test_passed = (val == 'X');
    val = load(mem_sim, 50);
    test_passed = test_passed && (val == 'Y');
    val = load(mem_sim, 58);
    test_passed = test_passed && (val == 'Z');
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 16: Check dirty bit after store
    printf("Test 16: Check dirty bit after store... ");
    store(mem_sim, 80, 'W');
    test_passed = (mem_sim->page_table[10].D == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 17: Check swap after page replacement
    printf("Test 17: Check swap after page replacement... ");
    for (int i = 5; i < 11; i++) {
        store(mem_sim, i * PAGE_SIZE, 'F' + i - 5);
    }
    lseek(mem_sim->swapfile_fd, 0, SEEK_SET);
    char swap_char;
    read(mem_sim->swapfile_fd, &swap_char, 1);
    test_passed = (swap_char == 'F');
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 18: Load from swapped out page
    printf("Test 18: Load from swapped out page... ");
    val = load(mem_sim, 0);
    test_passed = (val == 'a');
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 19: Check page table after complex operations
    printf("Test 19: Check page table after complex operations... ");
    load(mem_sim, 0);
    load(mem_sim, 44);
    load(mem_sim, 80);
    load(mem_sim, 124);
    store(mem_sim, 50, 'X');
    store(mem_sim, 70, 'A');
    store(mem_sim, 130, 'B');
    load(mem_sim, 160);
    int valid_count = 0;
    for (int i = 0; i < NUM_OF_PAGES; i++) {
        if (mem_sim->page_table[i].V == 1) {
            valid_count++;
        }
    }
    test_passed = (valid_count <= MEMORY_SIZE / PAGE_SIZE);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

// Test 20: Stress test with many operations
    printf("Test 20: Stress test with many operations... ");
    for (int i = 0; i < 50; i++) {
        if (i % 2 == 0) {
            load(mem_sim, i * 4);
        } else {
            store(mem_sim, i * 4, 'X');
        }
    }

// Check physical memory
    test_passed = 1;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (i % 8 == 4) {
            if (mem_sim->main_memory[i] != 'X') {
                test_passed = 0;
                break;
            }
        } else {
            if (mem_sim->main_memory[i] != '0') {
                test_passed = 0;
                break;
            }
        }
    }

// Check page table
    if (test_passed) {
        for (int i = 0; i < NUM_OF_PAGES; i++) {
            if (i < 5) {
                if (mem_sim->page_table[i].V != 0 || mem_sim->page_table[i].D != 0 ||
                    mem_sim->page_table[i].P != 1 || mem_sim->page_table[i].frame_swap != -1) {
                    test_passed = 0;
                    break;
                }
            } else if (i < 20) {
                if (mem_sim->page_table[i].V != 0 || mem_sim->page_table[i].D != 1 ||
                    mem_sim->page_table[i].P != 0 || mem_sim->page_table[i].frame_swap != i - 5) {
                    test_passed = 0;
                    break;
                }
            } else if (i < 25) {
                if (mem_sim->page_table[i].V != 1 || mem_sim->page_table[i].D != 1 ||
                    mem_sim->page_table[i].P != 0 || mem_sim->page_table[i].frame_swap != i - 20) {
                    test_passed = 0;
                    break;
                }
            } else {
                if (mem_sim->page_table[i].V != 0 || mem_sim->page_table[i].D != 0 ||
                    mem_sim->page_table[i].P != 0 || mem_sim->page_table[i].frame_swap != -1) {
                    test_passed = 0;
                    break;
                }
            }
        }
    }

// Check swap memory
    if (test_passed) {
        char swap_content[SWAP_SIZE];
        lseek(mem_sim->swapfile_fd, 0, SEEK_SET);
        read(mem_sim->swapfile_fd, swap_content, SWAP_SIZE);

        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 8; j++) {
                if (j == 4) {
                    if (swap_content[i*8 + j] != 'X') {
                        test_passed = 0;
                        break;
                    }
                } else if (i < 5) {
                    if (swap_content[i*8 + j] != 'f' + i) {
                        test_passed = 0;
                        break;
                    }
                } else {
                    if (swap_content[i*8 + j] != '0') {
                        test_passed = 0;
                        break;
                    }
                }
            }
            if (!test_passed) break;
        }

        for (int i = 15*8; i < SWAP_SIZE; i++) {
            if (swap_content[i] != '0') {
                test_passed = 0;
                break;
            }
        }
    }

    printf(test_passed ? "PASSED\n" : "FAILED\n");

    reset_system(mem_sim);

    // Test 21: Check store operations
    printf("Test 21: Check store operations... ");
    store(mem_sim, 65, 'A');
    store(mem_sim, 57, 'B');
    store(mem_sim, 49, 'C');
    store(mem_sim, 41, 'D');
    store(mem_sim, 78, 'E');
    store(mem_sim, 100, 'F');
    test_passed = (mem_sim->page_table[5].V == 1 && mem_sim->page_table[5].D == 1);
    test_passed = test_passed && (mem_sim->page_table[6].V == 1 && mem_sim->page_table[6].D == 1);
    test_passed = test_passed && (mem_sim->page_table[7].V == 1 && mem_sim->page_table[7].D == 1);
    test_passed = test_passed && (mem_sim->page_table[8].V == 0 && mem_sim->page_table[8].D == 1);
    test_passed = test_passed && (mem_sim->page_table[9].V == 1 && mem_sim->page_table[9].D == 1);
    test_passed = test_passed && (mem_sim->page_table[12].V == 1 && mem_sim->page_table[12].D == 1);
    printf(test_passed ? "PASSED\n" : "FAILED\n");

    printf("\nAll tests completed.\n");
}

int main(int argc, char *argv[]) {
    char val;
    struct sim_database *mem_sim;
    mem_sim = init_system ("exec_file", "swap_file" ,40,40,120);
    run_tests(mem_sim);
    clear_system(mem_sim);
    return 0;
}

// JE