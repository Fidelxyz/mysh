#ifndef __UTILS_STRING_H__
#define __UTILS_STRING_H__

#include <unistd.h>

/**
 * @brief Concatenates two paths into a single path string.
 *
 * @param base The base path string to concatenate
 * @param path The path string to append to the base
 * @return A newly allocated string containing the concatenated path, or NULL if
 * memory allocation fails.
 *
 * @warning The caller is responsible for freeing the returned string.
 */
char *concat_path(const char *base, const char *path);

/**
 * @brief Concatenates memory areas.
 *
 * On the first call, the desination buffer and its size should be specified in
 * dest and destsz. In each subsequent call that should append to the same
 * buffer, dest must be NULL.
 *
 * @param dest    Pointer to the destination buffer on the first call, or NULL
 * for subsequent calls
 * @param destsz  Maximum size allowed for dest on the first call
 * @param src     Pointer to the source buffer to copy from
 * @param n       Number of bytes to copy from source
 *
 * @return Pointer to the position after the last written byte in destination
 *
 * @note This function uses static variables mepcat_dest and mepcat_destsz to
 * maintain state
 * @note When dest is NULL, continues writing from last position using previous
 * state
 * @note Actual copied bytes will be minimum of n and remaining destination size
 */
void *mepcat(void *dest, size_t destsz, const void *src, size_t n);

/**
 * @brief Tokenize a string into an array of strings.
 *
 * @param str    The string to tokenize.
 * @param tokens Pointer receving the array of tokens. The array of tokens is
 * terminated by a NULL pointer.
 * @param delim  The delimiter to use for tokenization.
 *
 * @return The number of tokens in the array.
 * @note The caller is responsible for freeing the returned array and its
 * contents.
 * @note The output array tokens point to str, so str should live as long as
 * tokens.
 */
size_t tokenize(char *str, char ***tokens, const char *delim);

#endif
