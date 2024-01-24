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
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class CollectionSpace
     *  \brief Database operation interfaces of collection space
     */
   public class CollectionSpace
   {
       private string name;
       private Sequoiadb sdb;
       internal bool isBigEndian = false;

       /** \property Name
        *  \brief Return the name of current collection space
        *  \return The collection space name
        */
       public string Name
       {
           get { return name; }
       }

        /** \property SequoiaDB
         *  \brief Return the Sequoiadb handle of current collection space
         *  \return Sequoiadb object
         */
        public Sequoiadb SequoiaDB
        {
            get { return sdb; }
        }

        internal CollectionSpace(Sequoiadb sdb, string name)
        {
            this.name = name;
            this.sdb = sdb;
            this.isBigEndian = sdb.isBigEndian;
        }

        /** \fn DBCollection GetCollection(string collectionName)
         *  \brief Get the named collection
         *  \param collectionName The collection name
         *  \return The DBCollection handle
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCollection GetCollection(string collectionName)
        {
            if (collectionName == null || collectionName.Length == 0)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.TEST_CMD + " "
                             + SequoiadbConstants.COLLECTION;
            BsonDocument condition = new BsonDocument();
            BsonDocument dummyObj = new BsonDocument();
            string fullName = this.Name + "." + collectionName;
            condition.Add(SequoiadbConstants.FIELD_NAME, fullName);
            SDBMessage rtn = AdminCommand(command, condition, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }

            sdb.UpsertCache(fullName);
            return new DBCollection(this, collectionName);
        }

        /** \fn bool IsCollectionExist(string colName)
         *  \brief Verify the existence of collection in current colleciont space
         *  \param colName The collection name
         *  \return True if collection existed or False if not existed
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public bool IsCollectionExist(string colName)
        {
            if (colName == null || colName.Length == 0)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.TEST_CMD + " "
                             + SequoiadbConstants.COLLECTION;
            BsonDocument condition = new BsonDocument();
            BsonDocument dummyObj = new BsonDocument();
            string fullName = this.Name + "." + colName;
            condition.Add(SequoiadbConstants.FIELD_NAME, fullName);
            SDBMessage rtn = AdminCommand(command, condition, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags == 0)
            {
                sdb.UpsertCache(fullName);
                return true;
            }
            else if (flags == (int)Errors.errors.SDB_DMS_NOTEXIST)
            {
                sdb.RemoveCache(fullName);
                return false;
            }
            else
            {
                throw new BaseException(flags, rtn.ErrorObject);
            }
        }

        /** \fn DBCollection CreateCollection(string collectionName)
         *  \brief Create the named collection in current collection space
         *  \param collectionName The collection name
         *  \return The DBCollection handle
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCollection CreateCollection(string collectionName) 
        {
            return CreateCollection(collectionName, null);
        }

        /** \fn DBCollection CreateCollection(string collectionName, BsonDocument options)
         *  \brief Create the named collection in current collection space
         *  \param collectionName The collection name
         *  \param options The options for creating collection. Please reference
         *             <a href="http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1432190821-edition_id-@SDB_SYMBOL_VERSION">here</a>
         *             for more detail.
         *  \return The DBCollection handle
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCollection CreateCollection(string collectionName, BsonDocument options)
        {
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CREATE_CMD + " "
                             + SequoiadbConstants.COLLECTION;
            BsonDocument cObj = new BsonDocument();
            BsonDocument dummyObj = new BsonDocument();

            string fullName = this.Name + "." + collectionName;
            cObj.Add(SequoiadbConstants.FIELD_NAME, fullName);
            if ( options != null && options.ElementCount !=0 )
            {
                foreach (string key in options.Names)
                {
                    cObj.Add(options.GetElement(key));
                }
            }
            //cObj.Add(SequoiadbConstants.FIELD_SHARDINGKEY, options[SequoiadbConstants.FIELD_SHARDINGKEY]);

            SDBMessage rtn = AdminCommand(command, cObj, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags, rtn.ErrorObject);
            sdb.UpsertCache(fullName);
            return new DBCollection(this, collectionName);
        }

        /** \fn void DropCollection(string collectionName)
         *  \brief Remove the named collection of current collection space
         *  \param collectionName The collection name
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void DropCollection(string collectionName)
        {
            string fullName = this.Name + "." + collectionName;
            SDBMessage rtn = AdminCommand(SequoiadbConstants.DROP_CMD, SequoiadbConstants.COLLECTION,
                fullName);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags, rtn.ErrorObject);
            sdb.RemoveCache(fullName);
        }

        /** \fn List<String> GetCollectionNames()
         *  \brief Get the names of all collections in the current collection space.
         *  \return A List of collection names
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public List<String> GetCollectionNames()
        {
            List<String> result = new List<String>();
            BsonDocument subObj = new BsonDocument();
            subObj.Add("$gt", this.name + ".");
            subObj.Add("$lt", this.name + "/");
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", subObj);
            DBCursor cursor = sdb.GetList(SDBConst.SDB_LIST_COLLECTIONS, matcher, null, null);
            try
            {
                while (cursor.Next() != null)
                {
                    BsonValue clFullName = cursor.Current().GetValue("Name");
                    if (clFullName.IsString)
                        result.Add(clFullName.AsString);
                    else
                        throw new BaseException("SDB_DMS_RECORD_INVALID");
                }
            }
            finally
            {
                cursor.Close();
            }
            return result;
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
            SDBMessage rtnMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnMessage);
            return rtnMessage;
        }


       /** \fn void Alter ( BsonDocument options )
        *  \brief Alter collection space.
        *  \param [in] options The options of collection space to be changed, e.g. { "PageSize": 4096, "Domain": "mydomain" }.
        *
        *          PageSize     : The page size of the collection space
        *          LobPageSize  : The page size of LOB objects in the collection space
        *          Domain       : The domain which the collection space belongs to
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void Alter(BsonDocument options)
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
           flag = options.TryGetElement(SequoiadbConstants.FIELD_NAME_ALTER, out elem);
           if (false == flag)
           {
               newObj.Add(SequoiadbConstants.FIELD_NAME, name);
               newObj.Add(SequoiadbConstants.FIELD_OPTIONS, options);
           }
           else
           {
               newObj.Add(SequoiadbConstants.FIELD_NAME_ALTER_TYPE, SequoiadbConstants.SDB_ALTER_CS);
               newObj.Add(SequoiadbConstants.FIELD_NAME_VERSION, SequoiadbConstants.SDB_ALTER_VERSION);
               newObj.Add(SequoiadbConstants.FIELD_NAME, name);

               // append alters
               if (elem.Value.IsBsonDocument || elem.Value.IsBsonArray)
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
           }

           // cmd
           string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ALTER_CS;
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

       /** \fn void SetDomain ( BsonDocument options )
        *  \brief Alter collection space to set domain
        *  \param [in] options The options of collection space to be changed.
        *          Domain       : The domain which the collection space belongs to
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void SetDomain(BsonDocument options)
       {
           _AlterInternal(SequoiadbConstants.SDB_ALTER_SET_DOMAIN, options, false);
       }

       /** \fn void RemoveDomain()
        *  \brief Alter collection space to remove domain
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void RemoveDomain()
       {
           _AlterInternal(SequoiadbConstants.SDB_ALTER_REMOVE_DOMAIN, null, true);
       }

       /** \fn string GetDomainName()
        *  \brief Get the domain name of the current collection space. Returns an empty string
        *         if the current collection space has no owning domain.
        *  \return The domain name.
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public string GetDomainName()
       {
           string result = "";
           string cmd = "select Domain from $LIST_CS where Name = '" + this.name + "'";
           DBCursor cursor = sdb.Exec(cmd);
           try
           {
               BsonDocument record = cursor.Next();
               if (record != null)
               {
                   BsonValue tmp = record.GetValue("Domain");
                   if (tmp.IsString)
                   {
                       result = tmp.AsString;
                   }
                   else
                   {
                       result = "";
                   }
               }
           }
           finally
           {
               cursor.Close();
           }
           return result;
       }

       /** \fn void EnableCapped()
        *  \brief Alter collection space to enable capped
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void EnableCapped()
       {
           _AlterInternal(SequoiadbConstants.SDB_ALTER_ENABLE_CAPPED, null, true);
       }

       /** \fn void DisableCapped()
        *  \brief Alter collection space to disable capped
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void DisableCapped()
       {
           _AlterInternal(SequoiadbConstants.SDB_ALTER_DISABLE_CAPPED, null, true);
       }

       /** \fn void SetAttributes( BsonDocument options )
        *  \brief Alter collection space.
        *  \param [in] options The options of collection space to be changed, e.g. { "PageSize": 4096, "Domain": "mydomain" }.
        *
        *          PageSize     : The page size of the collection space
        *          LobPageSize  : The page size of LOB objects in the collection space
        *          Domain       : The domain which the collection space belongs to
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void SetAttributes(BsonDocument options)
       {
           _AlterInternal(SequoiadbConstants.SDB_ALTER_SET_ATTRIBUTES, options, false);
       }

       /** \fn void RenameCollection(String oldName, String newName)
        *  \brief Rename the collection.
        *  \param oldName The original name of current collection.
        *  \param newName The new name of current collection.
        *  \return void
        *  \exception SequoiaDB.BaseException
        *  \exception System.Exception
        */
       public void RenameCollection(String oldName, String newName)
       {
           RenameCollection(oldName, newName, null);
       }

       private void RenameCollection(String oldName, String newName, BsonDocument options)
       {
           if (oldName == null || oldName.Length == 0)
           {
               throw new BaseException("SDB_INVALIDARG");
           }
           if (newName == null || newName.Length == 0)
           {
               throw new BaseException("SDB_INVALIDARG");
           }

           // build cmd
           string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CMD_NAME_RENAME_COLLECTION;

           // build object
           BsonDocument obj = new BsonDocument();
           obj.Merge(options);
           obj.Add(SequoiadbConstants.FIELD_NAME_CELLECTIONSPACE, name);
           obj.Add(SequoiadbConstants.FIELD_NAME_OLDNAME, oldName);
           obj.Add(SequoiadbConstants.FIELD_NAME_NEWNAME, newName);

           SDBMessage rtn = AdminCommand(command, obj, null, null, null);
           int flags = rtn.Flags;
           if (flags != 0)
           {
               throw new BaseException(flags, rtn.ErrorObject);
           }
           string fullName = this.Name + "." + oldName;
           sdb.RemoveCache(fullName);
       }
   }
}
