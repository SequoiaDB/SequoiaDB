/*
 * Copyright (c) 2009, Natacha Porte
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "markdown.h"
//#include "ossVer.h"
#include "renderer.h"


#if defined (_WIN32)
#define snprintf _snprintf
#endif


/* block level callbacks - NULL skips the block */
// 1 void (*blockcode)(struct buf *ob, const struct buf *text, const struct buf *lang, void *opaque);
// 2 void (*blockquote)(struct buf *ob, const struct buf *text, void *opaque);
// 3 void (*blockhtml)(struct buf *ob,const  struct buf *text, void *opaque);
// 4 void (*header)(struct buf *ob, const struct buf *text, int level, void *opaque);
// 5 void (*hrule)(struct buf *ob, void *opaque);
// 6 void (*list)(struct buf *ob, const struct buf *text, int flags, void *opaque);
// 7 void (*listitem)(struct buf *ob, const struct buf *text, int flags, void *opaque);
// 8 void (*paragraph)(struct buf *ob, const struct buf *text, void *opaque);
// 9 void (*table)(struct buf *ob, const struct buf *header, const struct buf *body, void *opaque);
// 10 void (*table_row)(struct buf *ob, const struct buf *text, void *opaque);
// 11 void (*table_cell)(struct buf *ob, const struct buf *text, int flags, void *opaque);

/* span level callbacks - NULL or return 0 prints the span verbatim */
// 12 int (*autolink)(struct buf *ob, const struct buf *link, enum mkd_autolink type, void *opaque);
// 13 int (*codespan)(struct buf *ob, const struct buf *text, void *opaque);
// 14 int (*double_emphasis)(struct buf *ob, const struct buf *text, void *opaque);
// 15 int (*emphasis)(struct buf *ob, const struct buf *text, void *opaque);
// 16 int (*image)(struct buf *ob, const struct buf *link, const struct buf *title, const struct buf *alt, void *opaque);
// 17 int (*linebreak)(struct buf *ob, void *opaque);
// 18 int (*link)(struct buf *ob, const struct buf *link, const struct buf *title, const struct buf *content, void *opaque);
// 19 int (*raw_html_tag)(struct buf *ob, const struct buf *tag, void *opaque);
// 20 int (*triple_emphasis)(struct buf *ob, const struct buf *text, void *opaque);
// 21 int (*strikethrough)(struct buf *ob, const struct buf *text, void *opaque);
// 22 int (*superscript)(struct buf *ob, const struct buf *text, void *opaque);

/* low level callbacks - NULL copies input directly into the output */
// 23 void (*entity)(struct buf *ob, const struct buf *entity, void *opaque);
// 24 void (*normal_text)(struct buf *ob, const struct buf *text, void *opaque);

/* header and footer */
// 25 void (*doc_header)(struct buf *ob, const struct buf *text, void *opaque);
// 26 void (*doc_footer)(struct buf *ob, void *opaque);

int list_level = -1 ;
int sequence = 0 ;

#define DEFAULT_BUILD_TIME "2016-12-01"
#define DEFAULT_FUNC_NAME  "Method"
#define HEADER_NAME_ZN     "##名称##"
#define HEADER_NAME_EN     "##NAME##"
#define HEADER_SYNOPSIS_ZN "##语法##"
#define HEADER_SYNOPSIS_EN "##SYNOPSIS##"


