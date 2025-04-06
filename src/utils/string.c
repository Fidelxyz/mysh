#include "string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "minmax.h"

char *concat_path(const char *const base, const char *const path) {
    const size_t base_len = strlen(base);
    const size_t path_len = strlen(path);
    char *const  ret      = malloc(base_len + path_len + 2);
    char        *ret_end  = ret;

    ret_end    = memcpy(ret_end, base, base_len) + base_len;
    *ret_end++ = '/';
    ret_end    = memcpy(ret_end, path, path_len) + path_len;
    *ret_end   = '\0';

    return ret;
}

static void  *mepcat_dest   = NULL;
static size_t mepcat_destsz = 0;

void *mepcat(void *dest, size_t destsz, const void *src, size_t n) {
    if (dest != NULL) {
        mepcat_dest   = dest;
        mepcat_destsz = destsz;
    }

    if (src != NULL) {
        const size_t copy_n  = min(n, mepcat_destsz);
        mepcat_dest          = memcpy(mepcat_dest, src, copy_n) + copy_n;
        mepcat_destsz       -= copy_n;
    }

    return mepcat_dest;
}

/**
 * A node of a linked list of strings.
 */
struct StrList {
    char           *str;
    struct StrList *next;
};
typedef struct StrList StrList;

size_t tokenize(char *const str, char ***const tokens,
                const char *const delim) {
    assert(str != NULL);
    assert(tokens != NULL);

    StrList *const head = malloc(sizeof(StrList));
    memset(head, 0, sizeof(StrList));
    StrList *curr = head;

    size_t token_count = 0;
    char  *curr_token  = strtok(str, delim);
    while (curr_token != NULL) {
        token_count++;

        // go to a new node
        curr->next = malloc(sizeof(StrList));
        curr       = curr->next;
        // fill in the new node
        curr->str  = curr_token;
        curr->next = NULL;

        // next token
        curr_token = strtok(NULL, delim);
    }

    char **const ret_tokens = malloc((token_count + 1) * sizeof(char *));
    size_t       i          = 0;

    curr = head->next;
    free(head);
    while (curr != NULL) {
        ret_tokens[i++] = curr->str;

        StrList *tmp = curr;
        curr         = curr->next;
        free(tmp);
    }
    ret_tokens[i] = NULL;

    assert(i == token_count);

    *tokens = ret_tokens;
    return token_count;
}
