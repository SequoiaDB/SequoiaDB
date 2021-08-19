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

   Source File Name = catGTSMsgJob.hpp

   Descriptive Name = GTS message job

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_MSG_JOB_HPP_
#define CAT_GTS_MSG_JOB_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "rtnBackgroundJobBase.hpp"

namespace engine
{
   class _catGTSMsgHandler ;

   class _catGTSMsgJob: public _rtnBaseJob
   {
   public:
      _catGTSMsgJob( _catGTSMsgHandler* msgHandler, BOOLEAN isController, INT32 timeout ) ;
      virtual ~_catGTSMsgJob() ;

      OSS_INLINE BOOLEAN isController() const { return _isController ; }

   public:
      virtual RTN_JOB_TYPE type() const ;
      virtual const CHAR* name() const ;
      virtual BOOLEAN muteXOn( const _rtnBaseJob *pOther ) ;
      virtual INT32 doit() ;
      virtual BOOLEAN reuseEDU() const { return TRUE ; }

   private:
      _catGTSMsgHandler*   _msgHandler ;
      BOOLEAN              _isController ;
      INT32                _timeout ;
   } ;
   typedef _catGTSMsgJob catGTSMsgJob ;

   INT32 catStartGTSMsgJob( _catGTSMsgHandler* msgHandler,
                            BOOLEAN isController,
                            INT32 timeout ) ;
}

#endif /* CAT_GTS_MSG_JOB_HPP_ */

