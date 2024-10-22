#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>


#define LEVEL2_TABLE1_INDEX(addr, offset) (((addr) >> (offset + secondLevelBits)) & ((1 << firstLevelBits) - 1))
#define LEVEL2_TABLE2_INDEX(addr, offset) (((addr) >> (offset)) & ((1 << secondLevelBits) - 1))

#define LEVEL3_TABLE1_INDEX(addr, offset) (((addr) >> ((offset) + (thirdLevelBits) + (secondLevelBits))) & ((1 << (firstLevelBits)) - 1))
#define LEVEL3_TABLE2_INDEX(addr, offset) (((addr) >> ((offset) + (thirdLevelBits))) & ((1 << (secondLevelBits)) - 1))
#define LEVEL3_TABLE3_INDEX(addr, offset) (((addr) >> (offset)) & ((1 << (thirdLevelBits)) - 1))


// struct for the page of page table
// -------------------------------------------- struct DENSE
// index = address
typedef struct page{
    int addressInMemory;
    int valid; // if exists != -1
    int changed;
}page;

// struct for frame
typedef struct frame{
    int addressInTable;
    int changed; //TODO
    int lastAccess;
    int ref; //SECOND CHANCE
}frame;

// -------------------------------------------- struct INVERTED
typedef struct page_IPT{
    int addressInMemory;
    int addressVirtual;
    int valid; // if exists != -1
    int changed;
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
void rotateMemoryIndexInLast(int index);

frame *memory;
int memorySize;
int currentMemorySize;

page *pageTable;
int pageTableSize;
list *indexList;
list *circularList;

page_IPT *invertedPageTable;

page **level2PageTable;
int firstLevelBits;
int secondLevelBits;

page ***level3PageTable;
int thirdLevelBits;
int numBitsEndereco;

// initialize variables for report
int memoryCompleteSize;
int pageFaults = 0; 
int diskAccess = 0;
int memoryAccess = 0;
int memoryAccessRead = 0;
int memoryAccessWrite = 0;
int dirtyPages = 0;

// ------------------------------------------------- DENSE -------------------------------------------------
void denseFifo(int addr){
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = memorySize-1;
    
    if(pageTable[memory[0].addressInTable].changed){
        dirtyPages++;
        pageTable[memory[0].addressInTable].changed = 0;
    }
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

    if(pageTable[memory[randomNum].addressInTable].changed){
        dirtyPages++;
        pageTable[memory[randomNum].addressInTable].changed = 0;
    }
    pageTable[memory[randomNum].addressInTable].valid = 0;

    memory[randomNum].addressInTable = addr;
    memory[randomNum].lastAccess = memoryAccess; 
}  

void denseSecondChance(int addr){
    //buscar desde o indice de segunda chance até o ref
    node *index = circularList->first;
    while(memory[index->id].ref == 1){
        memory[index->id].ref = 0;
        index = index->next;
    }

    //ativar endereço passado
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = index->id;

    //desativar endereço que estava na posição
    if(pageTable[memory[index->id].addressInTable].changed){
        dirtyPages++;
        pageTable[memory[index->id].addressInTable].changed = 0;
    }
    pageTable[memory[index->id].addressInTable].valid = 0;
    
    //subsituir novo endereço na memória rearranjada
    memory[index->id].addressInTable = addr;
    memory[index->id].lastAccess = memoryAccess;

    //rotacionar lista circular
    circularList->first = index->next;
    circularList->first = index;
}  

void denseLru(int addr){
    int index = indexList->first->id;
    
    pageTable[addr].valid = 1;
    pageTable[addr].addressInMemory = index;

    if(pageTable[memory[index].addressInTable].changed){
        dirtyPages++;
        pageTable[memory[index].addressInTable].changed = 0;
    }
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
int invertedFifo(int addr){
    frame aux = memory[0];

    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
        invertedPageTable[memory[i+1].addressInTable].addressInMemory --;
    }

    invertedPageTable[aux.addressInTable].addressInMemory = memorySize-1;
    invertedPageTable[aux.addressInTable].addressVirtual = addr;
    if(invertedPageTable[aux.addressInTable].changed){
        dirtyPages++;
        invertedPageTable[aux.addressInTable].changed = 0;
    }
    invertedPageTable[aux.addressInTable].valid = 1;
    memory[memorySize-1].addressInTable = aux.addressInTable;
    memory[memorySize-1].lastAccess = memoryAccess; 

    return aux.addressInTable;
}

int invertedRandom(int addr){
    int randomNum = rand() % memorySize;

    invertedPageTable[memory[randomNum].addressInTable].addressVirtual = addr;
    if(invertedPageTable[memory[randomNum].addressInTable].changed){
        dirtyPages++;
        invertedPageTable[memory[randomNum].addressInTable].changed = 0;
    }
    memory[randomNum].lastAccess = memoryAccess;

    return memory[randomNum].addressInTable;
}  

int invertedSecondChance(int addr){
     //buscar desde o indice de segunda chance até o ref
    node *index = circularList->first;
    while(memory[index->id].ref == 1){
        memory[index->id].ref = 0;
        index = index->next;
    }

    //ativar endereço passado
    invertedPageTable[memory[index->id].addressInTable].addressInMemory = index->id;
    invertedPageTable[memory[index->id].addressInTable].addressVirtual = addr;

    //desativar endereço que estava na posição
    if(invertedPageTable[memory[index->id].addressInTable].changed){
        dirtyPages++;
        invertedPageTable[memory[index->id].addressInTable].changed = 0;
    }
    invertedPageTable[memory[index->id].addressInTable].valid = 1;
    
    //subsituir novo endereço na memória rearranjada
    memory[index->id].addressInTable = memory[index->id].addressInTable;
    memory[index->id].lastAccess = memoryAccess;

    //rotacionar lista circular
    circularList->first = index->next;
    circularList->first = index;

    return memory[index->id].addressInTable;
}  

int invertedLru(int addr){

    int index = indexList->first->id;

    invertedPageTable[memory[index].addressInTable].addressVirtual = addr;
    if(invertedPageTable[memory[index].addressInTable].changed){
        dirtyPages++;
        invertedPageTable[memory[index].addressInTable].changed = 0;
    }
    memory[index].lastAccess = memoryAccess;

    node *no = indexList->first;
    indexList->first = indexList->first->next;
    indexList->first->prev = NULL;
    no->next = NULL;
    no->prev = indexList->last;
    indexList->last->next = no;
    indexList->last = no;

    return memory[index].addressInTable;
}  

int searchInvertedPageTable(int table_addr){
    for(int i=0; i<memorySize; i++){
        if(invertedPageTable[i].addressVirtual == table_addr)
            return i;
    }

    return -1;
}


// ------------------------------------------------- LEVEL2 -------------------------------------------------

void level2Fifo(int addr, int offset){

    int firstLevelIndex = LEVEL2_TABLE1_INDEX(addr,offset); 
    int secondLevelIndex = LEVEL2_TABLE2_INDEX(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = memorySize-1;

    int old_addr = memory[0].addressInTable;
    int oldFirstLevelIndex = LEVEL2_TABLE1_INDEX(old_addr,offset); 
    int oldSecondLevelIndex = LEVEL2_TABLE2_INDEX(old_addr,offset);
    if(level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed){
        dirtyPages++;
        level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed = 0;
    }
    level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].valid = 0;
    
    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i+1];
    }
    memory[memorySize-1].addressInTable = addr;
    memory[memorySize-1].lastAccess = memoryAccess; 
}

