/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = expCLFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_CLFILE_HPP_
#define EXP_CLFILE_HPP_

#include "oss.hpp"
#include "ossIO.hpp"
#include "../client/bson/bson.h"
#include "../client/client.h"
#include "../util/utilDecodeRawbson.hpp"
#include "expOptions.hpp"
#include "expCL.hpp"
#include <string>

namespace exprt
{
   using namespace std ;

   class buffBuilder : public utilBuffBuilderBase
   {
   public:
      buffBuilder() : _size( 0 ),
                      _buff( NULL )
      {
      }

      ~buffBuilder()
      {
         if ( _buff )
         {
            SAFE_OSS_FREE( _buff ) ;
            _buff = NULL ;
            _size = 0 ;
         }
      }

      CHAR *getBuff( UINT32 size )
      {
         CHAR *tmp = _buff ;

         if ( size > _size )
         {
            tmp = (CHAR *)SDB_OSS_REALLOC( _buff, size ) ;

            if ( tmp )
            {
               _buff = tmp ;
               _size = size ;
            }
         }

         return tmp ;
      }

      UINT32 getBuffSize(){ return _size ; }

   private:
      UINT32 _size ;
      CHAR  *_buff ;
   } ;

   class expConvertor : public SDBObject
   {
   public :
      expConvertor(){}
      virtual ~expConvertor(){}
      virtual INT32 init() { return SDB_OK ; }
      virtual INT32 head( const CHAR *&toBuf, UINT32 &toSize ) 
      {
         toBuf = NULL ;
         toSize = 0 ;
         return SDB_OK ;
      }
      virtual INT32 tail( const CHAR *&toBuf, UINT32 &toSize ) 
      {
         toBuf = NULL ;
         toSize = 0 ;
         return SDB_OK ;
      }
      virtual INT32 convert( bson &fromRecord, 
                             const CHAR *&toBuf, 
                             UINT32 &toSize ) = 0 ;
   protected :
      buffBuilder _bufferBuilder ;
   } ;

   class expJsonConvertor : public expConvertor
   {
   public :
      expJsonConvertor( const expOptions &options, const expCL &cl ) :
         _fieldsBuf(NULL), _options(options), _cl(cl)
      {
      }

      virtual ~expJsonConvertor()
      {
         _freeFieldsBuf() ;
      }

      virtual INT32 init() ;
      virtual INT32 convert( bson &fromRecord, 
                             const CHAR *&toBuf, 
                             UINT32 &toSize )  ;
   protected :
      void  _freeFieldsBuf() ;
   protected :
      CHAR             *_fieldsBuf ;
      const expOptions &_options ;
      const expCL      &_cl ;
      utilDecodeBson    _decodeBson ;
   } ;

   class expCSVConvertor : public expJsonConvertor
   {
   public :
      expCSVConvertor( const expOptions &options, const expCL &cl ) :
         expJsonConvertor( options, cl ) {}
      virtual INT32 head( const CHAR *&toBuf, UINT32 &toSize ) ;
      virtual INT32 convert( bson &fromRecord, 
                             const CHAR *&toBuf, 
                             UINT32 &toSize )  ;
   } ;

   class expCLOutput : public SDBObject
   {
   public :
      virtual ~expCLOutput() {}
      virtual INT32 open() { return SDB_OK ; }
      virtual void  close() {}
      virtual INT32 output( const CHAR *buf, UINT32 size ) = 0 ;
   } ;

   class expCLFile : public expCLOutput
   {
   public :
      expCLFile( const expOptions &options, const string &fileName ) ;
      virtual ~expCLFile() ;
      virtual INT32 open() ;
      virtual void  close() ;
      virtual INT32 output( const CHAR *buf, UINT32 sz ) ;
   protected :
      INT32 _nextFile() ;
   protected :
      const expOptions &_options ;
      string            _fileName ;
      OSSFILE           _file ;
      BOOLEAN           _opened ;
      UINT64            _writedSize ;
      UINT32            _fileSuffix ;
   } ;
}
#endif