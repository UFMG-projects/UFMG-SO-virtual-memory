#include <stdlib.h>
#include <stdio.h>

// find the number of bits for the offset for find page
int findS(int PageSize){
    int s = 0;
    int tmp = PageSize * 1024;
    while(tmp > 1){
        tmp = tmp >> 1;
        s++;
    }
    return s;
}

int main(int argc, char* argv[]){
    //initialize the 4 arguments
    FILE *file;
    char *alg = argv[1]; //lru, second chance, fifo, random
    file = fopen(argv[2], "r"); 
    int pageSize = atoi(argv[3]); // 2 to 64; 2^?
    int memSize = atoi(argv[4]); //128 to 16384 

    int numPages = memSize/pageSize;
    int s = findS(pageSize);

    // alocate tad to virtual memory
    // TODO

    // initialize global variables
    // CHANGE THIS TO GLOBAL, IF NECESSARY
    // TODO: 
    int pageFaults; 
    int diskAccess;

    //file read
    unsigned addr;
    char rw;
    while(fscanf(file,"%x %c",&addr,&rw) != -1){
        addr = addr >> s;
    }


    //final report
    fclose(file);
    printf("\nExecutando o simulador...\n");
    printf("Arquivo de entrada: %s\n", argv[2]);
    printf("Tamanho da memoria: %iKB\n", memSize);
    printf("Tamanho das paginas: %iKB\n", pageSize);
    printf("Tecnica de reposicao: %s\n", alg);
    printf("Paginas lidas: %i\n", pageFaults);
    printf("Paginas escritas: %i\n", diskAccess);

    return 0;
}