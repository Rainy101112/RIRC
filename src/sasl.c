#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "irc.h"

#include <openssl/evp.h>
#include <openssl/buffer.h>

char *base64_encode(const unsigned char *input, int length) {
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = '\0';

    BIO_free_all(b64);
    return buff;
}

unsigned char *base64_decode(const char *input, int length, int *out_len) {
    BIO *b64, *bmem;

    int max_decode_len = (length * 3) / 4 + 1;
    unsigned char *buff = (unsigned char *)malloc(max_decode_len);
    if (!buff) return NULL;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bmem = BIO_new_mem_buf((void *)input, length);
    bmem = BIO_push(b64, bmem);

    *out_len = BIO_read(bmem, buff, length);

    if (*out_len < 0) {
        free(buff);
        BIO_free_all(bmem);
        return NULL;
    }

    BIO_free_all(bmem);
    return buff;
}


