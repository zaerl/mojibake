/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 * getopt.h - Windows compatibility layer for getopt
 *
 * This is a minimal implementation of getopt/getopt_long for Windows.
 * Based on public domain implementations.
 */

#pragma once

#ifndef MJB_GETOPT_H
#define MJB_GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

// External variables used by getopt
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

// Option structure for getopt_long
struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

// Argument types
#define no_argument       0
#define required_argument 1
#define optional_argument 2

// Function declarations
int getopt(int argc, char * const argv[], const char *optstring);
int getopt_long(int argc, char * const argv[], const char *optstring, const struct option *longopts,
    int *longindex);

#ifdef __cplusplus
}
#endif

#endif // MJB_GETOPT_H
