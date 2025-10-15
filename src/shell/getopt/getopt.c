/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 * getopt.h - Windows compatibility layer for getopt
 *
 * This is a minimal implementation of getopt/getopt_long for Windows.
 * Based on public domain implementations.
 */

#include <stdio.h>
#include <string.h>

#include "getopt.h"

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

static char *nextchar = NULL;

int getopt(int argc, char * const argv[], const char *optstring) {
    if(optind == 0) {
        optind = 1;
    }

    optarg = NULL;

    if(nextchar == NULL || *nextchar == '\0') {
        if(optind >= argc || argv[optind] == NULL || argv[optind][0] != '-' ||
            argv[optind][1] == '\0') {
            return -1;
        }

        if(strcmp(argv[optind], "--") == 0) {
            ++optind;

            return -1;
        }

        nextchar = argv[optind] + 1;
        optind++;
    }

    char c = *nextchar++;
    const char *option = strchr(optstring, c);

    if(option == NULL || c == ':') {
        optopt = c;

        if(opterr && *optstring != ':') {
            fprintf(stderr, "Unknown option: -%c\n", c);
        }

        return '?';
    }

    if(option[1] == ':') {
        if(*nextchar != '\0') {
            optarg = nextchar;
            nextchar = NULL;
        } else if(optind < argc) {
            optarg = argv[optind++];
        } else {
            optopt = c;

            if(opterr && *optstring != ':') {
                fprintf(stderr, "Option requires an argument: -%c\n", c);
            }

            return (optstring[0] == ':') ? ':' : '?';
        }
    }

    return c;
}

int getopt_long(int argc, char * const argv[], const char *optstring, const struct option *longopts,
    int *longindex) {
    if(optind == 0) {
        optind = 1;
    }

    optarg = NULL;

    if(optind >= argc || argv[optind] == NULL || argv[optind][0] != '-') {
        return -1;
    }

    if(strcmp(argv[optind], "--") == 0) {
        optind++;
        return -1;
    }

    // Handle long options (--option)
    if(argv[optind][0] == '-' && argv[optind][1] == '-') {
        const char *name = argv[optind] + 2;
        const char *equals = strchr(name, '=');
        size_t name_len = equals ? (size_t)(equals - name) : strlen(name);

        for(int i = 0; longopts[i].name != NULL; i++) {
            if(strncmp(name, longopts[i].name, name_len) == 0 &&
                strlen(longopts[i].name) == name_len) {
                if(longindex) {
                    *longindex = i;
                }

                optind++;

                if(longopts[i].has_arg == required_argument ||
                    longopts[i].has_arg == optional_argument) {
                    if(equals) {
                        optarg = (char *)(equals + 1);
                    } else if(longopts[i].has_arg == required_argument) {
                        if(optind < argc) {
                            optarg = argv[optind++];
                        } else {
                            if(opterr) {
                                fprintf(stderr, "Option --%s requires an argument\n",
                                    longopts[i].name);
                            }

                            return '?';
                        }
                    }
                }

                if(longopts[i].flag) {
                    *longopts[i].flag = longopts[i].val;

                    return 0;
                } else {
                    return longopts[i].val;
                }
            }
        }

        if(opterr) {
            fprintf(stderr, "Unknown option: %s\n", argv[optind]);
        }
        optind++;

        return '?';
    }

    // Handle short options (fallback to getopt())
    return getopt(argc, argv, optstring);
}
