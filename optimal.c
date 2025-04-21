#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define ALLOCATED_FRAMES 7
#define STRING_LEN 1000000
#define lrand() ((long)rand()*RAND_MAX + (long)rand())

// Function to locate a page in the frames
int locate(int* frames, int size, int page_no) {
    for (int i = 0; i < size; i++)
        if (frames[i] == page_no)
            return i;
    return -1;
}

// Function to generate the reference string
void ref_string_generator(int* ref_str, int P, int e, int m, double t) {
    int locus_position = 0; // starting position
    int ref_count = 0;

    while (ref_count < STRING_LEN) {
        int next_ref = lrand() % e + locus_position;
        if (!ref_count || next_ref != ref_str[ref_count - 1]) // different from previous one
            ref_str[ref_count++] = next_ref;
        else
            continue;

        if (ref_count % m == 0) { // displace the locus
            if (rand() < t * RAND_MAX) // jump
                locus_position = lrand() % (P - e + 1);
            else
                locus_position = (locus_position + 1) % (P - e + 1);
        }
    }
}

// Optimal page replacement algorithm
int optimal(int* ref_str, int size, int limit) {
    int page_faults = 0;
    int frames[size], i, cur, page_no, frame_no;

    for (i = 0; i < size; i++)
        frames[i] = -1; // empty frames

    for (i = 0, cur = 0; i < size; i++, cur++) {
        page_no = ref_str[cur];
        frame_no = locate(frames, size, page_no);
        if (frame_no >= 0) // already exists
            i--;
        else { // page fault
            frames[i] = page_no;
            page_faults++;
        }
    }

    for (; cur < STRING_LEN; cur++) {
        page_no = ref_str[cur];
        frame_no = locate(frames, size, page_no);
        if (frame_no >= 0) // already exists
            continue;

        page_faults++;
        unsigned unused = (1 << size) - 1; // binary 1111111
        int victim;

        for (int k = 1; k <= limit && unused && cur + k < STRING_LEN; k++) {
            victim = locate(frames, size, ref_str[cur + k]);
            if (victim >= 0)
                unused &= ~(1 << victim); // set unused bit to zero
        }

        if (!unused) { // all frames will be referred to in the future
            frames[victim] = page_no;
        } else { // at least one frame won't be referred to in the future
            victim = 0;
            while (unused % 2 == 0) {
                unused = unused >> 1;
                victim++;
            }
            frames[victim] = page_no;
        }
    }
    return page_faults;
}

// FIFO page replacement algorithm
int fifo(int* ref_str, int size) {
    int page_faults = 0;
    int frames[size];
    int front = 0;

    for (int i = 0; i < size; i++)
        frames[i] = -1;

    for (int i = 0; i < STRING_LEN; i++) {
        int page_no = ref_str[i];
        int frame_no = locate(frames, size, page_no);
        if (frame_no >= 0)
            continue;

        frames[front] = page_no;
        front = (front + 1) % size;
        page_faults++;
    }
    return page_faults;
}

// LRU page replacement algorithm
int lru(int* ref_str, int size) {
    int page_faults = 0;
    int frames[size];
    int last_used[size];

    for (int i = 0; i < size; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    for (int i = 0; i < STRING_LEN; i++) {
        int page_no = ref_str[i];
        int frame_no = locate(frames, size, page_no);

        if (frame_no >= 0) {
            last_used[frame_no] = i;
            continue;
        }

        page_faults++;
        int lru_index = 0;
        for (int j = 1; j < size; j++) {
            if (last_used[j] < last_used[lru_index])
                lru_index = j;
        }

        frames[lru_index] = page_no;
        last_used[lru_index] = i;
    }
    return page_faults;
}

// Second Chance page replacement algorithm
int second_chance(int* ref_str, int size) {
    int page_faults = 0;
    int frames[size];
    int ref_bits[size];
    int pointer = 0;

    for (int i = 0; i < size; i++) {
        frames[i] = -1;
        ref_bits[i] = 0;
    }

    for (int i = 0; i < STRING_LEN; i++) {
        int page_no = ref_str[i];
        int frame_no = locate(frames, size, page_no);

        if (frame_no >= 0) {
            ref_bits[frame_no] = 1;
            continue;
        }

        page_faults++;
        while (ref_bits[pointer] == 1) {
            ref_bits[pointer] = 0;
            pointer = (pointer + 1) % size;
        }

        frames[pointer] = page_no;
        ref_bits[pointer] = 1;
        pointer = (pointer + 1) % size;
    }
    return page_faults;
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 9) {
        fprintf(stderr, "Usage: %s -P <P> -e <e> -m <m> -t <t>\n", argv[0]);
        return 1;
    }

    int P = 0, e = 0, m = 0;
    double t = 0.0;

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-P") == 0)
            P = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "-e") == 0)
            e = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "-m") == 0)
            m = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "-t") == 0)
            t = atof(argv[i + 1]);
        else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            return 1;
        }
    }

    int* ref_str = (int*)malloc(STRING_LEN * sizeof(int));
    ref_string_generator(ref_str, P, e, m, t);

    int optimal_faults = optimal(ref_str, ALLOCATED_FRAMES, e * m);
    int fifo_faults = fifo(ref_str, ALLOCATED_FRAMES);
    int lru_faults = lru(ref_str, ALLOCATED_FRAMES);
    int second_chance_faults = second_chance(ref_str, ALLOCATED_FRAMES);

    printf("Page faults:\n");
    printf("Optimal: %d\n", optimal_faults);
    printf("FIFO: %d\n", fifo_faults);
    printf("LRU: %d\n", lru_faults);
    printf("Second Chance: %d\n", second_chance_faults);

    free(ref_str);
    return 0;
}