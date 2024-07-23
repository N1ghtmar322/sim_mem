//323071043
#include "mem_sim.h"

int check_next_frame(sim_database* sim_database);
int clear_page_swap(sim_database* sim_database, int index);

int next_frame = 0;
int frames[MEMORY_SIZE/PAGE_SIZE];

void clear_system(sim_database* mem_sim) {
    close(mem_sim->program_fd);
    close(mem_sim->swapfile_fd);
    free(mem_sim);
}

sim_database* init_system(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_heap_stack_size) {
    if (swap_file_name == NULL || exe_file_name == NULL)
        return NULL;
    sim_database* memory_sim = malloc(sizeof(sim_database));
    if (memory_sim == NULL) {
        perror("ERR");
        return NULL;
    }
    for (int i = 0; i < NUM_OF_PAGES; ++i) {
        if (i < text_size/PAGE_SIZE)
            memory_sim->page_table[i].P = 1;
        else
            memory_sim->page_table[i].P = 0;
        memory_sim->page_table[i].D = 0;
        memory_sim->page_table[i].V = 0;
        memory_sim->page_table[i].frame_swap = -1;
    }
    memory_sim->text_size = text_size;
    memory_sim->data_size = data_size;
    memory_sim->bss_heap_stack_size = bss_heap_stack_size;
    memory_sim->program_fd = open(exe_file_name, O_RDONLY);
    if (memory_sim->program_fd == -1) {
        perror("ERR");
        free(memory_sim);
        return NULL;
    }
    memset(memory_sim->main_memory, '0', MEMORY_SIZE);
    memory_sim->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (memory_sim->swapfile_fd == -1) {
        close(memory_sim->program_fd);
        perror("ERR");
        free(memory_sim);
        return NULL;
    }
    char ini[SWAP_SIZE];
    memset(ini, '0', SWAP_SIZE);
    if (write(memory_sim->swapfile_fd, ini, SWAP_SIZE) == -1) {
        close(memory_sim->swapfile_fd);
        close(memory_sim->program_fd);
        perror("ERR");
        free(memory_sim);
        return NULL;
    }
    memset(frames, -1, (MEMORY_SIZE/PAGE_SIZE) * sizeof(int));
    return memory_sim;
}

char load(sim_database* mem_sim, int address) {
    if (address < 0 || address > 200) {
        perror("ERR");
        return '\0';
    }
    int page_number = address >> 3;
    int offset = address & (PAGE_SIZE - 1);
    page_descriptor* page_temp = &mem_sim->page_table[page_number];
    if (page_temp->V == 0) {
        char buffer[PAGE_SIZE];
        if (frames[next_frame] != -1 && mem_sim->page_table[frames[next_frame]].D == 1) {
            int nswap = check_next_frame(mem_sim);
            if (nswap == -1 || nswap == -2)
                return '\0';
            lseek(mem_sim->swapfile_fd, nswap,SEEK_SET);
            if (write(mem_sim->swapfile_fd, &mem_sim->main_memory[next_frame * PAGE_SIZE], PAGE_SIZE) == -1) {
                perror("ERR");
                return '\0';
            }
            mem_sim->page_table[frames[next_frame]].frame_swap = nswap / PAGE_SIZE;
            mem_sim->page_table[frames[next_frame]].V = 0;
        }
        else if (frames[next_frame] != -1 && mem_sim->page_table[frames[next_frame]].D == 0) {
            mem_sim->page_table[frames[next_frame]].frame_swap = -1;
            mem_sim->page_table[frames[next_frame]].V = 0;
        }
        if (page_temp->D == 1) {
            lseek(mem_sim->swapfile_fd, (page_temp->frame_swap * PAGE_SIZE), SEEK_SET);
            if (read(mem_sim->swapfile_fd, buffer, PAGE_SIZE) == -1) {
                perror("ERR");
                return '\0';
            }
            if (clear_page_swap(mem_sim, page_temp->frame_swap) == -1)
                return '\0';
        }
        else {
            if (address < (mem_sim->text_size + mem_sim->data_size)) {
                lseek(mem_sim->program_fd, page_number * PAGE_SIZE, SEEK_SET);
                if (read(mem_sim->program_fd, buffer, PAGE_SIZE) == -1) {
                    perror("ERR");
                    return '\0';
                }
            }
            else
                memset(buffer, '0', PAGE_SIZE);
        }
        memcpy(&mem_sim->main_memory[next_frame * PAGE_SIZE], buffer, PAGE_SIZE);
        page_temp->V = 1;
        page_temp->frame_swap = next_frame;
        frames[next_frame] = page_number;
        if (next_frame >= ((MEMORY_SIZE/PAGE_SIZE) - 1))
            next_frame = 0;
        else
            next_frame++;
    }
    int temp = page_temp->frame_swap << 3;
    temp = temp | offset;
    return mem_sim->main_memory[temp];
}

