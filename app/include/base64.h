/**************************************************************************
 * base64.h
 *
 *  Create on: 07/06/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform Sirius Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef BASE64_H
#define BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <stdint.h>
#include <stdlib.h>

/**************************************************************************
 * DEFINES
 **************************************************************************/
#define b64_malloc(ptr)         malloc(ptr)
#define b64_realloc(ptr, size)  realloc(ptr, size)

static const char b64_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/

/**
 * Encode `unsigned char *' source with `size_t' size.
 * Returns a `char *' base64 encoded string.
 */
char *b64_encode(const unsigned char *, size_t);

/**
 * Dencode `char *' source with `size_t' size.
 * Returns a `unsigned char *' base64 decoded string.
 */
unsigned char *b64_decode(const char *, size_t);

/**
 * Dencode `char *' source with `size_t' size.
 * Returns a `unsigned char *' base64 decoded string + size of decoded string.
 */
unsigned char *b64_decode_ex (const char *, size_t, size_t *);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_H */
