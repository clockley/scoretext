#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <math.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <kcgi.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <curl/curl.h>
#include "grouptext.h"
#include <magic.h>
#include "pool.h"

#define DOCCONVERSIONURL "http://localhost:4567/uploadPDF"

typedef struct MemoryStruct {
	char *memory;
	size_t size;
} MemoryStruct;


bool loadAndReadPDFFile(char *, uint32_t, char **);