#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// struct for the pages
typedef struct page{
    unsigned id;
    struct page *next;
    struct page *prev;
    int changed; // ??????????????????????????????????
}page;

// struct for the page table
typedef struct page_table{
    page *first;
    page *last;
    int size;
}page_table;

// initialize global
page_table *table;
// initialize variables for report
int pageFaults = 0; 
int diskAccess = 0;
int memoryAccess = 0;

// function prototypes
int findS_offset(int PageSize);
void addPage(unsigned addr);
void updatePage(page* page_to_update);
page* checkPage(unsigned addr);
void updateAddress(unsigned addr, char* alg, int changed);
void LRU(unsigned addr);
void FIFO(unsigned addr, int changed);
void secondChance(unsigned addr);
void random(unsigned addr);

// find the number of bits for the offset for find page
int findS_offset(int PageSize){
    int s = 0;
    int tmp = PageSize * 1024;
    while(tmp > 1){
        tmp = tmp >> 1;
        s++;
    }
    return s;
}

void addPage(unsigned addr){
    page *new_page = malloc(sizeof(page));
    new_page->id = addr;
    new_page->prev = NULL;
    new_page->next = NULL;

    if(table->size == 0){
        table->first = new_page;
        table->last = new_page;
    }
    else if(table->size == 1){
        table->last = new_page;
        table->first->next = new_page;
        new_page->prev = table->first;
    }
    else{
        new_page->prev = table->last;
        table->last->next = new_page;
        table->last = new_page;
    }
    table->size++;
}

void updatePage(page* page_to_update){
    
}

page* checkPage(unsigned addr){
    page *aux = table->first;
    for(int i = 0; i < table->size; i++){
        if(aux->id == addr)
            return aux;
        aux = aux->next;
    }
    return NULL;
}


void updateAddress(unsigned addr, char* alg, int changed){
    if(strcmp(alg, "lru") == 0){
        LRU(addr);
    }
    else if(strcmp(alg, "fifo") == 0){
        FIFO(addr, changed);
    }
    else if(strcmp(alg, "2a") == 0){
        secondChance(addr);
    }
    else if(strcmp(alg, "random") == 0){
        random(addr);
    }
}

void LRU(unsigned addr){
    //TODO
}

void FIFO(unsigned addr, int changed){
    // 
    page *aux = table->first;
    // 
    table->first = table->first->next;
    table->first->prev = NULL;
    //  
    table->last->next = aux;
    aux->prev = table->last;
    aux->next = NULL;
    aux->id = addr;
    table->last = aux;

    if(changed)
        diskAccess++;
}

void secondChance(unsigned addr){
    //TODO
}

void random(unsigned addr){
    //TODO
}

int main(int argc, char* argv[]){
    //initialize the 4 arguments
    FILE *file;
    char *alg = argv[1]; //lru, second chance, fifo, random
    file = fopen(argv[2], "r"); 
    int pageSize = atoi(argv[3]); // 2 to 64; 2^?
    int memSize = atoi(argv[4]); //128 to 16384 

    int numPages = memSize/pageSize;
    int s = findS_offset(pageSize);

    // alocate tad to virtual memory
    table = malloc(sizeof(page_table));
    table->first = NULL;
    table->last = NULL;
    table->size = 0;

    //file read
    unsigned addr;
    char rw;

    int changed = 0;

    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        addr = addr >> s;
        memoryAccess++;

        if(tolower(rw) == 'w'){
            changed = 1;
            // check if page exist
            if(checkPage(addr) == NULL){
                // faults
                // add page
                
                pageFaults++;
                if(numPages > table->size)
                    addPage(addr);
                else
                    updateAddress(addr, alg, changed);
            }
            else{
                // just read
                continue;
            }
            
        }
        else if(tolower(rw) == 'r'){

            changed = 0;
            // check if page exist
            if(checkPage(addr) == NULL){
                // faults
                // add page
                pageFaults++;
                if(numPages > table->size)
                    addPage(addr);
                else
                    updateAddress(addr, alg, changed);
            }
            else{
                // just read
                continue;
            }
        }
    }


    //final report
    fclose(file);
    printf("\nExecutando o simulador...\n");
    printf("Arquivo de entrada: %s\n", argv[2]);
    printf("Tamanho da memoria: %iKB\n", memSize);
    printf("Tamanho das paginas: %iKB\n", pageSize);
    printf("Tecnica de reposicao: %s\n", alg);
    printf("Numero de acessos à memória: %i\n", memoryAccess);
    printf("Paginas lidas: %i\n", pageFaults);
    printf("Paginas escritas: %i\n", diskAccess);

    return 0;
}