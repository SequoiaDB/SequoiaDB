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
using SequoiaDB.Bson;


/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Yang Qincheng
 */
namespace SequoiaDB
{
    /** \class DBSequence
     *  \brief Database operation interfaces of sequence
     */
    public class DBSequence
    {
        private string name;
        private string collectionFullName;
        private Sequoiadb sdb;
        internal bool isBigEndian = false;

        internal DBSequence(string name, Sequoiadb sdb)
        {
            this.name = name;
            this.sdb = sdb;
            this.isBigEndian = sdb.isBigEndian;
        }

        /** \property Name
         *  \brief Return the name of current sequence
         *  \return The sequence name
         */
        public string Name
        {
            get { return name; }
        }

        /** \fn BsonDocument Fetch(int fetchNum)
         *  \brief Fetch a bulk of continuous values.
         *  \param fetchNum The number of values to be fetched
         *  \return A BsonDocument that contains the following fields:
         *          <ul>
         *            <li>NextValue(long) : The next value and also the first returned value.
         *            <li>ReturnNum(int)  : The number of values returned.
         *            <li>Increment(int)  : Increment of values.
         *          </ul>
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public BsonDocument Fetch(int fetchNum)
        {
            if (fetchNum < 1)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "fetchNum cannot be less than 1");
            }

            BsonDocument matcher = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_NAME, this.name);
            matcher.Add(SequoiadbConstants.FIELD_NAME_FETCH_NUM, fetchNum);
            SDBMessage rtn = AdminCommand("", Operation.MSG_BS_SEQUENCE_FETCH_REQ, matcher);
            // check return flag
            int flags = rtn.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }
            DBCursor cursor = new DBCursor(rtn, this.sdb);
            BsonDocument result = cursor.Next();
            cursor.Close();
            return result;
        }

        /** \fn long GetCurrentValue()
         *  \brief Get the current value of sequence.
         *  \return The current value
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetCurrentValue()
        {
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.GET_SEQ_CURR_VAL;
            BsonDocument matcher = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_NAME, this.name);
            SDBMessage rtn = AdminCommand(command, Operation.OP_QUERY, matcher);
            // check return flag
            int flags = rtn.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }
            DBCursor cursor = new DBCursor(rtn, this.sdb);
            BsonDocument obj = cursor.Next();
            cursor.Close();
            if (obj != null)
            {
                BsonElement currentValue = obj.GetElement(SequoiadbConstants.FIELD_NAME_CURRENT_VALUE);
                if (currentValue != null)
                {
                    return (long)currentValue.Value;
                }
            }
            string errMsg = "Failed to get " + SequoiadbConstants.FIELD_NAME_CURRENT_VALUE + " from response message";
            throw new BaseException((int)Errors.errors.SDB_NET_BROKEN_MSG, errMsg);
        }

        /** \fn long GetNextValue()
         *  \brief Get the next value of sequence.
         *  \return The next value
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetNextValue()
        {
            BsonDocument obj = Fetch(1);
            string key = "NextValue";
            if (obj != null)
            {
                BsonElement result = obj.GetElement(key);
                if (result != null)
                {
                    return (long)result.Value;
                }
            }
            string errMsg = "Failed to get " + key + " from response message";
            throw new BaseException((int)Errors.errors.SDB_NET_BROKEN_MSG, errMsg);
        }

        /** \fn void Restart(long startValue)
         *  \brief Restart sequence from the given value.
         *  \param startValue The start value.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Restart(long startValue)
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_NAME_START_VALUE, startValue);
            _AlterInternal(SequoiadbConstants.SEQ_OPT_RESTART, matcher);
        }

        /** \fn void SetAttributes(BsonDocument options)
         *  \brief Alter sequence.
         *  \param options The options specified by user, details as bellow:
         *                <ul>
         *                  <li>CurrentValue(long) : The current value of sequence
         *                  <li>StartValue(long)   : The start value of sequence
         *                  <li>MinValue(long)     : The minimum value of sequence
         *                  <li>MaxValue(long)     : The maxmun value of sequence
         *                  <li>Increment(int)     : The increment value of sequence
         *                  <li>CacheSize(int)     : The cache size of sequence
         *                  <li>AcquireSize(int)   : The acquire size of sequence
         *                  <li>Cycled(boolean)    : The cycled flag of sequence
         *                </ul>
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void SetAttributes(BsonDocument options)
        {
            _AlterInternal(SequoiadbConstants.SDB_ALTER_SET_ATTRIBUTES, options);
        }

        /** \fn void SetCurrentValue(long value)
         *  \brief Set the current value to sequence.
         *  \param value The expected current value
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void SetCurrentValue(long value)
        {
            BsonDocument obj = new BsonDocument();
            obj.Add(SequoiadbConstants.FIELD_NAME_EXPECT_VALUE, value);
            _AlterInternal(SequoiadbConstants.SEQ_OPT_SET_CURR_VALUE, obj);
        }

        private void _AlterInternal(string actionName, BsonDocument options)
        {
            if (options != null && options.Contains(SequoiadbConstants.FIELD_NAME))
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ALTER_SEQUENCE;

            BsonDocument obj = new BsonDocument();
            BsonDocument subObj = new BsonDocument();
            subObj.Add(SequoiadbConstants.FIELD_NAME, this.name);
            subObj.Add(options);

            obj.Add(SequoiadbConstants.FIELD_NAME_ACTION, actionName);
            obj.Add(SequoiadbConstants.FIELD_OPTIONS, subObj);
            SDBMessage rtn = AdminCommand(command, Operation.OP_QUERY, obj);
            // check return flag
            int flags = rtn.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }
        }

        private SDBMessage AdminCommand(string command, Operation opCode, BsonDocument matcher)
        {
            BsonDocument dummyObj = new BsonDocument();
            IConnection connection = sdb.Connection;
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = opCode;
            sdbMessage.CollectionFullName = command;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.Flags = 0;
            //sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.SkipRowsCount = 0;
            sdbMessage.ReturnRowsCount = -1;
            // matcher
            if (matcher == null)
            {
                sdbMessage.Matcher = dummyObj;
            }
            else
            {
                sdbMessage.Matcher = matcher;
            }
            sdbMessage.Selector = dummyObj;
            sdbMessage.OrderBy = dummyObj;
            sdbMessage.Hint = dummyObj;

            byte[] request = SDBMessageHelper.BuildQueryRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            return rtnSDBMessage;
        }
    }
}
