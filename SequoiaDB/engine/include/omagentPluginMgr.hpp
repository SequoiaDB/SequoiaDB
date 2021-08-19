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

   Source File Name = omagentPluginMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/05/2018  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_PLUGIN_MGR_HPP__
#define OMAGENT_PLUGIN_MGR_HPP__

#include "omagentDef.hpp"
#include "pmdOptionsMgr.hpp"
#include "omagentTask.hpp"

namespace engine
{
   class _omAgentPluginMgr
   {
   public:
      _omAgentPluginMgr() ;
      virtual ~_omAgentPluginMgr() ;

      virtual INT32 init ( _pmdCfgRecord *pOptions ) ;
      virtual INT32 active () ;
      virtual INT32 deactive () ;
      virtual INT32 fini () ;

      virtual void onConfigChange() ;

   private:
      INT32 _startPlugin() ;
      INT32 _stopPlugin() ;
      INT32 _startTask( _omaTask *pTask ) ;
      BOOLEAN _isGeneral() ;

   private:
      _pmdCfgRecord *_options ;
   } ;
   typedef _omAgentPluginMgr omAgentPluginMgr ;
}

#endif