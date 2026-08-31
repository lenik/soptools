#ifndef STREAM_COPY_HPP
#define STREAM_COPY_HPP

#include <stdio.h>

int copy_stream(FILE *in, FILE *out);
int copy_file(const char *prog, const char *path);

#endif /* STREAM_COPY_HPP */
