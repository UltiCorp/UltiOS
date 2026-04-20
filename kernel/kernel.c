#define VGA ((volatile char*)0xB8000)
#define WIDTH 80
#define WHITE 0x0F

unsigned char fg_color = 0x0F;
unsigned char bg_color = 0x00;

unsigned char make_color(unsigned char fg, unsigned char bg) {
    return (bg << 4) | (fg & 0x0F);
}

void cls() {
    unsigned char color = make_color(fg_color, bg_color);
    for (int i = 0; i < WIDTH * 25 * 2; i += 2) {
        VGA[i]   = ' ';
        VGA[i+1] = color;
    }
}

void print_at(const char* str, int row, int col, unsigned char color) {
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

int strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

int strlen(const char* s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void strcpy(char* dst, const char* src) {
    int i = 0;
    while (src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

int is_empty(char* s) {
    int i = 0;
    while (s[i]) {
        if (s[i] != ' ') return 0;
        i++;
    }
    return 1;
}

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

void itoa(int n, char* buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    int neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    int i = 0;
    char tmp[20];
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    if (neg) tmp[i++] = '-';
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

int calc(const char* expr) {
    int i = 0;
    int a = 0;
    int neg = 0;
    if (expr[i] == '-') { neg = 1; i++; }
    while (is_digit(expr[i])) { a = a * 10 + (expr[i] - '0'); i++; }
    if (neg) a = -a;
    while (expr[i] == ' ') i++;
    char op = expr[i++];
    while (expr[i] == ' ') i++;
    int b = 0;
    int neg2 = 0;
    if (expr[i] == '-') { neg2 = 1; i++; }
    while (is_digit(expr[i])) { b = b * 10 + (expr[i] - '0'); i++; }
    if (neg2) b = -b;
    if (op == '+') return a + b;
    if (op == '-') return a - b;
    if (op == '*') return a * b;
    if (op == '/' && b != 0) return a / b;
    return 0;
}

unsigned int get_ticks() {
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return lo;
}

unsigned int boot_ticks = 0;

unsigned int get_seconds() {
    unsigned int now = get_ticks();
    unsigned int diff = now - boot_ticks;
    return diff / 400000000;
}

void clear_line(int row) {
    unsigned char color = make_color(fg_color, bg_color);
    for (int i = 0; i < WIDTH; i++) {
        int pos = (row * WIDTH + i) * 2;
        VGA[pos]   = ' ';
        VGA[pos+1] = color;
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

char scancode_to_char(unsigned char sc, int shift) {
    char map_low[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,
        0,'q','w','e','r','t','y','u','i','o','p','[',']',0,
        0,'a','s','d','f','g','h','j','k','l',';','\'','`',
        0,'\\','z','x','c','v','b','n','m',',','.','/',0,
        0,0,' '
    };
    char map_high[] = {
        0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,
        0,'Q','W','E','R','T','Y','U','I','O','P','{','}',0,
        0,'A','S','D','F','G','H','J','K','L',':','"','~',
        0,'|','Z','X','C','V','B','N','M','<','>','?',0,
        0,0,' '
    };
    if (sc < sizeof(map_low)) return shift ? map_high[sc] : map_low[sc];
    return 0;
}

void print_prompt(int row) {
    unsigned char color = make_color(fg_color, bg_color);
    print_at("UltiCMD > ", row, 0, color);
}

unsigned char parse_color(char c) {
    if (c == 'r') return 0x04;
    if (c == 'R') return 0x0C;
    if (c == 'g') return 0x02;
    if (c == 'G') return 0x0A;
    if (c == 'b') return 0x01;
    if (c == 'B') return 0x09;
    if (c == 'y') return 0x06;
    if (c == 'Y') return 0x0E;
    if (c == 'w') return 0x07;
    if (c == 'W') return 0x0F;
    if (c == 'p') return 0x05;
    if (c == 'P') return 0x0D;
    if (c == 'k') return 0x00;
    return 0xFF;
}

void reboot() {
    __asm__ volatile (
        "mov $0xFE, %al\n"
        "outb %al, $0x64\n"
    );
}

int execute(char* cmd, int row) {
    clear_line(row);
    clear_line(row + 1);
    unsigned char color = make_color(fg_color, bg_color);

    if (strncmp(cmd, "echo ", 5) == 0) {
        print_at(cmd + 5, row, 0, color);
        return 1;
    } else if (strcmp(cmd, "echo") == 0) {
        return 1;
    } else if (strcmp(cmd, "version") == 0) {
        print_at("UltiOS v0.0.2", row, 0, make_color(0x0B, bg_color));
        return 3;
    } else if (strcmp(cmd, "about") == 0) {
        print_at("  UltiOS", row, 0, make_color(0x0B, bg_color));
        print_at("  -------", row + 1, 0, make_color(0x07, bg_color));
        print_at("  Versao : v0.0.2", row + 2, 0, make_color(0x0F, bg_color));
        print_at("  Marcos Ulti [DEV]", row + 3, 0, make_color(0x0F, bg_color));
        return 5;
    } else if (strcmp(cmd, "time") == 0) {
        char buf[20];
        itoa(get_seconds(), buf);
        char out[40];
        int i = 0, j = 0;
        char prefix[] = "uptime: ";
        while (prefix[j]) out[i++] = prefix[j++];
        j = 0;
        while (buf[j]) out[i++] = buf[j++];
        char suffix[] = "s";
        j = 0;
        while (suffix[j]) out[i++] = suffix[j++];
        out[i] = 0;
        print_at(out, row, 0, make_color(0x0E, bg_color));
        return 1;
    } else if (strcmp(cmd, "reboot") == 0) {
        reboot();
        return 1;
    } else if (strncmp(cmd, "color ", 6) == 0) {
        char type = cmd[6];
        char val  = cmd[8];
        if (type == 'f') {
            unsigned char c = parse_color(val);
            if (c != 0xFF) fg_color = c;
            else print_at("uso: color f <cor> | color b <cor>", row, 0, make_color(0x0C, bg_color));
        } else if (type == 'b') {
            unsigned char c = parse_color(val);
            if (c != 0xFF) { bg_color = c & 0x07; cls(); }
            else print_at("uso: color f <cor> | color b <cor>", row, 0, make_color(0x0C, bg_color));
        } else {
            print_at("uso: color f <cor> | color b <cor>", row, 0, make_color(0x0C, bg_color));
        }
        return 1;
    } else if (strncmp(cmd, "calc ", 5) == 0) {
        int result = calc(cmd + 5);
        char buf[20];
        itoa(result, buf);
        print_at(buf, row, 0, make_color(0x0E, bg_color));
        return 1;
    } else if (strcmp(cmd, "ping") == 0) {
        print_at("pong", row, 0, make_color(0x0A, bg_color));
        return 1;
    } else if (strcmp(cmd, "help") == 0) {
        print_at("ping  echo  version  calc  color  time  reboot  clear", row, 0, make_color(0x0B, bg_color));
        return 1;
    } else if (strcmp(cmd, "clear") == 0) {
        cls();
        return 2;
    } else {
        print_at("comando nao encontrado: ", row, 0, make_color(0x0C, bg_color));
        print_at(cmd, row, 24, make_color(0x0C, bg_color));
        return 1;
    }
}

#define HIST_SIZE 8

void kernel_main() {
    boot_ticks = get_ticks();
    cls();

    int row = 0;
    int col = 9;
    char buf[70];
    int buf_i = 0;
    int shift = 0;

    char history[HIST_SIZE][70];
    int hist_count = 0;
    int hist_pos = -1;
    for (int i = 0; i < HIST_SIZE; i++)
        for (int j = 0; j < 70; j++)
            history[i][j] = 0;

    for (int i = 0; i < 70; i++) buf[i] = 0;

    print_prompt(row);
    set_cursor(row, col);

    while (1) {
        unsigned char sc = read_key();

        if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
        if (sc & 0x80) continue;

        if (sc == 0x48) {
            if (hist_count == 0) continue;
            if (hist_pos == -1) hist_pos = hist_count - 1;
            else if (hist_pos > 0) hist_pos--;
            for (int i = col; i >= 9; i--) {
                int pos = (row * WIDTH + i) * 2;
                VGA[pos] = ' '; VGA[pos+1] = make_color(fg_color, bg_color);
            }
            strcpy(buf, history[hist_pos]);
            buf_i = strlen(buf);
            col = 9 + buf_i;
            print_at(buf, row, 9, make_color(fg_color, bg_color));
            set_cursor(row, col);
            continue;
        }

        if (sc == 0x50) {
            if (hist_pos == -1) continue;
            for (int i = col; i >= 9; i--) {
                int pos = (row * WIDTH + i) * 2;
                VGA[pos] = ' '; VGA[pos+1] = make_color(fg_color, bg_color);
            }
            if (hist_pos < hist_count - 1) {
                hist_pos++;
                strcpy(buf, history[hist_pos]);
                buf_i = strlen(buf);
                col = 9 + buf_i;
                print_at(buf, row, 9, make_color(fg_color, bg_color));
            } else {
                hist_pos = -1;
                for (int i = 0; i < 70; i++) buf[i] = 0;
                buf_i = 0;
                col = 9;
            }
            set_cursor(row, col);
            continue;
        }

        if (sc == 0x1C) {
            buf[buf_i] = 0;
            if (is_empty(buf)) {
                for (int i = 0; i < 70; i++) buf[i] = 0;
                buf_i = 0;
                hist_pos = -1;
                continue;
            }
            if (hist_count < HIST_SIZE) {
                strcpy(history[hist_count++], buf);
            } else {
                for (int i = 0; i < HIST_SIZE - 1; i++)
                    strcpy(history[i], history[i + 1]);
                strcpy(history[HIST_SIZE - 1], buf);
            }
            hist_pos = -1;

            int result = execute(buf, row + 1);
            for (int i = 0; i < 70; i++) buf[i] = 0;
            buf_i = 0;

            if (result == 2) {
                row = 0;
            } else if (result == 5) {
                row += 5;
                if (row >= 25) { cls(); row = 0; }
            } else if (result == 3) {
                row += 2;
                if (row >= 25) { cls(); row = 0; }
            } else {
                row += 2;
                if (row >= 25) { cls(); row = 0; }
            } 

            print_prompt(row);
            col = 9;
            set_cursor(row, col);
            continue;
        }

        if (sc == 0x0E) {
            if (col > 9 && buf_i > 0) {
                col--;
                buf_i--;
                int pos = (row * WIDTH + col) * 2;
                VGA[pos]   = ' ';
                VGA[pos+1] = make_color(fg_color, bg_color);
                set_cursor(row, col);
            }
            continue;
        }

        char c = scancode_to_char(sc, shift);
        if (c == 0 || col >= WIDTH - 1) continue;

        buf[buf_i++] = c;
        int pos = (row * WIDTH + col) * 2;
        VGA[pos]   = c;
        VGA[pos+1] = make_color(fg_color, bg_color);
        col++;
        set_cursor(row, col);
    }
}