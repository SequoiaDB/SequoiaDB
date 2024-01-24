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

   Source File Name = omagentHelper.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_HELPER_HPP_
#define OMAGENT_HELPER_HPP_

#include "core.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ossTypes.hpp"
#include "../bson/bson.hpp"
#include "omagentSyncCmd.hpp"
#include "omagentBackgroundCmd.hpp"

using namespace bson ;

namespace engine
{
   BOOLEAN omaIsCommand ( const CHAR *name ) ;

   INT32 omaParseCommand ( const CHAR *name,
                           _omaCommand **ppCommand ) ;

   INT32 omaInitCommand ( _omaCommand *pCommand ,INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn, const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff, const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;

   INT32 omaRunCommand ( _omaCommand *pCommand, CHAR **ppBody,
                         INT32 &bodyLen ) ;

   INT32 omaRunCommand ( _omaCommand *pCommand, BSONObj &result ) ;

   INT32 omaReleaseCommand ( _omaCommand **ppCommand ) ;

   // build reply buffer
   INT32 omaBuildReplyMsgBody ( CHAR **ppBuffer, INT32 *bufferSize,
                                SINT32 numReturned,
                                vector<BSONObj> *objList ) ;
   INT32 omaBuildReplyMsgBody ( CHAR **ppBuffer, INT32 *bufferSize,
                                SINT32 numReturned,
                                const BSONObj *bsonobj ) ;

}

#endif // OMAGENT_HELPER_HPP_

