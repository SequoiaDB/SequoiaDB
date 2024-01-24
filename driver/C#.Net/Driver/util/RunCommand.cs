/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

using System;
using System.Collections.Generic;
using System.Collections;
using System.IO;
using SequoiaDB.Bson;

namespace SequoiaDB
{
    class RunCommand
    {
        private static readonly Logger logger = new Logger("RunCommand");

        internal static DBCursor RunGeneralCmd(Sequoiadb sdb, string command,
                                               BsonDocument matcher, BsonDocument selector,
                                               BsonDocument orderBy, BsonDocument hint,
                                               int flags, ulong reqID, long skipNum, long retNum)
        {
            IConnection connection = sdb.Connection;
            bool isBigEndian = sdb.isBigEndian;
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_QUERY;
            sdbMessage.CollectionFullName = command;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.Flags = flags;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = reqID;
            sdbMessage.SkipRowsCount = skipNum;
            sdbMessage.ReturnRowsCount = retNum;
            sdbMessage.Matcher = (null == matcher) ? dummyObj : matcher;
            sdbMessage.Selector = (null == selector) ? dummyObj : selector;
            sdbMessage.OrderBy = (null == orderBy) ? dummyObj : orderBy;
            sdbMessage.Hint = (null == hint) ? dummyObj : hint;

            // check
            if (connection == null)
                throw new BaseException("SDB_NETWORK");
            // build msg
            byte[] request = SDBMessageHelper.BuildQueryRequest(sdbMessage, isBigEndian);
            // send
            connection.SendMessage(request);
            // extract
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            // check return msg header
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            // check whether error happen
            int errorCode = rtnSDBMessage.Flags;
            if (errorCode != 0)
            {
                // TODO: need to throw error detail(return from engine) out
                throw new BaseException(errorCode, rtnSDBMessage.ErrorObject);
            }
            // try to build return cursor
            if (SequoiadbConstants.DEFAULT_CONTEXTID == rtnSDBMessage.ContextIDList[0] &&
                 (rtnSDBMessage.ObjectList == null || rtnSDBMessage.ObjectList.Count == 0))
            {
                return null;
            }
            else
            {
                return new DBCursor( rtnSDBMessage, sdb );
            }
        }
    }
}
