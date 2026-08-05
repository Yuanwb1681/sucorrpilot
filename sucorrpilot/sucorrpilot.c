#include "sucorrpilot.h"
#include "cwp.h"
#include "header.h"
#include "par.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <glob.h>
#include <unistd.h>

int read_pilot_trace(FILE *fp, segy *pilot){
    if(fread(pilot, 1, HDRBYTES, fp)!=HDRBYTES) return 0;
    if(pilot->ns<=0||pilot->ns>SU_NFLTS) return 0;
    if(fread(pilot->data, sizeof(float), pilot->ns, fp)!=pilot->ns) return 0;
    return 1;
}

int check_sampling_parameters(const segy *tr, const segy *pilot){
    if(tr==NULL||pilot==NULL) return 0;
    if(tr->ns==0||pilot->ns==0) {
        warn("Invild ns:input ns =%u ,pilot ns=%u",(unsigned int)tr->ns,(unsigned int)pilot->ns);
        return 0;
    }
    if(tr->dt==0||pilot->dt==0) {
        warn("Invild dt:input dt =%u ,pilot dt=%u",(unsigned int)tr->dt,(unsigned int)pilot->dt);
        return 0;
    }
    if(tr->dt!=pilot->dt) {
        warn("sampling interval mismatch: input dt=%u , pilot dt=%u",(unsigned int)tr->dt,(unsigned int)pilot->dt);
        return 0;
    }

    return 1;
}

int init_corr_workspace(CorrWorkspace *workspace, const segy *pilot, int trace_ns, float outtime){
    int min_length;

    if(workspace==NULL||pilot==NULL) return 0;

    if(trace_ns<=0 ||pilot->ns<=0 ||pilot->dt==0) return 0;

    if(outtime<=0.0f) return 0;

    memset(workspace, 0, sizeof(*workspace));

    workspace->trace_ns=trace_ns;
    workspace->pilot_ns=pilot->ns;
    workspace->dt_sec=pilot->dt*1.0e-6f;

    min_length=trace_ns+pilot->ns-1;

    workspace->nfft=npfar(min_length);
    workspace->nfreq=workspace->nfft/2+1;

    workspace->out_ns=NINT((double)(outtime*1.0e6/(double)pilot->dt))+1;

    if(workspace->out_ns>workspace->nfft) workspace->out_ns=workspace->nfft;
    
    if(workspace->out_ns>SU_NFLTS) {
        warn("output ns=%d exceeds SU_NFLTS=%d",workspace->out_ns,SU_NFLTS);
        return 0;
    }
    
    workspace->time_buffer=ealloc1float(workspace->nfft);
    workspace->trace_spectrum=ealloc1complex(workspace->nfreq);
    workspace->pilot_spectrum=ealloc1complex(workspace->nfreq);

    memset(workspace->time_buffer, 0, workspace->nfft*sizeof(float));
    memcpy(workspace->time_buffer, pilot->data, pilot->ns*sizeof(float));
    pfarc(1, workspace->nfft, workspace->time_buffer, workspace->pilot_spectrum);

    return 1;

}

int correlate_trace(const segy *tr, segy *out, CorrWorkspace *workspace){

    int i;

    float trace_real;
    float trace_imag;

    float pilot_real;
    float pilot_imag;

    float scale;

    if(tr==NULL||workspace==NULL||out==NULL) return 0;

    if(tr->ns!=workspace->trace_ns) return 0;

    memset(workspace->time_buffer, 0, workspace->nfft*sizeof(float));
    memcpy(workspace->time_buffer, tr->data, tr->ns*sizeof(float));
    pfarc(1, workspace->nfft, workspace->time_buffer, workspace->trace_spectrum);

    for(i=0;i<workspace->nfreq;i++){
        trace_real=workspace->trace_spectrum[i].r;
        trace_imag=workspace->trace_spectrum[i].i;

        pilot_real=workspace->pilot_spectrum[i].r;
        pilot_imag=workspace->pilot_spectrum[i].i;

        workspace->trace_spectrum[i].r=trace_real*pilot_real+trace_imag*pilot_imag;
        workspace->trace_spectrum[i].i=trace_imag*pilot_real-trace_real*pilot_imag;
    }

    pfacr(-1, workspace->nfft, workspace->trace_spectrum, workspace->time_buffer);

    scale=workspace->dt_sec/workspace->nfft;

    *out=*tr;

    out->ns=workspace->out_ns;

    for(i=0;i<workspace->out_ns;i++)
        out->data[i]=workspace->time_buffer[i]*scale;
    
    return 1;
}    