void level2Random(int addr, int offset){
    int randomNum = rand() % memorySize;

    int firstLevelIndex = LEVEL2_TABLE1_INDEX(addr,offset); 
    int secondLevelIndex = LEVEL2_TABLE2_INDEX(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = randomNum;

    int old_addr = memory[randomNum].addressInTable;
    int oldFirstLevelIndex = LEVEL2_TABLE1_INDEX(old_addr,offset); 
    int oldSecondLevelIndex = LEVEL2_TABLE2_INDEX(old_addr,offset);
    if(level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed){
        dirtyPages++;
        level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed = 0;
    }
    level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].valid = 0;

    memory[randomNum].addressInTable = addr;
    memory[randomNum].lastAccess = memoryAccess; 
}  

void level2SecondChance(int addr, int offset){
    //buscar desde o indice de segunda chance até o ref
    node *index = circularList->first;
    while(memory[index->id].ref == 1){
        memory[index->id].ref = 0;
        index = index->next;
    }

    //ativar endereço passado
    int firstLevelIndex = LEVEL2_TABLE1_INDEX(addr,offset); 
    int secondLevelIndex = LEVEL2_TABLE2_INDEX(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = index->id;

    //desativar endereço que estava na posição
    int old_addr = memory[index->id].addressInTable;
    int oldFirstLevelIndex = LEVEL2_TABLE1_INDEX(old_addr,offset); 
    int oldSecondLevelIndex = LEVEL2_TABLE2_INDEX(old_addr,offset);
    if(level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed){
        dirtyPages++;
        level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed = 0;
    }
    level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].valid = 0;
    
    //subsituir novo endereço na memória rearranjada
    memory[index->id].addressInTable = addr;
    memory[index->id].lastAccess = memoryAccess;

    //rotacionar lista circular
    circularList->first = index->next;
    circularList->first = index;
}  

