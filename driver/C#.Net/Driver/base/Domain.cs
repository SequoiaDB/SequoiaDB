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

using System.Collections.Generic;
using System;
using SequoiaDB.Bson;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Zhaobo Tan
 */
namespace SequoiaDB
{
    /** \class Domain
     *  \brief Database operation interfaces of domain
     */
	public class Domain
	{
        private string name;
        private Sequoiadb sdb;
        internal bool isBigEndian = false;

        /** \property Name
         *  \brief Return the name of current domain
         *  \return The domain name
         */
        public string Name
        {
            get { return name; }
        }

        /** \property SequoiaDB
         *  \brief Return the Sequoiadb handle of current domain
         *  \return Sequoiadb object
         */
        public Sequoiadb SequoiaDB
        {
            get { return sdb; }
        }

        internal Domain(Sequoiadb sdb, string name)
        {
            this.name = name;
            this.sdb = sdb;
            this.isBigEndian = sdb.isBigEndian;
        }

        /** \fn void Alter(BsonDocumet options)
         *  \brief Alter current domain.
         *  \param [in] options The options user wants to alter
         *  
         *      Groups:    The list of replica groups' names which the domain is going to contain.
         *                 eg: { "Groups": [ "group1", "group2", "group3" ] }, it means that domain
         *                 changes to contain "group1", "group2" and "group3".
         *                 We can add or remove groups in current domain. However, if a group has data
         *                 in it, remove it out of domain will be failing.
         *      AutoSplit: Alter current domain to have the ability of automatically split or not. 
         *                 If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
         *                 the data of this collection will be split(hash split) into all the groups in this domain automatically.
         *                 However, it won't automatically split data into those groups which were add into this domain later.
         *                 eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Alter(BsonDocument options)
        {
            BsonValue tmp;

            // check argument
            if (null == options)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // alter collection
            if (options.TryGetValue(SequoiadbConstants.FIELD_NAME_ALTER, out tmp))
            {
                _AlterV2(options);
            }
            else
            {
                _AlterV1(options);
            }
        }

        private void _AlterV1(BsonDocument options)
        {
            // check
            if (null == options)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // append argument
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_NAME, this.name);
            newObj.Add(SequoiadbConstants.FIELD_OPTIONS, options);
            // cmd
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ALTER_CMD + " "
                             + SequoiadbConstants.DOMAIN;
            // run command
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, newObj, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }
        }

        private void _AlterV2(BsonDocument options)
        {
            // check argument
            if (null == options)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // build a bson to send
            BsonElement elem;
            bool flag = false;
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_NAME_ALTER_TYPE, SequoiadbConstants.SDB_ALTER_DOMAIN);
            newObj.Add(SequoiadbConstants.FIELD_NAME_VERSION, SequoiadbConstants.SDB_ALTER_VERSION);
            newObj.Add(SequoiadbConstants.FIELD_NAME, this.name);
            // append alters
            flag = options.TryGetElement(SequoiadbConstants.FIELD_NAME_ALTER, out elem);
            if (true == flag && (elem.Value.IsBsonDocument || elem.Value.IsBsonArray))
            {
                newObj.Add(elem);
            }
            else
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // append options
            flag = false;
            flag = options.TryGetElement(SequoiadbConstants.FIELD_OPTIONS, out elem);
            if (true == flag)
            {
                if (elem.Value.IsBsonDocument)
                {
                    newObj.Add(elem);
                }
                else
                {
                    throw new BaseException("SDB_INVALIDARG");
                }
            }

            // cmd
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ALTER_CMD + " "
                             + SequoiadbConstants.DOMAIN;
            // run command
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, newObj, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }
        }

        private void _AlterInternal(string taskName, BsonDocument arguments, Boolean allowNullArgs)
        {
            if (null == arguments && !allowNullArgs)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            BsonDocument alterObj = new BsonDocument();
            BsonDocument tmpObj = new BsonDocument();
            tmpObj.Add(SequoiadbConstants.FIELD_NAME, taskName);
            tmpObj.Add(SequoiadbConstants.FIELD_NAME_ARGS, arguments);
            alterObj.Add(SequoiadbConstants.FIELD_NAME_ALTER, tmpObj);
            Alter(alterObj);
        }

        /** \fn void SetAttributes(BsonDocumet options)
         *  \brief Alter current domain to set attributes.
         *  \param [in] options The options user wants to alter
         *  
         *      Groups:    The list of replica groups' names which the domain is going to contain.
         *                 eg: { "Groups": [ "group1", "group2", "group3" ] }, it means that domain
         *                 changes to contain "group1", "group2" and "group3".
         *                 We can add or remove groups in current domain. However, if a group has data
         *                 in it, remove it out of domain will be failing.
         *      AutoSplit: Alter current domain to have the ability of automatically split or not. 
         *                 If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
         *                 the data of this collection will be split(hash split) into all the groups in this domain automatically.
         *                 However, it won't automatically split data into those groups which were add into this domain later.
         *                 eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void SetAttributes(BsonDocument options)
        {
            _AlterInternal(SequoiadbConstants.SDB_ALTER_SET_ATTRIBUTES, options, false);
        }

        /** \fn void AddGroups(BsonDocumet options)
         *  \brief Alter current domain to add groups.
         *  \param [in] options The options user wants to alter
         *  
         *      Groups:    The list of replica groups' names which the domain is going to add.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void AddGroups(BsonDocument options)
        {
            _AlterInternal(SequoiadbConstants.SDB_ALTER_ADD_GROUPS, options, false);
        }

        /** \fn void SetGroups(BsonDocumet options)
         *  \brief Alter current domain to set groups.
         *  \param [in] options The options user wants to alter
         *  
         *      Groups:    The list of replica groups' names which the domain is going to contain.
         *                 We can add or remove groups in current domain. However, if a group has data
         *                 in it, remove it out of domain will be failing.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void SetGroups(BsonDocument options)
        {
            _AlterInternal(SequoiadbConstants.SDB_ALTER_SET_GROUPS, options, false);
        }

        /** \fn void removeGroups(BsonDocumet options)
         *  \brief Alter current domain to remove groups.
         *  \param [in] options The options user wants to alter
         *  
         *      Groups:    The list of replica groups' names which the domain is going to remove.
         *                 If a group has data in it, remove it out of domain will be failing.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void RemoveGroups(BsonDocument options)
        {
            _AlterInternal(SequoiadbConstants.SDB_ALTER_REMOVE_GROUPS, options, false);
        }

        /** \fn DBCursor ListCS()
         *  \brief List all the collection spaces in current domain.
         *  \return The DBCursor of result
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor ListCS()
        {
            return ListCSCL(SDBConst.SDB_LIST_CS_IN_DOMAIN);
        }

        /** \fn DBCursor ListCL()
         *  \brief List all the collections in current domain.
         *  \return The DBCursor of result
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor ListCL()
        {
            return ListCSCL(SDBConst.SDB_LIST_CL_IN_DOMAIN);
        }
        
        private DBCursor ListCSCL(int type)
        {
            // append argument
            BsonDocument matcher = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_DOMAIN, this.name);
            selector.Add(SequoiadbConstants.FIELD_NAME, BsonNull.Value);
            // get cs or cl in current domain 
            DBCursor cursor = this.sdb.GetList(type, matcher, selector, null);
            return cursor;
        }

        private SDBMessage AdminCommand(string cmdType, string contextType, string contextName)
        {
            IConnection connection = sdb.Connection;
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage sdbMessage = new SDBMessage();
            string commandString = SequoiadbConstants.ADMIN_PROMPT + cmdType + " " + contextType;

            BsonDocument cObj = new BsonDocument();
            cObj.Add(SequoiadbConstants.FIELD_NAME, contextName);
            sdbMessage.OperationCode = Operation.OP_QUERY;
            sdbMessage.Matcher = cObj;
            sdbMessage.CollectionFullName = commandString;
            sdbMessage.Flags = 0;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.SkipRowsCount = -1;
            sdbMessage.ReturnRowsCount = -1;
            sdbMessage.Selector = dummyObj;
            sdbMessage.OrderBy = dummyObj;
            sdbMessage.Hint = dummyObj;

            byte[] request = SDBMessageHelper.BuildQueryRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            return rtnSDBMessage;
        }

        private SDBMessage AdminCommand(string command, BsonDocument matcher, BsonDocument selector,
                                        BsonDocument orderBy, BsonDocument hint)
        {
            BsonDocument dummyObj = new BsonDocument();
            IConnection connection = sdb.Connection;
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_QUERY;
            sdbMessage.CollectionFullName = command;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.Flags = 0;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.SkipRowsCount = 0;
            sdbMessage.ReturnRowsCount = -1;
            // matcher
            if (null == matcher)
            {
                sdbMessage.Matcher = dummyObj;
            }
            else
            {
                sdbMessage.Matcher = matcher;
            }
            // selector
            if (null == selector)
            {
                sdbMessage.Selector = dummyObj;
            }
            else
            {
                sdbMessage.Selector = selector;
            }
            // orderBy
            if (null == orderBy)
            {
                sdbMessage.OrderBy = dummyObj;
            }
            else
            {
                sdbMessage.OrderBy = orderBy;
            }
            // hint
            if (null == hint)
            {
                sdbMessage.Hint = dummyObj;
            }
            else
            {
                sdbMessage.Hint = hint;
            }

            byte[] request = SDBMessageHelper.BuildQueryRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            return rtnSDBMessage;
        }

	}
}
