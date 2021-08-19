/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilIniParser.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/19/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilIniParser.h"
#include "ossMem.h"
#include "ossUtil.h"

#define UTIL_INI_COMMENT_PARSE_LINE    1
#define UTIL_INI_COMMENT_PARSE_BLOCK   2
#define UTIL_INI_COMMENT_PARSE_ALL     3

#define UTIL_INI_CHAR_SPACE         ' '
#define UTIL_INI_CHAR_TAB           '\t'
#define UTIL_INI_CHAR_CR            '\r'
#define UTIL_INI_CHAR_LF            '\n'
#define UTIL_INI_CHAR_SEMICOLON     ';'
#define UTIL_INI_CHAR_HASHMARK      '#'
#define UTIL_INI_CHAR_DOUBLEMARK    '"'
#define UTIL_INI_CHAR_SINGLEMARK    '\''
#define UTIL_INI_CHAR_LEFTBRACKET   '['
#define UTIL_INI_CHAR_RIGHTBRACKET  ']'
#define UTIL_INI_CHAR_EQUALSIGN     '='
#define UTIL_INI_CHAR_COLON         ':'
#define UTIL_INI_CHAR_BACKSLASH     '\\'

#define UTIL_INI_ERROR_INVALIDARG         "Invalid Argument"
#define UTIL_INI_ERROR_INVALIDNAME        "Invalid section name"
#define UTIL_INI_ERROR_INVALIDKEY         "Invalid item key"
#define UTIL_INI_ERROR_SECTION_NOTEXIST   "Section not exist"
#define UTIL_INI_ERROR_ITEM_NOTEXIST      "Item not exist"
#define UTIL_INI_ERROR_OOM                "Out of Memory"
#define UTIL_INI_ERROR_DELIMITERNOTEXIST  "Delimiter does not exist"

/* create */
static utilIniSection *createSection( utilIniHandler *handler ) ;
static utilIniItem *createItemByHandler( utilIniHandler *handler ) ;
static utilIniItem *createItemBySection( utilIniSection *section ) ;

/* get */
static utilIniItem *getItemByHandler( utilIniHandler *handler,
                                      const CHAR *key,
                                      BOOLEAN isComment ) ;
static utilIniItem *getItemBySection( utilIniSection *section,
                                      const CHAR *name, const CHAR *key,
                                      BOOLEAN isComment, UINT32 flags ) ;
static utilIniItem *getItem( utilIniItem *item, const CHAR *key,
                             BOOLEAN isComment, UINT32 flags ) ;
static utilIniSection *getSection( utilIniSection *section, const CHAR *name,
                                   UINT32 flags ) ;

static utilIniSection *getLastSection( utilIniSection *section ) ;
static utilIniItem *getLastItem( utilIniItem *item ) ;

/* append */
static void appendSection2Handler( utilIniHandler *handler,
                                   utilIniSection *section ) ;
static void appendItem2Handler( utilIniHandler *handler,
                                utilIniItem *item ) ;
static void appendItem2Section( utilIniSection *section,
                                utilIniItem *item ) ;

/* parse */
static const CHAR *parseComment( utilIniHandler *handler, const CHAR *str,
                                 INT32 type, utilIniString *comment ) ;

static const CHAR *parseSection( utilIniHandler *handler,
                                 utilIniSection *section, const CHAR *str ) ;
static const CHAR *parseSectionName( utilIniSection *section, const CHAR *str );

static const CHAR *parseItem( utilIniHandler *handler, utilIniItem *item,
                              const CHAR *str ) ;

static const CHAR *parseItemKey( utilIniItem *item, const CHAR *str,
                                 UINT32 flags ) ;
static const CHAR *parseItemValue( utilIniItem *item, const CHAR *str,
                                   UINT32 flags ) ;

/* skip */
static const CHAR *skip( const CHAR *str ) ;
static const CHAR *rskip( const CHAR *str ) ;
static const CHAR *skipEmptyLine( const CHAR *str ) ;

/* string */
static BOOLEAN utilStrcmp( const CHAR *str1, const CHAR *str2, INT32 length,
                           UINT32 flags ) ;

/* release */
static void releaseSection( utilIniSection *section ) ;
static void releaseItem( utilIniItem *item ) ;
static void releaseString( utilIniString *val ) ;

static INT32 setString( utilIniString *val, const CHAR *value, INT32 length ) ;

