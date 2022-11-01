#include <stdio.h>
#include <ctype.h>  
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios orig_termios;

void die (const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr");
}

void enableRawMode() {
    if ( tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    char c;
    int nread; 

    // This will read in a new number. If read() does not read in 1 byte, we check for an error. Once nread
    // places 1 byte in c, the loop exits and c is returned. 
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1) die("read");
    }
    
    return c;
}

char editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }

    return c;
}

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

char **wordBank() {
    // get number of words first
    int n = 0;
    char c;
    int fd = open("../data/word_bank.txt", O_RDONLY);

    if (fd == -1) die("open");

    while ((n = read(fd, &c, 1)) == 1) {
        write(STDOUT_FILENO, &c, 1);
    }
    
    return 0;
}

int main() {
    enableRawMode();

    wordBank();
    
    char *phrase = "type this phrase to test typing";
    int phraseIdx = 0;

    char lastChar = '\0';

    printf("%s\r\n\n", phrase);
    
    clock_t t;
    t = clock();

    while (1) {
        if (phrase[phraseIdx] == '\0') break;
        
        lastChar = editorProcessKeypress();

        if (lastChar != 0) {
            write(STDOUT_FILENO, "\x1b[30m", 5);
            if (phrase[phraseIdx] == lastChar) {
                phraseIdx++;
                write(STDOUT_FILENO, "\x1b[42m", 5);
            } else {
                write(STDOUT_FILENO, "\x1b[41m", 5);
            }

            write(STDOUT_FILENO, &lastChar, 1);

            write(STDOUT_FILENO, "\x1b[37m", 5);
            write(STDOUT_FILENO, "\x1b[40m", 5);
        }
    }

    printf("\r\n\n");

    t = clock() - t;
    double wpm = 6 / ((((double)t)/CLOCKS_PER_SEC) * 60);

    printf("wpm: %lf", wpm);

    return 0;
}

