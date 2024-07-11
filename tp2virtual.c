#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>


#define level2_table1_index(addr, offset) ((addr >> (offset + offset / 2)) & ((1 << (offset / 2)) - 1))
#define level2_table2_index(addr, offset) ((addr >> offset) & ((1 << (offset / 2)) - 1))


// struct for the page of page table
// -------------------------------------------- struct DENSE
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

// -------------------------------------------- struct INVERTED
typedef struct page_IPT{
    int addressInMemory;
    int addressVirtual;
    int valid; // if exists != -1
}page_IPT;


// -------------------------------------------- 
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
frame *memory;
int memorySize;
int currentMemorySize;

page *pageTable;
int pageTableSize;
list *indexList;

page_IPT *invertedPageTable;

page **level2PageTable;

// initialize variables for report
int pageFaults = 0; 
int diskAccess = 0;
int memoryAccess = 0;

// ------------------------------------------------- DENSE -------------------------------------------------
void denseFifo(int addr){
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = memorySize-1;
    pageTable[memory[0].addressInTable].valid = 0;
    
    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
    }
    memory[memorySize-1].addressInTable = addr;
    memory[memorySize-1].lastAccess = memoryAccess; 
}

void denseRandom(int addr){
    int randomNum = rand() % memorySize;

    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = randomNum;
    pageTable[memory[randomNum].addressInTable].valid = 0;

    memory[randomNum].addressInTable = addr;
    memory[randomNum].lastAccess = memoryAccess; 
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
    memory[memorySize-1].lastAccess = memoryAccess;
}  

void denseLru(int addr){
    int index = indexList->first->id;
    
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = index;
    pageTable[memory[index].addressInTable].valid = 0;

    memory[index].addressInTable = addr;
    memory[index].lastAccess = memoryAccess; 

    node *no = indexList->first;
    indexList->first = indexList->first->next;
    indexList->first->prev = NULL;
    no->next = NULL;
    no->prev = indexList->last;
    indexList->last->next = no;
    indexList->last = no;
}  

// ------------------------------------------------- INVERTED -------------------------------------------------
void invertedFifo(int addr){
    frame aux = memory[0];

    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
        invertedPageTable[memory[i+1].addressInTable].addressInMemory --;
    }

    invertedPageTable[aux.addressInTable].addressInMemory = memorySize-1;
    invertedPageTable[aux.addressInTable].addressVirtual = addr;
    invertedPageTable[aux.addressInTable].valid = 1;
    memory[memorySize-1].addressInTable = aux.addressInTable;
    memory[memorySize-1].lastAccess = memoryAccess; 
}

void invertedRandom(int addr){
    int randomNum = rand() % memorySize;

    invertedPageTable[memory[randomNum].addressInTable].addressVirtual = addr;
    memory[randomNum].lastAccess = memoryAccess;
}  

void invertedSecondChance(int addr){

    // Find the index of the page that has ref 0 (no second chance, rarely accessed)
    int index = 0;
    while (memory[index].ref == 1){
        memory[index].ref = 0;
        index = (index + 1) % memorySize;
    }

    frame aux = memory[index];

    for (int i = index; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
        invertedPageTable[memory[i+1].addressInTable].addressInMemory --;
    }

    invertedPageTable[aux.addressInTable].addressInMemory = memorySize-1;
    invertedPageTable[aux.addressInTable].addressVirtual = addr;
    invertedPageTable[aux.addressInTable].valid = 1;
    memory[memorySize-1].addressInTable = aux.addressInTable;
    memory[memorySize-1].lastAccess = memoryAccess;
}  

void invertedLru(int addr){

    int index = indexList->first->id;

    invertedPageTable[memory[index].addressInTable].addressVirtual = addr;
    memory[index].lastAccess = memoryAccess;

    node *no = indexList->first;
    indexList->first = indexList->first->next;
    indexList->first->prev = NULL;
    no->next = NULL;
    no->prev = indexList->last;
    indexList->last->next = no;
    indexList->last = no;
}  

int hashFunction(int table_addr){
    return table_addr % memorySize;
}

int searchInvertedPageTable(int table_addr){
    int index = hashFunction(table_addr);
    if(invertedPageTable[index].addressVirtual == table_addr)
        return index;
    
    int origin_index = index;
    index++;
    while (invertedPageTable[index].addressVirtual != table_addr){
        if(invertedPageTable[index].valid == 0 || index == origin_index)
             return -1;
        index = (index + 1) % memorySize;
    }

    return index;
}

