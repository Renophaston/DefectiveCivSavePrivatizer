/* 
 * File:   main.c
 * Author: Renophaston <Renophaston@users.noreply.github.com>
 *
 * Created on July 13, 2014, 8:49 AM
 * 
 * TODO: Get filenames from the command line
 * TODO: check whether files exist (both read and write)
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>


/*
 * Holds the contents of a Civ V save file and the size of that data.
 */
struct SaveData {
    unsigned char * contents;
    unsigned int size;
};

/*
 * Returns the size of the file named by filename
 */
static unsigned int get_file_size(const char * filename) {
    struct stat file_stats;
    if ( stat(filename, &file_stats) != 0 ) {
        fprintf(stderr, "'stat' failed for '%s': %s.\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return file_stats.st_size;
}

/*
 * Fills save_data with the contents of filename and size of data.
 * REMEMBER TO FREE save_data.contents!
 * 
 * Returns 0 on success, non-zero on failure.  (But actually I think we exit
 * on any possible failure.)
 */
static int read_file(const char * filename, struct SaveData * save_data) {
    unsigned int size;
    unsigned char * contents;
    FILE * file;
    size_t bytes_read;
    int status;
    
    size = get_file_size(filename);
    contents = malloc(size + 1);
    if (!contents) {
        fprintf(stderr, "Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    
    file = fopen (filename, "r");
    if (!file) {
        fprintf(stderr, "Unable to open file '%s' for reading: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    bytes_read = fread(contents, sizeof(unsigned char), size, file);
    /* FIXME: comparing unsigned int to size_t. Does that matter? */
    if (bytes_read != size) {
        fprintf(stderr, "Error reading %s; expected %d bytes but got %d: %s\n", filename, size, bytes_read, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    status = fclose(file);
    if (status != 0) {
        fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* Fill the struct */
    save_data->contents = contents;
    save_data->size = size;
    
    /* Potential success! */
    return 0;
}

/*
 * 
 */
int main(int argc, char** argv) {
    
    const char * infilename = "CurrentGame_AI.Civ5Save";
    const char * outfilename = "out.Civ5Save";
    struct SaveData save_data;
    FILE * outfile;
    size_t bytes_written;
    
    read_file(infilename, &save_data);
    
    /* read in file contents */
    fprintf(stdout, "Read %d bytes into memory.\n", save_data.size);
    
    /* write out new file */
    outfile = fopen (outfilename, "w");
    if (!outfile) {
        fprintf(stderr, "Unable to open file '%s' for reading: %s\n", outfile, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    bytes_written = fwrite(save_data.contents, sizeof(save_data.contents[0]), save_data.size, outfile);
    fclose(outfile);
    
    free (save_data.contents);
    
    if (bytes_written != save_data.size) {
        fprintf(stderr, "Error writing '%s'; expected %d bytes but wrote %d: %s\n", outfile, save_data.size, bytes_written, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Wrote new file '%s'.", outfilename);
   
    return (EXIT_SUCCESS);
}
