#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 100

int bin2h(FILE *f_in, FILE* f_out, char* name_ptr, char* ptr, char* out_name, char* file_ext, struct stat st) {
    off_t i;
    unsigned char png_byte, last_byte_octal, output_length;
    char define_name[FILENAME_MAX];

    if (file_ext && strlen(file_ext) > 0)
    {
        if (*ptr)
        {
            sprintf(define_name,"_%s%s", file_ext, ".h");
            strcpy(ptr, define_name);
        }
        else
        {
            strcat(out_name, "_");
            strcat(out_name, file_ext);
            strcat(out_name, ".h");
        }
    }
    else
    {
        if (*ptr)
        {
            strcpy(ptr, ".h");
        }
        else {
            strcat(out_name, ".h");
        }
    }

    f_out = fopen (out_name, "wb+");
    if (!f_out)
    {
        fprintf (stderr, "this requires the output file to be created\n");
        return -1;
    }

    fprintf (stderr, "creating %s, please hold on.\n", out_name);


    ptr = out_name;
    while (*ptr)
    {
        if (*ptr == '.') *ptr = '_';
        ptr++;
    }

    for (i=0; i<2; i++)
    {
        fprintf (f_out, "#%s", !i ? "ifndef " : "define ");
        fprintf (f_out, "_");
        ptr = out_name;
        while (*ptr)
        {
            fprintf (f_out, "%c", toupper(*ptr));
            ptr++;
        }
        fprintf (f_out, "_\n");
    }

    fprintf (f_out, "\n"
        "static const struct {\n"
        "\tsize_t size;\n"
        "\tunsigned char data [%lu];\n"
        "} ", (unsigned long)st.st_size);

    ptr = name_ptr;
    while (*ptr)
    {
        if (*ptr == '.')
            fprintf (f_out, "_");
        else
            fprintf (f_out, "%c", tolower(*ptr));
        ptr++;
    }

    fprintf (f_out, " = {\n"
        "\t%lu,\n\"", (unsigned long)st.st_size);

    last_byte_octal = 0;
    output_length = 1;
    for (i=0; i<st.st_size; i++)
    {
        png_byte = fgetc (f_in);
        if (png_byte == '\\')
        {
            output_length += 2;
            if (output_length >= MAX_LINE_LENGTH)
            {
                fprintf (f_out, "\"\n\"");
                output_length = 3;
            }
            fprintf (f_out, "\\\\");
            last_byte_octal = 0;
        }
        else if (png_byte == 0x09)
        {
            output_length += 2;
            if (output_length >= MAX_LINE_LENGTH)
            {
                fprintf (f_out, "\"\n\"");
                output_length = 3;
            }
            fprintf (f_out, "\\t");
            last_byte_octal = 0;
        }
        else if (png_byte == '?')
        {
            output_length += 2;
            if (output_length >= MAX_LINE_LENGTH)
            {
                fprintf (f_out, "\"\n\"");
                output_length = 3;
            }
            fprintf (f_out, "\\?");
            last_byte_octal = 0;
        }
        else if (png_byte < ' ' || png_byte == '\"')
        {
            output_length += (png_byte < 8) ? 2 : 3;
            last_byte_octal = 1;
            if (output_length >= MAX_LINE_LENGTH)
            {
                fprintf (f_out, "\"\n\"");
                output_length = (png_byte < 8) ? 3 : 4;
            }
            fprintf (f_out, "\\%o", png_byte);
        }
        else if (png_byte > '~')
        {
            output_length += 4;
            if (output_length >= MAX_LINE_LENGTH)
            {
                fprintf (f_out, "\"\n\"");
                output_length = 5;
            }
            fprintf (f_out, "\\x%X", png_byte);
            last_byte_octal = 1;
        }
        else
        {
            output_length += (last_byte_octal && isxdigit(png_byte)) ? 3 : 1;
            if (output_length >= MAX_LINE_LENGTH)
            {
                fprintf (f_out, "\"\n\"");
                output_length = 2;
                last_byte_octal = 0;
            }
            if (last_byte_octal && isxdigit(png_byte)) fprintf (f_out, "\"\"");
            fprintf (f_out, "%c", png_byte);
            last_byte_octal = 0;
        }
    }
    fprintf (f_out, "\"\n};\n\n#endif\n");

    fclose (f_in);
    fclose (f_out);
}

