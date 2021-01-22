#include <stdio.h>
int MAIN();
int read() {
    int a;
    scanf("%d", &a);
    return a;
}

int fread() {
    float b;
    scanf("%f", &b);
    return b;
}

void write(char* s) {
    printf("%s", s);
}

void write(int a) {
    printf("%d", a);
}

void write(float b) {
    printf("%f", b);
}

int main() {
    MAIN();
}
