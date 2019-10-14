#ifndef SIMPLESH_PSPLIT_H
#define SIMPLESH_PSPLIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>

#include "simplesh_macros.h"

int min(int a, int b) { return (a < b ? a : b); }

int run_psplit(char **argv, int argc) {
    if (fork_or_panic("fork PSPLIT") == 0) {
        int opt, flag, n;
        enum Mode { DEFAULT, LINES, BYTES } mode = DEFAULT;
        const long int MAXPROCS = 25;
        long int nlines = 1, nbytes = 1000, bsize = 1000, procs = 1;

        // Options processing
        optind = 1;
        while ((opt = getopt(argc, argv, "l:b:s:p:h")) != -1) {
            switch (opt) {
                case 'l':
                    if (mode == DEFAULT) {
                        mode = LINES;
                    } else {
                        puts("psplit: Opciones incompatibles");
                        exit(-1);
                    }
                    // strtol returns 0 if the argument is not a number,
                    // which we don't need to check as 0 is not a valid input.
                    nlines = strtol(optarg, NULL, 0);
                    break;

                case 'b':
                    if (mode == DEFAULT) {
                        mode = BYTES;
                    } else {
                        puts("psplit: Opciones incompatibles");
                        exit(-1);
                    }
                    nbytes = strtol(optarg, NULL, 0);
                    break;

                case 's':
                    bsize = strtol(optarg, NULL, 0);
                    if (bsize < 1 || bsize > (1 << 20)) {
                        puts("psplit: Opción -s no válida");
                        exit(-1);
                    }
                    break;

                case 'p':
                    procs = strtol(optarg, NULL, 0);
                    if (procs < 1 || procs > MAXPROCS) {
                        puts("psplit: Opción -p no válida");
                        exit(-1);
                    }
                    break;

                case 'h':
                    puts(
                        "Uso: psplit [-l NLINES] [-b NBYTES] [-s BSIZE] [-p "
                        "PROCS] "
                        "[FILE1] [FILE2]...");
                    puts("     Opciones:");
                    puts("     -l NLINES Número máximo de líneas por fichero.");
                    puts("     -b NBYTES Número máximo de bytes por fichero.");
                    puts(
                        "     -s BSIZE  Tamaño en bytes de los bloques leído "
                        "de [FILEn] o stdin.");
                    puts(
                        "     -p PROCS  Número máximo de procesos "
                        "simultáneos.");
                    puts("     -h        Ayuda.");
                    exit(0);
                    break;

                default:
                case '?':
                case ':':
                    puts(
                        "Uso: psplit [-l NLINES] [-b NBYTES] [-s BSIZE] [-p "
                        "PROCS] "
                        "[FILE1] [FILE2]...");
                    break;
            }
        }

        if (optind >= argc) {
            // TODO entrada estandar
            exit(0);
        }

        fprintf(stderr, "name: %s\n", argv[optind]);
        switch (mode) {
            case LINES:

                break;

            default:
                nbytes = 1000;
            case BYTES:;
                int infd = 0, outfd = -1, numFile = 0, numFileLen = 1;
                int available = 0, canWrite = 0;
                char buffer[2 * bsize];
                int r;
                do {
                    r = read(infd, buffer + available, bsize);
                    available += r;
                    int off = 0;
                    // Write in blocks of bsize or to fill the remaining part of
                    // the file
                    while (available > bsize || (available > 0 && r == 0)) {
                        if (canWrite == 0) {
                            if (close(outfd) < 0) {
                                // TODO ur fucked
                            }
                            numFile++;
                            if (numFile % 10 == 0) numFileLen++;
                            char
                                fileName[strlen(argv[optind]) + numFileLen + 1];
                            sprintf(fileName, "%s%d", argv[optind], numFile);
                            if ((outfd = creat(fileName, S_IRWXU)) == -1) {
                                // TODO ur fucked
                            }
                        }
                        int w =
                            write(outfd, buffer + off, min(bsize, canWrite));
                        if (w == -1) {
                            // TODO ur fucked
                        }
                        off += w;
                        canWrite -= w;
                        available -= w;
                    }
                    for (int i = 0; i < available; i++) {
                        buffer[i] = buffer[i + off];
                    }
                } while (r > 0);
                if (r == -1) {
                    // TODO ur fucked
                }
                break;
        }
        exit(0);
    } else {
        TRY(wait(NULL));
    }
    return 0;
}

#endif