int h2bin(FILE *f_in, FILE* f_out, char* name_ptr, char* ptr, char* out_name, struct stat st) {
    off_t i;
    unsigned char png_byte;
    unsigned int start_line = 0;

    if (*ptr)
    {
        while(*ptr != '_') --ptr;
        *ptr = '\0';
        strcpy(ptr, ".png");
    }
    else
    {
        int pos = 0;
        int fn_len = strlen(out_name);
        for(pos = fn_len - 1; pos >= 0; pos--)
        {
            if (out_name[pos] == '_')
            {
                out_name[pos] = '\0';
                break;
            }
        }
        strcat(out_name, ".png");
    }

    f_out = fopen(out_name, "wb+");
    if (!f_out)
    {
        fprintf (stderr, "this requires the output file to be created\n");
        return -1;
    }

    fprintf (stderr, "creating %s, please hold on.\n", out_name);

    while (!feof(f_in))
    {
        png_byte = fgetc(f_in);
        if (png_byte == '"')
        {
            start_line = 1;
            break;
        }
    }

    while (!feof(f_in))
    {
        png_byte = fgetc(f_in);
        //printf("char: %c\n",png_byte);
        if (png_byte == 0x0D || png_byte == 0x0A)
        {
            start_line = 0;
        }
        else if (png_byte == '"')
        {
            if (!start_line) start_line = 1;
            else start_line = 1;
        }
        else if (png_byte == '\\')
        {
            png_byte = fgetc(f_in); // get next char
            if (png_byte == 't')
            {
                fprintf(f_out, "%c", 0x09);
            }
            else if (png_byte == '?')
            {
                fprintf(f_out, "%c", '?');
            }
            else if (png_byte == '\\')
            {
                fprintf(f_out, "%c", '\\');
            }
            else if (png_byte >= '0' && png_byte <= '9') // is octal
            {
                unsigned char octal_num[128] = {""};
                int i = 0;
                octal_num[i++] = png_byte;
                while (!feof(f_in))
                {
                    png_byte = fgetc(f_in);
                    octal_num[i++] = png_byte;
                    if ( !(png_byte >= '0' && png_byte <= '9') )
                    {
                        fseek(f_in, -1, SEEK_CUR);
                        break;
                    }
                }
                fprintf(f_out, "%c", strtol(octal_num, NULL, 8));
            }
            else if (png_byte == 'x')
            {
                png_byte = fgetc(f_in); // get next char
                if ( ((png_byte >= '0' && png_byte <= '9') || (png_byte >= 'A' && png_byte <= 'F')) ) // is octal
                {
                    unsigned char hex_num[128] = {""};
                    int i = 0;
                    hex_num[i++] = png_byte;
                    while (!feof(f_in))
                    {
                        png_byte = fgetc(f_in);
                        hex_num[i++] = png_byte;
                        if ( !((png_byte >= '0' && png_byte <= '9') || (png_byte >= 'A' && png_byte <= 'F')) )
                        {
                            fseek(f_in, -1, SEEK_CUR);
                            break;
                        }
                    }
                    fprintf(f_out, "%c", strtol(hex_num, NULL, 16));
                }
            }
        }
        else
        {
            if (start_line) fprintf (f_out, "%c", png_byte);
        }
    }

    fclose (f_in);
    fclose (f_out);
}

int main (int argc, char **argv)
{
    FILE *f_in, *f_out;
    char *name_ptr, *ptr;
    off_t i;
    struct stat st;
    char out_name[FILENAME_MAX] = {""}, file_ext[FILENAME_MAX] = {""};

    if (argc != 2)
    {
        fprintf (stderr, "this requires an input file\n");
        return -1;
    }

    f_in = fopen (argv[1], "rb");
    if (!f_in)
    {
        fprintf (stderr, "this requires the input file to exist\n");
        return -1;
    }

    if (stat(argv[1], &st))
    {
        fprintf (stderr, "this requires the input file to have no problems\n");
        return -1;
    }

    name_ptr = strrchr (argv[1], '/');      // *nix path
    if (!name_ptr)
        name_ptr = strrchr (argv[1], '\\'); // Wind*ws path
    if (name_ptr)
        name_ptr++;
    else
        name_ptr = argv[1];

    if (!*name_ptr || strlen(name_ptr) >= FILENAME_MAX)
    {
        fprintf (stderr, "this requires a reasonable length for the file name\n");
        return -1;
    }
    strcpy (out_name, name_ptr);

    ptr = out_name;
    while (*ptr)
    {
        //*ptr = tolower(*ptr);
        ptr++;
    }

    ptr = strrchr (out_name, '.');
    if (*ptr)
    {
        ++ptr;
        strcpy(file_ext, ptr);
    }
    --ptr;

    if (!strcmp(file_ext,"h"))
    {
        h2bin(f_in, f_out, name_ptr, ptr, out_name, st);
    }
    else
    {
        bin2h(f_in, f_out, name_ptr, ptr, out_name, file_ext, st);
    }

    fprintf (stderr, "done.\n");

    return 0;
}
