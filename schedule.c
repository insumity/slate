#include <stdio.h>

int main(int argc, char* argv[]) {

    char* read_policy = argv[1];
    char* read_program = argv[2];

    printf("%s %s\n", read_policy, read_program);

    return 0;
}