/*
void _get_build_time( char *buf, int size )
{
   const char *build = SDB_ENGINE_BUILD_CURRENT ;
   int len           = 0 ;
   const char *pos         = NULL ;
   
   assert( buf && size > 0 ) ;
   memset( buf, 0, size ) ;
   
   // get build time
   pos = strrchr( build, '-' ) ;
   if ( pos ) {
      int len = pos - build ;
      len = len < size ? len : size - 1 ;
      strncpy( buf, build, len ) ;
   } else {
      len = strlen(DEFAULT_BUILD_TIME) ;
      len = len < size ? len : size - 1 ;
      strncpy( buf, DEFAULT_BUILD_TIME, len )  ;
   }
   buf[len] = '\0' ;
}

void _get_version( char *buf, int size )
{
   assert( buf && size > 4 ) ;
   memset( buf, 0, size ) ;
   
   snprintf( buf, size, "%d.%d", 
             SDB_ENGINE_VERISON_CURRENT, 
             SDB_ENGINE_SUBVERSION_CURRENT ) ;
}
*/
void _get_func_name( const struct buf *text, char *buf, int size )
{   
   const char *beg = NULL ;
   const char *end = NULL ;
   int len = 0, len2 = 0 ;
   int name_len = 0;
   int i = 0 ;
   char *p = NULL ;

   assert( buf && size > 0 ) ;
   memset( buf, 0, size ) ;
   if ( !text || !text->data || 0 == text->size ) {
      strncpy( buf, DEFAULT_FUNC_NAME, size - 1 ) ;
      goto done ;
   }

   beg = strstr( (const char *)text->data, HEADER_NAME_ZN ) ;
   name_len = strlen( HEADER_NAME_ZN ) ;
   if (!beg) {
       beg = strstr( (const char *)text->data, HEADER_NAME_EN ) ;
       name_len = strlen( HEADER_NAME_EN ) ;
   }
   end = strstr( (const char *)text->data, HEADER_SYNOPSIS_ZN ) ;
   if (!end) {
       end = strstr( (const char *)text->data, HEADER_SYNOPSIS_EN ) ;
   }
   if ( !beg || !end ) {
      strncpy( buf, DEFAULT_FUNC_NAME, size - 1 ) ;
      goto done ;
   }

   beg += name_len ;
   len = end - beg ;
   if ( 0 == len ) {
      strncpy( buf, DEFAULT_FUNC_NAME, size - 1 ) ;
      goto done ;
   }
   
   p = (char *)malloc( len + 1 ) ;
   if ( !p ) {
      strncpy( buf, DEFAULT_FUNC_NAME, size - 1 ) ;
      goto done ;
   }
   memset( p, 0, len + 1 ) ;
   len2 = text->size - (beg - (const char *)text->data) ;
   len = (len < len2) ? len : len2 ;
   strncpy( p, beg, len ) ;
   p[len] = '\0' ;
   // trim useless characters
   for ( i = 0; i < len; i++ ) {
      if ( '\n' == p[0] || '\r' == p[0] || ' ' == p[0] ) {
         memmove( p, p + 1, strlen(p + 1) + 1 ) ;
         continue ;
      } else {
         break ;
      }
   }
   // get out of the function name
   beg = p ;
   end = strchr( beg, ' ' ) ;
   len = end - beg < size ? end - beg : size - 1 ;
   strncpy( buf, beg, len ) ;
   buf[len] = '\0' ;
   
done:
   if ( p ) {
      free( p ) ;
   }
   return ;
}

int _handle_specified_chars( struct buf *buf, 
                             const char *src, int len, 
                             const char *chars, int num )
{
   int i = 0 ;
   int j = 0 ;
   
   assert( src && len > 0 ) ;
   // for 'len' does not contain '\0', so we use "i <= len"
   for ( i = 0; i < len; i++ ) 
   {
      for ( j = 0; j < num; j++ )
      {
         if ( src[i] == chars[j] )
         {
            bufputc( buf, '\\' ) ;
            break ;
         }
      }
      bufputc( buf, src[i] ) ;
   }
   return 1;
}

int _has_fenced_chars( const char *src, int len )
{
   int i = 0, n = 0;
   char c;
   
   assert( src && len > 0 ) ;
   /* skipping initial spaces */
   for( i = 0; i < len && src[i] == ' '; i++ ) {}
   /* looking at the hrule uint8_t */
   if (i + 2 >= len || !(src[i] == '~' || src[i] == '`')) {
      return 0;
   }

   c = src[i];

   /* the whole line must be the uint8_t or whitespace */
   while (i < len && src[i] == c) {
      n++; i++;
   }

   if (n < 3) return 0;

   return i;
}

