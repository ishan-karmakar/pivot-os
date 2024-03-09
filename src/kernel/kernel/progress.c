#include <kernel/progress.h>
#include <drivers/framebuffer.h>
#include <io/stdio.h>

size_t length, bar_length, clength;

void create_progress(size_t plength) {
    length = plength;
    bar_length = screen_num_cols - 2; // Save space for beginning and ending bracket
    clength = 0;
    screen_y++;
    update_progress(0);
}

void update_progress(size_t completed) {
    size_t bar_completed = completed * bar_length / length;
    if (bar_completed == clength)
        return;
    clength = bar_completed;
    screen_y--;
    screen_x = 0;
    printf("[");
    for (size_t i = 0; i < clength; i++)
        printf("-");
    for (size_t i = 0; i < (100 - clength); i++)
        printf(" ");
    printf("]");
    flush_screen();
}

// Completed / Total Length -> Bar Completed / Bar Length