void level2Lru(int addr, int offset){
    int index = indexList->first->id;
    
    int firstLevelIndex = LEVEL2_TABLE1_INDEX(addr,offset); 
    int secondLevelIndex = LEVEL2_TABLE2_INDEX(addr,offset);
    level2PageTable[firstLevelIndex][secondLevelIndex].valid = 1;
    level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory = index;

    int old_addr = memory[index].addressInTable;
    int oldFirstLevelIndex = LEVEL2_TABLE1_INDEX(old_addr,offset); 
    int oldSecondLevelIndex = LEVEL2_TABLE2_INDEX(old_addr,offset);
    if(level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed){
        dirtyPages++;
        level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].changed = 0;
    }
    level2PageTable[oldFirstLevelIndex][oldSecondLevelIndex].valid = 0;

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

// ------------------------------------------------- LEVEL3 -------------------------------------------------

// Algoritmo FIFO para 3 níveis
void level3Fifo(int addr, int offset){
    int firstLevelIndex = LEVEL3_TABLE1_INDEX(addr, offset);
    int secondLevelIndex = LEVEL3_TABLE2_INDEX(addr, offset);
    int thirdLevelIndex = LEVEL3_TABLE3_INDEX(addr, offset);
    
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].valid = 1;
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory = memorySize - 1;

    int old_addr = memory[0].addressInTable;
    int oldFirstLevelIndex = LEVEL3_TABLE1_INDEX(old_addr, offset); 
    int oldSecondLevelIndex = LEVEL3_TABLE2_INDEX(old_addr, offset);
    int oldThirdLevelIndex = LEVEL3_TABLE3_INDEX(old_addr, offset);

    if(level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed){
        dirtyPages++;
        level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed = 0;
    }
    level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].valid = 0;

    for (int i = 0; i < memorySize - 1; i++){
        memory[i] = memory[i + 1];
    }
    
    memory[memorySize - 1].addressInTable = addr;
    memory[memorySize - 1].lastAccess = memoryAccess;
}

// Algoritmo Random para 3 níveis
void level3Random(int addr, int offset){
    int randomNum = rand() % memorySize;

    int firstLevelIndex = LEVEL3_TABLE1_INDEX(addr, offset);
    int secondLevelIndex = LEVEL3_TABLE2_INDEX(addr, offset);
    int thirdLevelIndex = LEVEL3_TABLE3_INDEX(addr, offset);
    
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].valid = 1;
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory = randomNum;

    int old_addr = memory[randomNum].addressInTable;
    int oldFirstLevelIndex = LEVEL3_TABLE1_INDEX(old_addr, offset); 
    int oldSecondLevelIndex = LEVEL3_TABLE2_INDEX(old_addr, offset);
    int oldThirdLevelIndex = LEVEL3_TABLE3_INDEX(old_addr, offset);

    if(level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed){
        dirtyPages++;
        level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed = 0;
    }
    level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].valid = 0;

    memory[randomNum].addressInTable = addr;
    memory[randomNum].lastAccess = memoryAccess;
}

