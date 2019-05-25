/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = ossCmdRunner.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_OSSRUNNER_HPP_
#define SPT_OSSRUNNER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossNPipe.hpp"
#include "ossProc.hpp"
#include "ossEvent.hpp"

#include <vector>
#include <map>
#include <string>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std ;

namespace engine
{
   class _ossCmdRunner : public SDBObject, public ossIExecHandle
   {
   public:
      _ossCmdRunner() ;
      virtual ~_ossCmdRunner() ;

   public:
      /*
         timeout is ms, ignored when isBackground = TRUE
         dupOut: whether duplicate the exe's stdout
      */
      INT32 exec( const CHAR *cmd, UINT32 &exit,
                  BOOLEAN isBackground = FALSE,
                  INT64 timeout = -1,
                  BOOLEAN needResize = FALSE,
                  OSSHANDLE *pHandle = NULL,
                  BOOLEAN addShellPrefix = FALSE,
                  BOOLEAN dupOut = TRUE ) ;

      INT32 done() ;

      INT32 read( string &out, BOOLEAN readEOF = TRUE ) ;

      OSSPID getPID() const { return _id ; }

   protected:
      virtual void  handleInOutPipe( OSSPID pid,
                                     OSSNPIPE * const npHandleStdin,
                                     OSSNPIPE * const npHandleStdout ) ;

      void  asyncRead() ;
      void  monitor() ;

      INT32 _readOut( string &out, BOOLEAN readEOF = TRUE ) ;

   private:
      OSSNPIPE       _out ;
      OSSPID         _id ;
      BOOLEAN        _stop ;
      ossEvent       _event ;
      ossEvent       _monitorEvent ;
      BOOLEAN        _hasRead ;
      string         _outStr ;
      INT32          _readResult ;
      INT64          _timeout ;

      boost::thread  *_pThread ;

   } ;
   typedef class _ossCmdRunner ossCmdRunner ;
}

#endif // SPT_OSSRUNNER_HPP_

