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
    
    file = fopen (filename, "rb");
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
 * Makes the changes in the data to turn it into a private game.
 * Returns 0 on possible success.
 * Returns non-zero on failure.d
 *   1: Save file too small
 *   2: Too many matches
 *   3: Not enough matches
 */
static int make_private(const struct SaveData * save_data) {
    /* We want to find this twice, and replace [7] (0x01) with 0x03 */
    const unsigned char pattern_1[] = 
    {
        0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40
    };
    /* We want to find this once, and switch the third byte back from this
     * from 0x00 -> 0x01.
     */
    const unsigned char pattern_2[] =
    {
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x40
    };
    
    /* our current position in the data */
    int position = 0;
    /* count the number of occurrences; if it's not two, we've failed */
    int match_count = 0;
    
    if ( save_data->size < sizeof(pattern_1) ) {
        /* ERROR: Not enough data in file */
        return 1;
    }
    
    /* loop through the whole file, looking for a match */
    while (position <= save_data->size - sizeof(pattern_1)) {
        int i;
        /* check for the pattern, then move forward one byte and check again,
         * until we find it or don't.
         */
        for (i = 0; i < sizeof(pattern_1); i++) {
            if (save_data->contents[position + i] != pattern_1[i]) {
                break;
            }
            if (i == sizeof(pattern_1) - 1) {
                fprintf(stdout, "Found pattern 1 at position %x.\n", position);
                /* count this occurrence */
                match_count++;
                /* make the change */
                save_data->contents[position + 7] = 0x03;
            }
            if (match_count > 2) {
                /* too many matches */
                fprintf(stderr, "Found too many matches of pattern 1.\n");
                return 2;
            }
        }
        position++;
    }
    
    if (match_count != 2) {
        /* exactly two matches, or we've failed */
        fprintf(stderr, "Found too few matches of pattern 1.\n");
        return 3;
    }
    
    /* START SECOND PATTERN */
    /* our current position in the data */
    position = 0;
    /* count the number of occurrences; if it's not two, we've failed */
    match_count = 0;
    
    /* loop through the whole file, looking for a match */
    while (position <= save_data->size - sizeof(pattern_2)) {
        int i;
        /* check for the pattern, then move forward one byte and check again,
         * until we find it or don't.
         */
        for (i = 0; i < sizeof(pattern_2); i++) {
            if (save_data->contents[position + i] != pattern_2[i]) {
                break;
            }
            if (i == sizeof(pattern_2) - 1) {
                /* make the change */
                /* don't go beyond the beginning of the file */
                if ( (save_data->contents[position - 3] == 0x00) && ((position - 3) >= 0) ) {
                    fprintf(stdout, "Found pattern 2 at position %x.\n", position);
                    /* count this occurrence */
                    match_count++;
                    save_data->contents[position - 3] = 0x01;
                }
            }
            if (match_count > 1) {
                /* too many matches */
                fprintf(stderr, "Found too many matches of pattern 2.\n");
                return 2;
            }
        }
        position++;
    }
    
    if (match_count != 1) {
        /* exactly one match, or we've failed */
        fprintf(stderr, "Found no matches of pattern 2.\n");
        return 3;
    }
    
    return 0;
}

/*
 * 
 */
int main(int argc, char** argv) {
    
    const char * infilename = "111Wu Zetian_0000 BC-4000.Civ5Save";
    const char * outfilename = "out.Civ5Save";
    struct SaveData save_data;
    FILE * outfile;
    size_t bytes_written;
    
    read_file(infilename, &save_data);
    
    /* read in file contents */
    fprintf(stdout, "Read %d bytes into memory.\n", save_data.size);
    
    /* Modify the data */
    if (make_private(&save_data) != 0) {
        fprintf(stdout, "Failure identifying bytes to change.\n");
        exit(EXIT_FAILURE);
    }
    
    /* write out new file */
    outfile = fopen (outfilename, "wb");
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
    fprintf(stdout, "Wrote new file '%s'.\n", outfilename);
    
    return (EXIT_SUCCESS);
}