int _remove_fenced_chars( struct buf *buf, 
                          const char *src, int len )
{
   int beg = 0, end = 0;
   int i = 0;

   assert( src && len > 0 ) ;
   while ( beg < len ) {
      for ( end = beg + 1; end < len && src[end - 1] != '\n'; end++ ) {}
      i = _has_fenced_chars( src + beg, end - beg ) ;
      if ( 0 == i ) {
         bufput( buf, src + beg, end - beg );
      }
      beg = end;
   }
   return 1;
}

int _handle_linebreak_in_quote( struct buf *buf, 
                       const char *src, int len )
{
    int beg = 0, end = 0;

    assert( src && len > 0 );
    while ( beg < len ) {
        // skipping to the next line
        end = beg;
        while ( end < len && src[end] != '\n' && src[end] != '\r' ) {
            end++;
        }

        // adding the line body if present
        if ( end > beg ) {
            bufput( buf, src + beg, end - beg ) ;
        }

        while ( end < len && ( src[end] == '\n' || src[end] == '\r' ) ) {
            // we handle '\n', "\r\n" and '\r' here
            if ( src[end] == '\n' || (end + 1 < len && src[end + 1] != '\n' ) ) {
                if ( end > 1 && buf->data[buf->size - 1] != '\n' ) {
                    bufputs( buf, "  \n" ) ;
                } 
                else if ( end > 1 && buf->data[buf->size - 1] == '\n' ) {
                    bufputs( buf, ">\n") ;
                }
                else {
                    bufputc( buf, '\n' ) ;
                }          
            }
            end++;
        }
        beg = end;
    }
    return 1;
}

/********************
 * GENERIC RENDERER *
 ********************/

// 1: block code
static void rndr_blockcode(struct buf *ob, const struct buf *text,
                           const struct buf *lang, void *opaque)
{
   struct buf *tmp_buf1 = NULL ;
   struct buf *tmp_buf2 = NULL ;
   struct buf *tmp_buf3 = NULL ;
   const char chars[] = { '>' } ;
   tmp_buf1 = bufnew( 64 ) ;
   tmp_buf2 = bufnew( 64 ) ;
   tmp_buf3 = bufnew( 64 ) ;

   _handle_specified_chars( tmp_buf1, 
                            (const char*)(text->data), 
                            (int)(text->size), 
                            (const char*)(chars), 
                            (int)(sizeof(chars)/sizeof(char)) ) ;
   _remove_fenced_chars( tmp_buf2, 
                         (const char*)(tmp_buf1->data), 
                         (int)(tmp_buf1->size) ) ;
   _handle_linebreak_in_quote( tmp_buf3, (const char*)(tmp_buf2->data), 
                         (int)(tmp_buf2->size) ) ;

   // 1. i am going to change blockcode to fencedcode,
   // because it will be more perfect for displaying by pandoc
   // 2. i add 2 space before ">", because i think block codes
   // are belong to other blocks, just like list block and so on
   bufputs( ob, "  > " ) ;
   bufput( ob, tmp_buf3->data, tmp_buf3->size ) ;
//   bufput( ob, tmp_buf2->data, tmp_buf2->size ) ;
   bufputs( ob, "\n" ) ;

   bufrelease( tmp_buf1 ) ;
   bufrelease( tmp_buf2 ) ;
   bufrelease( tmp_buf3 ) ;
   return ;
}

// 2: block quote
static void rndr_blockquote(struct buf *ob, const struct buf *text, void *opaque)
{
   bufput( ob, text->data, text->size ) ;
   return ;
}



// 4: header
static void rndr_header(struct buf *ob, const struct buf *text, 
                        int level, void *opaque)
{
   int i = 0 ;
   int num = level > 1 ? level - 1 : level ;
   	
   assert(level >= 1 && level <= 6) ;
   for(i = 0; i < num; i++) 
   {
      bufputc(ob, '#') ;
   }
   bufput(ob, text->data, text->size) ;
   for(i = 0; i < num; i++) 
   {
      bufputc(ob, '#') ;
   }
}