// Algoritmo Second Chance para 3 níveis
void level3SecondChance(int addr, int offset){
    //buscar desde o indice de segunda chance até o ref
    node *index = circularList->first;
    while(memory[index->id].ref == 1){
        memory[index->id].ref = 0;
        index = index->next;
    }

    //ativar endereço passado
    int firstLevelIndex = LEVEL3_TABLE1_INDEX(addr, offset);
    int secondLevelIndex = LEVEL3_TABLE2_INDEX(addr, offset);
    int thirdLevelIndex = LEVEL3_TABLE3_INDEX(addr, offset);
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].valid = 1;
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory = index->id;

    //desativar endereço que estava na posição
    int old_addr = memory[index->id].addressInTable;
    int oldFirstLevelIndex = LEVEL3_TABLE1_INDEX(old_addr, offset); 
    int oldSecondLevelIndex = LEVEL3_TABLE2_INDEX(old_addr, offset);
    int oldThirdLevelIndex = LEVEL3_TABLE3_INDEX(old_addr, offset);
    if(level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed){
        dirtyPages++;
        level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed = 0;
    }
    level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].valid = 0;
    
    //subsituir novo endereço na memória rearranjada
    memory[index->id].addressInTable = addr;
    memory[index->id].lastAccess = memoryAccess;

    //rotacionar lista circular
    circularList->first = index->next;
    circularList->first = index;
}

// Algoritmo LRU para 3 níveis
void level3Lru(int addr, int offset){
    int index = indexList->first->id;

    int firstLevelIndex = LEVEL3_TABLE1_INDEX(addr, offset);
    int secondLevelIndex = LEVEL3_TABLE2_INDEX(addr, offset);
    int thirdLevelIndex = LEVEL3_TABLE3_INDEX(addr, offset);
    
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].valid = 1;
    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory = index;

    int old_addr = memory[index].addressInTable;
    int oldFirstLevelIndex = LEVEL3_TABLE1_INDEX(old_addr, offset); 
    int oldSecondLevelIndex = LEVEL3_TABLE2_INDEX(old_addr, offset);
    int oldThirdLevelIndex = LEVEL3_TABLE3_INDEX(old_addr, offset);
    
    if(level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed){
        dirtyPages++;
        level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].changed = 0;
    }
    level3PageTable[oldFirstLevelIndex][oldSecondLevelIndex][oldThirdLevelIndex].valid = 0;

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
        pageTable[i].changed = 0;
    }
}

void initPageTableInverted(){
    invertedPageTable = (page_IPT *)malloc(memorySize * sizeof(page_IPT));
    for(int i = 0; i < memorySize; i++){
        invertedPageTable[i].addressInMemory = -1;
        invertedPageTable[i].addressVirtual = -1;
        invertedPageTable[i].valid = 0;
        invertedPageTable[i].changed = 0;
    }
}

void initFirstLevelPageTable2Level() {
    // calculate the size of each level
    firstLevelBits = numBitsEndereco / 2;
    secondLevelBits = numBitsEndereco - firstLevelBits;
    int firstLevelSize = 1 << firstLevelBits;
    int secondLevelSize = 1 << secondLevelBits;

    level2PageTable = (page **)malloc(firstLevelSize * sizeof(page *));
    for (int i = 0; i < firstLevelSize; i++) {
        level2PageTable[i] = NULL;
    }
}

void initSecondLevelPageTable2Level(int index) {
    int secondLevelSize = 1 << secondLevelBits;
    level2PageTable[index] = (page *)malloc(secondLevelSize * sizeof(page));
    for (int i = 0; i < secondLevelSize; i++) {
        level2PageTable[index][i].addressInMemory = -1;
        level2PageTable[index][i].valid = 0;
        level2PageTable[index][i].changed = 0;
    }
}

void initFirstLevelPageTable3Level() {
    // calculate the size of each level
    firstLevelBits = numBitsEndereco / 3;
    secondLevelBits = numBitsEndereco / 3;
    thirdLevelBits = numBitsEndereco - firstLevelBits - secondLevelBits;
    int firstLevelSize = 1 << firstLevelBits;
    int secondLevelSize = 1 << secondLevelBits;
    int thirdLevelSize = 1 << thirdLevelBits;

    level3PageTable = (page ***)malloc(firstLevelSize * sizeof(page **));
    for (int i = 0; i < firstLevelSize; i++) {
        level3PageTable[i] = NULL;
    }
}

