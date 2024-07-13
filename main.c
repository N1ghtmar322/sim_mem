#include "sim_mem.h"

int main(void) {
    sim_database* mem_sim = init_system ("exec_file", "swap_file" ,40,40,120);
    if (mem_sim == NULL)
        exit(1);
    char val;
    val = load (mem_sim , 44);
    if (val == '\0')
        clear_system(mem_sim);
    val = load (mem_sim , 46);
    if (val == '\0')
        clear_system(mem_sim);
    val = load (mem_sim , 2);
    if (val == '\0')
        clear_system(mem_sim);
    store(mem_sim , 50, 'X');
    val = load (mem_sim ,16);
    if (val == '\0')
        clear_system(mem_sim);
    store (mem_sim ,70, 'A');
    store(mem_sim ,55,'Y');
    store (mem_sim ,15,'Z');
    val = load (mem_sim ,23);
    if (val == '\0')
        clear_system(mem_sim);
    print_memory(mem_sim);
    print_swap(mem_sim);
    clear_system(mem_sim);
}
