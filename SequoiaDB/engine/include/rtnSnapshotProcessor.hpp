/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = rtnSnapshotProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/19/2022  YQC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_SNAPSHOT_PROCESSOR_HPP_
#define RTN_SNAPSHOT_PROCESSOR_HPP_

#include "ossTypes.h"
#include "rtnFetchBase.hpp"
#include "utilPooledObject.hpp"
#include "utilUniqueID.hpp"

using namespace bson ;

namespace engine
{
   typedef ossPoolList< BSONObj > SNAP_INFO_RESULT_LIST ;

   /*
      _rtnCSSnapshotProcessor
   */
   class _rtnCSSnapshotProcessor : public _IRtnMonProcessor
   {
   public:
      _rtnCSSnapshotProcessor() ;
      virtual ~_rtnCSSnapshotProcessor() ;

      virtual INT32   pushIn( const BSONObj &obj ) ;
      virtual INT32   output( BSONObj &obj, BOOLEAN &hasOut ) ;

      virtual INT32   done( BOOLEAN &hasOut ) ;
      virtual BOOLEAN eof() const ;

   protected:
      BOOLEAN _eof ;
      BOOLEAN _hasDone ;
      BSONObj _obj ;
   } ;
   typedef _rtnCSSnapshotProcessor rtnCSSnapshotProcessor ;
   typedef utilSharePtr< _rtnCSSnapshotProcessor > rtnCSSnapshotProcessorPtr ;

}

#endif // RTN_SNAPSHOT_PROCESSOR_HPP_