SDB_EXPORT void utilIniInit( utilIniHandler *handler, UINT32 flags )
{
   if ( handler )
   {
      if ( !( UTIL_INI_SEMICOLON & flags || UTIL_INI_HASHMARK & flags ) )
      {
         flags |= UTIL_INI_SEMICOLON ;
      }

      if ( !( UTIL_INI_EQUALSIGN & flags || UTIL_INI_COLON & flags ) )
      {
         flags |= UTIL_INI_EQUALSIGN ;
      }

      handler->errnum             = SDB_OK ;
      handler->flags              = flags ;
      handler->lastComment.ownmem = FALSE ;
      handler->lastComment.length = 0 ;
      handler->lastComment.str    = NULL ;
      handler->item               = NULL ;
      handler->section            = NULL ;
      handler->errMsg             = NULL ;
   }
}

SDB_EXPORT void utilIniRelease( utilIniHandler *handler )
{
   if ( handler )
   {
      releaseString( &handler->lastComment ) ;

      releaseItem( handler->item ) ;

      releaseSection( handler->section ) ;
   }
}

SDB_EXPORT INT32 utilIniParse( utilIniHandler *handler, const CHAR *content )
{
   UINT32 flags = handler->flags ;
   const CHAR *cur = content ;

   if( NULL == handler || NULL == content )
   {
      goto done ;
   }

   cur = skipEmptyLine( cur ) ;

   while ( *cur )
   {
      utilIniString comment = { FALSE, 0, NULL } ;

      if ( ( UTIL_INI_SEMICOLON & flags && UTIL_INI_CHAR_SEMICOLON == *cur ) ||
           ( UTIL_INI_HASHMARK & flags && UTIL_INI_CHAR_HASHMARK == *cur ) )
      {
         cur = parseComment( handler, cur, UTIL_INI_COMMENT_PARSE_ALL,
                             &comment ) ;
      }

      if ( *cur )
      {
         if ( UTIL_INI_CHAR_LEFTBRACKET == *cur )
         {
            utilIniSection *section = createSection( handler ) ;

            if ( NULL == section )
            {
               handler->errMsg = UTIL_INI_ERROR_OOM ;
               handler->errnum = SDB_OOM ;
               goto error ;
            }

            ossMemcpy( &section->comment, &comment, sizeof( utilIniString ) ) ;

            cur = parseSection( handler, section, cur ) ;
         }
         else if ( UTIL_INI_CHAR_LF == *cur )
         {
            cur = skipEmptyLine( cur + 1 ) ;
         }
         else
         {
            utilIniItem *item = NULL ;
            utilIniItem tmp ;

            ossMemset( &tmp, 0, sizeof( utilIniItem ) ) ;

            cur = parseItem( handler, &tmp, cur ) ;
            if ( handler->errnum )
            {
               goto error ;
            }

            item = createItemByHandler( handler ) ;
            if ( NULL == item )
            {
               handler->errMsg = UTIL_INI_ERROR_OOM ;
               handler->errnum = SDB_OOM ;
               goto error ;
            }

            ossMemcpy( &tmp.pre_comment, &comment, sizeof( utilIniString ) ) ;
            ossMemcpy( item, &tmp, sizeof( utilIniItem ) ) ;
         }
      }
      else
      {
         ossMemcpy( &handler->lastComment, &comment, sizeof( utilIniString ) ) ;
         break ;
      }
   }

done:
   return handler->errnum ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniGetValue( utilIniHandler *handler, const CHAR *section,
                                  const CHAR *key, const CHAR **value,
                                  INT32 *length )
{
   INT32 rc = SDB_OK ;
   utilIniItem *item = NULL ;

   if ( NULL == handler || NULL == key || 0 == ossStrlen( key ) ||
        NULL == value || NULL == length )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      item = getItemByHandler( handler, key, FALSE ) ;
   }
   else
   {
      item = getItemBySection( handler->section, section, key,
                               FALSE, handler->flags ) ;
   }

   if ( NULL == item )
   {
      handler->errMsg = UTIL_INI_ERROR_ITEM_NOTEXIST ;
      rc = SDB_FIELD_NOT_EXIST ;
      goto error ;
   }

   *length = item->value.length ;
   *value  = item->value.str ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniGetSectionComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR **comment,
                                           INT32 *length )
{
   INT32 rc = SDB_OK ;
   utilIniSection *tmpSection = NULL ;

   if ( NULL == handler || NULL == section || 0 == ossStrlen( section ) ||
        NULL == comment || NULL == length )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   tmpSection = getSection( handler->section, section, handler->flags ) ;
   if ( NULL == tmpSection )
   {
      handler->errMsg = UTIL_INI_ERROR_SECTION_NOTEXIST ;
      rc = SDB_FIELD_NOT_EXIST ;
      goto error ;
   }

   *length  = tmpSection->comment.length ;
   *comment = tmpSection->comment.str ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniGetItemPreComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR **comment,
                                           INT32 *length )
{
   INT32 rc = SDB_OK ;
   utilIniItem *item = NULL ;

   if ( NULL == handler || NULL == key || 0 == ossStrlen( key ) ||
        NULL == comment || NULL == length )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      item = getItemByHandler( handler, key, FALSE ) ;
   }
   else
   {
      item = getItemBySection( handler->section, section, key,
                               FALSE, handler->flags ) ;
   }

   if ( NULL == item )
   {
      handler->errMsg = UTIL_INI_ERROR_ITEM_NOTEXIST ;
      rc = SDB_FIELD_NOT_EXIST ;
      goto error ;
   }

   *length  = item->pre_comment.length ;
   *comment = item->pre_comment.str ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniGetItemPosComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR **comment,
                                           INT32 *length )
{
   INT32 rc = SDB_OK ;
   utilIniItem *item = NULL ;

   if ( NULL == handler || NULL == key || 0 == ossStrlen( key ) ||
        NULL == comment || NULL == length )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      item = getItemByHandler( handler, key, FALSE ) ;
   }
   else
   {
      item = getItemBySection( handler->section, section, key,
                               FALSE, handler->flags ) ;
   }

   if ( NULL == item )
   {
      handler->errMsg = UTIL_INI_ERROR_ITEM_NOTEXIST ;
      rc = SDB_FIELD_NOT_EXIST ;
      goto error ;
   }

   *length  = item->pos_comment.length ;
   *comment = item->pos_comment.str ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniGetLastComment( utilIniHandler *handler,
                                        const CHAR **comment,
                                        INT32 *length )
{
   INT32 rc = SDB_OK ;

   if ( NULL == handler || NULL == comment || NULL == length )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *length  = handler->lastComment.length ;
   *comment = handler->lastComment.str ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniSetValue( utilIniHandler *handler, const CHAR *section,
                                  const CHAR *key, const CHAR *value,
                                  INT32 length )
{
   INT32 rc = SDB_OK ;
   INT32 keyLen = ossStrlen( key ) ;
   utilIniItem *tmpItem = NULL ;

   if ( NULL == key || 0 == keyLen || NULL == value )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      tmpItem = getItemByHandler( handler, key, FALSE ) ;
      if ( NULL == tmpItem )
      {
         tmpItem = getItemByHandler( handler, key, TRUE ) ;
         if ( NULL == tmpItem )
         {
            tmpItem = createItemByHandler( handler ) ;
            if ( NULL == tmpItem )
            {
               handler->errMsg = UTIL_INI_ERROR_OOM ;
               rc = SDB_OOM ;
               goto error ;
            }

            rc = setString( &tmpItem->key, key, keyLen ) ;
            if ( rc )
            {
               handler->errMsg = UTIL_INI_ERROR_OOM ;
               goto error ;
            }
         }
         else
         {
            tmpItem->isComment = FALSE ;
         }
      }
   }
   else
   {
      utilIniSection *tmpSection = getSection( handler->section, section,
                                               handler->flags ) ;

      if ( NULL == tmpSection )
      {
         INT32 sectionLen = ossStrlen( section ) ;

         tmpSection = createSection( handler ) ;
         if ( NULL == tmpSection )
         {
            handler->errMsg = UTIL_INI_ERROR_OOM ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = setString( &tmpSection->name, section, sectionLen ) ;
         if ( rc )
         {
            handler->errMsg = UTIL_INI_ERROR_OOM ;
            goto error ;
         }

         tmpItem = createItemBySection( tmpSection ) ;
         if ( NULL == tmpItem )
         {
            handler->errMsg = UTIL_INI_ERROR_OOM ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = setString( &tmpItem->key, key, keyLen ) ;
         if ( rc )
         {
            handler->errMsg = UTIL_INI_ERROR_OOM ;
            goto error ;
         }
      }
      else
      {
         tmpItem = getItem( tmpSection->item, key, FALSE, handler->flags ) ;
         if ( NULL == tmpItem )
         {
            tmpItem = getItem( tmpSection->item, key, TRUE, handler->flags ) ;
            if ( NULL == tmpItem )
            {
               tmpItem = createItemBySection( tmpSection ) ;
               if ( NULL == tmpItem )
               {
                  handler->errMsg = UTIL_INI_ERROR_OOM ;
                  rc = SDB_OOM ;
                  goto error ;
               }

               rc = setString( &tmpItem->key, key, keyLen ) ;
               if ( rc )
               {
                  handler->errMsg = UTIL_INI_ERROR_OOM ;
                  goto error ;
               }
            }
            else
            {
               tmpItem->isComment = FALSE ;
            }
         }
      }
   }

   rc = setString( &tmpItem->value, value, length ) ;
   if ( rc )
   {
      handler->errMsg = UTIL_INI_ERROR_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniSetSectionComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *comment,
                                           INT32 length )
{
   INT32 rc = SDB_OK ;
   utilIniSection *tmpSection = NULL ;

   if ( NULL == handler || NULL == section )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   tmpSection = getSection( handler->section, section, handler->flags ) ;
   if ( NULL == tmpSection )
   {
      rc = SDB_FIELD_NOT_EXIST ;
      handler->errMsg = UTIL_INI_ERROR_SECTION_NOTEXIST ;
      goto error ;
   }

   rc = setString( &tmpSection->comment, comment, length ) ;
   if ( rc )
   {
      handler->errMsg = UTIL_INI_ERROR_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniSetItemPreComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR *comment,
                                           INT32 length )
{
   INT32 rc = SDB_OK ;
   utilIniItem *item = NULL ;

   if ( NULL == handler || NULL == key || 0 == ossStrlen( key ) )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      item = getItemByHandler( handler, key, FALSE ) ;
   }
   else
   {
      item = getItemBySection( handler->section, section, key,
                               FALSE, handler->flags ) ;
   }

   if ( NULL == item )
   {
      rc = SDB_FIELD_NOT_EXIST ;
      handler->errMsg = UTIL_INI_ERROR_ITEM_NOTEXIST ;
      goto error ;
   }

   rc = setString( &item->pre_comment, comment, length ) ;
   if ( rc )
   {
      handler->errMsg = UTIL_INI_ERROR_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniSetItemPosComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR *comment,
                                           INT32 length )
{
   INT32 rc = SDB_OK ;
   utilIniItem *item = NULL ;

   if ( NULL == handler || NULL == key || 0 == ossStrlen( key ) )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      item = getItemByHandler( handler, key, FALSE ) ;
   }
   else
   {
      item = getItemBySection( handler->section, section, key,
                               FALSE, handler->flags ) ;
   }

   if ( NULL == item )
   {
      rc = SDB_FIELD_NOT_EXIST ;
      handler->errMsg = UTIL_INI_ERROR_ITEM_NOTEXIST ;
      goto error ;
   }

   rc = setString( &item->pos_comment, comment, length ) ;
   if ( rc )
   {
      handler->errMsg = UTIL_INI_ERROR_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniSetLastComment( utilIniHandler *handler,
                                        const CHAR *comment,
                                        INT32 length )
{
   INT32 rc = SDB_OK ;

   if ( NULL == handler )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = setString( &handler->lastComment, comment, length ) ;
   if ( rc )
   {
      handler->errMsg = UTIL_INI_ERROR_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 utilIniSetCommentItem( utilIniHandler *handler,
                                        const CHAR *section,
                                        const CHAR *key,
                                        BOOLEAN isComment )
{
   INT32 rc = SDB_OK ;
   utilIniItem *item = NULL ;

   if ( NULL == handler || NULL == key || 0 == ossStrlen( key ) )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDARG ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == section || 0 == ossStrlen( section ) )
   {
      if ( isComment )
      {
         item = getItemByHandler( handler, key, FALSE ) ;
      }
      else
      {
         item = getItemByHandler( handler, key, FALSE ) ;
         if ( NULL == item )
         {
            item = getItemByHandler( handler, key, TRUE ) ;
         }
      }
   }
   else
   {
      if ( isComment )
      {
         item = getItemBySection( handler->section, section, key,
                                  FALSE, handler->flags ) ;
      }
      else
      {
         item = getItemBySection( handler->section, section, key,
                                  FALSE, handler->flags ) ;
         if ( NULL == item )
         {
            item = getItemBySection( handler->section, section, key,
                                     TRUE, handler->flags ) ;
         }
      }
   }

   if ( NULL == item )
   {
      handler->errMsg = UTIL_INI_ERROR_ITEM_NOTEXIST ;
      rc = SDB_FIELD_NOT_EXIST ;
      goto error ;
   }

   item->isComment = isComment ;

done:
   return rc ;
error:
   goto done ;
}

static utilIniItem *getItemByHandler( utilIniHandler *handler, const CHAR *key,
                                      BOOLEAN isComment )
{
   INT32 length = ossStrlen( key ) ;
   utilIniItem *item = handler->item ;

   while( item )
   {
      if ( item->key.length == length && item->isComment == isComment &&
           utilStrcmp( item->key.str, key, length, handler->flags ) )
      {
         break ;
      }

      item = item->next ;
   }

   return item ;
}

static utilIniItem *getItemBySection( utilIniSection *section,
                                      const CHAR *name, const CHAR *key,
                                      BOOLEAN isComment, UINT32 flags )
{
   utilIniItem *item = NULL ;
   utilIniSection *tmpSection = NULL ;

   tmpSection = getSection( section, name, flags ) ;

   if ( tmpSection )
   {
      item = getItem( tmpSection->item, key, isComment, flags ) ;
   }

   return item ;
}

static utilIniItem *getItem( utilIniItem *item, const CHAR *key,
                             BOOLEAN isComment, UINT32 flags )
{
   INT32 length = ossStrlen( key ) ;
   utilIniItem *tmpItem = item ;

   while ( tmpItem )
   {
      if ( tmpItem->key.length == length && tmpItem->isComment == isComment &&
           utilStrcmp( tmpItem->key.str, key, length, flags ) )
      {
         break ;
      }

      tmpItem = tmpItem->next ;
   }

   return tmpItem ;
}

static utilIniSection *getSection( utilIniSection *section, const CHAR *name,
                                   UINT32 flags )
{
   INT32 length = ossStrlen( name ) ;
   utilIniSection *tmpSection = section ;

   while ( tmpSection )
   {
      if ( tmpSection->name.length == length &&
           utilStrcmp( tmpSection->name.str, name, length, flags ) )
      {
         break ;
      }

      tmpSection = tmpSection->next ;
   }

   return tmpSection ;
}

static utilIniSection *createSection( utilIniHandler *handler )
{
   utilIniSection *section = NULL ;

   section = (utilIniSection *)SDB_OSS_MALLOC( sizeof( utilIniSection ) ) ;
   if ( section )
   {
      ossMemset( section, 0, sizeof( utilIniSection ) ) ;

      appendSection2Handler( handler, section ) ;
   }

   return section ;
}

static utilIniItem *createItemByHandler( utilIniHandler *handler )
{
   utilIniItem *item = NULL ;

   item = (utilIniItem *)SDB_OSS_MALLOC( sizeof( utilIniItem ) ) ;
   if ( item )
   {
      ossMemset( item, 0, sizeof( utilIniItem ) ) ;

      appendItem2Handler( handler, item ) ;
   }

   return item ;
}

static utilIniItem *createItemBySection( utilIniSection *section )
{
   utilIniItem *item = NULL ;

   item = (utilIniItem *)SDB_OSS_MALLOC( sizeof( utilIniItem ) ) ;
   if ( item )
   {
      ossMemset( item, 0, sizeof( utilIniItem ) ) ;

      appendItem2Section( section, item ) ;
   }

   return item ;
}

static utilIniSection *getLastSection( utilIniSection *section )
{
   if ( section->next )
   {
      return getLastSection( section->next ) ;
   }
   else
   {
      return section ;
   }
}

static utilIniItem *getLastItem( utilIniItem *item )
{
   if ( item->next )
   {
      return getLastItem( item->next ) ;
   }
   else
   {
      return item ;
   }
}

static void appendSection2Handler( utilIniHandler *handler,
                                   utilIniSection *section )
{
   if ( handler->section )
   {
      utilIniSection *last = getLastSection( handler->section ) ;

      last->next = section ;
   }
   else
   {
      handler->section = section ;
   }
}

static void appendItem2Handler( utilIniHandler *handler,
                                utilIniItem *item )
{
   if ( handler->item )
   {
      utilIniItem *last = getLastItem( handler->item ) ;

      last->next = item ;
   }
   else
   {
      handler->item = item ;
   }
}

static void appendItem2Section( utilIniSection *section,
                                utilIniItem *item )
{
   if ( section->item )
   {
      utilIniItem *last = getLastItem( section->item ) ;

      last->next = item ;
   }
   else
   {
      section->item = item ;
   }
}

static const CHAR *parseSection( utilIniHandler *handler,
                                 utilIniSection *section, const CHAR *str )
{
   str = parseSectionName( section, str ) ;

   if ( section->name.length == 0 )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDNAME ;
      handler->errnum = SDB_INVALIDARG ;
      goto error ;
   }

   while( *str )
   {
      const CHAR *begin = NULL ;
      utilIniItem *item = NULL ;
      utilIniItem tmp ;

      ossMemset( &tmp, 0, sizeof( utilIniItem ) ) ;

      begin = str ;
      str = parseComment( handler, str, UTIL_INI_COMMENT_PARSE_ALL,
                          &tmp.pre_comment ) ;

      str = parseItem( handler, &tmp, str ) ;
      if ( handler->errnum )
      {
         goto error ;
      }

      if ( NULL == tmp.key.str && NULL == tmp.value.str )
      {
         str = begin ;
         break ;
      }

      item = createItemBySection( section ) ;
      if ( NULL == item )
      {
         handler->errnum = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( item, &tmp, sizeof( utilIniItem ) ) ;

      if ( UTIL_INI_CHAR_LEFTBRACKET == *str )
      {
         break ;
      }
   }

done:
   return str ;
error:
   goto done ;
}

static const CHAR *parseSectionName( utilIniSection *section, const CHAR *str )
{
   INT32 len = 0 ;

   //[xxxxxx

   ++str ;
   section->name.ownmem = FALSE ;
   section->name.str = (CHAR *)str ;

   while( *str && UTIL_INI_CHAR_RIGHTBRACKET != *str )
   {
      ++str ;
      ++len ;
   }

   if ( UTIL_INI_CHAR_RIGHTBRACKET == *str )
   {
      //[xxxxxx]
      str = skip( str + 1 ) ;

      if ( UTIL_INI_CHAR_LF == *str )
      {
         //[xxxxxx] \n
         section->name.length = len ;
         str = skipEmptyLine( str + 1 ) ;
      }
      else if ( '\0' == *str )
      {
         //[xxxxxx] \0
         section->name.length = len ;
      }
   }

   return str ;
}

static const CHAR *parseItem( utilIniHandler *handler, utilIniItem *item,
                              const CHAR *str )
{
   UINT32 flags = handler->flags ;

   if ( UTIL_INI_CHAR_LEFTBRACKET == *str || '\0' == *str )
   {
      goto done ;
   }
   else if ( ( UTIL_INI_SEMICOLON & flags && UTIL_INI_CHAR_SEMICOLON == *str )||
             ( UTIL_INI_HASHMARK & flags && UTIL_INI_CHAR_HASHMARK == *str ) )
   {
      item->isComment = TRUE ;
      str = skip( str + 1 ) ; 
   }

   str = parseItemKey( item, str, flags ) ;

   if ( item->key.length == 0 )
   {
      handler->errMsg = UTIL_INI_ERROR_INVALIDKEY ;
      handler->errnum = SDB_INVALIDARG ;
      goto error ;
   }

   str = skip( str ) ;

   if ( ( UTIL_INI_EQUALSIGN & flags && UTIL_INI_CHAR_EQUALSIGN == *str ) ||
        ( UTIL_INI_COLON & flags && UTIL_INI_CHAR_COLON == *str ) )
   {
      str = skip( str + 1 ) ;
   }
   else
   {
      handler->errMsg = UTIL_INI_ERROR_DELIMITERNOTEXIST ;
      handler->errnum = SDB_INVALIDARG ;
      goto error ;
   }

   str = parseItemValue( item, str, flags ) ;

   if ( UTIL_INI_CHAR_CR == *str || UTIL_INI_CHAR_LF == *str )
   {
      str = skipEmptyLine( str + 1 ) ;
   }
   else if ( ( UTIL_INI_SEMICOLON & flags && UTIL_INI_CHAR_SEMICOLON == *str )||
             ( UTIL_INI_HASHMARK & flags && UTIL_INI_CHAR_HASHMARK == *str ) )
   {
      str = parseComment( handler, str, UTIL_INI_COMMENT_PARSE_LINE,
                          &item->pos_comment ) ;
      str = skipEmptyLine( str ) ;
   }

done:
   return str ;
error:
   goto done ;
}

static const CHAR *parseItemKey( utilIniItem *item, const CHAR *str,
                                 UINT32 flags )
{
   BOOLEAN isEscape = FALSE ;

   item->key.ownmem = FALSE ;
   item->key.str = (CHAR *)str ;

   while( *str )
   {
      if ( isEscape )
      {
         ++str ;
         isEscape = FALSE ;
         continue ;
      }
      else if ( UTIL_INI_ESCAPE & flags && UTIL_INI_CHAR_BACKSLASH == *str )
      {
         ++str ;
         isEscape = TRUE ;
         continue ;
      }

      if ( UTIL_INI_CHAR_SPACE == *str || UTIL_INI_CHAR_TAB == *str ||
           UTIL_INI_CHAR_CR == *str || UTIL_INI_CHAR_LF == *str ||
           ( UTIL_INI_EQUALSIGN & flags && UTIL_INI_CHAR_EQUALSIGN == *str ) ||
           ( UTIL_INI_COLON & flags && UTIL_INI_CHAR_COLON == *str ) )
      {
         break ;
      }

      ++str ;
   }

   item->key.length = str - item->key.str ;

   return str ;
}

static const CHAR *parseItemValue( utilIniItem *item, const CHAR *str,
                                   UINT32 flags )
{
   BOOLEAN isEscape = FALSE ;
   const CHAR *rstr = NULL ;

   item->value.ownmem = FALSE ;
   item->value.str = (CHAR *)str ;

   while( *str )
   {
      if ( isEscape )
      {
         ++str ;
         isEscape = FALSE ;
         continue ;
      }
      else if ( UTIL_INI_ESCAPE & flags && UTIL_INI_CHAR_BACKSLASH == *str )
      {
         ++str ;
         isEscape = TRUE ;
         continue ;
      }

      if ( UTIL_INI_CHAR_CR == *str || UTIL_INI_CHAR_LF == *str ||
           ( UTIL_INI_SEMICOLON & flags && UTIL_INI_CHAR_SEMICOLON == *str ) ||
           ( UTIL_INI_HASHMARK & flags && UTIL_INI_CHAR_HASHMARK == *str ) )
      {
         break ;
      }

      ++str ;
   }

   rstr = rskip( str - 1 ) ;

   item->value.length = rstr - item->value.str + 1 ;

   if ( UTIL_INI_DOUBLE_QUOMARK & flags &&
        UTIL_INI_CHAR_DOUBLEMARK == *item->value.str )
   {
      if ( UTIL_INI_CHAR_DOUBLEMARK == item->value.str[item->value.length - 1] )
      {
         ++item->value.str ;
         item->value.length -= 2 ;
      }
   }
   else if ( UTIL_INI_SINGLE_QUOMARK & flags &&
             UTIL_INI_CHAR_SINGLEMARK == *item->value.str )
   {
      if ( UTIL_INI_CHAR_SINGLEMARK == item->value.str[item->value.length - 1] )
      {
         ++item->value.str ;
         item->value.length -= 2 ;
      }
   }

   return str ;
}

static const CHAR *parseComment( utilIniHandler *handler, const CHAR *str,
                                 INT32 type, utilIniString *comment )
{
   INT32 len = 0 ;
   UINT32 flags = handler->flags ;
   const CHAR *begin    = str ;
   const CHAR *blockEnd = str ;
   const CHAR *end      = str ;

   while ( *str &&
          ( ( UTIL_INI_SEMICOLON & flags && UTIL_INI_CHAR_SEMICOLON == *str ) ||
            ( UTIL_INI_HASHMARK & flags && UTIL_INI_CHAR_HASHMARK == *str ) ) )
   {
      BOOLEAN isEscape = FALSE ;
      utilIniItem tmpItem ;

      str = skip( str + 1 ) ;

      if ( '\0' == *str )
      {
         len = 1 ;
         break ;
      }

      parseItem( handler, &tmpItem, str ) ;
      if ( handler->errnum )
      {
         handler->errnum = SDB_OK ;
         handler->errMsg = NULL ;

         while( *str && UTIL_INI_CHAR_LF != *str )
         {
            if ( isEscape )
            {
               ++str ;
               isEscape = FALSE ;
               continue ;
            }
            else if ( UTIL_INI_ESCAPE & flags && UTIL_INI_CHAR_BACKSLASH == *str )
            {
               ++str ;
               isEscape = TRUE ;
               continue ;
            }

            ++str ;
         }
      }
      else
      {
         str = end ;
         len = blockEnd - begin ;
         break ;
      }

      blockEnd = str ;

      if ( UTIL_INI_COMMENT_PARSE_ALL == type )
      {
         len = blockEnd - begin ;

         if ( '\0' != *str )
         {
            ++str ;
         }

         str = skipEmptyLine( str ) ;
      }
      else
      {
         len = blockEnd - begin ;

         if ( '\0' != *str )
         {
            ++str ;
         }

         str = skip( str ) ;
      }

      end = str ;

      if ( UTIL_INI_COMMENT_PARSE_LINE == type )
      {
         break ;
      }
   }

   if ( 0 < len )
   {
      comment->str = (CHAR *)begin ;
   }

   comment->ownmem = FALSE ;
   comment->length = len ;

   return str ;
}

static const CHAR *skip( const CHAR *str )
{
   while( str && *str &&
         ( UTIL_INI_CHAR_SPACE == *str || UTIL_INI_CHAR_TAB == *str ) )
   {
      ++str ;
   }
   return str ;
}

static const CHAR *rskip( const CHAR *str )
{
   while( str && *str &&
         ( UTIL_INI_CHAR_SPACE == *str || UTIL_INI_CHAR_TAB == *str ) )
   {
      --str ;
   }
   return str ;
}

static const CHAR *skipEmptyLine( const CHAR *str )
{
   while( str && *str &&
          ( UTIL_INI_CHAR_SPACE == *str || UTIL_INI_CHAR_TAB == *str ||
            UTIL_INI_CHAR_CR == *str || UTIL_INI_CHAR_LF == *str ) )
   {
      ++str ;
   }
   return str ;
}

static BOOLEAN utilStrcmp( const CHAR *str1, const CHAR *str2, INT32 length,
                           UINT32 flags )
{
   BOOLEAN notCase = UTIL_INI_NOTCASE & flags ;
   BOOLEAN result = FALSE ;

   if ( notCase )
   {
      result = ( 0 == ossStrncasecmp( str1, str2, length ) ) ;
   }
   else
   {
      result = ( 0 == ossStrncmp( str1, str2, length ) ) ;
   }

   return result ;
}

static void releaseSection( utilIniSection *section )
{
   utilIniSection *cur = NULL ;
   utilIniSection *next = section ;

   while( next )
   {
      cur  = next ;
      next = cur->next ;

      releaseString( &cur->name ) ;
      releaseString( &cur->comment ) ;

      releaseItem( cur->item ) ;

      SAFE_OSS_FREE( cur ) ;
   }
}

static void releaseItem( utilIniItem *item )
{
   utilIniItem *cur = NULL ;
   utilIniItem *next = item ;

   while( next )
   {
      cur  = next ;
      next = cur->next ;

      releaseString( &cur->key ) ;
      releaseString( &cur->value ) ;
      releaseString( &cur->pre_comment ) ;
      releaseString( &cur->pos_comment ) ;

      SAFE_OSS_FREE( cur ) ;
   }
}

static void releaseString( utilIniString *val )
{
   if ( val && val->ownmem && val->str )
   {
      SAFE_OSS_FREE( val->str ) ;
   }
}

static INT32 setString( utilIniString *val, const CHAR *value, INT32 length )
{
   INT32 rc = SDB_OK ;

   releaseString( val ) ;

   if ( val )
   {
      CHAR *buff = (CHAR *)SDB_OSS_MALLOC( length + 1 ) ;

      if ( NULL == buff )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( buff, value, length ) ;
      buff[length] = 0 ;

      val->ownmem = TRUE ;
      val->str    = buff ;
      val->length = length ;
   }

done:
   return rc ;
error:
   goto done ;
}