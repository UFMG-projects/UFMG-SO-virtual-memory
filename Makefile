CC=gcc
CFLAGS=-g -w
LIBS=-lm

# Alvo para compilar o programa
tp2virtual: tp2virtual.c
	$(CC) $(CFLAGS) tp2virtual.c -o tp2virtual $(LIBS)

# Alvo clean para remover o binário compilado
clean:
	rm -f tp2virtual saidas.txt

# gera as saídas para análise dado um arquivo(files/compilador.log) e um tamanho fixo de memória(1024)
# alterar ARQUIVO para diferentes resultados
page_graf: tp2virtual
	@echo "" > saidas.txt
	@echo "TamanhoPagina PageFaults DirtyPages" >> saidas.txt
	@for j in fifo secondChance random lru; do \
		echo "\nAlgoritmo: $$j\n" >> saidas.txt; \
		for i in 2 4 8 16 32 64; do \
			./tp2virtual $$j files/compilador.log $$i 1024> temp_output.txt; \
			pageFaults=$$(grep "Page Faults:" temp_output.txt | awk '{print $$3}'); \
			dirtyPages=$$(grep "Paginas Sujas:" temp_output.txt | awk '{print $$3}'); \
			echo "$$i $$pageFaults $$dirtyPages" >> saidas.txt; \
		done; \
	done
	@rm temp_output.txt

# gera as saídas para análise dado um arquivo(files/compilador.log) e um tamanho fixo de página(8)
# alterar ARQUIVO
mem_graf: tp2virtual
	@echo "" > saidas.txt
	@echo "TamanhoPagina PageFaults DirtyPages" >> saidas.txt
	@for j in fifo secondChance random lru; do \
		echo "\nAlgoritmo: $$j\n" >> saidas.txt; \
		for i in 128 256 512 1024 2048 4096 8192 16384; do \
			./tp2virtual $$j files/compilador.log 8 $$i> temp_output.txt; \
			pageFaults=$$(grep "Page Faults:" temp_output.txt | awk '{print $$3}'); \
			dirtyPages=$$(grep "Paginas Sujas:" temp_output.txt | awk '{print $$3}'); \
			echo "$$i $$pageFaults $$dirtyPages" >> saidas.txt; \
		done; \
	done
	@rm temp_output.txt