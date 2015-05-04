#include <stdio.h>
#include <string.h>

unsigned char palette[256 * 3];
char transparent[12]; // the red, green, blue of the transparent color, 
                     // as hex, with slashes between, prefaced by "rgb:"
                     // (for pnmto png)
int debug = 0;

int convert_cel(const char *celfile, const char *pnmfile) {
    FILE    *fpcel,
            *fppnm;
    unsigned char header[32],
             file_mark,
             bpp;
    int      height, width,
             offx, offy;// might need these later
    int      i, j;
    size_t   n_read;

    fpcel = fopen(celfile, "r");

    n_read = fread(header, 4, 1, fpcel);

    if (n_read < 1) {
        fprintf(stderr, "Unable to read header.\n");
        return -1;
    }

    if (strncmp ((const char *) header, "KiSS", 4)) {
        // if the header does NOT start with KiSS
        if (debug) {
            fprintf(stderr, "Old style KiSS cell.\n");
        }
        bpp = 4;
        width = header[0] + (256 * header[1]);
        height = header[2] + (256 * header[3]);
        offx = 0;
        offy = 0;
    }
    else {
        fread(header, 28, 1, fpcel);

        if (n_read < 1) {
            fprintf(stderr, "Unable to read rest of header.\n");
            return -1;
        }

        file_mark = header[0];
        if (file_mark != 0x20 && file_mark != 0x21) {
            fprintf(stderr, "%s is not a CEL image file.\n", celfile);
            return -1;
        }
        
        bpp = header[1];
        width = header[4] + (256 * header[5]);
        height = header[6] + (256 * header[7]);
        offx = header[8] + (256 * header[9]);
        offy = header[10] + (256 * header[11]);
    }

    if (debug) {
        fprintf(stderr, "Width: %d, height: %d \n", width, height);
        fprintf(stderr, "Offx: %d, offy: %d \n", offx, offy);
        fprintf(stderr, "Bits-per-pixel: %d \n", bpp);
    }

    if (width < 0 || height < 0) {
        fprintf(stderr, "Invalid width or height.");
        return -1;
    }

    // open fppnm stream
    fppnm = fopen(pnmfile, "w+");
    // write header
    fprintf(fppnm, "P3\n");
    fprintf(fppnm, "%d %d\n", width, height);
    fprintf(fppnm, "255 \n");

    for (i = 0; i < height  && !feof(fpcel); ++i) {
    // for each row in the picture

        switch (bpp) {
            case 4: 
                for (j = 0; j < (width+1)/2; j++) {
                // for every two pixels in each row
                    
                    // read one byte into the buffer
                    unsigned char buffer;
                    fread(&buffer, 1, 1, fpcel);
                    
                    // split the byte into two numbers
                    int num1 = (buffer & 0xf0) >> 4;
                    int num2 = buffer & 0x0f;

                    // print the numbers if in debug mode 
                    if (debug > 1) {
                        fprintf(stderr, "%d ", num1);
                        fprintf(stderr, "%d ", num2);
                    }

                    if ((width / 2 == 0) || j < (width/2)) {
                    // if this is not the end of the line
                        // lookup the R, G, and B values and print
                        // both pixels to the output file
                        fprintf(fppnm, "%u %u %u %u %u %u ", 
                            (unsigned) palette[num1*3], 
                            (unsigned) palette[num1*3+1],
                            (unsigned) palette[num1*3+2],
                            (unsigned) palette[num2*3], 
                            (unsigned) palette[num2*3+1],
                            (unsigned) palette[num2*3+2]);
                    }
                    else {
                    // if this is the end of the line
                        // print just the first pixel
                        fprintf(fppnm, "%u %u %u ", 
                            (unsigned) palette[num1*3], 
                            (unsigned) palette[num1*3+1],
                            (unsigned) palette[num1*3+2]);
                    }
                }

                break;
            case 8:
                for (j = 0; j < width; j++) {
                // for each pixel in each row
                    
                    // read one byte into the buffer,
                    unsigned char buffer;
                    fread(&buffer, 1, 1, fpcel);
                    
                    // convert the byte to an int
                    int num;
                    num = buffer;
                    
                    // print the number if in debug mode 
                    if (debug > 1) {
                        fprintf(stderr, "%d ", num);
                    }

                    // lookup the R, G, and B values 
                    fprintf(fppnm, "%u %u %u ", (unsigned) palette[num*3], 
                                                (unsigned) palette[num*3+1],
                                                (unsigned) palette[num*3+2]);
                }
        }
        // end the row with a newline
        fprintf(fppnm, "\n");
        if (debug > 1) {
            fprintf(stderr, "\n");
        }
    }
    
    fclose(fpcel);
    fclose(fppnm);

    return 0;
}

