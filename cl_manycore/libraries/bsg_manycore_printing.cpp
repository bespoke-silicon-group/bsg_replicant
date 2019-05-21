#include <bsg_manycore_printing.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <map>
#include <string>

using std::map;
using std::string;

typedef struct prefix_info {
    FILE *file;
    bool newline;
} prefix_info_t;

typedef map<string, prefix_info_t> prefix_map;

static prefix_map fmap = {
    {BSG_PRINT_PREFIX_DEBUG, {BSG_PRINT_STREAM_DEBUG, true}},
    {BSG_PRINT_PREFIX_ERROR, {BSG_PRINT_STREAM_ERROR, true}},
    {BSG_PRINT_PREFIX_WARN,  {BSG_PRINT_STREAM_WARN,  true}},
    {BSG_PRINT_PREFIX_INFO,  {BSG_PRINT_STREAM_INFO,  true}},
};

int bsg_pr_prefix(const char *prefix, const char *fmt, ...)
{
    string prefix_string(prefix);
    string fmt_string(fmt);
    prefix_info_t *info;
    va_list ap;
    int r = -1, count = 0;
    bool newline;
    
    auto it = fmap.find(prefix_string);
    if (it != fmap.end()) {
        info = &it->second;
    } else {
        return r;
    }

    // lock our file to make our print atomic
    flockfile(info->file);    
    va_start(ap, fmt);
    newline = info->newline;
    
    string::size_type  cl_start = 0, cl_end;    
    do { // for each line in fmt     
        // find end of line
        cl_end = fmt_string.find('\n', cl_start);
        cl_end = (cl_end == string::npos ? fmt_string.size() : cl_end);
        
        // print prefix if this is the start of a new line
        if (newline)
            fprintf(info->file, "%s", prefix);        
        
        // print to the end of the line 
        string lfmt = fmt_string.substr(cl_start, cl_end-cl_start);
        
        count += vfprintf(info->file, lfmt.c_str(), ap);
        
        // move cl_start forward
        cl_start = cl_end;

        // decide on newline
        if (cl_end != fmt_string.size()) {
            // print the newline character and set 'newline' to true
            count += fprintf(info->file, "\n");
            cl_start++; // advance one so we don't print the 'newline' twice
            newline = true;
        } else {
            // this was the last line printed
            // if it was an empty line just set newline to false
            newline = lfmt.empty() ? true : false;
        }

        // until we've reached the end of our format string
    } while (cl_start < fmt_string.size());    
    
    // success
    r = count;
        
exit_func:
    // setup for the next call
    info->newline = newline;
    va_end(ap);
    funlockfile(info->file);
    return r;
}
#ifdef COSIM
uint64_t bsg_realtime(){
	uint64_t val;
	fprintf(stderr,"bsg_realtime()\n");
        sv_bsg_realtime(&val);
        return val;
}
#endif
