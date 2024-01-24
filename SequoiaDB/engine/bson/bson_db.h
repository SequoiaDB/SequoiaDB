/** @file bson_db.h

    This file contains the implementation of BSON-related methods that are required
    by the MongoDB database server.

    Normally, for standalone BSON usage, you do not want this file - it will tend to
    pull in some other files from the MongoDB project. Thus, bson.h (the main file
    one would use) does not include this file.
*/

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "util/optime.h"
//#include "util/time_support.h"

/*#ifndef log
#define log(...) std::cerr
#endif*/

namespace bson {

    /**
     * Append a timestamp value to a bson.
     *
     * @param b the bson to append to.
     * @param fieldName the key for the timestampe value.
     * @param time milliseconds since epoch(but stored in seconds).
     * @param inc microseconds in range of [0us, 999999us].
     *
     * @return the current instance of BSONObjBuilder.
     */
    inline BSONObjBuilder& BSONObjBuilder::appendTimestamp(
      const StringData& fieldName, long long time, unsigned int inc) {
        long long ms = 0;
        unsigned int us = 0;
        if ( inc < 1000000 )
        {
            ms = time;
            us = inc;
        }
        else
        {
            ms = time + ( inc / 1000000 ) * 1000 ;
            us  = inc % 1000000 ;
        }
        OpTime t( (ms / 1000) , us );
        appendTimestamp( fieldName , t.asDate() );
        return *this;
    }

    inline OpTime BSONElement::_opTime() const {
      if(type() == bson::Date || type() == Timestamp)
        return OpTime(*reinterpret_cast< const unsigned long long* >(value()));
      return OpTime();
    }

    inline string BSONElement::_asCode() const {
        switch( type() ) {
        case bson::String:
        case Code:
            return string(valuestr(), valuestrsize()-1);
        case CodeWScope:
            return string(codeWScopeCode(), *(int*)(valuestr())-1);
        default:
            break ;
            //log() << "can't convert type: " << (int)(type()) << " to code"
            //      << endl;
        }
        uassert( 10062 ,  "not code" , 0 );
        return "";
    }

/*    inline BSONObjBuilder& BSONObjBuilderValueStream::operator<<(
      DateNowLabeler& id) {
        _builder->appendDate(_fieldName, jsTime());
        _fieldName = 0;
        return *_builder;
    }*/

    inline BSONObjBuilder& BSONObjBuilderValueStream::operator<<(
      MinKeyLabeler& id) {
        _builder->appendMinKey(_fieldName);
        _fieldName = 0;
        return *_builder;
    }

    inline BSONObjBuilder& BSONObjBuilderValueStream::operator<<(
      MaxKeyLabeler& id) {
        _builder->appendMaxKey(_fieldName);
        _fieldName = 0;
        return *_builder;
    }

   #define BSON_ABBREV_MIN_SIZE     ( 100 )
   #define BSON_ABBREV_MAX_SIZE     ( 160 )
   #define BSON_ABBREV_REPEAT_SIZE  ( 10 )
   #define BSON_ABBREV_PREFIX_SIZE  ( 60 )
   #define BSON_ABBREV_REMAIN_SIZE  ( 30 )
   #define BSON_ABBREV_BUFFER_SIZE  ( 64 )
   #define BSON_ABBREV_STRING_SIZE  ( BSON_ABBREV_BUFFER_SIZE - 1 )