void store(sim_database* mem_sim, int address, char value) {
    if (address < 0 || address > 200) {
        perror("ERR");
        clear_system(mem_sim);
        exit(4);
    }
    if (address < mem_sim->text_size)
        return;
    int page_number = address >> 3;
    int offset = address & (PAGE_SIZE - 1);
    page_descriptor* page_temp = &mem_sim->page_table[page_number];
    if (page_temp->V == 0) {
        char buffer[PAGE_SIZE];
        if (frames[next_frame] != -1 && mem_sim->page_table[frames[next_frame]].D == 1) {
            int nswap = check_next_frame(mem_sim);
            if (nswap == -1 || nswap == -2) {
                clear_system(mem_sim);
                exit(4);
            }
            lseek(mem_sim->swapfile_fd, nswap,SEEK_SET);
            if (write(mem_sim->swapfile_fd, &mem_sim->main_memory[next_frame * PAGE_SIZE], PAGE_SIZE) == -1) {
                perror("ERR");
                clear_system(mem_sim);
                exit(4);
            }
            mem_sim->page_table[frames[next_frame]].frame_swap = nswap / PAGE_SIZE;
            mem_sim->page_table[frames[next_frame]].V = 0;
        }
        else if (frames[next_frame] != -1 && mem_sim->page_table[frames[next_frame]].D == 0) {
            mem_sim->page_table[frames[next_frame]].frame_swap = -1;
            mem_sim->page_table[frames[next_frame]].V = 0;
        }
        if (page_temp->D == 1) {
            lseek(mem_sim->swapfile_fd, (page_temp->frame_swap * PAGE_SIZE), SEEK_SET);
            if (read(mem_sim->swapfile_fd, buffer, PAGE_SIZE) == -1) {
                perror("ERR");
                clear_system(mem_sim);
                exit(4);
            }
            if (clear_page_swap(mem_sim, page_temp->frame_swap) == -1) {
                clear_system(mem_sim);
                exit(4);
            }
        }
        else {
            if (address < (mem_sim->text_size + mem_sim->data_size)) {
                lseek(mem_sim->program_fd, page_number * PAGE_SIZE, SEEK_SET);
                if (read(mem_sim->program_fd, buffer, PAGE_SIZE) == -1) {
                    perror("ERR");
                    clear_system(mem_sim);
                    exit(4);
                }
            }
            else
                memset(buffer, '0', PAGE_SIZE);
        }
        memcpy(&mem_sim->main_memory[next_frame * PAGE_SIZE], buffer, PAGE_SIZE);
        page_temp->V = 1;
        page_temp->frame_swap = next_frame;
        frames[next_frame] = page_number;
        if (next_frame >= 4)
            next_frame = 0;
        else
            next_frame++;
    }
    int temp = page_temp->frame_swap << 3;
    temp = temp | offset;
    page_temp->D = 1;
    mem_sim->main_memory[temp] = value;
}

void print_memory(sim_database* mem_sim) {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", mem_sim->main_memory[i]);
    }
}

void print_swap(sim_database* mem_sim) {
    char str[PAGE_SIZE];
    int i;
    printf("\n Swap memory\n");
    lseek(mem_sim->swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(mem_sim->swapfile_fd, str, PAGE_SIZE) == PAGE_SIZE) {
        for(i = 0; i < PAGE_SIZE; i++) {
            printf("[%c]\t", str[i]);
        }
        printf("\n");
    }
}

void print_page_table(sim_database* mem_sim) {
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \t Frame_swap\n");
    for(i = 0; i < NUM_OF_PAGES; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n", i, mem_sim->page_table[i].V,
        mem_sim->page_table[i].D,
        mem_sim->page_table[i].P, mem_sim->page_table[i].frame_swap);
    }
}

int check_next_frame(sim_database* sim_database) {
    if (sim_database->page_table[frames[next_frame]].D == 1) {
        char temp_buffer[PAGE_SIZE];
        for (int i = 0; i < (SWAP_SIZE / PAGE_SIZE); i++) {
            lseek(sim_database->swapfile_fd, i*PAGE_SIZE, SEEK_SET);
            if (read(sim_database->swapfile_fd, temp_buffer, PAGE_SIZE) == -1) {
                perror("ERR");
                return -2;
            }
            char temp[PAGE_SIZE];
            memset(temp, '0', PAGE_SIZE);
            if (strncmp(temp_buffer, temp, PAGE_SIZE) == 0)
                return i * PAGE_SIZE;
        }
        return -1;
    }
    return -1;
}

int clear_page_swap(sim_database* sim_database, int index) {
    if (lseek(sim_database->swapfile_fd, index * PAGE_SIZE, SEEK_SET) == -1) {
        perror("ERR");
        return -1;
    }
    char* buffer = "00000000";
    if (write(sim_database->swapfile_fd, buffer, PAGE_SIZE) == -1) {
        perror("ERR");
        return -1;
    }
    return 0;
}


