#include "libtasquake/console_input.h"
#include <chrono>
#include <thread>

void print_line(const char* line) {
    printf("print line called!\n");
    puts(line);
}

int main() {
    stdin_input input;
    input.callback = print_line;
    tasquake_stdin_init(&input);

    while(1) {
        tasquake_stdin_read(&input);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