int read_palette(const char *palfile) {
    FILE   *fppal;
    char    header[32],
            file_mark,
            buffer[2],
            bpp;
    int     colors;
    int     i;
    size_t  n_read;

    fppal = fopen(palfile, "r");

    n_read = fread(header, 4, 1, fppal);
    if (n_read < 1) {
        fprintf(stderr, "Bad palette header.\n");
    }

    if (strncmp ((const char *) header, "KiSS", 4)) {
        if (debug) {           
            fprintf(stderr, "Old style palette\n");
        }

        colors = 16;

        // seeks back to the start of the file
        fseek(fppal, 0, SEEK_SET);

        // the rest of this is the same as "case 12" below"
        // DRY?
        // could pull the switch case out of the if and 
        // switch on colors?
        for (i = 0; i < colors; i++) {
            n_read = fread(buffer, 1, 2, fppal);

            if (n_read < 2) {
                fprintf(stderr, "Error reading palette.");
                return -1;
            }

            palette[i*3] = buffer[0] & 0xf0;
            palette[i*3+1]=(buffer[1] & 0x0f) * 16;
            palette[i*3+2]=(buffer[0] & 0x0f) * 16;
        }
    }
    else {
        fprintf(stderr, "New style palette\n");

        n_read = fread(header+4, 28, 1, fppal);
        if (n_read < 1) {
            fprintf(stderr, "Can't read palette header after \"KiSS\".\n");
        }
        
        file_mark = header[4];
        if (file_mark != 0x10) {
            fprintf(stderr, "Filemark should be %c and is actually %c.\n", 0x10, file_mark);
            return -1;
        }

        bpp = header[5];
        colors = header[8] + header[9] * 256;
        
        if (debug) {
            fprintf(stderr,"Bits per pixel: %d \n", bpp);
            fprintf(stderr, "Number colors: %d \n", colors);
        }
    
        switch (bpp) {
            case 12:
                //for each color
                for (i = 0; i < colors; i++) {
                    // read two bytes into the buffer
                    n_read = fread(buffer, 1, 2, fppal);
                    if (n_read < 2) {
                        fprintf(stderr, "Error while reading palette.");
                        return -1;
                    }

                    // three colors are stored in two bytes
                    //     buffer[0]    |    buffer[1]
                    //  0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7
                    //  color 1 color 3 |   ---   color 2 
                    palette[i*3] = buffer[0] & 0xf0;
                    palette[i*3+1]=(buffer[1] & 0x0f) * 16;
                    palette[i*3+2]=(buffer[0] & 0x0f) * 16;
                }
                break;
            case 24:
                // goes through fppal grabs 3 bytes colors times
                // and stores it in palette
                fread(palette, colors, 3, fppal);
                break;
            default:
                fprintf(stderr, "Invalid bits-per-pixel of %d", bpp);
        }
    }

    sprintf(transparent, "rgb:%x/%x/%x", (unsigned) palette[0], 
                                         (unsigned) palette[1],
                                         (unsigned) palette[2]);

    fclose(fppal);

    return 0;
}


int main (int argc, char *argv[]) {

    char *input_file;
    char *palette_file;
    char *output_file;

    if (argc < 4) {
        fprintf(stderr, "Usage: cel2png (-d) <cel file> <palette file> <out file> \n");
        return -1;
    }

    if (argc > 4) {
        
        if (strncmp(argv[1], "-d", 2) == 0) {
            if (strncmp(argv[1], "-d2", 3) == 0) {
                debug = 2;
            }
            else {
                debug = 1;
            }
            input_file = argv[2];
            palette_file = argv[3];
            output_file = argv[4];
        }
        else {
            fprintf(stderr, "Usage: cel2png (-d) <cel file> <palette file> <out file>\n");
            return -1;
        }
    }
    else {
        debug = 0;
        input_file = argv[1];
        palette_file = argv[2];
        output_file = argv[3];
    }

    fprintf(stderr,"Read palette %s \n", palette_file);
    read_palette (palette_file);
    fprintf(stdout, "%s", transparent);

    fprintf(stderr,"Read cel %s \n", input_file);
    convert_cel (input_file, output_file);
    
    fprintf(stderr,"Done \n");
    
    return 0;

}