int ensure_directory(const char *directory){
    struct stat st;
    if(directory==NULL) return 0;
    
    if(stat(directory, &st)==0){
        if(S_ISDIR(st.st_mode)) return 1;

        warn("%s exists but is not a directory",directory);
        return 0;
    }

    if(mkdir(directory,0755)!=0){
        warn("cannot creat directory %s:%s",directory,strerror(errno));
        return 0;
    }
    return 1;
}

const char *get_basename(const char *path){
    const char *slash;

    if(path==NULL) return NULL;

    slash = strrchr(path, '/');

    if(slash==NULL) return path;

    return slash+1;
}

int join_path(char *path, size_t path_size, const char *directory, const char *filename){
    int written;
    size_t directory_length;

    if(path==NULL||directory==NULL||filename==NULL||path_size==0) return 0;

    directory_length=strlen(directory);

    if(directory_length==0){
        warn("directory path is empty");
        return 0;
    }

    if(directory[directory_length-1]=='/') {
        written = snprintf(path, path_size, "%s%s",directory,filename);
    }else {
        written=snprintf(path, path_size, "%s/%s",directory,filename);
    }

    if(written<0||(size_t)written>=path_size){
        warn("path is too long:%s/%s",directory,filename);
        return 0;
    }
    return 1;
}

int clean_su_files(const char *directory, int verbose){
    char pattern[1024];
    glob_t file_list;
    size_t i;
    int deleted_count=0;

    if(directory==NULL) return -1;

    if(!join_path(pattern, sizeof(pattern), directory, "*.su")){
        warn("failed to build clean pattern for directory %s",directory);
        return -1;
    }

    memset(&file_list, 0, sizeof(file_list));

    {
        int status=glob(pattern, 0, NULL, &file_list);

        if(status==GLOB_NOMATCH){
            if(verbose) warn("output directory contains no old su files");

            globfree(&file_list);
            return 0;
        }
        if(status!=0){
            warn("failed to scan output directory %s",directory);
            globfree(&file_list);
            return -1;
        }
    }
    
    for(i=0 ;i<file_list.gl_pathc;i++){
        if(unlink(file_list.gl_pathv[i])!=0){
            warn("cannot delete old output file %s:%s",file_list.gl_pathv[i],strerror(errno));
            globfree(&file_list);
            return -1;
        }
        deleted_count++;

        if(verbose>=2)
            warn("delete old output file %s",file_list.gl_pathv[i]);
    }

    globfree(&file_list);

    if(verbose)
        warn("remove %d old su files %s",deleted_count,directory);
    
    return deleted_count;
}

void free_corr_workspace(CorrWorkspace *workspace){
    if(workspace==NULL) return;

    if(workspace->time_buffer!=NULL){
        free1float(workspace->time_buffer);
        workspace->time_buffer=NULL;
    }
    if(workspace->trace_spectrum!=NULL){
        free1complex(workspace->trace_spectrum);
        workspace->trace_spectrum=NULL;
    }
    if(workspace->pilot_spectrum!=NULL){
        free1complex(workspace->pilot_spectrum);
        workspace->pilot_spectrum=NULL;
    }
    workspace->trace_ns=0;
    workspace->pilot_ns=0;
    workspace->out_ns=0;

    workspace->nfft=0;
    workspace->nfreq=0;

    workspace->dt_sec=0;
}
