#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

// struct for the page of page table DENSE
// index = address
typedef struct page{
    int addressInMemory;
    int valid; // if exists != -1
}page;

// struct for frame
typedef struct frame{
    int addressInTable;
    int changed;
    int lastAccess;
    int ref; //SECOND CHANCE
}frame;

// aux structs
// lista duplamente encadeada
typedef struct node{
    struct node *next;
    struct node *prev;
    int id; //alter by need
} node;

typedef struct list{
    int size;
    struct node *first;
    struct node *last;
} list;

// initialize global
page *pageTable;
frame *memory;
int pageTableSize;
int memorySize;
int currentMemorySize;
list *indexList;

// initialize variables for report
int pageFaults = 0; 
int diskAccess = 0;
int memoryAccess = 0;

// function prototypes
int findS_offset(int PageSize);
void denseFifo(int addr);


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

void denseFifo(int addr){
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = memorySize-1;
    pageTable[memory[0].addressInTable].valid = 0;
    
    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
    }
    memory[memorySize-1].addressInTable = addr;
    memory[memorySize-1].lastAccess = 0; // TODO
}

void denseRandom(int addr){
    int randomNum = rand() % memorySize;

    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = randomNum;
    pageTable[memory[randomNum].addressInTable].valid = 0;

    memory[randomNum].addressInTable = addr;
    memory[randomNum].lastAccess = 0; // TODO
}  

void denseSecondChance(int addr){
    
    // Allocate the new page in the table
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = memorySize-1;

    // Find the index of the page that has ref 0 (no second chance, rarely accessed)
    int index = 0;
    while (memory[index].ref == 1){
        memory[index].ref = 0;
        index = (index + 1) % memorySize;
    }
    
    pageTable[memory[index].addressInTable].valid = 0;

    for (int i = index; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
    }
    
    memory[memorySize-1].addressInTable = addr;
    memory[memorySize-1].lastAccess = 0;
}  

void denseLru(int addr){
    int index = indexList->first->id;
    
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = index;
    pageTable[memory[index].addressInTable].valid = 0;

    memory[index].addressInTable = addr;
    memory[index].lastAccess = 0; // TODO

    node *no = indexList->first;
    indexList->first = indexList->first->next;
    indexList->first->prev = NULL;
    no->next = NULL;
    no->prev = indexList->last;
    indexList->last->next = no;
    indexList->last = no;
}  

void initPageTable(){
    pageTable = (page *)malloc(pageTableSize * sizeof(page));
    for(int i = 0; i < pageTableSize; i++){
        pageTable[i].addressInMemory = -1;
        pageTable[i].valid = 0;
    }
}

void initMemory(){
    memory = (frame *)malloc(memorySize * sizeof(frame));
    for(int i = 0; i < memorySize; i++){
        memory[i].addressInTable = -1;
        memory[i].changed = 0;
        memory[i].lastAccess = 0;
        memory[i].ref = 0; //SECOND CHANCE
    }
}

void initAuxList(){
    indexList = malloc(sizeof(list));
    indexList->size = 0;
    indexList->first = NULL;
    indexList->last = NULL;
}

void addList(){
    node *no = malloc(sizeof(node));
    no->id = currentMemorySize;
    no->prev = NULL;
    no->next = NULL;

    if(indexList->size == 0){
        indexList->first = no;
        indexList->last = no;
    }
    else if(indexList->size == 1){
        indexList->last = no;
        indexList->first->next = no;
        no->prev = indexList->first;
    }
    else{
        no->prev = indexList->last;
        indexList->last->next = no;
        indexList->last = no;
    }
    indexList->size++;
}

void updateList(int indexInMemory){
    node *aux = indexList->first;
    if(indexList->size == 1 || indexList->last->id == indexInMemory)
        return;

    if(indexList->first->id == indexInMemory){
        indexList->first = indexList->first->next;
        indexList->first->prev = NULL;
        aux->next = NULL;
        aux->prev = indexList->last;
        indexList->last->next =  aux;
        indexList->last = aux;
        return;
    }
        

    for(int i = 0; i < indexList->size; i++){
        if(aux->id == indexInMemory){
            aux->prev->next = aux->next;
            aux->next->prev = aux->prev;

            aux->next = NULL;
            aux->prev = indexList->last;
            indexList->last->next = aux;
            indexList->last = aux;
            return;
        }
        aux = aux->next;
    }
}

void simulateVirtualMemory(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;
    int changed = 0;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        

        // addr = [p][d]
        memoryAccess++;
        int tableAddr = addr >> offset;
        if(pageTable[tableAddr].valid == 1){
            memory[pageTable[tableAddr].addressInMemory].ref = 1;
            if(tolower(rw) == 'w'){
                memory[pageTable[tableAddr].addressInMemory].changed = 1;
                memory[pageTable[tableAddr].addressInMemory].lastAccess = 0; //TODO
            }

            //LRU
            if(strcmp(alg, "lru") == 0)
                updateList(pageTable[tableAddr].addressInMemory);
        }
        else {
            pageFaults++;
            if(memorySize > currentMemorySize){
                pageTable[tableAddr].valid = 1;
                pageTable[tableAddr].addressInMemory = currentMemorySize;

                memory[currentMemorySize].addressInTable = tableAddr;
                memory[currentMemorySize].lastAccess = 0; //TODO             

                //LRU
                if(strcmp(alg, "lru") == 0)
                    addList();
                
                currentMemorySize++;  
            }
            else {
                diskAccess++;
                if(strcmp(alg, "fifo") == 0){
                    denseFifo(tableAddr);
                }
                else if(strcmp(alg, "lru") == 0){
                    denseLru(tableAddr);
                }
                else if(strcmp(alg, "secondChance") == 0){
                    denseSecondChance(tableAddr);
                }
                else if(strcmp(alg, "random") == 0){
                    denseRandom(tableAddr);
                }
            }
        }
    }
}

int main(int argc, char* argv[]){
    //initialize the 4 arguments
    FILE *file;
    char *alg = argv[1]; //lru, second chance, fifo, random
    file = fopen(argv[2], "r"); 
    int pageSize = atoi(argv[3]); // 2 to 64; 2^?
    memorySize = atoi(argv[4]); //128 to 16384 

    int numPages = memorySize/pageSize;
    int s = findS_offset(pageSize);

    pageTableSize = pow(2, (32 - s));

    printf("\nExecutando o simulador...\n");
    printf("Arquivo de entrada: %s\n", argv[2]);
    printf("Tamanho da memoria: %iKB\n", memorySize);
    printf("Tamanho das paginas: %iKB\n", pageSize);
    printf("Tecnica de reposicao: %s\n", alg);

    // alocate tad to virtual memory
    // simulate virtual memory
    char *table_structure = argv[5];
    if(argv[5] == NULL)
        table_structure = "dense";
        
    if(strcmp(table_structure, "2level") == 0){
        int i;
    }
    else if(strcmp(table_structure, "3level") == 0){
        int i;
    }
    else if(strcmp(table_structure, "inverted") == 0){
        int i;
    }
    else if(strcmp(table_structure, "dense") == 0){ //dense
        initPageTable();
        initMemory();
        if(strcmp(alg, "lru") == 0)
            initAuxList();
        simulateVirtualMemory(file, s, alg);
    }

    //final report
    printf("Numero de acessos a memoria: %i\n", memoryAccess);
    printf("Paginas lidas(page faults): %i\n", pageFaults);
    printf("Paginas escritas: %i\n", diskAccess);
    
    free(pageTable);
    free(memory);
    if(strcmp(alg, "lru") == 0)
            free(indexList);
    fclose(file);

    return 0;
}