   inline StringData BSONObjBuilder::_genAbbrevStr( StackBufBuilder &builder,
                                                    const StringData &value )
   {
      const char *valuePtr = value.data() ;
      unsigned valueLen = value.size() ;

      if ( valueLen > BSON_ABBREV_MIN_SIZE )
      {
         StringData tempValue( valuePtr, BSON_ABBREV_PREFIX_SIZE ) ;
         builder.appendStr( tempValue, false ) ;

         unsigned printedLen = BSON_ABBREV_PREFIX_SIZE ;
         unsigned remainLen = valueLen - BSON_ABBREV_PREFIX_SIZE ;
         const char *repeatPtr = valuePtr + BSON_ABBREV_PREFIX_SIZE ;
         const char *remainPtr = valuePtr + BSON_ABBREV_PREFIX_SIZE + 1 ;

         while ( TRUE )
         {
            if ( *repeatPtr != *remainPtr || '\0' == *remainPtr )
            {
               unsigned repeatLen = remainPtr - repeatPtr ;
               if ( repeatLen > BSON_ABBREV_REPEAT_SIZE )
               {
                  char tempBuffer[ BSON_ABBREV_BUFFER_SIZE + 1 ] = { 0 } ;
                  unsigned tempLen = 0 ;
#if defined (_WIN32) || defined (_WIN64)
                  _snprintf( tempBuffer, BSON_ABBREV_STRING_SIZE,
                             "...<%c repeat %u times>...", *repeatPtr,
                             repeatLen ) ;
                  tempBuffer[ BSON_ABBREV_BUFFER_SIZE ] = '\0' ;
#else
                  snprintf( tempBuffer, BSON_ABBREV_STRING_SIZE,
                            "...<%c repeat %u times>...", *repeatPtr,
                            repeatLen ) ;
#endif
                  tempLen = strlen( tempBuffer ) ;
                  if ( tempLen + printedLen < BSON_ABBREV_MAX_SIZE )
                  {
                     StringData tempValue( tempBuffer, tempLen ) ;
                     builder.appendStr( tempValue, false ) ;
                     printedLen += tempLen ;
                     remainLen -= repeatLen ;
                  }
                  else
                  {
                     // too long
                     break ;
                  }
                  repeatPtr = remainPtr ;
               }
               else
               {
                  break ;
               }

               if ( '\0' == *remainPtr )
               {
                  break ;
               }
            }
            else
            {
               ++ remainPtr ;
            }
         }

         // if the remain size is still too long, print in abbreviation mode
         if ( remainLen > BSON_ABBREV_REMAIN_SIZE )
         {
            char tempBuffer[ BSON_ABBREV_BUFFER_SIZE + 1 ] = { 0 } ;
#if defined (_WIN32) || defined (_WIN64)
            _snprintf( tempBuffer, BSON_ABBREV_STRING_SIZE,
                       " ...<%u characters more>...", remainLen ) ;
            tempBuffer[ BSON_ABBREV_BUFFER_SIZE ] = '\0' ;
#else
            snprintf( tempBuffer, BSON_ABBREV_STRING_SIZE,
                      " ...<%u characters more>...", remainLen ) ;
#endif
            builder.appendStr( tempBuffer, true ) ;
         }
         else
         {
            // otherwise, print  the remain characters
            StringData remainValue( remainPtr, remainLen ) ;
            builder.appendStr( remainValue, true ) ;
         }

         valuePtr = builder.buf() ;
         valueLen = builder.len() - 1 ;
      }

      return StringData( valuePtr, valueLen ) ;
   }

