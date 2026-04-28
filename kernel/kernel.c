#define VGA ((volatile char*)0xB8000)
#define WIDTH 80
#define WHITE 0x0F

unsigned char fg_color = 0x0F;
unsigned char bg_color = 0x00;

#define MAX_NODES 64
#define MAX_NAME  32
#define MAX_CONTENT 256
#define MAX_CHILDREN 16

typedef enum { NODE_FILE, NODE_DIR } NodeType;

typedef struct Node {
    char name[MAX_NAME];
    NodeType type;
    char content[MAX_CONTENT];
    int children[MAX_CHILDREN];
    int child_count;
    int parent;
    int used;
} Node;

Node fs[MAX_NODES];
int fs_count = 0;
int current_dir = 0;

void fs_init() {
    for (int i = 0; i < MAX_NODES; i++) {
        fs[i].used = 0; fs[i].child_count = 0; fs[i].parent = 0;
        fs[i].content[0] = 0; fs[i].name[0] = 0;
        for (int j = 0; j < MAX_CHILDREN; j++) fs[i].children[j] = -1;
    }
    fs[0].used = 1; fs[0].type = NODE_DIR; fs[0].parent = 0;
    fs[0].name[0] = '/'; fs[0].name[1] = 0; fs_count = 1;
}

void strncpy_s(char* dst, const char* src, int n) {
    int i = 0;
    while (i < n-1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

int fs_find(int dir, const char* name) {
    for (int i = 0; i < fs[dir].child_count; i++) {
        int idx = fs[dir].children[i]; int j = 0;
        while (fs[idx].name[j] && name[j] && fs[idx].name[j] == name[j]) j++;
        if (!fs[idx].name[j] && !name[j]) return idx;
    }
    return -1;
}

int fs_mkdir(int dir, const char* name) {
    if (fs_count >= MAX_NODES) return -1;
    if (fs[dir].child_count >= MAX_CHILDREN) return -1;
    if (fs_find(dir, name) != -1) return -2;
    int idx = fs_count++;
    fs[idx].used = 1; fs[idx].type = NODE_DIR; fs[idx].parent = dir;
    fs[idx].child_count = 0; fs[idx].content[0] = 0;
    strncpy_s(fs[idx].name, name, MAX_NAME);
    for (int j = 0; j < MAX_CHILDREN; j++) fs[idx].children[j] = -1;
    fs[dir].children[fs[dir].child_count++] = idx;
    return idx;
}

int fs_touch(int dir, const char* name) {
    if (fs_count >= MAX_NODES) return -1;
    if (fs[dir].child_count >= MAX_CHILDREN) return -1;
    if (fs_find(dir, name) != -1) return -2;
    int idx = fs_count++;
    fs[idx].used = 1; fs[idx].type = NODE_FILE; fs[idx].parent = dir;
    fs[idx].child_count = 0; fs[idx].content[0] = 0;
    strncpy_s(fs[idx].name, name, MAX_NAME);
    for (int j = 0; j < MAX_CHILDREN; j++) fs[idx].children[j] = -1;
    fs[dir].children[fs[dir].child_count++] = idx;
    return idx;
}

unsigned char make_color(unsigned char fg, unsigned char bg) {
    return (bg << 4) | (fg & 0x0F);
}

void cls() {
    unsigned char color = make_color(fg_color, bg_color);
    for (int i = 0; i < WIDTH * 25 * 2; i += 2) {
        VGA[i] = ' '; VGA[i+1] = color;
    }
}

void print_at(const char* str, int row, int col, unsigned char color) {
    int i = 0;
    while (str[i]) {
        int pos = (row*WIDTH+col+i)*2;
        VGA[pos] = str[i]; VGA[pos+1] = color; i++;
    }
}

void draw_boot_header() {
    print_at("UltiOS - uShell v0.1.0", 0, 0, make_color(0x0B, bg_color));
    for (int i = 0; i < WIDTH; i++) {
        int pos = (1*WIDTH+i)*2;
        VGA[pos] = '-'; VGA[pos+1] = make_color(0x07, bg_color);
    }
}

void enable_cursor() {
    /* shape: scanline 13-15 = underline cursor */
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0A),"Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0D),"Nd"((unsigned short)0x3D5));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0B),"Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0F),"Nd"((unsigned short)0x3D5));
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

