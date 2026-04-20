#define VGA ((volatile char*)0xB8000)
#define WIDTH 80
#define WHITE 0x0F
#define GRAY  0x08

void cls() {
    for (int i = 0; i < WIDTH * 25 * 2; i += 2) {
        VGA[i]   = ' ';
        VGA[i+1] = WHITE;
    }
}

void print_at(const char* str, int row, int col, char color) {
    int i = 0;
    while (str[i]) {
        int pos = (row * WIDTH + col + i) * 2;
        VGA[pos]   = str[i];
        VGA[pos+1] = color;
        i++;
    }
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

int is_empty(char* s) {
    int i = 0;
    while (s[i]) {
        if (s[i] != ' ') return 0;
        i++;
    }
    return 1;
}

void clear_line(int row) {
    for (int i = 0; i < WIDTH; i++) {
        int pos = (row * WIDTH + i) * 2;
        VGA[pos]   = ' ';
        VGA[pos+1] = WHITE;
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

void print_prompt(int row) {
    print_at("UltiCMD >_ ", row, 0, WHITE);
}

int execute(char* cmd, int row) {
    clear_line(row);
    if (strcmp(cmd, "ping") == 0) {
        print_at("pong", row, 0, 0x0A);
        return 1;
    } else if (strcmp(cmd, "help") == 0) {
        print_at("comandos: ping, help, clear", row, 0, 0x0B);
        return 1;
    } else if (strcmp(cmd, "clear") == 0) {
        cls();
        return 2;
    } else {
        print_at("comando nao encontrado: ", row, 0, 0x0C);
        print_at(cmd, row, 24, 0x0C);
        return 1;
    }
}

void kernel_main() {
    cls();

    int row = 0;
    int col = 10;
    char buf[70];
    int buf_i = 0;

    for (int i = 0; i < 70; i++) buf[i] = 0;

    print_prompt(row);
    set_cursor(row, col);

    while (1) {
        unsigned char sc = read_key();
        if (sc & 0x80) continue;

        if (sc == 0x1C) {
            buf[buf_i] = 0;

            if (is_empty(buf)) {
                for (int i = 0; i < 70; i++) buf[i] = 0;
                buf_i = 0;
                continue;
            }

            int result = execute(buf, row + 1);

            for (int i = 0; i < 70; i++) buf[i] = 0;
            buf_i = 0;

            if (result == 2) {
                row = 0;
            } else {
                row += 2;
                if (row >= 25) { cls(); row = 0; }
            }

            print_prompt(row);
            col = 10;
            set_cursor(row, col);
            continue;
        }

        if (sc == 0x0E) {
            if (col > 10 && buf_i > 0) {
                col--;
                buf_i--;
                int pos = (row * WIDTH + col) * 2;
                VGA[pos]   = ' ';
                VGA[pos+1] = WHITE;
                set_cursor(row, col);
            }
            continue;
        }

        char c = scancode_to_char(sc);
        if (c == 0 || col >= WIDTH - 1) continue;

        buf[buf_i++] = c;
        int pos = (row * WIDTH + col) * 2;
        VGA[pos]   = c;
        VGA[pos+1] = WHITE;
        col++;
        set_cursor(row, col);
    }
}