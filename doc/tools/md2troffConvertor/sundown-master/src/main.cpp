/*
 * Copyright (c) 2011, Vicent Marti
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#if defined (_WIN32)
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include "markdown.h"
#include "renderer.h"
#include "buffer.h"

#if defined (_WIN32)
#define NEW_LINE                      "\r\n"
#else
#define NEW_LINE                      "\n"
#endif

#define MD_EXCLUDE_BEGIN              "<!--manpage_exclude_begin-->"
#define MD_EXCLUDE_END                "<!--manpage_exclude_end-->"
#define TROFF_INDENT_BEGEIN_W         ".RS 4\r\n.IP\r\n"
#define TROFF_INDENT_END_W            TROFF_INDENT_BEGEIN_W
#define TROFF_INDENT_BEGEIN_L         ".RS 4\n.IP\n"
#define TROFF_INDENT_END_L            TROFF_INDENT_BEGEIN_L

#define FINE_NAME_LEN                 128
#define READ_UNIT                     1024
#define OUTPUT_UNIT                   64

static void _usage()
{
    fprintf(stderr,
        "usage: ./md2TroffTool -i input_file -o output_file [-d] [-p]"NEW_LINE);
    exit(-1);
}

static void _writeFile(const struct buf *b, const char *fileName)
{
    FILE *file_fd = NULL;
    file_fd = fopen(fileName, "w");
    fwrite(b->data, 1, b->size, file_fd);
    fclose(file_fd);
}

static int _filterContents(struct buf *b, const char *pBegin, const char *pEnd)
{
    int rc    = 0;
    const char *f   = NULL;
    const char *e   = NULL;

retry:
    f = NULL, e = NULL;
    f = (const char *)strstr((const char*)(b->data), pBegin);
    e = (const char *)strstr((const char*)(b->data), pEnd);
    if (NULL == f && NULL == e)
    {
        goto done;
    }
    else if (NULL !=f && NULL == e)
    {
        fprintf(stderr,"Error: Lack %s in the source file"NEW_LINE, pEnd);
        rc = -1; 
        goto error;
    }
    else if (NULL == f && NULL != e)
    {
        fprintf(stderr,"Error: Lack %s in the source file"NEW_LINE, pBegin);
        rc = -1; 
        goto error;
    }
    e += strlen(pEnd);
    /* we get both positions, let's filter the content */
    {
        int len = e - f;
        int size = b->size - (e - (const char*)(b->data));
        memmove((void *)f, e, size);
        memset((void *)(f + size), 0, len);
        b->size -= len;
        goto retry;
    }
done:
    return rc;
error:
    goto done;
}

static int _replace ( struct buf* src, const char* repchar)
{
    int rc = 0;
    const char* p = NULL;

retry:
    p = NULL;
    p = (const char*)strstr((const char*)(src->data),repchar);
    if (NULL == p)
    {
        goto done;
    }
    {
        memmove((void *)p, p+1, strlen(p)-1);
        src->size--;
        goto retry ;
    }
done:
    return rc;
}

/* main - main function, interfacing STDIO with the parser */
int main(int argc, char **argv)
{
    struct buf *ib   = NULL;
    struct buf *ob   = NULL;
    FILE *in         = NULL;
    int rc           = 0;
    int ret          = -1;
    int debug        = 0;
    int handle_troff = 0;
    int show_usage   = 0;
    int c            = -1;
    char in_file[FINE_NAME_LEN]  = { 0 };
    char out_file[FINE_NAME_LEN] = { 0 };

    struct sd_callbacks callbacks;
    struct sd_markdown *markdown = NULL;

    while (-1  != (c = getopt(argc, argv, "i:o:dp")))
    {
        switch(c)
        {
        case 'i':
            strncpy(in_file, optarg, FINE_NAME_LEN);
            show_usage |= 1;
            break;
        case 'o':
            strncpy(out_file, optarg, FINE_NAME_LEN);
            show_usage |= 2;
            break;
        case 'd':
            debug = 1;
            break;
        case 'p':
            handle_troff = 1;
            break;
        default:
            show_usage |= 4;
            break;
        }
    }

    if (0 == (show_usage & 1) ||
        0 == (show_usage & 2) ||
        1 == (show_usage & 4))
    {
        _usage();
    }

    /* opening the file if given from the command line */
    in = fopen(in_file, "r");
    if (!in) {
        fprintf(stderr,"Unable to open input file \"%s\": %s"NEW_LINE, 
                in_file, strerror(errno));
        rc = -1;
        goto error;
    }

    /* reading everything from input */
    ib = bufnew(READ_UNIT);
    bufgrow(ib, READ_UNIT);
    while ((ret = fread(ib->data + ib->size, 1, ib->asize - ib->size, in)) > 0) {
        ib->size += ret;
        bufgrow(ib, ib->size + READ_UNIT);
    }
    fclose(in);

    if (handle_troff)
    {
        /* filter content */
        _filterContents(ib, TROFF_INDENT_BEGEIN_W, TROFF_INDENT_END_W);
        _filterContents(ib, TROFF_INDENT_BEGEIN_L, TROFF_INDENT_END_L);
        //replace the abnormal blank spcaces
        _replace(ib, "\\ ") ;
        _writeFile(ib, out_file);
        goto done;
    }

    /* filter content */
    _filterContents(ib, MD_EXCLUDE_BEGIN, MD_EXCLUDE_END);

    /* show the filtered file contents*/
    if (debug)
    {
        char tmp_file[FINE_NAME_LEN] = { 0 };
        int len = (strlen(out_file) <= (FINE_NAME_LEN - 7)) ? 
            strlen(out_file) : (FINE_NAME_LEN - 7);
        strncpy(tmp_file, out_file, len);
        strncpy(tmp_file + len, ".debug", 7);
        _writeFile(ib, tmp_file);
    }

    /* performing markdown parsing */
    ob = bufnew(OUTPUT_UNIT);

    pandoc_markdown_renderer(&callbacks);
    markdown = sd_markdown_new(0, 16, &callbacks, NULL);

    /* convert from github's markdown to pandoc's markdown */
    sd_markdown_render(ob, ib->data, ib->size, markdown);
    sd_markdown_free(markdown);

    /* flush the result to file */
    _writeFile(ob, out_file);
    
done:
    /* cleanup */
    if (NULL != ib) bufrelease(ib);
    if (NULL != ob) bufrelease(ob);
    return rc;
error:
    goto done;
}

/* vim: set filetype=c: */
