#ifndef SUCORRPILOT_H
#define SUCORRPILOT_H

#include "su.h"
#include "segy.h"
#include "header.h"
#include "cwp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct{
    int trace_ns;
    int pilot_ns;
    int out_ns;

    int nfft;
    int nfreq;

    float dt_sec;

    float *time_buffer;

    complex *trace_spectrum;
    complex *pilot_spectrum;
} CorrWorkspace;

/*Read a pilot trace*/
int read_pilot_trace(FILE *fp, segy *pilot);

/*Check raw_su and pilot data parameter*/
int check_sampling_parameters(const segy *tr, const segy *pilot);

/*Inital workspace*/
int init_corr_workspace(CorrWorkspace *workspace, const segy *pilot, int trace_ns, float outtime);

/*Linear cross-correlation*/
int correlate_trace(const segy *tr, segy *out, CorrWorkspace *workspace);

/*Free corr_workspace*/
void free_corr_workspace(CorrWorkspace *workspace);

/* Create output directory if it does not exist. */
int ensure_directory(const char *directory);

/* Join directory and file name into a complete path. */
int join_path(char *path, size_t path_size,
        const char *directory, const char *filename);

/* Return the file name part of a path. */
const char *get_basename(const char *path);

/*Delete all .su file in output directory*/
int clean_su_files(const char *directory,int verbose);

#endif