int strlen(const char* s) { int i = 0; while (s[i]) i++; return i; }

void strcpy(char* dst, const char* src) {
    int i = 0; while (src[i]) { dst[i] = src[i]; i++; } dst[i] = 0;
}

void concat(char* out, const char* a, const char* b) {
    int i = 0, j = 0;
    while (a[i]) out[j++] = a[i++]; i = 0;
    while (b[i]) out[j++] = b[i++]; out[j] = 0;
}

int is_empty(char* s) {
    int i = 0;
    while (s[i]) { if (s[i] != ' ') return 0; i++; }
    return 1;
}

int is_digit(char c) { return c >= '0' && c <= '9'; }

void itoa(int n, char* buf) {
    if (n == 0) { buf[0]='0'; buf[1]=0; return; }
    int neg=0; if (n<0) { neg=1; n=-n; }
    int i=0; char tmp[20];
    while (n>0) { tmp[i++]='0'+(n%10); n/=10; }
    if (neg) tmp[i++]='-';
    int j=0; while (i>0) buf[j++]=tmp[--i]; buf[j]=0;
}

int calc(const char* expr) {
    int i=0, a=0, neg=0;
    if (expr[i]=='-') { neg=1; i++; }
    while (is_digit(expr[i])) { a=a*10+(expr[i]-'0'); i++; }
    if (neg) a=-a;
    while (expr[i]==' ') i++;
    char op=expr[i++];
    while (expr[i]==' ') i++;
    int b=0, neg2=0;
    if (expr[i]=='-') { neg2=1; i++; }
    while (is_digit(expr[i])) { b=b*10+(expr[i]-'0'); i++; }
    if (neg2) b=-b;
    if (op=='+') return a+b; if (op=='-') return a-b;
    if (op=='*') return a*b; if (op=='/'&&b!=0) return a/b;
    return 0;
}

struct idt_entry {
    unsigned short base_lo;
    unsigned short sel;
    unsigned char  zero;
    unsigned char  flags;
    unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr { unsigned short limit; unsigned int base; } __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr   idtp;
extern void idt_load(struct idt_ptr*);
extern void timer_handler();

void idt_set_gate(int n, unsigned int base) {
    idt[n].base_lo = base&0xFFFF; idt[n].base_hi = (base>>16)&0xFFFF;
    idt[n].sel = 0x08; idt[n].zero = 0; idt[n].flags = 0x8E;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry)*256)-1;
    idtp.base  = (unsigned int)&idt;
    for (int i=0; i<256; i++) {
        idt[i].base_lo=0; idt[i].base_hi=0;
        idt[i].sel=0; idt[i].zero=0; idt[i].flags=0;
    }
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x11),"Nd"((unsigned short)0x20));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x11),"Nd"((unsigned short)0xA0));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x20),"Nd"((unsigned short)0x21));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x28),"Nd"((unsigned short)0xA1));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x04),"Nd"((unsigned short)0x21));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x02),"Nd"((unsigned short)0xA1));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x01),"Nd"((unsigned short)0x21));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x01),"Nd"((unsigned short)0xA1));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0xFE),"Nd"((unsigned short)0x21));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0xFF),"Nd"((unsigned short)0xA1));
    idt_set_gate(32, (unsigned int)timer_handler);
    idt_load(&idtp);
    __asm__ volatile ("sti");
}

volatile unsigned int pit_ticks = 0;
void pit_tick() {
    pit_ticks++;
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x20),"Nd"((unsigned short)0x20));
}
void pit_init() {
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x36),"Nd"((unsigned short)0x43));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)(11932&0xFF)),"Nd"((unsigned short)0x40));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)(11932>>8)),"Nd"((unsigned short)0x40));
}

unsigned int get_uptime_seconds() { return pit_ticks/100; }

