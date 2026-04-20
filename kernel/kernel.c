#define VGA ((volatile char*)0xB8000)
#define WIDTH 80
#define WHITE 0x0F

void cls() {
    for (int i = 0; i < WIDTH * 25 * 2; i += 2) {
        VGA[i]   = ' ';
        VGA[i+1] = WHITE;
    }
}

void print(const char* str, int row, int col) {
    int i = 0;
    while (str[i]) {
        int pos = (row * WIDTH + col + i) * 2;
        VGA[pos]   = str[i];
        VGA[pos+1] = WHITE;
        i++;
    }
}

void set_cursor(int row, int col) {
    unsigned short pos = row * WIDTH + col;
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x0F), "Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)(pos & 0xFF)), "Nd"((unsigned short)0x3D5));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)0x0E), "Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)(pos >> 8)), "Nd"((unsigned short)0x3D5));
}

unsigned char read_key() {
    unsigned char status, key;
    do {
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((unsigned short)0x64));
    } while (!(status & 1));
    __asm__ volatile ("inb %1, %0" : "=a"(key) : "Nd"((unsigned short)0x60));
    return key;
}

char scancode_to_char(unsigned char sc) {
    char map[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,
        0,'q','w','e','r','t','y','u','i','o','p','[',']',0,
        0,'a','s','d','f','g','h','j','k','l',';','\'','`',
        0,'\\','z','x','c','v','b','n','m',',','.','/',0,
        0,0,' '
    };
    if (sc < sizeof(map)) return map[sc];
    return 0;
}

void new_prompt(int* row, int* col) {
    (*row)++;
    if (*row >= 25) *row = 0;
    print("UltiCMD >_ ", *row, 0);
    *col = 10;
    set_cursor(*row, *col);
}

void kernel_main() {
    cls();
    int row = 0;
    int col = 10;
    print("UltiCMD >_ ", row, 0);
    set_cursor(row, col);

    while (1) {
        unsigned char sc = read_key();
        if (sc & 0x80) continue;

        if (sc == 0x1C) {
            new_prompt(&row, &col);
            continue;
        }

        if (sc == 0x0E) {
            if (col > 10) {
                col--;
                int pos = (row * WIDTH + col) * 2;
                VGA[pos]   = ' ';
                VGA[pos+1] = WHITE;
                set_cursor(row, col);
            }
            continue;
        }

        char c = scancode_to_char(sc);
        if (c == 0) continue;
        if (col >= WIDTH) continue;

        int pos = (row * WIDTH + col) * 2;
        VGA[pos]   = c;
        VGA[pos+1] = WHITE;
        col++;
        set_cursor(row, col);
    }
}
