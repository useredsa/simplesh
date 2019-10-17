#include "psplit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>

#include "macros.h"

enum Mode { DEFAULT, LINES, BYTES };

const long int MAXPROCS = 25;
const long int MINPROCS = 1;
const long int MAXBSIZE = (1<<20);
const long int MINBSIZE = 1;
const enum Mode DEFAULT_MODE = BYTES;
const long int DEFAULT_LENGTH = 1000;
const long int DEFAULT_BSIZE = 1000;
const long int DEFAULT_PROCS = 1;

enum Mode mode = DEFAULT;   // Selected mode
long int length;            // Length of the file in bytes or lines, depending on mode
long int bsize;             // Size of the read and write blocks
long int procs;             // Number of processes to use

int min(int a, int b) { return (a < b ? a : b); }

/**
 * @brief Procesado de opciones
 * La función se llama psplit para los mensajes de error.
 */
int psplit(char **argv, int argc) {
    mode = DEFAULT;
    bsize = DEFAULT_BSIZE;
    procs = DEFAULT_PROCS;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, "hl:b:s:p:")) != -1) {
        switch (opt) {
            case 'l':
                if (mode == DEFAULT) {
                    mode = LINES;
                } else {
                    ERROR("Opciones incompatibles\n");
                    exit(-1);
                }
                // strtol returns 0 if the argument is not a number,
                // which we don't need to check as 0 is not a valid input.
                length = strtol(optarg, NULL, 0);
                if (length <= 0) {
                    ERROR("Opción -l no válida\n");
                    exit(-1);
                }
                break;

            case 'b':
                if (mode == DEFAULT) {
                    mode = BYTES;
                } else {
                    ERROR("Opciones incompatibles\n");
                    exit(-1);
                }
                length = strtol(optarg, NULL, 0);
                if (length <= 0) {
                    ERROR("Opción -b no válida\n");
                    exit(-1);
                }
                break;

            case 's':
                bsize = strtol(optarg, NULL, 0);
                if (bsize < MINBSIZE || bsize > MAXBSIZE) {
                    ERROR("Opción -s no válida\n");
                    exit(-1);
                }
                break;

            case 'p':
                procs = strtol(optarg, NULL, 0);
                if (procs < MINPROCS || procs > MAXPROCS) {
                    ERROR("Opción -p no válida\n");
                    exit(-1);
                }
                break;

            case 'h':
                printf(
                    "Uso: psplit [-l NLINES] [-b NBYTES] [-s BSIZE] [-p PROCS] "
                    "[FILE1] [FILE2]...\nOpciones:\n"
                    "     -l NLINES Número máximo de líneas por fichero.\n"
                    "     -b NBYTES Número máximo de bytes por fichero.\n"
                    "     -s BSIZE  Tamaño en bytes de los bloques leído de [FILEn] o stdin\n"
                    "     -p PROCS  Número máximo de procesos simultáneos.\n"
                    "     -h        Ayuda\n\n");
                exit(0);

            default:
            case '?':
            case ':':
                fprintf(stderr,
                    "Uso: psplit [-l NLINES] [-b NBYTES] [-s BSIZE] [-p PROCS] "
                    "[FILE1] [FILE2]...\n");
                exit(-1);
        }
    }

    if (mode == DEFAULT) {
        mode = DEFAULT_MODE;
        length = DEFAULT_LENGTH;
    }
    return 0;
}

int create_sub_file(char *basename, int number) {
    int numFileLen = 0;
    for (int i = number; i > 0; i/=10) numFileLen++;
    char fileName[strlen(basename) + numFileLen + 1];
    sprintf(fileName, "%s%d", basename, number);
    return creat(fileName, S_IRWXU);
}

void divide_in_bytes(char *basename) {
    int numFile = 0, outfd;
    TRY(outfd = create_sub_file(basename, numFile));
    int available = 0, canWrite = length;
    char buffer[2 * bsize];
    int r;
    do {
        TRY(r = read(0, buffer + available, bsize));
        available += r;
        int off = 0;
        // Write in blocks of bsize or to fill the remaining part of
        // the file
        while (available > bsize || (available > 0 && r == 0)) {
            if (canWrite == 0) {
                TRY(close(outfd));
                TRY(outfd = create_sub_file(basename, ++numFile));
                canWrite = length;
            }
            int toWrite = min(min(available, bsize), canWrite);
            int w;
            TRY(w = write(outfd, buffer + off, toWrite));
            canWrite -= w;
            off += w;
            available -= w;
        }
        for (int i = 0; i < available; i++) {
            buffer[i] = buffer[i + off];
        }
    } while (r > 0);
    TRY(close(outfd));
}

void divide_in_lines(char *basename) {
    int numFile = 0, outfd;
    TRY(outfd = create_sub_file(basename, numFile));
    int available = 0, canWrite = length;
    char buffer[2 * bsize];
    int r;
    do {
        TRY(r = read(0, buffer + available, bsize));
        available += r;
        int off = 0;
        // Write in blocks of bsize or to fill the remaining part of
        // the file
        while (available > bsize || (available > 0 && r == 0)) {
            if (canWrite == 0) {
                TRY(close(outfd));
                TRY(outfd = create_sub_file(basename, ++numFile));
                canWrite = length;
            }
            int toWrite = min(bsize, available);
            for (int i = off, neol = 0; i < off + toWrite; i++) {
                if (buffer[i] == '\n') {
                    if (++neol == canWrite) {
                        toWrite = i - off + 1;
                        break;
                    }
                }
            }
            int w;
            TRY(w = write(outfd, buffer + off, toWrite));
            for (int i = off; i < off + w; i++) {
                if (buffer[i] == '\n') {
                    canWrite--;
                }
            }
            off += w;
            available -= w;
        }
        for (int i = 0; i < available; i++) {
            buffer[i] = buffer[i + off];
        }
    } while (r > 0);
    TRY(close(outfd));
}

int run_psplit(char **argv, int argc) {
    int pid;
    if ((pid = fork_or_panic("fork start psplit")) == 0) {
        psplit(argv, argc);
        if (optind >= argc) {
            // no hay más argumentos que procesar
            // trabajamos con la entrada estándar
            if (fork_or_panic("fork psplit") == 0) {
                if (mode == BYTES) {
                    divide_in_bytes("stdin");
                } else {
                    divide_in_lines("stdin");
                }
                exit(0);
            }
            TRY(wait(0));
            exit(0); // we could control return value
        }

        int created = 0;
        //int return_stat = 0;
        while (optind < argc) {
            if (created >= procs) {
                // int status; // for use with wait to control return value
                TRY(wait(0));
                // if (status != 0)
                //     return_stat = status;
                created--;
            }
            if (fork_or_panic("fork psplit") == 0) {
                TRY(close(0));
                TRY(open(argv[optind], O_RDONLY));
                if (mode == BYTES) {
                    divide_in_bytes(argv[optind]);
                } else {
                    divide_in_lines(argv[optind]);
                }
                TRY(close(0));
                exit(0);
            }
            created++;
            optind++;
        }
        while (created > 0) {
            // int status; // for use with wait to control return value
            TRY(wait(0));
            // if (status != 0)
            //     return_stat = status;
            created--;
        }    
        exit(0);
    }
    waitpid(pid, 0, 0);
    return 0;
}