// 6 list
static void rndr_liststart()
{
   list_level++ ;
}

static void rndr_listfinish()
{
   list_level-- ;
   sequence = 0 ;
}

static void rndr_list(struct buf *ob, const struct buf *text, 
                      int flags, void *opaque)
{
   bufput( ob, text->data, text->size ) ;
   return ;
}

// 7 list_item
static void rndr_listitem(struct buf *ob, const struct buf *text, 
                          int flags, void *opaque)
{
   // add the symbols
   if ( flags & MKD_LIST_ORDERED )
   {
      bufprintf( ob, "%d. ", ++sequence ) ;
   }
   else 
   {
      bufput( ob, "* ", 2 ) ;
   }
   bufput( ob, text->data, text->size ) ;
   
   return ;
}


// 8: paragraph
static void rndr_paragraph(struct buf *ob, const struct buf *text, void *opaque)
{
   if ( text->data == NULL || text->size == 0 ) {
      return ;
   }
   bufput( ob, text->data, text->size ) ;
   bufputs( ob, "\n" ) ;
}

// 9: table
void rndr_table(struct buf *ob, const struct buf *header, 
                const struct buf *body, void *opaque)
{
   return ;
}

// 10: table row
void rndr_table_row(struct buf *ob, const struct buf *text, void *opaque)
{
   return ;
}

// 11:
void rndr_table_cell(struct buf *ob, const struct buf *text,
                     int flags, void *opaque)
{
   return ;
}

// 17: linebreak
static int rndr_linebreak(struct buf *ob, void *opaque)
{
   bufputs( ob, "  \n" ) ;
   return 1;
}

// 18: link
static int rndr_link(struct buf *ob, const struct buf *link, 
                     const struct buf *title, 
                     const struct buf *content, void *opaque)
{
   bufput(ob, content->data, content->size);
   // TODO: check
   return 1;
}

// 25: header need by pandoc to gem the header of man page
void doc_header(struct buf *ob, const struct buf *text, void *opaque)
{
#define BUF_LEN  64
#define BUF_LEN2 128
   char func_name[BUF_LEN] ;
   char build_time[BUF_LEN] ;
   char version[BUF_LEN] ;
   char header[BUF_LEN2] ;

   memset(build_time, 0, BUF_LEN);
   memset(func_name, 0, BUF_LEN);
   memset(version, 0, BUF_LEN);
   memset(header, 0, BUF_LEN2);

   _get_func_name( text, func_name, BUF_LEN ) ;
   /*
   _get_build_time( build_time, BUF_LEN ) ;
   _get_version( version, BUF_LEN ) ;
   snprintf( header, BUF_LEN2,
             "%% %s(1) SequoiaDB User Manuals | Version %s\n\n",
            func_name, version ) ;
   */
   snprintf( header, BUF_LEN2,
             "%% %s(1) SequoiaDB User Manuals \n\n", func_name ) ;

   bufputs( ob, header ) ;
}

/// buil the callbacks
void pandoc_markdown_renderer(struct sd_callbacks *callbacks)
{

   static const struct sd_callbacks cb_default = {
      rndr_blockcode, // 1
      rndr_blockquote, // 2
      NULL, // 3
      rndr_header, //4
      //rndr_header_ignore,
      NULL, // 5
      rndr_list, //6 
      rndr_listitem, // 7
      rndr_paragraph, // 8
      rndr_table, // 9
      rndr_table_row, // 10
      rndr_table_cell, // 11

      NULL, // 12
      NULL, // 13
      NULL, // 14
      NULL, // 15
      NULL, // 16
      rndr_linebreak, // 17
      rndr_link, // 18
      NULL, // 19
      NULL, // 20
      NULL, // 21
      NULL, // 22

      NULL, // 23
      NULL, // 24

      doc_header, // 25
      NULL, // 26
      // user define callbacks and values
      rndr_liststart,
      rndr_listfinish,
   };
   /* Prepare the callbacks */
   memcpy(callbacks, &cb_default, sizeof(struct sd_callbacks));
}
