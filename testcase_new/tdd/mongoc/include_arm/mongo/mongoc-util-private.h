/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MONGOC_UTIL_PRIVATE_H
#define MONGOC_UTIL_PRIVATE_H

#if !defined (MONGOC_I_AM_A_DRIVER) && !defined (MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

/* string comparison functions for Windows */
#ifdef _WIN32
# define strcasecmp  _stricmp
# define strncasecmp _strnicmp
#endif

BSON_BEGIN_DECLS


char *_mongoc_hex_md5 (const char *input);

void _mongoc_usleep (int64_t usec);


BSON_END_DECLS


#endif /* MONGOC_UTIL_PRIVATE_H */