   inline BSONObjBuilder &BSONObjBuilder::appendEx(
                                             const StringData &fieldName,
                                             const BSONElement &element,
                                             const BSONObjBuilderOption &option )
   {
      if ( option.isEnabled() )
      {
         // append value
         switch ( element.type() )
         {
            // $minKey
            case MinKey:
            {
               if ( option.isClientReadable )
               {
                  BSONObjBuilder mBuilder( subobjStart( fieldName) ) ;
                  mBuilder.append( "$minElement", 1 ) ;
                  mBuilder.doneFast() ;
               }
               else
               {
                  appendAs( element, fieldName ) ;
               }
               break ;
            }
            // $maxKey
            case MaxKey:
            {
               if ( option.isClientReadable )
               {
                  BSONObjBuilder mBuilder( subobjStart( fieldName) ) ;
                  mBuilder.append( "$maxElement", 1 ) ;
                  mBuilder.doneFast() ;
               }
               else
               {
                  appendAs( element, fieldName ) ;
               }
               break ;
            }
            // array
            case Array:
            {
               BSONObj subObject = element.embeddedObject() ;
               BSONArrayBuilder subArrBuilder( subarrayStart( fieldName ) ) ;
               BSONObjIterator subArrIter( subObject ) ;
               while( subArrIter.more() )
               {
                  BSONElement subArrEle( subArrIter.next() ) ;
                  subArrBuilder.appendEx( subArrEle, option ) ;
               }
               subArrBuilder.doneFast() ;
               break ;
            }
            case Object:
            {
               appendEx( fieldName, element.embeddedObject(), option ) ;
               break ;
            }
            case BinData:
            {
               if ( option.isAbbrevMode )
               {
                  // append in base64 mode
                  BSONObjBuilder binBuilder( subobjStart( fieldName ) ) ;
                  int dataLen = 0 ;
                  const char *dataBuf = element.binDataClean( dataLen ) ;
                  bool binPrinted = false ;
                  if ( dataLen > 0 )
                  {
                     TrivialAllocator al ;
                     int base64Len = getEnBase64Size( dataLen ) ;
                     char *base64Buf = (char *)al.Malloc( base64Len + 1 ) ;
                     if ( NULL != base64Buf )
                     {
                        memset( base64Buf, 0, base64Len + 1 ) ;
                        if ( base64Encode( dataBuf, dataLen,
                                           base64Buf, base64Len ) >= 0 )
                        {
                           StringData base64Value( base64Buf, base64Len ) ;
                           StackBufBuilder binValueBuilder ;
                           StringData binValue = _genAbbrevStr( binValueBuilder,
                                                                base64Value ) ;
                           binBuilder.append( "$binary", binValue ) ;
                           binPrinted = true ;
                        }
                        al.Free( base64Buf ) ;
                     }
                  }

                  if ( !binPrinted )
                  {
                     binBuilder.append( "$binary", "" ) ;
                  }
                  binBuilder.append( "$type", (INT32)( element.binDataType() ) ) ;
                  binBuilder.doneFast() ;
               }
               else
               {
                  appendAs( element, fieldName ) ;
               }
               break ;
            }
            case CodeWScope:
            {
               if ( option.isAbbrevMode )
               {
                  // code field
                  StackBufBuilder codeBuilder ;
                  const char *codeValuePtr = element.codeWScopeCode() ;
                  unsigned codeValueLen = strlen( codeValuePtr ) ;
                  StringData origValue( codeValuePtr, codeValueLen ) ;
                  StringData codeValue = _genAbbrevStr( codeBuilder,
                                                        origValue ) ;

                  // scope field
                  BSONObjBuilder scopeBuilder( 0 ) ;
                  scopeBuilder.appendEx( element.codeWScopeObject(), option ) ;
                  BSONObj scopeObject = scopeBuilder.done() ;

                  appendCodeWScope( fieldName, codeValue, scopeObject ) ;
               }
               else
               {
                  appendAs( element, fieldName ) ;
               }
               break ;
            }
            case Code:
            case Symbol:
            case bson::String:
            {
               if ( option.isAbbrevMode )
               {
                  StackBufBuilder valueBuilder ;
                  const char *stringValuePtr = element.valuestr() ;
                  unsigned stringValueLen = element.valuestrsize() - 1 ;
                  StringData origValue( stringValuePtr, stringValueLen ) ;
                  StringData stringValue = _genAbbrevStr( valueBuilder,
                                                          origValue ) ;
                  if ( Code == element.type() )
                  {
                     appendCode( fieldName, stringValue ) ;
                  }
                  else if ( Symbol == element.type() )
                  {
                     appendSymbol( fieldName, stringValue ) ;
                  }
                  else
                  {
                     append( fieldName, stringValue ) ;
                  }
               }
               else
               {
                  appendAs( element, fieldName ) ;
               }
               break ;
            }
            default:
            {
               appendAs( element, fieldName ) ;
               break ;
            }
         }
      }
      else
      {
         appendAs( element, fieldName ) ;
      }

      return ( *this ) ;
   }

   inline BSONObjBuilder &BSONObjBuilder::appendEx(
                                          const BSONObj &object,
                                          const BSONObjBuilderOption &option )
   {
      if ( option.isEnabled() )
      {
         BSONObjIterator iter( object ) ;
         while ( iter.more() )
         {
            BSONElement element( iter.next() ) ;
            appendEx( element, option ) ;
         }
      }
      else
      {
         appendElements( object ) ;
      }
      return (*this) ;
   }

}
