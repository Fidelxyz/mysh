#include "cd.h"

#include <stddef.h>

#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../io_helpers.h"
#include "../utils/string.h"

#define MAX_PATH           (MAX_STR_LEN * 2)
#define PATH_DELIM         '/'
#define EXPANDED_TRIP_DOTS "../../"
#define EXPANDED_QUAD_DOTS "../../../"

typedef struct {
    const char *path;
} CdArgs;

static RetVal parse_cd_args(CdArgs *args, const size_t argc,
                            char *const *const argv) {
    *args = (CdArgs){.path = NULL};

    for (size_t i = 1; i < argc; i++) {
        const char *token = argv[i];

        if (args->path != NULL) {
            display_error(
                "ERROR: Too many arguments: cd takes a single path\n");
            return RETVAL_FAILURE;
        }

        args->path = token;
    }

    if (args->path == NULL) {
        args->path = ".";
    }

    return RETVAL_SUCCESS;
}

/**
 * @warning The caller is responsible for freeing the returned string.
 */
char *expand_path(const char *path) {
    char *expanded      = malloc(MAX_PATH + 1);
    char *expanded_tail = mepcat(expanded, MAX_PATH, NULL, 0);

    const char *part_begin = path;
    const char *part_end;
    do {
        part_end = strchrnul(part_begin, PATH_DELIM);
        // *part_end == '/' || *part_end == '\0'

        if (strncmp(part_begin, ".../", 4) == 0 ||
            strncmp(part_begin, "...\0", 4) == 0) {
            // expand triple dots
            expanded_tail =
                mepcat(NULL, 0, EXPANDED_TRIP_DOTS, strlen(EXPANDED_TRIP_DOTS));

        } else if (strncmp(part_begin, "..../", 5) == 0 ||
                   strncmp(part_begin, "....\0", 5) == 0) {
            // expand quadruple dots
            expanded_tail =
                mepcat(NULL, 0, EXPANDED_QUAD_DOTS, strlen(EXPANDED_QUAD_DOTS));

        } else {
            expanded_tail =
                mepcat(NULL, 0, part_begin, part_end + 1 - part_begin);
        }

        assert(expanded_tail < expanded + MAX_PATH);

        part_begin = part_end + 1;
    } while (*part_end != '\0');

    *expanded_tail = '\0';

    return expanded;
}

RetVal bn_cd(const size_t argc, char *const *const argv) {
    CdArgs args;
    if (FAILED(parse_cd_args(&args, argc, argv))) {
        return RETVAL_FAILURE;
    };

    char *const path = expand_path(args.path);
    DEBUG_PRINT("DEBUG: cd to %s\n", path);

    if (chdir(path) != 0) {
        display_error("ERROR: Invalid path\n");
        free(path);
        return RETVAL_FAILURE;
    }

    free(path);
    return RETVAL_SUCCESS;
}
