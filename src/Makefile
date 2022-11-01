TARGET = main 
CC = gcc
CFLAGS = -g -std=c99 -Wall -Wvla -Werror -fsanitize=address,undefined

main: main.c
	gcc $(CFLAGS) main.c -o main
clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM
