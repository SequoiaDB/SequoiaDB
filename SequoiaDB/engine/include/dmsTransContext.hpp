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

   /*
      _dmsTBTransContext define
   */
   class _dmsTBTransContext : public _IContext
   {
      public:
         _dmsTBTransContext( _dmsMBContext *pMBContext,
                             DMS_ACCESS_TYPE accessType ) ;
         virtual ~_dmsTBTransContext() ;

      protected:
         INT32       _checkAccess() ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume() ;

      protected:
         _dmsMBContext           *_pMBContext ;
         DMS_ACCESS_TYPE         _accessType ;

   } ;
   typedef _dmsTBTransContext dmsTBTransContext ;

   class _rtnIXScanner ;
   /*
      _dmsIXTransContext define
   */
   class _dmsIXTransContext : public _dmsTBTransContext
   {
      public:
         _dmsIXTransContext( _dmsMBContext *pMBContext,
                             DMS_ACCESS_TYPE accessType,
                             _rtnIXScanner *pScanner ) ;
         virtual ~_dmsIXTransContext() ;

         BOOLEAN  isCursorSame() const ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume() ;

      protected:
         _rtnIXScanner           *_pScanner ;
         BOOLEAN                 _isReadonly ;
         BOOLEAN                 _isSame ;

   } ;
   typedef _dmsIXTransContext dmsIXTransContext ;

}

#endif /* DMS_TRANS_CONTEXT_HPP__ */

