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

   Source File Name = dmsTransContext.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_TRANS_CONTEXT_HPP__
#define DMS_TRANS_CONTEXT_HPP__

#include "sdbInterface.hpp"
#include "dms.hpp"

using namespace bson ;

namespace engine
{

   class _dmsMBContext ;
   class _rtnScanner ;

   /*
      _dmsScanTransContext define
   */
   class _dmsScanTransContext : public _IContext
   {
      public:
         _dmsScanTransContext( _dmsMBContext *pMBContext,
                               _rtnScanner *pScanner,
                               DMS_ACCESS_TYPE accessType ) ;
         virtual ~_dmsScanTransContext() ;

      protected:
         INT32       _checkAccess() ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume() ;

         virtual void reset()
         {
            _isCursorSame = TRUE ;
         }

         virtual BOOLEAN isCursorSame() const
         {
            return _isCursorSame ;
         }

      protected:
         _dmsMBContext           *_pMBContext ;
         _rtnScanner             *_pScanner ;
         DMS_ACCESS_TYPE         _accessType ;
         BOOLEAN                 _isCursorSame ;

   } ;
   typedef _dmsScanTransContext dmsScanTransContext ;
   typedef _dmsScanTransContext dmsTBTransContext ;
   typedef _dmsScanTransContext dmsIXTransContext ;

}

#endif /* DMS_TRANS_CONTEXT_HPP__ */