void format_uptime(unsigned int secs, char* out) {
    unsigned int d=secs/86400; secs%=86400;
    unsigned int h=secs/3600; secs%=3600;
    unsigned int m=secs/60, s=secs%60;
    int i=0; char tmp[10];
    if (d>0){itoa(d,tmp);int j=0;while(tmp[j])out[i++]=tmp[j++];out[i++]='d';out[i++]=' ';}
    if (h>0||d>0){itoa(h,tmp);int j=0;while(tmp[j])out[i++]=tmp[j++];out[i++]='h';out[i++]=' ';}
    if (m>0||h>0||d>0){itoa(m,tmp);int j=0;while(tmp[j])out[i++]=tmp[j++];out[i++]='m';out[i++]=' ';}
    itoa(s,tmp);int j=0;while(tmp[j])out[i++]=tmp[j++];out[i++]='s';out[i]=0;
}

void scroll() {
    unsigned char color = make_color(fg_color, bg_color);
    for (int row=1; row<24; row++)
        for (int col=0; col<WIDTH; col++) {
            int dst=(row*WIDTH+col)*2, src=((row+1)*WIDTH+col)*2;
            VGA[dst]=VGA[src]; VGA[dst+1]=VGA[src+1];
        }
    for (int col=0; col<WIDTH; col++) {
        int pos=(24*WIDTH+col)*2;
        VGA[pos]=' '; VGA[pos+1]=color;
    }
}

void clear_line(int row) {
    unsigned char color = make_color(fg_color, bg_color);
    for (int i=0; i<WIDTH; i++) {
        int pos=(row*WIDTH+i)*2;
        VGA[pos]=' '; VGA[pos+1]=color;
    }
}

void set_cursor(int row, int col) {
    unsigned short pos = row*WIDTH+col;
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0F),"Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)(pos&0xFF)),"Nd"((unsigned short)0x3D5));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)0x0E),"Nd"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0,%1"::"a"((unsigned char)(pos>>8)),"Nd"((unsigned short)0x3D5));
}

unsigned char read_key() {
    unsigned char status, key;
    do { __asm__ volatile ("inb %1,%0":"=a"(status):"Nd"((unsigned short)0x64)); } while (!(status&1));
    __asm__ volatile ("inb %1,%0":"=a"(key):"Nd"((unsigned short)0x60));
    return key;
}

char scancode_to_char(unsigned char sc, int shift) {
    char map_low[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,
        0,'q','w','e','r','t','y','u','i','o','p','[',']',0,
        0,'a','s','d','f','g','h','j','k','l',';','\'','`',
        0,'\\','z','x','c','v','b','n','m',',','.','/',0,0,0,' '
    };
    char map_high[] = {
        0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,
        0,'Q','W','E','R','T','Y','U','I','O','P','{','}',0,
        0,'A','S','D','F','G','H','J','K','L',':','"','~',
        0,'|','Z','X','C','V','B','N','M','<','>','?',0,0,0,' '
    };
    if (sc < sizeof(map_low)) return shift ? map_high[sc] : map_low[sc];
    return 0;
}

int print_prompt(int row) {
    char path[128];
    path[0]='c'; path[1]=':'; path[2]='\\'; path[3]=0;
    if (current_dir != 0) {
        char parts[8][MAX_NAME];
        int depth=0, node=current_dir;
        while (node!=0 && depth<8) {
            int j=0;
            while (fs[node].name[j]) { parts[depth][j]=fs[node].name[j]; j++; }
            parts[depth][j]=0; depth++; node=fs[node].parent;
        }
        int i=3;
        for (int d=depth-1; d>=0; d--) {
            int j=0;
            while (parts[d][j]) path[i++]=parts[d][j++];
            if (d>0) path[i++]='\\';
        }
        path[i]=0;
    }
    char prompt[140]; int i=0, j=0;
    while (path[j]) prompt[i++]=path[j++];
    prompt[i++]='>'; prompt[i]=0;
    print_at(prompt, row, 0, make_color(0x0F, bg_color));
    return strlen(prompt);
}

unsigned char parse_color(char c) {
    if (c=='r') return 0x04; if (c=='R') return 0x0C;
    if (c=='g') return 0x02; if (c=='G') return 0x0A;
    if (c=='b') return 0x01; if (c=='B') return 0x09;
    if (c=='y') return 0x06; if (c=='Y') return 0x0E;
    if (c=='w') return 0x07; if (c=='W') return 0x0F;
    if (c=='p') return 0x05; if (c=='P') return 0x0D;
    if (c=='k') return 0x00; return 0xFF;
}

