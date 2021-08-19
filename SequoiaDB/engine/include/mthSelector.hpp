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

   Source File Name = mthSelector.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SELECTOR_HPP_
#define MTH_SELECTOR_HPP_

#include "mthSColumnMatrix.hpp"

namespace engine
{
   class _mthSelector : public SDBObject
   {
   public:
      _mthSelector() ;
      ~_mthSelector() ;

   public:
      OSS_INLINE BOOLEAN isInitialized() const
      {
         return _init ;
      }

      OSS_INLINE const bson::BSONObj getPattern() const
      {
         return _matrix.getPattern() ;
      }

      OSS_INLINE void setStringOutput ( BOOLEAN strOut )
      {
         _stringOutput = strOut ;
      }

      OSS_INLINE BOOLEAN getStringOutput () const
      {
         return _stringOutput ;
      }

   public:
      INT32 loadPattern( const bson::BSONObj &pattern, 
                         BOOLEAN strictDataMode = FALSE ) ;

      INT32 select( const bson::BSONObj &source,
                    bson::BSONObj &target ) ;

      INT32 move( _mthSelector &other ) ;

      void clear() ;

   private:
      INT32 _buildCSV( const bson::BSONObj &obj,
                       bson::BSONObj &csv ) ;

      INT32 _resortObj( const bson::BSONObj &pattern,
                        const bson::BSONObj &src,
                        bson::BSONObj &obj ) ;
   private:
      mthSColumnMatrix _matrix ;
      BOOLEAN _init ;

      ///csv
      BOOLEAN _stringOutput ;
      BOOLEAN _strictDataMode ;
      INT32 _stringOutputBufferSize ;
      CHAR *_stringOutputBuffer ;
   } ;
   typedef class _mthSelector mthSelector ;
}

#endif

