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

#ifndef UPSKIRT_PANDOC_H
#define UPSKIRT_PANDOC_H

#include <stdlib.h>
#include "markdown.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum callback_flags {
	CALLBACK_LINEITEM_IGNORE = (1 << 0),
   CALLBACK_PARAGRAPH_COPY = (1 << 1)
};


extern void pandoc_markdown_renderer(struct sd_callbacks *callbacks) ;


#ifdef __cplusplus
}
#endif

#endif

