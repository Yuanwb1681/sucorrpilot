#include <stdio.h>
#include <time.h>
#include <string.h>
#include <glob.h>

#include "par.h"
#include "sucorrpilot.h"


char *sdoc[] = {
    "SUCORRPILOT - 批量完成 SU 数据与控制信号的互相关",
    "",
    "用法：",
    "  sucorrpilot input_dir=raw_su output_dir=corr_su ",
    "              pilot_dir=control_signal outtime=4.0 verbose=1",
    "",
    "  input_dir=      输入 SU 文件目录",
    "  output_dir=     输出 SU 文件目录",
    "  pilot_dir=      控制信号 SU 文件",
    "  outtime=4.0     互相关结果输出长度(s)",
    "  clean=1         删除输出目录中的su文件",
    "                  1=全部删除  0=保留并覆盖同名文件",
    "  verbose=1       是否输出运行信息",
    "",
    NULL
};

int main(int argc,char **argv){
    char *input_dir, *output_dir, *pilot_dir;
    
    float outtime;
    int verbose;
    int clean;

    glob_t file_list;
    glob_t pilot_list;

    char input_pattern[1024];
    char pilot_pattern[1024];
    char out_path[1024];

    const char *input_name;
    const char *pilot_name;

    FILE *fp_in;
    FILE *fp_out;
    FILE *fp_pilot;

    segy tr;
    segy pilot;
    segy out;

    size_t i;
    size_t total_files;

    int file_trace_count;
    long long total_trace_count=0;

    CorrWorkspace corr_workspace;
    int corr_initialized=0;

    clock_t start_time=clock();
    double elapsed_time;

    initargs(argc,argv);
    requestdoc(1);

    memset(&corr_workspace, 0, sizeof(corr_workspace));

    if(!getparstring("input_dir", &input_dir)) input_dir="02_raw_su";
    if(!getparstring("output_dir", &output_dir)) output_dir="03_corr_su";
    if(!getparstring("pilot_dir", &pilot_dir)) pilot_dir="05_control_signal";

    if(!getparfloat("outtime", &outtime)) outtime=4.0f;
    if(!getparint("clean", &clean)) clean=1;
    if(!getparint("verbose", &verbose)) verbose=1;

    checkpars();

    if(outtime<=0.0f) err("out time must >0");

    if(verbose){
        warn("input SU file is %s", input_dir);
        warn("pilot SU file is %s", pilot_dir);
        warn("output SU file is %s", output_dir);
        warn("output time is %f", outtime);
    }

    if(!ensure_directory(output_dir)) err("cannot prepare outpare output directory:%s",output_dir);
    if(strcmp(input_dir, output_dir)==0) err("input_dir and output_dir must not be the same");
    if(strcmp(pilot_dir, output_dir)==0) err("pilot_dir and output_dir must not be the same");
    if(clean){
        int removed_count;
        removed_count = clean_su_files(output_dir, verbose);
        if(removed_count<0)
            err("failed to clean output directory :%s",output_dir);
    }

    if(!join_path(pilot_pattern, sizeof(pilot_pattern), pilot_dir, "*.su"))
        err("failed to build pilot search pattern");

    memset(&pilot_list, 0, sizeof(pilot_list));

    if(glob(pilot_pattern, 0, NULL, &pilot_list)!=0)
        err("no pilot su file found in directory  :%s",pilot_dir);

    if(pilot_list.gl_pathc!=1){
        size_t pilot_count=pilot_list.gl_pathc;

        globfree(&pilot_list);

        err("pilot directory must contain exactly one su file, but found %lu",(unsigned long)pilot_count);
    }

    pilot_name=pilot_list.gl_pathv[0];

    if(verbose) warn("pilot file name is %s",pilot_name);

    fp_pilot=fopen(pilot_name,"rb");
    
    if(fp_pilot==NULL){
        globfree(&pilot_list);
        err("cannot open pilot file :%s",pilot_name);
    }

    if(!read_pilot_trace(fp_pilot, &pilot)){
        fclose(fp_pilot);
        err("failed to read pilot trace :%s",pilot_name);

        globfree(&pilot_list);
    }

    fclose(fp_pilot);
    fp_pilot=NULL;

    if(verbose){
        warn("pilot ns = %u",(unsigned int)pilot.ns);
        warn("pilot dt = %u",(unsigned int)pilot.dt);
    }

    if(!join_path(input_pattern, sizeof(input_pattern), input_dir, "*.su")){
        globfree(&pilot_list);
        err("failed to build input search pattern");
    }

    memset(&file_list, 0, sizeof(file_list));

    if(glob(input_pattern, 0, NULL, &file_list)!=0){
        globfree(&pilot_list);
        err("no input su file in directory : %s",input_dir);
    }

    total_files=file_list.gl_pathc;

    if(verbose)
        warn("found %lu input su files",(unsigned long)total_files);

    for (i=0; i<total_files; i++) {
        input_name=get_basename(file_list.gl_pathv[i]);

        if(input_name==NULL)
            err("failed to get basename form path %s",file_list.gl_pathv[i]);

        if(!join_path(out_path, sizeof(out_path), output_dir, input_name))
            err("failed to build output path for %s",input_name);

        if(verbose){
            warn("[%lu/%lu] processing %s", (unsigned long)(i + 1),
                (unsigned long)total_files, input_name);
        }

        fp_in=fopen(file_list.gl_pathv[i], "rb");
        if(fp_in==NULL)
            err("cannot open input file %s",file_list.gl_pathv[i]);

        fp_out=fopen(out_path, "wb");
        if (fp_out==NULL){
            fclose(fp_in);
            err("cannot create output file %s",out_path);
        }

        file_trace_count=0;
        while (fgettr(fp_in, &tr)) {
            if(!check_sampling_parameters(&tr,&pilot)){
                fclose(fp_in);
                fclose(fp_out);

                free_corr_workspace(&corr_workspace);
                
                err("sampling parameter mismatch: file=%s trace=%d",input_name,file_trace_count+1);
            }
            if(!corr_initialized){
                if(!init_corr_workspace(&corr_workspace, &pilot, tr.ns, outtime)){
                    fclose(fp_in);
                    fclose(fp_out);
                    err("failed to initialize FFT correlation workspace");
                }

                corr_initialized=1;

                if(verbose){
                    warn("FFT correlation initialized");
                    warn("trace ns  = %d", corr_workspace.trace_ns);
                    warn("pilot ns  = %d", corr_workspace.pilot_ns);
                    warn("nfft      = %d", corr_workspace.nfft);
                    warn("nfreq     = %d", corr_workspace.nfreq);
                    warn("output ns = %d", corr_workspace.out_ns);
                    warn("dt        = %.9f s", corr_workspace.dt_sec);
                }
                
            }
            if(!correlate_trace(&tr, &out, &corr_workspace)){
                fclose(fp_in);
                fclose(fp_out);
                free_corr_workspace(&corr_workspace);
                err("correlation failed : file=%s trace=%d",input_name,file_trace_count+1);
            }


            fputtr(fp_out, &out);
                
            file_trace_count++;

        }
        fclose(fp_in);
        fclose(fp_out);

        fp_in=NULL;
        fp_out=NULL;

        total_trace_count+=file_trace_count;
        
        if(verbose){
            warn("[%lu/%lu] completed %s: %d traces", (unsigned long)(i + 1), (unsigned long)total_files,
             input_name, file_trace_count);
        }
    
    }

    elapsed_time=(double)(clock()-start_time)/CLOCKS_PER_SEC;

    free_corr_workspace(&corr_workspace);

    globfree(&file_list);
    globfree(&pilot_list);

    if (verbose) {
        warn("all files completed");
        warn("total files  = %lu", (unsigned long)total_files);
        warn("total traces = %lld", total_trace_count);
        warn("elapsed time = %.3f s", elapsed_time);
}

    return EXIT_SUCCESS;
}