void reboot() {
    __asm__ volatile ("mov $0xFE,%al\n outb %al,$0x64\n");
}

int execute(char* cmd, int row) {
    clear_line(row); clear_line(row+1); clear_line(row+2); clear_line(row+3);

    if (strncmp(cmd, "echo ", 5) == 0) {
        print_at(cmd+5, row, 0, make_color(fg_color, bg_color)); return 1;
    } else if (strcmp(cmd, "echo") == 0) {
        return 1;
    } else if (strcmp(cmd, "version") == 0) {
        print_at("UltiOS v0.1.0", row, 0, make_color(0x0B, bg_color)); return 1;
    } else if (strcmp(cmd, "osinfo") == 0) {
        char uptime[40]; char upline[60];
        format_uptime(get_uptime_seconds(), uptime);
        concat(upline, "Uptime : ", uptime);
        print_at("UltiOS - Marcos Ulti [DEV]", row,   0, make_color(0x0B, bg_color));
        print_at("----------------------------",       row+1, 0, make_color(0x07, bg_color));
        print_at("Versao : v0.1.0",                   row+2, 0, make_color(0x0F, bg_color));
        print_at(upline,                               row+3, 0, make_color(0x0E, bg_color));
        return 5;
    } else if (strcmp(cmd, "uptime") == 0) {
        char uptime[40]; char out[50];
        format_uptime(get_uptime_seconds(), uptime);
        concat(out, "uptime: ", uptime);
        print_at(out, row, 0, make_color(0x0E, bg_color)); return 1;
    } else if (strcmp(cmd, "reboot") == 0) {
        reboot(); return 1;
    } else if (strcmp(cmd, "color") == 0) {
        print_at("uso: color f/b <cor> | color reset", row, 0, make_color(0x0C, bg_color)); return 1;
    } else if (strcmp(cmd, "color reset") == 0) {
        fg_color=0x0F; bg_color=0x00; cls(); draw_boot_header(); return 2;
    } else if (strncmp(cmd, "color ", 6) == 0) {
        char type=cmd[6]; char val=cmd[8];
        if (type=='f') {
            unsigned char c=parse_color(val);
            if (c!=0xFF) fg_color=c;
            else print_at("uso: color f/b <cor> | color reset", row, 0, make_color(0x0C, bg_color));
        } else if (type=='b') {
            unsigned char c=parse_color(val);
            if (c!=0xFF) { bg_color=c&0x07; cls(); draw_boot_header(); }
            else print_at("uso: color f/b <cor> | color reset", row, 0, make_color(0x0C, bg_color));
        } else {
            print_at("uso: color f/b <cor> | color reset", row, 0, make_color(0x0C, bg_color));
        }
        return 1;
    } else if (strncmp(cmd, "calc ", 5) == 0) {
        int result=calc(cmd+5); char buf[20]; itoa(result, buf);
        print_at(buf, row, 0, make_color(0x0E, bg_color)); return 1;
    } else if (strcmp(cmd, "ping") == 0) {
        print_at("pong", row, 0, make_color(0x0A, bg_color)); return 1;
    } else if (strncmp(cmd, "make ", 5) == 0) {
        char* rest = cmd + 5;
        if (strncmp(rest, "folder ", 7) == 0) {
            char* name = rest + 7;
            int r = fs_mkdir(current_dir, name);
            if (r==-1) print_at("erro: limite de nos atingido", row, 0, make_color(0x0C, bg_color));
            else if (r==-2) print_at("erro: ja existe", row, 0, make_color(0x0C, bg_color));
            else { char out[40]; concat(out, "criado: ", name); print_at(out, row, 0, make_color(0x0A, bg_color)); }
        } else if (strncmp(rest, "text ", 5) == 0) {
            char* name = rest + 5;
            int r = fs_touch(current_dir, name);
            if (r==-1) print_at("erro: limite de nos atingido", row, 0, make_color(0x0C, bg_color));
            else if (r==-2) print_at("erro: ja existe", row, 0, make_color(0x0C, bg_color));
            else { char out[40]; concat(out, "criado: ", name); print_at(out, row, 0, make_color(0x0A, bg_color)); }
        } else {
            print_at("uso: make folder <nome> | make text <nome>", row, 0, make_color(0x0C, bg_color));
        }
        return 1;
    } else if (strcmp(cmd, "list") == 0) {
        if (fs[current_dir].child_count == 0) {
            print_at("(vazio)", row, 0, make_color(0x07, bg_color)); return 1;
        }
        int col=0; int r=row;
        for (int i=0; i<fs[current_dir].child_count; i++) {
            int idx=fs[current_dir].children[i];
            char entry[MAX_NAME+2]; int j=0;
            if (fs[idx].type==NODE_DIR) entry[j++]='[';
            int k=0; while (fs[idx].name[k]) entry[j++]=fs[idx].name[k++];
            if (fs[idx].type==NODE_DIR) entry[j++]=']';
            entry[j]=0;
            int elen=strlen(entry);
            if (col+elen+2 >= WIDTH) { r++; col=0; }
            print_at(entry, r, col, fs[idx].type==NODE_DIR ? make_color(0x09,bg_color) : make_color(0x0F,bg_color));
            col+=elen+2;
        }
        return 1;
    } else if (strncmp(cmd, "read ", 5) == 0) {
        char* name=cmd+5;
        int idx=fs_find(current_dir, name);
        if (idx==-1) print_at("erro: nao encontrado", row, 0, make_color(0x0C, bg_color));
        else if (fs[idx].type==NODE_DIR) print_at("erro: e uma pasta", row, 0, make_color(0x0C, bg_color));
        else if (fs[idx].content[0]==0) print_at("(arquivo vazio)", row, 0, make_color(0x07, bg_color));
        else print_at(fs[idx].content, row, 0, make_color(0x0F, bg_color));
        return 1;
    } else if (strncmp(cmd, "change ", 7) == 0) {
        char* name=cmd+7;
        if (strcmp(name, "..") == 0) {
            if (current_dir!=0) current_dir=fs[current_dir].parent;
        } else {
            int idx=fs_find(current_dir, name);
            if (idx==-1) print_at("erro: nao encontrado", row, 0, make_color(0x0C, bg_color));
            else if (fs[idx].type!=NODE_DIR) print_at("erro: nao e uma pasta", row, 0, make_color(0x0C, bg_color));
            else current_dir=idx;
        }
        return 1;
    } else if (strncmp(cmd, "write ", 6) == 0) {
        char* rest=cmd+6; char name[MAX_NAME]; int i=0;
        while (rest[i] && rest[i]!=' ') { name[i]=rest[i]; i++; }
        name[i]=0;
        if (rest[i]==0) { print_at("uso: write <arquivo> <texto>", row, 0, make_color(0x0C, bg_color)); return 1; }
        char* text=rest+i+1;
        int idx=fs_find(current_dir, name);
        if (idx==-1) { print_at("erro: arquivo nao encontrado", row, 0, make_color(0x0C, bg_color)); return 1; }
        if (fs[idx].type==NODE_DIR) { print_at("erro: e uma pasta", row, 0, make_color(0x0C, bg_color)); return 1; }
        int j=0;
        while (text[j] && j<MAX_CONTENT-1) { fs[idx].content[j]=text[j]; j++; }
        fs[idx].content[j]=0;
        print_at("escrito.", row, 0, make_color(0x0A, bg_color)); return 1;
    } else if (strncmp(cmd, "remove ", 7) == 0) {
        char* name=cmd+7;
        int idx=fs_find(current_dir, name);
        if (idx==-1) { print_at("erro: nao encontrado", row, 0, make_color(0x0C, bg_color)); return 1; }
        if (fs[idx].type==NODE_DIR && fs[idx].child_count>0) {
            print_at("erro: pasta nao esta vazia", row, 0, make_color(0x0C, bg_color)); return 1;
        }
        int parent=fs[idx].parent;
        for (int i=0; i<fs[parent].child_count; i++) {
            if (fs[parent].children[i]==idx) {
                for (int j=i; j<fs[parent].child_count-1; j++)
                    fs[parent].children[j]=fs[parent].children[j+1];
                fs[parent].child_count--; break;
            }
        }
        fs[idx].used=0; fs[idx].name[0]=0; fs[idx].child_count=0;
        char out[40]; concat(out, "removido: ", name);
        print_at(out, row, 0, make_color(0x0A, bg_color)); return 1;
    } else if (strncmp(cmd, "rename ", 7) == 0) {
        char* rest=cmd+7; char oldname[MAX_NAME]; int i=0;
        while (rest[i] && rest[i]!=' ') { oldname[i]=rest[i]; i++; }
        oldname[i]=0;
        if (rest[i]==0) { print_at("uso: rename <origem> <destino>", row, 0, make_color(0x0C, bg_color)); return 1; }
        char* newname=rest+i+1;
        int idx=fs_find(current_dir, oldname);
        if (idx==-1) { print_at("erro: nao encontrado", row, 0, make_color(0x0C, bg_color)); return 1; }
        if (fs_find(current_dir, newname)!=-1) { print_at("erro: nome ja existe", row, 0, make_color(0x0C, bg_color)); return 1; }
        strncpy_s(fs[idx].name, newname, MAX_NAME);
        char out[60]; concat(out, "renomeado para: ", newname);
        print_at(out, row, 0, make_color(0x0A, bg_color)); return 1;
    } else if (strncmp(cmd, "help ", 5) == 0) {
        char* topic=cmd+5;
        if (strcmp(topic, "color") == 0) {
            print_at("color f <cor>  -- muda cor do texto", row,   0, make_color(0x0B, bg_color));
            print_at("color b <cor>  -- muda cor do fundo", row+1, 0, make_color(0x0B, bg_color));
            print_at("color reset    -- volta ao padrao",   row+2, 0, make_color(0x0B, bg_color));
            print_at("cores: r R g G b B y Y w W p P k",   row+3, 0, make_color(0x07, bg_color));
            return 5;
        } else if (strcmp(topic, "calc") == 0) {
            print_at("calc <expr>  -- ex: calc 2 + 2", row,   0, make_color(0x0B, bg_color));
            print_at("operadores: + - * /",            row+1, 0, make_color(0x07, bg_color));
            return 3;
        } else if (strcmp(topic, "echo") == 0) {
            print_at("echo <texto>  -- imprime texto", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "make") == 0) {
            print_at("make folder <nome>  -- cria uma pasta", row,   0, make_color(0x0B, bg_color));
            print_at("make text <nome>    -- cria um arquivo", row+1, 0, make_color(0x0B, bg_color));
            return 3;
        } else if (strcmp(topic, "read") == 0) {
            print_at("read <nome>  -- mostra conteudo do arquivo", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "write") == 0) {
            print_at("write <arquivo> <texto>  -- escreve no arquivo", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "remove") == 0) {
            print_at("remove <nome>  -- remove arquivo ou pasta vazia", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "rename") == 0) {
            print_at("rename <origem> <destino>  -- renomeia", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "change") == 0) {
            print_at("change <pasta>  -- entra na pasta", row,   0, make_color(0x0B, bg_color));
            print_at("change ..       -- volta um nivel",  row+1, 0, make_color(0x0B, bg_color));
            return 3;
        } else if (strcmp(topic, "list") == 0) {
            print_at("list  -- lista arquivos e pastas", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "reboot") == 0) {
            print_at("reinicia o sistema", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "uptime") == 0) {
            print_at("uptime  -- mostra tempo desde o boot", row, 0, make_color(0x0B, bg_color)); return 1;
        } else if (strcmp(topic, "osinfo") == 0) {
            print_at("osinfo  -- info do sistema", row, 0, make_color(0x0B, bg_color)); return 1;
        } else {
            print_at("sem parametros adicionais", row, 0, make_color(0x0B, bg_color)); return 1;
        }
    } else if (strcmp(cmd, "help") == 0) {
        print_at("ping echo version osinfo uptime calc color reboot clear", row,   0, make_color(0x0B, bg_color));
        print_at("make  read  write  remove  rename  list  change", row+1, 0, make_color(0x0B, bg_color));
        return 3;
    } else if (strcmp(cmd, "clear") == 0) {
        cls(); return 2;
    } else {
        print_at("comando nao encontrado: ", row, 0, make_color(0x0C, bg_color));
        print_at(cmd, row, 24, make_color(0x0C, bg_color)); return 1;
    }
}