int addInInvertedPageTable(int table_addr){
    int index = hashFunction(table_addr);
    if(!invertedPageTable[index].valid){
        invertedPageTable[index].valid = 1;
        invertedPageTable[index].addressInMemory = currentMemorySize;
        invertedPageTable[index].addressVirtual = table_addr;

        return index;
    }
    
    index++;
    while (invertedPageTable[index].valid){
        index = (index + 1) % memorySize;
    }

    invertedPageTable[index].valid = 1;
    invertedPageTable[index].addressInMemory = currentMemorySize;
    invertedPageTable[index].addressVirtual = table_addr;

    return index;
}

// ------------------------------------------------- LEVEL2 -------------------------------------------------

void level2Fifo(int addr, int offset){

    int firstLevelIndex = level2_table1_index(addr,offset); 
    int secondLevelIndex = level2_table2_index(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = memorySize-1;

    int old_addr = memory[0].addressInTable;
    int oldFirstLevelIndex = level2_table1_index(old_addr,offset); 
    int oldSecondLevelIndex = level2_table2_index(old_addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 0;
    
    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
    }
    memory[memorySize-1].addressInTable = addr;
    memory[memorySize-1].lastAccess = memoryAccess; 
}

void level2Random(int addr, int offset){
    int randomNum = rand() % memorySize;

    int firstLevelIndex = level2_table1_index(addr,offset); 
    int secondLevelIndex = level2_table2_index(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = randomNum;

    int old_addr = memory[randomNum].addressInTable;
    int oldFirstLevelIndex = level2_table1_index(old_addr,offset); 
    int oldSecondLevelIndex = level2_table2_index(old_addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 0;

    memory[randomNum].addressInTable = addr;
    memory[randomNum].lastAccess = memoryAccess; 
}  

void level2SecondChance(int addr, int offset){

    int firstLevelIndex = level2_table1_index(addr,offset); 
    int secondLevelIndex = level2_table2_index(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = memorySize-1;

    // Find the index of the page that has ref 0 (no second chance, rarely accessed)
    int index = 0;
    while (memory[index].ref == 1){
        memory[index].ref = 0;
        index = (index + 1) % memorySize;
    }

    int old_addr = memory[index].addressInTable;
    int oldFirstLevelIndex = level2_table1_index(old_addr,offset); 
    int oldSecondLevelIndex = level2_table2_index(old_addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 0;

    for (int i = index; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
    } 
    memory[memorySize-1].addressInTable = addr;
    memory[memorySize-1].lastAccess = memoryAccess;
}  

void level2Lru(int addr, int offset){
    int index = indexList->first->id;
    
    int firstLevelIndex = level2_table1_index(addr,offset); 
    int secondLevelIndex = level2_table2_index(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = index;

    int old_addr = memory[index].addressInTable;
    int oldFirstLevelIndex = level2_table1_index(old_addr,offset); 
    int oldSecondLevelIndex = level2_table2_index(old_addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 0;

    memory[index].addressInTable = addr;
    memory[index].lastAccess = memoryAccess; 

    node *no = indexList->first;
    indexList->first = indexList->first->next;
    indexList->first->prev = NULL;
    no->next = NULL;
    no->prev = indexList->last;
    indexList->last->next = no;
    indexList->last = no;
}  



// *************************************** AUX ********************************
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

void initPageTable(){
    pageTable = (page *)malloc(pageTableSize * sizeof(page));
    for(int i = 0; i < pageTableSize; i++){
        pageTable[i].addressInMemory = -1;
        pageTable[i].valid = 0;
    }
}

void initPageTableInverted(){
    invertedPageTable = (page_IPT *)malloc(memorySize * sizeof(page_IPT));
    for(int i = 0; i < memorySize; i++){
        invertedPageTable[i].addressInMemory = -1;
        invertedPageTable[i].addressVirtual = 0;
        invertedPageTable[i].valid = 0;
    }
}

void initFirstLevelPageTable() {
    level2PageTable = (page **)malloc(sqrt(pageTableSize) * sizeof(page *));
    for (int i = 0; i < sqrt(pageTableSize); i++) {
        level2PageTable[i] = NULL;
    }
}

void initSecondLevelPageTable(int index) {
    level2PageTable[index] = (page *)malloc(sqrt(pageTableSize) * sizeof(page));
    for (int i = 0; i < sqrt(pageTableSize); i++) {
        level2PageTable[index][i].addressInMemory = -1;
        level2PageTable[index][i].valid = 0;
    }
}

void initMemory(){
    memory = (frame *)malloc(memorySize * sizeof(frame));
    for(int i = 0; i < memorySize; i++){
        memory[i].addressInTable = -1;
        memory[i].changed = 0;
        memory[i].lastAccess = memoryAccess;
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


// *************************************** SIMULATE ********************************
void simulateVirtualMemory(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        

        // addr = [p][d]
        memoryAccess++;
        int tableAddr = addr >> offset;
        if(pageTable[tableAddr].valid == 1){
            memory[pageTable[tableAddr].addressInMemory].ref = 1;
            if(tolower(rw) == 'w'){
                memory[pageTable[tableAddr].addressInMemory].changed = 1;
                memory[pageTable[tableAddr].addressInMemory].lastAccess = memoryAccess;
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
                memory[currentMemorySize].lastAccess = memoryAccess;             

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

void simulateVirtualMemoryInverted(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){

        memoryAccess++;
        // addr = [p][d]
        int tableAddr = addr >> offset;
        int tablePosition = searchInvertedPageTable(tableAddr);
        if(tablePosition >= 0){
            
            memory[invertedPageTable[tablePosition].addressInMemory].ref = 1;
            if(tolower(rw) == 'w'){
                memory[invertedPageTable[tablePosition].addressInMemory].changed = 1;
                memory[invertedPageTable[tablePosition].addressInMemory].lastAccess = memoryAccess;
            }

            //LRU
            if(strcmp(alg, "lru") == 0)
                updateList(invertedPageTable[tablePosition].addressInMemory); 
        }
        else {
            pageFaults++;
            if(memorySize > currentMemorySize){
                tablePosition = addInInvertedPageTable(tableAddr);

                memory[currentMemorySize].addressInTable = tablePosition;
                memory[currentMemorySize].lastAccess = memoryAccess;              

                //LRU
                if(strcmp(alg, "lru") == 0)
                    addList();
                
                currentMemorySize++;  
            }
            else {
                diskAccess++;
                if(strcmp(alg, "fifo") == 0){
                    invertedFifo(tableAddr);
                }
                else if(strcmp(alg, "lru") == 0){
                    invertedLru(tableAddr);
                }
                else if(strcmp(alg, "secondChance") == 0){
                    invertedSecondChance(tableAddr);
                }
                else if(strcmp(alg, "random") == 0){
                    invertedRandom(tableAddr);
                }
            }
        }
    }
}


// *************************************** SIMULATE ********************************
void simulateVirtualMemory2Level(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        
        memoryAccess++;
        
        int firstLevelIndex = level2_table1_index(addr,offset); 
        int secondLevelIndex = level2_table2_index(addr,offset);

        if (level2PageTable[firstLevelIndex] == NULL) {
            initSecondLevelPageTable(firstLevelIndex);
        }

        if(level2PageTable[firstLevelIndex][secondLevelIndex].valid){
            memory[level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory].ref = 1;
            if(tolower(rw) == 'w'){
                memory[level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory].changed = 1;
                memory[level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory].lastAccess = memoryAccess;
            }

            //LRU
            if(strcmp(alg, "lru") == 0)
                updateList(level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory);
        }
        else {
            pageFaults++;
            if(memorySize > currentMemorySize){
                level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
                level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = currentMemorySize;

                memory[currentMemorySize].addressInTable = addr;
                memory[currentMemorySize].lastAccess = memoryAccess;             

                //LRU
                if(strcmp(alg, "lru") == 0)
                    addList();
                
                currentMemorySize++;  
            }
            else {
                diskAccess++;
                if(strcmp(alg, "fifo") == 0){
                    level2Fifo(addr, offset);
                }
                else if(strcmp(alg, "lru") == 0){
                    level2Lru(addr, offset);
                }
                else if(strcmp(alg, "secondChance") == 0){
                    level2SecondChance(addr, offset);
                }
                else if(strcmp(alg, "random") == 0){
                    level2Random(addr, offset);
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
        
    initMemory();
    if(strcmp(alg, "lru") == 0)
            initAuxList();

    if(strcmp(table_structure, "2level") == 0){
        initFirstLevelPageTable();
        simulateVirtualMemory2Level(file, s, alg);
    }
    else if(strcmp(table_structure, "3level") == 0){
        int i;
    }
    else if(strcmp(table_structure, "inverted") == 0){
        initPageTableInverted();
        simulateVirtualMemoryInverted(file, s, alg);
    }
    else if(strcmp(table_structure, "dense") == 0){ 
        initPageTable();
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