void initSecondLevelPageTable3Level(int firstLevelIndex) {
    int secondLevelSize = 1 << secondLevelBits;
    level3PageTable[firstLevelIndex] = (page **)malloc(secondLevelSize * sizeof(page *));
    for (int i = 0; i < secondLevelSize; i++) {
        level3PageTable[firstLevelIndex][i] = NULL;
    }
}

void initThirdLevelPageTable3Level(int firstLevelIndex, int secondLevelIndex) {
    int thirdLevelSize = 1 << thirdLevelBits;
    level3PageTable[firstLevelIndex][secondLevelIndex] = (page *)malloc(thirdLevelSize * sizeof(page));
    for (int i = 0; i < thirdLevelSize; i++) {
        level3PageTable[firstLevelIndex][secondLevelIndex][i].addressInMemory = -1;
        level3PageTable[firstLevelIndex][secondLevelIndex][i].valid = 0;
        level3PageTable[firstLevelIndex][secondLevelIndex][i].changed = 0;
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

void initAuxList(list **lista){
    *lista = malloc(sizeof(list));
    (*lista)->size = 0;
    (*lista)->first = NULL;
    (*lista)->last = NULL;
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

void addCircularList(){
    node *no = malloc(sizeof(node));
    no->id = currentMemorySize;
    no->prev = NULL;
    no->next = circularList->first; //ele vai ser o ultimo e tem que apontar pro primeiro

    if(circularList->size == 0){
        circularList->first = no;
        circularList->last = no;
    }
    else if(circularList->size == 1){
        circularList->last = no;
        circularList->first->next = no;
        circularList->first->prev = no;
        no->prev = circularList->first;
    }
    else{
        no->prev = circularList->last;
        circularList->last->next = no;
        circularList->last = no;
        circularList->first->prev = no;
    }
    circularList->size++;
}

// *************************************** SIMULATE ********************************
void simulateVirtualMemory(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        
        // addr = [p][d]
        memoryAccess++;
        if(tolower(rw) == 'w')
            memoryAccessWrite++;
        else
            memoryAccessRead++;

        int tableAddr = addr >> offset;
        if(pageTable[tableAddr].valid == 1){
            if(tolower(rw) == 'w'){
                pageTable[tableAddr].changed = 1; 
                memory[pageTable[tableAddr].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(pageTable[tableAddr].changed)
                    pageTable[tableAddr].changed = 1;
                else
                    pageTable[tableAddr].changed = 0;
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
                //SECONDCHANCE
                if(strcmp(alg, "secondChance") == 0)
                    addCircularList();
                
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

            if(tolower(rw) == 'w'){
                pageTable[tableAddr].changed = 1; 
                memory[pageTable[tableAddr].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(pageTable[tableAddr].changed)
                    pageTable[tableAddr].changed = 1;
                else
                    pageTable[tableAddr].changed = 0;
            }
        }
        memory[pageTable[tableAddr].addressInMemory].ref = 1;
    }
}

void simulateVirtualMemoryInverted(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){

        memoryAccess++;
        if(tolower(rw) == 'w')
            memoryAccessWrite++;
        else
            memoryAccessRead++;

        // addr = [p][d]
        int tableAddr = addr >> offset;
        int tablePosition = searchInvertedPageTable(tableAddr);

        if(tablePosition >= 0){

            if(tolower(rw) == 'w'){
                invertedPageTable[tablePosition].changed = 1; 
                memory[invertedPageTable[tablePosition].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(invertedPageTable[tablePosition].changed)
                    invertedPageTable[tablePosition].changed = 1;
                else
                    invertedPageTable[tablePosition].changed = 0;
            }

            //LRU
            if(strcmp(alg, "lru") == 0)
                updateList(invertedPageTable[tablePosition].addressInMemory); 
        }
        else {
            pageFaults++;
            if(memorySize > currentMemorySize){              

                invertedPageTable[currentMemorySize].addressVirtual = tableAddr;
                invertedPageTable[currentMemorySize].addressInMemory = currentMemorySize;

                memory[currentMemorySize].addressInTable = currentMemorySize;
                memory[currentMemorySize].lastAccess = memoryAccess;  

                //LRU
                if(strcmp(alg, "lru") == 0)
                    addList();

                //SECONDCHANCE
                if(strcmp(alg, "secondChance") == 0)
                    addCircularList();

                tablePosition = currentMemorySize;

                currentMemorySize++;  
            }
            else {
                diskAccess++;
                if(strcmp(alg, "fifo") == 0){
                    tablePosition = invertedFifo(tableAddr);
                }
                else if(strcmp(alg, "lru") == 0){
                    tablePosition = invertedLru(tableAddr);
                }
                else if(strcmp(alg, "secondChance") == 0){
                    tablePosition = invertedSecondChance(tableAddr);
                }
                else if(strcmp(alg, "random") == 0){
                    tablePosition = invertedRandom(tableAddr);
                }
            }
            
            if(tolower(rw) == 'w'){
                invertedPageTable[tablePosition].changed = 1; 
                memory[invertedPageTable[tablePosition].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(invertedPageTable[tablePosition].changed)
                    invertedPageTable[tablePosition].changed = 1;
                else
                    invertedPageTable[tablePosition].changed = 0;
            }
            
        }
        
        memory[invertedPageTable[tablePosition].addressInMemory].ref = 1;
    }
}

void simulateVirtualMemory2Level(FILE *file, int offset, char *alg){
    //file read
    unsigned addr;
    char rw;

    int firstLevelIndex;
    int secondLevelIndex;

    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        
        memoryAccess++;
        if(tolower(rw) == 'w')
            memoryAccessWrite++;
        else
            memoryAccessRead++;

        firstLevelIndex = LEVEL2_TABLE1_INDEX(addr,offset); 
        secondLevelIndex = LEVEL2_TABLE2_INDEX(addr,offset);

        if (level2PageTable[firstLevelIndex] == NULL) {
            initSecondLevelPageTable2Level(firstLevelIndex);
        }

        if(level2PageTable[firstLevelIndex][secondLevelIndex].valid){
            if(tolower(rw) == 'w'){
                level2PageTable[firstLevelIndex][secondLevelIndex].changed = 1; 
                memory[level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(level2PageTable[firstLevelIndex][secondLevelIndex].changed)
                    level2PageTable[firstLevelIndex][secondLevelIndex].changed = 1;
                else
                    level2PageTable[firstLevelIndex][secondLevelIndex].changed = 0;
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
                
                //SECONDCHANCE
                if(strcmp(alg, "secondChance") == 0)
                    addCircularList();

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

            if(tolower(rw) == 'w'){
                level2PageTable[firstLevelIndex][secondLevelIndex].changed = 1; 
                memory[level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(level2PageTable[firstLevelIndex][secondLevelIndex].changed)
                    level2PageTable[firstLevelIndex][secondLevelIndex].changed = 1;
                else
                    level2PageTable[firstLevelIndex][secondLevelIndex].changed = 0;
            }
        }
        memory[level2PageTable[firstLevelIndex][secondLevelIndex].addressInMemory].ref = 1;
    }
}

void simulateVirtualMemory3Level(FILE *file, int offset, char *alg) {
    unsigned addr;
    char rw;

    while (fscanf(file, "%x %c", &addr, &rw) != -1) {
        
        memoryAccess++;
        if(tolower(rw) == 'w')
            memoryAccessWrite++;
        else
            memoryAccessRead++;

        int firstLevelIndex = LEVEL3_TABLE1_INDEX(addr, offset);
        int secondLevelIndex = LEVEL3_TABLE2_INDEX(addr, offset);
        int thirdLevelIndex = LEVEL3_TABLE3_INDEX(addr, offset);

        if (level3PageTable[firstLevelIndex] == NULL) {
            initSecondLevelPageTable3Level(firstLevelIndex);
        }

        if (level3PageTable[firstLevelIndex][secondLevelIndex] == NULL) {
            initThirdLevelPageTable3Level(firstLevelIndex, secondLevelIndex);
        }

        if (level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].valid) {
            if(tolower(rw) == 'w'){
                level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed = 1; 
                memory[level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed)
                    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed = 1;
                else
                    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed = 0;
            }

            if (strcmp(alg, "lru") == 0) {
                updateList(level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory);
            }
        } else {
            pageFaults++;
            if (memorySize > currentMemorySize) {
                level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].valid = 1;
                level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory = currentMemorySize;

                memory[currentMemorySize].addressInTable = addr;
                memory[currentMemorySize].lastAccess = memoryAccess;

                //LRU
                if (strcmp(alg, "lru") == 0) 
                    addList();
                
                //SECONDCHANCE
                if(strcmp(alg, "secondChance") == 0)
                    addCircularList();

                currentMemorySize++;
            } else {
                diskAccess++;
                if (strcmp(alg, "fifo") == 0) {
                    level3Fifo(addr, offset);
                } else if (strcmp(alg, "lru") == 0) {
                    level3Lru(addr, offset);
                } else if (strcmp(alg, "secondChance") == 0) {
                    level3SecondChance(addr, offset);
                } else if (strcmp(alg, "random") == 0) {
                    level3Random(addr, offset);
                }
            }

            if(tolower(rw) == 'w'){
                level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed = 1; 
                memory[level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory].lastAccess = memoryAccess;
            }
            else{
                if(level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed)
                    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed = 1;
                else
                    level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].changed = 0;
            }
        }
        memory[level3PageTable[firstLevelIndex][secondLevelIndex][thirdLevelIndex].addressInMemory].ref = 1;
    }
}


int main(int argc, char* argv[]){
    printf("\nParametros:\n argv[1]: algoritmo(fifo, random, lru, secondChance) \n argv[2]: file_path\n argv[3]: page_size\n argv[4]: memory_size\n argv[5](opcional): table_structure(dense*, 2level, 3level, inverted)\n\n");
    //initialize the 4 arguments
    FILE *file;
    char *alg = argv[1]; //lru, second chance, fifo, random
    file = fopen(argv[2], "r"); 
    int pageSize = atoi(argv[3]); // 2 to 64; 2^?
    memoryCompleteSize = atoi(argv[4]); //128 to 16384 

    //memory size is the the number of pages in the memory
    memorySize = memoryCompleteSize/pageSize;
    int s = findS_offset(pageSize);

    numBitsEndereco = 32 - s;
    pageTableSize = pow(2, (numBitsEndereco));
    
    //verify alg
    if(strcmp(alg, "fifo") != 0 && strcmp(alg, "random") != 0 && strcmp(alg, "lru") != 0 && strcmp(alg, "secondChance") != 0){
        printf("ERRO: Coloque um algoritmo valido: fifo, random, lru, secondChance\n");
        return 0;
    }

    
    char *table_structure = argv[5];
    if(argv[5] == NULL)
        table_structure = "dense";

    printf("\n--------------------------------\nExecutando o simulador...\n\n");
    printf("Arquivo de entrada: %s\n", argv[2]);
    printf("Tamanho da memoria: %iKB\n", memoryCompleteSize);
    printf("Tamanho das paginas: %iKB\n", pageSize);
    printf("Quantidade de paginas: %i\n", memorySize);
    printf("Tecnica de reposicao: %s\n", alg);
    printf("Estrutura da tabela: %s\n\n", table_structure);

    // alocate tad to virtual memory
    // simulate virtual memory
    initMemory();
    if(strcmp(alg, "lru") == 0)
            initAuxList(&indexList);
    if(strcmp(alg, "secondChance") == 0)
            initAuxList(&circularList);

    if(strcmp(table_structure, "2level") == 0){
        initFirstLevelPageTable2Level();
        simulateVirtualMemory2Level(file, s, alg);
    }
    else if(strcmp(table_structure, "3level") == 0){
        initFirstLevelPageTable3Level();
        simulateVirtualMemory3Level(file, s, alg);
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
    printf("Numero de acessos de escrita: %i\n", memoryAccessWrite);
    printf("Numero de acessos de leitura: %i\n", memoryAccessRead);
    printf("Page Faults: %i\n", pageFaults);
    printf("Paginas Sujas: %i\n", dirtyPages);
    
    free(pageTable);
    free(memory);
    if(strcmp(alg, "lru") == 0)
            free(indexList);
    if(strcmp(alg, "secondChance") == 0)
            free(circularList);
    fclose(file);

    return 0;
}