#define HIST_SIZE 8

void kernel_main() {
    pit_init();
    idt_init();
    fs_init();
    fg_color=0x0F; bg_color=0x00;
    cls();
    draw_boot_header();
    enable_cursor();

    int row=2, col=0, buf_i=0, shift=0, prompt_len=0;
    char buf[70];
    char history[HIST_SIZE][70];
    int hist_count=0, hist_pos=-1;

    for (int i=0; i<HIST_SIZE; i++)
        for (int j=0; j<70; j++) history[i][j]=0;
    for (int i=0; i<70; i++) buf[i]=0;

    prompt_len=print_prompt(row);
    col=prompt_len;
    set_cursor(row, col);

    while (1) {
        unsigned char sc=read_key();

        if (sc==0x2A||sc==0x36) { shift=1; continue; }
        if (sc==0xAA||sc==0xB6) { shift=0; continue; }
        if (sc&0x80) continue;

        if (sc==0x48) {
            /* seta cima: histórico anterior */
            if (hist_count==0) continue;
            if (hist_pos==-1) hist_pos=hist_count-1;
            else if (hist_pos>0) hist_pos--;
            for (int i=col; i>=prompt_len; i--) {
                int pos=(row*WIDTH+i)*2;
                VGA[pos]=' '; VGA[pos+1]=make_color(fg_color,bg_color);
            }
            strcpy(buf, history[hist_pos]);
            buf_i=strlen(buf); col=prompt_len+buf_i;
            print_at(buf, row, prompt_len, make_color(fg_color,bg_color));
            set_cursor(row, col); continue;
        }

        if (sc==0x50) {
            /* seta baixo: histórico mais novo */
            if (hist_pos==-1) continue;
            for (int i=col; i>=prompt_len; i--) {
                int pos=(row*WIDTH+i)*2;
                VGA[pos]=' '; VGA[pos+1]=make_color(fg_color,bg_color);
            }
            if (hist_pos<hist_count-1) {
                hist_pos++;
                strcpy(buf, history[hist_pos]);
                buf_i=strlen(buf); col=prompt_len+buf_i;
                print_at(buf, row, prompt_len, make_color(fg_color,bg_color));
            } else {
                hist_pos=-1;
                for (int i=0; i<70; i++) buf[i]=0;
                buf_i=0; col=prompt_len;
            }
            set_cursor(row, col); continue;
        }

        if (sc==0x1C) {
            buf[buf_i]=0;
            if (is_empty(buf)) {
                for (int i=0; i<70; i++) buf[i]=0;
                buf_i=0; hist_pos=-1; continue;
            }
            if (hist_count<HIST_SIZE) strcpy(history[hist_count++], buf);
            else {
                for (int i=0; i<HIST_SIZE-1; i++) strcpy(history[i], history[i+1]);
                strcpy(history[HIST_SIZE-1], buf);
            }
            hist_pos=-1;

            if (row+4 >= 25) {
                int needed=(row+4)-24;
                for (int s=0; s<needed; s++) scroll();
                row-=needed;
            }

            int result=execute(buf, row+1);
            for (int i=0; i<70; i++) buf[i]=0;
            buf_i=0;

            if (result==2) { row=0; }  /* clear: volta ao topo sem header */
            else if (result==5) { row+=5; if (row>=25) { scroll(); row=24; } }
            else if (result==3) { row+=3; if (row>=25) { scroll(); row=24; } }
            else { row+=2; if (row>=25) { scroll(); row=24; } }

            prompt_len=print_prompt(row);
            col=prompt_len;
            set_cursor(row, col); continue;
        }

        if (sc==0x0E) {
            if (col>prompt_len && buf_i>0) {
                col--; buf_i--;
                int pos=(row*WIDTH+col)*2;
                VGA[pos]=' '; VGA[pos+1]=make_color(fg_color,bg_color);
                set_cursor(row, col);
            }
            continue;
        }

        char c=scancode_to_char(sc, shift);
        if (c==0 || col>=WIDTH-1) continue;

        buf[buf_i++]=c;
        int pos=(row*WIDTH+col)*2;
        VGA[pos]=c; VGA[pos+1]=make_color(fg_color,bg_color);
        col++; set_cursor(row, col);
    }
}