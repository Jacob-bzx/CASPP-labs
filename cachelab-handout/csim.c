#define _XOPEN_SOURCE 700
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct
{
    int valid;
    size_t tag;
    long serial_number;
} Line;

typedef struct
{
    Line *lines;
    int count;
} Set;

int main(int argc, char *argv[])
{
    // parse parameters
    int s = 0, E = 0, b = 0;
    int hits = 0, misses = 0, evictions = 0;
    char *filePath = "filepath";
    int c;
    int serial_number = 0;
    while ((c = getopt(argc, argv, "s:E:b:t:")) != -1)
    {
        switch (c)
        {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            filePath = optarg;
            break;
        default:
            printf("unexpected option %d\n", c);
            break;
        }
    }
#ifdef DEBUG
    printf("s=%d, E=%d, b=%d, t=%s\n", s, E, b, filePath);
#endif
    int S = 1 << s;
    // int B = 1 << b;
    size_t index_mask = (S - 1) << b;
    // size_t offset_mask = B - 1;
    Set *sets = (Set *)calloc(S, sizeof(Set));
    if (!sets)
    {
        printf("malloc error");
        exit(1);
    }
    for (int i = 0; i < S; i++)
    {
        sets[i].count = 0;
        sets[i].lines = (Line *)calloc(E, sizeof(Line));
        if (!sets[i].lines)
        {
            printf("malloc error");
            exit(1);
        }
        for (int j = 0; j < E; j++)
        {
            sets[i].lines[j].valid = 0;
            sets[i].lines[j].tag = 0;
        }
    }

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filePath, "r");
    if (fp == NULL)
    {
        printf("file %s doesn't exist\n", filePath);
        exit(1);
    }

    while ((read = getline(&line, &len, fp)) != -1)
    {
#ifdef DEBUG
        printf("%s", line);
#endif
        if (line[0] != ' ')
        {
            continue;
        }
        char command;
        char address_string[256];
        if (sscanf(line, " %c %s,%*c", &command, address_string))
        {
            size_t address = strtol(address_string, NULL, 16);
            size_t tag = address >> (s + b);
            size_t index = (address & index_mask) >> b;
            // size_t offset = address & offset_mask;
            int found = 0;
            //hits
            for (int i = 0; i < E; i++)
            {

                if (sets[index].lines[i].valid && tag == sets[index].lines[i].tag)
                {
                    found = 1;
                    hits++;
#ifdef DEBUG
                    printf("hits ");
#endif
                    sets[index].lines[i].serial_number = ++serial_number;
                    break;
                }
            }
            //misses
            if (!found)
            {
                misses++;
#ifdef DEBUG
                printf("miss ");
#endif
                // but no eviction
                if (sets[index].count < E)
                {
                    for (int i = 0; i < E; i++)
                    {
                        if (sets[index].lines[i].valid == 0)
                        {
                            sets[index].lines[i].valid = 1;
                            sets[index].lines[i].tag = tag;
                            sets[index].count++;
                            sets[index].lines[i].serial_number = ++serial_number;
                            break;
                        }
                    }
                }
                else
                {
                    // eviction
                    evictions++;
#ifdef DEBUG
                    printf("eviction ");
#endif
                    long min_serial_number = time(0);
                    int evict_index = -1;
                    for (int i = 0; i < E; i++)
                    {
                        if (sets[index].lines[i].serial_number < min_serial_number)
                        {
                            min_serial_number = sets[index].lines[i].serial_number;
                            evict_index = i;
                        }
                    }
                    sets[index].lines[evict_index].tag = tag;
                    sets[index].lines[evict_index].serial_number = ++serial_number;
                }
            }
            if (command == 'M')
            {
                hits++;
#ifdef DEBUG
                printf("hits ");
#endif
            }
#ifdef DEBUG
            printf("\n");
#endif
        }
    }
    fclose(fp);

    printSummary(hits, misses, evictions);
    return 0;
}
