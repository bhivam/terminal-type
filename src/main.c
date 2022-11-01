#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define CTRL_KEY(k) ((k)&0x1f)

struct termios orig_termios;

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

char editorReadKey()
{
    char c;
    int nread;

    // This will read in a new number. If read() does not read in 1 byte, we check for an error. Once nread
    // places 1 byte in c, the loop exits and c is returned.
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1)
            die("read");
    }

    return c;
}

char editorProcessKeypress()
{
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }

    return c;
}

void editorRefreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

int randint(int n) 
{
    if ((n-1) == RAND_MAX) {
        return rand();
    }
    else 
    {
        // The reason why there would be a skew if the just did rand() % n is that if RAND_MAX was say 13 and n was 4, then 0 and 1 would have one more occurance than 2 and 3. This skews the randomness.
        // by finding where the rand_max should be according to our n, we can generate a less skewed example by throwing out anything above the correct rand_max.
        int end = RAND_MAX / n;
        end = end * n;
        
        int r;
        while ((r = rand()) >= end);

        return r % n;

    }
}

char *wordBank()
{
    // get number of words first
    int n = 0;
    char c;
    int fd = open("../data/word_bank.txt", O_RDONLY);

    if (fd == -1)
        die("open");

    int largestWordLen = 0;
    int currentWordLen = 0;
    int numWords = 0;
    char lastChar = ' ';

    while ((n = read(fd, &c, 1)) == 1)
    {
        if (c == ' ' || c == '\n')
        {
            if (currentWordLen > largestWordLen)
                largestWordLen = currentWordLen;
            currentWordLen = 0;
        }
        else
        {
            if (lastChar == ' ' || lastChar == '\n')
                numWords++;
            currentWordLen++;
        }

        lastChar = c;
    }

    if (lastChar != ' ' && lastChar != '\n')
        numWords++;

    char **words = malloc(sizeof(char *) * numWords);
    for (int i = 0; i < numWords; i++) 
    {
        words[i] = malloc(sizeof(char) * (largestWordLen + 1));
    }

    fd = open("../data/word_bank.txt", O_RDONLY);
    if (fd == -1) 
        die("open");
    
    int j = 0;
    int k = 0;
    while ((n = read(fd, &c, 1)) == 1 && j < numWords) 
    {
        if (c == ' ' || c == '\n') 
        {
            words[j][k] = '\0';
            j++;
            k = 0;
        }
        else 
        {
            words[j][k] = c;
            k++;
        }
    }

    char *phrase = malloc((1 + 30 * (largestWordLen + 1)) * sizeof(char));
    int phraseIdx = 0;

    for (int i = 0; i < 30; i++) 
    {
        int randIdx = randint(numWords);
        for (char *t = words[randIdx]; *t != '\0'; t++)
        {
            phrase[phraseIdx] = *t;
            phraseIdx++;
        }

        if (i != 29)
        {
            phrase[phraseIdx] = ' ';
            phraseIdx++;
        }
    }

    phrase[phraseIdx] = '\0';

    for (int i = 0; i < numWords; i++)
    {
        free(words[i]);
    }
    free(words);

    return phrase;
}

int main()
{
    enableRawMode();
    editorRefreshScreen();

    char *phrase = wordBank();
    int phraseIdx = 0;

    char *instruction = "Press Space to Continue";

    char lastChar = '\0';

    printf("%s\r\n\n", phrase);

    write(STDOUT_FILENO, instruction, strlen(instruction));

    while (editorProcessKeypress() != ' ');

    write(STDOUT_FILENO, "\x1b[2K", 4);

    write(STDOUT_FILENO, "\r", 1);

    clock_t t;
    t = clock();

    while (1)
    {
        if (phrase[phraseIdx] == '\0')
            break;

        lastChar = editorProcessKeypress();
    
        if (lastChar != 0)
        {
            write(STDOUT_FILENO, "\x1b[30m", 5);
            
            if (phrase[phraseIdx] == lastChar)
            {
                phraseIdx++;
                write(STDOUT_FILENO, "\x1b[42m", 5);
            }
            else
            {
                write(STDOUT_FILENO, "\x1b[41m", 5);
            }

            write(STDOUT_FILENO, &lastChar, 1);

            write(STDOUT_FILENO, "\x1b[37m", 5);
            write(STDOUT_FILENO, "\x1b[40m", 5);
        }
    }

    printf("\r\n\n");

    t = clock() - t;
    double wpm = 30 / ((((double)t) / CLOCKS_PER_SEC) * 60);

    printf("wpm: %lf", wpm);

    free(phrase);

    return 0;
}
