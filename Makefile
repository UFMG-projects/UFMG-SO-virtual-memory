CC=gcc
CFLAGS=-g -w

# Alvo para compilar o programa
tp2virtual: tp2virtual.c
	$(CC) $(CFLAGS) tp2virtual.c -o tp2virtual

# Alvo clean para remover o binário compilado
clean:
	rm -f tp2virtual
