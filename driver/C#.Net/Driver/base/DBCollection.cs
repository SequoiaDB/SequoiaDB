using System.Collections.Generic;
using System;
using SequoiaDB.Bson;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class DBCollection
     *  \brief Database operation interfaces of collection
     */
    public class DBCollection
   {
        private string name;
        private string collectionFullName;
        private Sequoiadb sdb;
        private CollectionSpace collSpace;
        private IConnection connection;
        private bool ensureOID = true;
        internal bool isBigEndian = false;

        //private readonly Logger logger = new Logger("DBCollection");

        /** \property EnsureOID
         *  \brief Get or set whether insert oid in records when bulk insert
         *  \return True for ensure, false for not
         */
        public bool EnsureOID
        {
            get { return ensureOID; }
            set { ensureOID = value; }
        }

        /** \property Name
         *  \brief Return the name of current collection
         *  \return The collection name
         */
        public string Name
        {
            get { return name; }
        }

        /** \property FullName
         *  \brief Return the full name of current collection
         *  \return The collection name
         */
        public string FullName
        {
            get { return collectionFullName; }
        }

        /** \property CollSpace
         *  \ brief Return the Collection Space handle of current collection
         *  \return CollectionSpace object
         */
        public CollectionSpace CollSpace
        {
            get { return collSpace; }
        }

        internal DBCollection(CollectionSpace cs, string name)
        {
            this.name = name;
            this.sdb = cs.SequoiaDB;
            this.collSpace = cs;
            this.collectionFullName = cs.Name + "." + name;
            this.connection = cs.SequoiaDB.Connection;
            this.isBigEndian = cs.isBigEndian;
        }

        /* \fn void Rename(string newName)
         *  \brief Rename the collection
         *  \param newName The new collection name
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        /*
        public void Rename(string newName)
        {
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.RENAME_CMD + " "
                             + SequoiadbConstants.COLLECTION;
            BsonDocument matcher = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_COLLECTIONSPACE, collSpace.Name);
            matcher.Add(SequoiadbConstants.FIELD_OLDNAME, name);
            matcher.Add(SequoiadbConstants.FIELD_NEWNAME, newName);

            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, matcher, dummyObj, dummyObj, dummyObj, -1, -1);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            else
            {
                this.name = newName;
                this.collectionFullName = collSpace.Name + "." + newName;
            }
        }
        */

        /** \fn void Split(string sourceGroupName, string destGroupName,
                           BsonDocument splitCondition, BsonDocument splitEndCondition)
         *  \brief Split the collection from one group into another group by range
         *  \param sourceGroupName The source group
         *  \param destGroupName The destination group
         *  \param splitCondition The split condition
         *  \param splitEndCondition The split end condition or null
         *		eg:If we create a collection with the option {ShardingKey:{"age":1},ShardingType:"Hash",Partition:2^10},
    	 *		we can fill {age:30} as the splitCondition, and fill {age:60} as the splitEndCondition. when split,
    	 *		the targe group will get the records whose age's hash value are in [30,60). If splitEndCondition is null,
    	 *		they are in [30,max).
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Split(string sourceGroupName, string destGroupName,
            BsonDocument splitCondition, BsonDocument splitEndCondition)
        {
            // check argument
            if ((null == sourceGroupName || sourceGroupName.Equals("")) ||
                (null == destGroupName || destGroupName.Equals("")) ||
                null == splitCondition) {
                    throw new BaseException("SDB_INVALIDARG");
            }
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.SPLIT_CMD;
            BsonDocument matcher = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            matcher.Add(SequoiadbConstants.FIELD_SOURCE, sourceGroupName);
            matcher.Add(SequoiadbConstants.FIELD_TARGET, destGroupName);
            matcher.Add(SequoiadbConstants.FIELD_SPLITQUERY, splitCondition);
            if(null != splitEndCondition)
                matcher.Add(SequoiadbConstants.FIELD_SPLITENDQUERY, splitEndCondition);

            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, matcher, dummyObj, dummyObj, dummyObj, -1, -1, 0);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn void Split(string sourceGroupName, string destGroupName, double percent)
         *  \brief Split the collection from one group into another group by percent
         *  \param sourceGroupName The source group
         *  \param destGroupName The destination group
         *  \param percent percent The split percent, Range:(0.0, 100.0]
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Split(string sourceGroupName, string destGroupName, double percent)
        {
            // check argument
            if ((null == sourceGroupName || sourceGroupName.Equals("")) ||
                (null == destGroupName || destGroupName.Equals("")) ||
                (percent <= 0.0 || percent > 100.0))
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.SPLIT_CMD;
            BsonDocument matcher = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            matcher.Add(SequoiadbConstants.FIELD_SOURCE, sourceGroupName);
            matcher.Add(SequoiadbConstants.FIELD_TARGET, destGroupName);
            matcher.Add(SequoiadbConstants.FIELD_SPLITPERCENT, percent);

            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, matcher, dummyObj, dummyObj, dummyObj, -1, -1, 0);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn long SplitAsync(String sourceGroupName,
		 *	                    String destGroupName,
		 *	                    BsonDocument splitCondition,
		 *	                    BsonDocument splitEndCondition)
	     *  \brief Split the specified collection from source group to target group by range asynchronously.
         *  \param sourceGroupName the source group name
         *  \param destGroupName the destination group name
         *  \param splitCondition
	     *            the split condition
         *  \param splitEndCondition
	     *            the split end condition or null
	     *            eg:If we create a collection with the option {ShardingKey:{"age":1},ShardingType:"Hash",Partition:2^10},
         *				 we can fill {age:30} as the splitCondition, and fill {age:60} as the splitEndCondition. when split,
         *			 	 the targe group will get the records whose age's hash values are in [30,60). If splitEndCondition is null,
         *			 	 they are in [30,max).
         *  \return return the task id, we can use the return id to manage the sharding which is run backgroup.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
	     *  \see listTask, cancelTask
	     */
        public long SplitAsync(String sourceGroupName,
			                   String destGroupName,
			                   BsonDocument splitCondition,
			                   BsonDocument splitEndCondition)
        {
            // check argument
            if (sourceGroupName == null || sourceGroupName.Equals("") ||
                destGroupName == null || destGroupName.Equals("") ||
                splitCondition == null)
                throw new BaseException("SDB_INVALIDARG");
            // build a bson to send
            BsonDocument asyncObj = new BsonDocument();
            asyncObj.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            asyncObj.Add(SequoiadbConstants.FIELD_SOURCE, sourceGroupName);
            asyncObj.Add(SequoiadbConstants.FIELD_TARGET, destGroupName);
            asyncObj.Add(SequoiadbConstants.FIELD_SPLITQUERY, splitCondition);
            if (splitEndCondition != null && splitEndCondition.ElementCount != 0)
                asyncObj.Add(SequoiadbConstants.FIELD_SPLITENDQUERY, splitEndCondition);
            asyncObj.Add(SequoiadbConstants.FIELD_ASYNC, true);
            // build run command
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.SPLIT_CMD;
            // run command
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtnSDBMessage = AdminCommand(commandString, asyncObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // build cursor object to get result from database
            DBCursor cursor = new DBCursor(rtnSDBMessage, this);
            BsonDocument result = cursor.Next();
            if (result == null)
                throw new BaseException("SDB_CAT_TASK_NOTFOUND");
            bool flag = result.Contains(SequoiadbConstants.FIELD_TASKID);
            if (!flag)
                throw new BaseException("SDB_CAT_TASK_NOTFOUND");
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            long taskid = result.GetValue(SequoiadbConstants.FIELD_TASKID).AsInt64;
            return taskid;
        }

        /** \fn long SplitAsync(String sourceGroupName,
		 *	                    String destGroupName,
		 *	                    double percent)
	     *  \brief Split the specified collection from source group to target group by percent asynchronously.
         *  \param sourceGroupName the source group name
         *  \param destGroupName the destination group name
         *  \param percent
	     *            the split percent, Range:(0,100]
         *  \return return the task id, we can use the return id to manage the sharding which is run backgroup.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
	     *  \see listTask, cancelTask
	     */
        public long SplitAsync(String sourceGroupName,
                               String destGroupName,
                               double percent)
        {
            // check argument
            if (sourceGroupName == null || sourceGroupName.Equals("") ||
                destGroupName == null || destGroupName.Equals("") ||
                percent <= 0.0 || percent > 100.0)
                throw new BaseException("SDB_INVALIDARG");
            // build a bson to send
            BsonDocument asyncObj = new BsonDocument();
            asyncObj.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            asyncObj.Add(SequoiadbConstants.FIELD_SOURCE, sourceGroupName);
            asyncObj.Add(SequoiadbConstants.FIELD_TARGET, destGroupName);
            asyncObj.Add(SequoiadbConstants.FIELD_SPLITPERCENT, percent);
            asyncObj.Add(SequoiadbConstants.FIELD_ASYNC, true);
            // build run command
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.SPLIT_CMD;
            // run command
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtnSDBMessage = AdminCommand(commandString, asyncObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // build cursor object to get result from database
            DBCursor cursor = new DBCursor(rtnSDBMessage, this);
            BsonDocument result = cursor.Next();
            if (result == null)
                throw new BaseException("SDB_CAT_TASK_NOTFOUND");
            bool flag = result.Contains(SequoiadbConstants.FIELD_TASKID);
            if (!flag)
                throw new BaseException("SDB_CAT_TASK_NOTFOUND");
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            long taskid = result.GetValue(SequoiadbConstants.FIELD_TASKID).AsInt64;
            return taskid;
        }

        /** \fn BsonValue Insert(BsonDocument record)
         *  \brief Insert a document into current collection
         *  \param insertor The Bson document of insertor, can't be null
         *  \return The value of field "_id" in "insertor", if "insertor" has no field "_id",
         *          API will add one and return the value which type is ObjectId
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public BsonValue Insert(BsonDocument record) 
        {
            if (record == null)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_INSERT;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.CollectionFullName = collectionFullName;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.Insertor = record;

            ObjectId objId;
            BsonValue retVal;
            if (!record.TryGetValue(SequoiadbConstants.OID, out retVal))
            {
                objId = ObjectId.GenerateNewId();
                retVal = objId;
                record.Add(SequoiadbConstants.OID, objId);
            }
            List<BsonDocument> list = new List<BsonDocument>(1);
            list.Add(record);
            BulkInsert(list, 0);
            return retVal;
        }

        /** \fn void BulkInsert(List<BsonDocument> records, int flag)
         *  \brief Insert a bulk of bson objects into current collection
         *  \param records The Bson document of insertor list, can't be null
         *  \param flag SDBConst.FLG_INSERT_CONTONDUP or 0
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void BulkInsert(List<BsonDocument> records, int flag)
        {
            if (records == null || records.Count == 0)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG);
            }
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_INSERT;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.CollectionFullName = collectionFullName;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            if (flag != 0 && flag != SDBConst.FLG_INSERT_CONTONDUP)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid input flag");
            }
            sdbMessage.Flags = flag;

            byte[] request = SDBMessageHelper.BuildBulkInsertRequest(sdbMessage, records, EnsureOID, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            int errorCode = rtnSDBMessage.Flags;
            if (errorCode != 0)
            {
                throw new BaseException(errorCode);
            }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn void Delete(BsonDocument matcher)
         *  \brief Delete the matching document of current collection
         *  \param matcher
         *            The matching condition, delete all the documents if null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Delete(BsonDocument matcher)
        {
            Delete(matcher, new BsonDocument());
        }

        /** \fn void Delete(BsonDocument matcher, BsonDocument hint)
         *  \brief Delete the matching document of current collection
         *  \param matcher
         *            The matching condition, delete all the documents if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Delete(BsonDocument matcher, BsonDocument hint)
        {
            SDBMessage sdbMessage = new SDBMessage();
            BsonDocument dummyObj = new BsonDocument();
            if (matcher == null)
            {
                matcher = dummyObj;
            }
            if (hint == null)
            {
                hint = dummyObj;
            }

            sdbMessage.OperationCode = Operation.OP_DELETE;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.Flags = 0;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;

            sdbMessage.CollectionFullName = collectionFullName;
            sdbMessage.RequestID = 0;
            sdbMessage.Matcher = matcher;
            sdbMessage.Hint = hint;

            byte[] request = SDBMessageHelper.BuildDeleteRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            int errorCode = rtnSDBMessage.Flags;
            if (errorCode != 0)
            {
                throw new BaseException(errorCode);
            }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
       }

        /** \fn void Update(DBQuery query)
         *  \brief Update the document of current collection
         *  \param query DBQuery with matching condition, updating rule and hint
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \note When flag is set to 0, it won't work to update the "ShardingKey" field, but the
         *        other fields take effect
         */
        public void Update(DBQuery query)
        {
            _Update(query.Flag, query.Matcher, query.Modifier, query.Hint);
        }

        /** \fn void Update(BsonDocument matcher, BsonDocument modifier, BsonDocument hint)
         *  \brief Update the document of current collection
         *  \param matcher
         *            The matching condition, update all the documents if null
         *  \param modifier
         *            The updating rule, can't be null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \note It won't work to update the "ShardingKey" field, but the other fields take effect
         */
        public void Update(BsonDocument matcher, BsonDocument modifier, BsonDocument hint)
        {
            _Update(0, matcher, modifier, hint);
        }

        /** \fn void Update(BsonDocument matcher, BsonDocument modifier, BsonDocument hint, int flag)
         *  \brief Update the document of current collection
         *  \param matcher
         *            The matching condition, update all the documents if null
         *  \param modifier
         *            The updating rule, can't be null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param flag
         *            The update flag, default to be 0. Please see the definition
         *            of follow flags for more detail.
         *
         *      SDBConst.FLG_UPDATE_KEEP_SHARDINGKEY
         *
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \note When flag is set to 0, it won't work to update the "ShardingKey" field, but the
         *        other fields take effect
         */
        public void Update(BsonDocument matcher, BsonDocument modifier, BsonDocument hint, int flag)
        {
            _Update(flag, matcher, modifier, hint);
        }

        /** \fn void Upsert(BsonDocument matcher, BsonDocument modifier, BsonDocument hint)
         *  \brief Update the document of current collection, insert if no matching
         *  \param matcher
         *            The matching condition, update all the documents
         *            if null(that's to say, we match all the documents)
         *  \param modifier
         *            The updating rule, can't be null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \note It won't work to upsert the "ShardingKey" field, but the other fields take effect
         */
        public void Upsert(BsonDocument matcher, BsonDocument modifier, BsonDocument hint)
        {
            _Update(SequoiadbConstants.FLG_UPDATE_UPSERT, matcher, modifier, hint);
        }

        /** \fn void Upsert(BsonDocument matcher, BsonDocument modifier, BsonDocument hint, BsonDocument setOnInsert)
         *  \brief Update the document of current collection, insert if no matching
         *  \param matcher
         *            The matching condition, update all the documents
         *            if null(that's to say, we match all the documents)
         *  \param modifier
         *            The updating rule, can't be null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param setOnInsert The setOnInsert assigns the specified values to the fileds when insert
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \note It won't work to upsert the "ShardingKey" field, but the other fields take effect
         */
        public void Upsert(BsonDocument matcher, BsonDocument modifier, BsonDocument hint, BsonDocument setOnInsert)
        {
            Upsert(matcher, modifier, hint, setOnInsert, 0);
        }

        /** \fn void Upsert(BsonDocument matcher, BsonDocument modifier, BsonDocument hint, BsonDocument setOnInsert, int flag)
         *  \brief Update the document of current collection, insert if no matching
         *  \param matcher
         *            The matching condition, update all the documents
         *            if null(that's to say, we match all the documents)
         *  \param modifier
         *            The updating rule, can't be null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param setOnInsert
         *            The setOnInsert assigns the specified values to the fileds when insert
         *  \param flag
         *            The upsert flag, default to be 0. Please see the definition
         *            of follow flags for more detail.
         *
         *      SDBConst.FLG_UPDATE_KEEP_SHARDINGKEY
         *
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \note When flag is set to 0, it won't work to update the "ShardingKey" field, but the
         *        other fields take effect
         */
        public void Upsert(BsonDocument matcher, BsonDocument modifier, BsonDocument hint,
                           BsonDocument setOnInsert, int flag)
        {
            BsonDocument newHint;
            if (setOnInsert != null)
            {
                newHint = new BsonDocument(); ;
                if (hint != null)
                {
                    newHint.Merge(hint);
                }
                newHint.Add(SequoiadbConstants.FIELD_SET_ON_INSERT, setOnInsert);
            }
            else
            {
                newHint = hint;
            }
            flag |= SequoiadbConstants.FLG_UPDATE_UPSERT;
            _Update(flag, matcher, modifier, newHint);
        }

        /** \fn DBCursor Query()
         *  \brief Find all documents of current collection
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Query()
        {
            return Query(null, null, null, null, 0, -1);
        }

        /** \fn DBCursor Query(DBQuery query)
         *  \brief Find documents of current collection with DBQuery
         *  \param query DBQuery with matching condition, selector, order rule, hint, SkipRowsCount and ReturnRowsCount
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Query(DBQuery query)
        {
            BsonDocument dummyObj = new BsonDocument();
            BsonDocument matcher = query.Matcher;
            BsonDocument selector = query.Selector;
            BsonDocument orderBy = query.OrderBy;
            BsonDocument hint = query.Hint;
            long skipRows = query.SkipRowsCount;
            long returnRows = query.ReturnRowsCount;
            int flag = query.Flag;

            return Query(matcher, selector, orderBy, hint, skipRows, returnRows, flag);
        }

        /** \fn DBCursor Query(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint)
         *  \brief Find documents of current collection
         *  \param query
         *            The matching rule, return all the documents if null
         *  \param selector
         *            The selective rule, return the whole document if null
         *  \param orderBy
         *            The ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Query(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint)
        {
            return Query(query, selector, orderBy, hint, 0, -1);
        }

        /** \fn DBCursor Query(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
         *                     long skipRows, long returnRows)
         *  \brief Find documents of current collection
         *  \param query
         *            The matching rule, return all the documents if null
         *  \param selector
         *            The selective rule, return the whole document if null
         *  \param orderBy
         *            The ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param skipRows
         *            Skip the first numToSkip documents, never skip if this parameter is 0
         *  \param returnRows
         *            Return the specified amount of documents,
         *            when returnRows is 0, return nothing,
         *            when returnRows is -1, return all the documents
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Query(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
                              long skipRows, long returnRows)
        {
            return Query(query, selector, orderBy, hint, skipRows, returnRows, 0);
        }

        /** \fn DBCursor Query(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
         *                     long skipRows, long returnRows, int flag)
         *  \brief Find documents of current collection
         *  \param query
         *            The matching rule, return all the documents if null
         *  \param selector
         *            The selective rule, return the whole document if null
         *  \param orderBy
         *            The ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param skipRows
         *            Skip the first numToSkip documents, never skip if this parameter is 0
         *  \param returnRows
         *            Return the specified amount of documents,
         *            when returnRows is 0, return nothing,
         *            when returnRows is -1, return all the documents
         *  \param flag
         *            The query flag, default to be 0. Please see the definition
         *            of follow flags for more detail. Usage:
         *            e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
         *
         *      DBQuery.FLG_QUERY_FORCE_HINT
         *      DBQuery.FLG_QUERY_PARALLED
         *      DBQuery.FLG_QUERY_WITH_RETURNDATA
         *
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Query(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
                              long skipRows, long returnRows, int flag)
        {
            int newFlags = DBQuery.RegulateFlag(flag);
            BsonDocument dummyObj = new BsonDocument();
            if (query == null)
            {
                query = dummyObj;
            }
            if (selector == null)
            {
                selector = dummyObj;
            }
            if (orderBy == null)
            {
                orderBy = dummyObj;
            }
            if (hint == null)
            {
                hint = dummyObj;
            }
            if (returnRows < 0)
            {
                returnRows = -1;
            }
            if (returnRows == 1)
            {
                newFlags = newFlags | DBQuery.FLG_QUERY_WITH_RETURNDATA;
            }
            SDBMessage rtnSDBMessage = AdminCommand(collectionFullName, query, selector,
                                                    orderBy, hint, skipRows, returnRows, newFlags);
            int errorCode = rtnSDBMessage.Flags;
            if (errorCode != 0)
                if (errorCode == SequoiadbConstants.SDB_DMS_EOC)
                {
                    return null;
                }
                else
                {
                    throw new BaseException(errorCode);
                }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return new DBCursor(rtnSDBMessage, this);
        }

        private DBCursor _queryAndModify(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
                                         BsonDocument update, long skipRows, long returnRows, int flag, bool isUpdate, bool returnNew)
        {
            BsonDocument modify = new BsonDocument();
            if (isUpdate)
            {
                if (update == null)
                {
                    throw new BaseException("SDB_INVALIDARG");
                }

                modify.Add(SequoiadbConstants.FIELD_OP, SequoiadbConstants.FIELD_OP_VALUE_UPDATE);
                modify.Add(SequoiadbConstants.FIELD_OP_UPDATE, update);
                modify.Add(SequoiadbConstants.FIELD_RETURNNEW, returnNew);
            }
            else
            {
                modify.Add(SequoiadbConstants.FIELD_OP, SequoiadbConstants.FIELD_OP_VALUE_REMOVE);
                modify.Add(SequoiadbConstants.FIELD_OP_REMOVE, true);
            }

            BsonDocument newHint = new BsonDocument();
            if (hint != null)
            {
                newHint.Merge(hint);
            }
            newHint.Add(SequoiadbConstants.FIELD_MODIFY, modify);

            flag |= DBQuery.FLG_QUERY_MODIFY;
            return Query(query, selector, orderBy, newHint, skipRows, returnRows, flag);
        }

        /** \fn DBCursor QueryAndUpdate(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
         *                              BsonDocument update, long skipRows, long returnRows, int flag, bool returnNew)
         *  \brief Find documents of current collection and update
         *  \param query
         *            The matching rule, return all the documents if null
         *  \param selector
         *            The selective rule, return the whole document if null
         *  \param orderBy
         *            The ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param skipRows
         *            Skip the first numToSkip documents, never skip if this parameter is 0
         *  \param returnRows
         *            Return the specified amount of documents,
         *            when returnRows is 0, return nothing,
         *            when returnRows is -1, return all the documents
         *  \param update The update rule, can't be null
         *  \param flag
         *            The query flag, default to be 0. Please see the definition
         *            of follow flags for more detail. Usage:
         *            e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
         *
         *      DBQuery.FLG_QUERY_FORCE_HINT
         *      DBQuery.FLG_QUERY_PARALLED
         *      DBQuery.FLG_QUERY_WITH_RETURNDATA
         *      DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE
         *
         *  \param returnNew When true, returns the updated document rather than the original
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor QueryAndUpdate(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
                                       BsonDocument update, long skipRows, long returnRows, int flag, bool returnNew)
        {
            return _queryAndModify(query, selector, orderBy, hint, update, skipRows, returnRows, flag, true, returnNew);
        }

        /** \fn DBCursor QueryAndRemove(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
         *                              long skipRows, long returnRows, int flag)
         *  \brief Find documents of current collection and remove
         *  \param query
         *            The matching rule, return all the documents if null
         *  \param selector
         *            The selective rule, return the whole document if null
         *  \param orderBy
         *            The ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param skipRows
         *            Skip the first numToSkip documents, never skip if this parameter is 0
         *  \param returnRows
         *            Return the specified amount of documents,
         *            when returnRows is 0, return nothing,
         *            when returnRows is -1, return all the documents
         *  \param flag
         *            The query flag, default to be 0. Please see the definition
         *            of follow flags for more detail. Usage:
         *            e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
         *
         *      DBQuery.FLG_QUERY_FORCE_HINT
         *      DBQuery.FLG_QUERY_PARALLED
         *      DBQuery.FLG_QUERY_WITH_RETURNDATA
         *
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor QueryAndRemove(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
                                       long skipRows, long returnRows, int flag)
        {
            return _queryAndModify(query, selector, orderBy, hint, null, skipRows, returnRows, flag, false, false);
        }

        /** \fn DBCursor Explain(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
         *                       long skipRows, long returnRows, int flag, BsonDocument options)
         *  \brief Find documents of current collection
         *  \param query
         *            The matching rule, return all the documents if null
         *  \param selector
         *            The selective rule, return the whole document if null
         *  \param orderBy
         *            The ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan);
         *            {"":null} means table scan. when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param skipRows
         *            Skip the first numToSkip documents, never skip if this parameter is 0
         *  \param returnRows
         *            Return the specified amount of documents,
         *            when returnRows is 0, return nothing,
         *            when returnRows is -1, return all the documents
         *  \param flag 
         *            The query flag, default to be 0. Please see the definition of follow flags for more detail. 
         *            Usage: e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
         *
         *      DBQuery.FLG_QUERY_FORCE_HINT
         *      DBQuery.FLG_QUERY_PARALLED
         *      DBQuery.FLG_QUERY_WITH_RETURNDATA
         *
         *  \param [in] options The rules of query explain, the options are as below:
         *
         *      Run     : Whether execute query explain or not, true for excuting query explain then get
         *                the data and time information; false for not excuting query explain but get the
         *                query explain information only. e.g. {Run:true}
         *  \return The DBCursor of matching documents or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Explain(BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint,
                                long skipRows, long returnRows, int flag, BsonDocument options)
        {
            BsonDocument newObj = new BsonDocument();
            if (null != hint)
            {
                newObj.Add(SequoiadbConstants.FIELD_HINT, hint);
            }
            if (null != options)
            {
                newObj.Add(SequoiadbConstants.FIELD_OPTIONS, options);
            }

            return Query(query, selector, orderBy, newObj, skipRows, returnRows, flag | DBQuery.FLG_QUERY_EXPLAIN);
        }

        /** \fn DBCursor GetIndexes()
         *  \brief Get all the indexes of current collection
         *  \return A cursor of all indexes or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor GetIndexes()
        {
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.GET_INXES;
            BsonDocument dummyObj = new BsonDocument();
            BsonDocument obj = new BsonDocument();
            obj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);

            SDBMessage rtn = AdminCommand(commandString, dummyObj, dummyObj, dummyObj, obj, -1, -1, 0);

            int flags = rtn.Flags;
            if (flags != 0)
                if (flags == SequoiadbConstants.SDB_DMS_EOC)
                    return null;
                else
                {
                    throw new BaseException(flags);
                }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return new DBCursor(rtn, this);
        }

        /** \fn DBCursor GetIndex(string name)
         *  \brief Get the named index of current collection
         *  \param name The index name
         *  \return A index, if not exist then return null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor GetIndex(string name)
        {
            if (name == null)
                return GetIndexes();
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.GET_INXES;
            BsonDocument dummyObj = new BsonDocument();
            BsonDocument obj = new BsonDocument();
            BsonDocument conndition = new BsonDocument();
            obj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            conndition.Add(SequoiadbConstants.IXM_INDEXDEF + "." + SequoiadbConstants.IXM_NAME,
                    name);

            SDBMessage rtn = AdminCommand(commandString, conndition, dummyObj, dummyObj, obj, -1, -1, 0);

            int flags = rtn.Flags;
            if (flags != 0)
                if (flags == SequoiadbConstants.SDB_DMS_EOC)
                    return null;
                else
                {
                    throw new BaseException(flags);
                }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return new DBCursor(rtn, this);
        }

        /** \fn void CreateIndex(string name, BsonDocument key, bool isUnique, bool isEnforced)
         *  \brief Create a index with name and key
         *  \param name The index name
         *  \param key The index key
         *  \param isUnique Whether the index elements are unique or not
         *  \param isEnforced Whether the index is enforced unique.
         *                    This element is meaningful when isUnique is group to true.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void CreateIndex(string name, BsonDocument key, bool isUnique, bool isEnforced)
        {
            _CreateIndex(name, key, isUnique, isEnforced, SequoiadbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE);
        }

        /** \fn void CreateIndex(string name, BsonDocument key, bool isUnique, bool isEnforced, int sortBufferSize)
         *  \brief Specify sort buffer size to create a index
         *  \param name The index name
         *  \param key The index key
         *  \param isUnique Whether the index elements are unique or not
         *  \param isEnforced Whether the index is enforced unique.
         *                    This element is meaningful when isUnique is group to true.
         *  \param sortBufferSize The size of sort buffer used when creating index, the unit is MB,
         *                        zero means don't use sort buffer.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void CreateIndex(string name, BsonDocument key, bool isUnique, bool isEnforced, int sortBufferSize)
        {
            _CreateIndex(name, key, isUnique, isEnforced, sortBufferSize);
        }

        /** \fn void DropIndex(string name)
         *  \brief Remove the named index of current collection
         *  \param name The index name
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void DropIndex(string name)
        {
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.DROP_INX;
            BsonDocument dummyObj = new BsonDocument();
            BsonDocument dropObj = new BsonDocument();
            BsonDocument index = new BsonDocument();
            index.Add("", name);
            dropObj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            dropObj.Add(SequoiadbConstants.FIELD_INDEX, index);
            SDBMessage rtn = AdminCommand(commandString, dropObj, dummyObj, dummyObj, dummyObj, -1, -1, 0);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn long GetCount(BsonDocument matcher, BsonDocument hint)
         *  \brief Get the count of matching documents in current collection
         *  \param matcher
         *          The matching rule, when condition is null, the return amount contains all the records.
         *  \param hint   
         *          Specified the index used to scan data. e.g. {"":"ageIndex"} means 
         *          using index "ageIndex" to scan data(index scan); 
         *          {"":null} means table scan. when hint is null, 
         *          database automatically match the optimal index to scan data.
         *  \return The count of matching documents
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetCount(BsonDocument matcher, BsonDocument hint)
        {
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.GET_COUNT;
            BsonDocument dummyObj = new BsonDocument();
            BsonDocument newHint = new BsonDocument();
            if (matcher == null)
            {
                matcher = dummyObj;
            }
            newHint.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            if (hint != null)
            {
                newHint.Add(SequoiadbConstants.FIELD_HINT, hint);
            }
            SDBMessage rtnSDBMessage = AdminCommand(commandString, matcher, dummyObj, dummyObj, newHint, 0, -1, 0);
            int errorCode = rtnSDBMessage.Flags;
            if (errorCode != 0)
                throw new BaseException(errorCode);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            List<BsonDocument> rtn = GetMoreCommand(rtnSDBMessage);
            return rtn[0][SequoiadbConstants.FIELD_TOTAL].AsInt64;
        }

        /** \fn long GetCount(BsonDocument matcher)
         *  \brief Get the count of matching documents in current collection
         *  \param matcher
         *          The matching rule, when condition is null, the return amount contains all the records.
         *  \return The count of matching documents
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetCount(BsonDocument matcher)
        {
            return GetCount(matcher, null);
        }

        /** \fn DBCursor Aggregate(List<BsonDocument> obj)
         *  \brief Execute aggregate operation in specified collection
         *  \param insertor The array of bson objects, can't be null
         *  \return The DBCursor of result or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public DBCursor Aggregate(List<BsonDocument> obj)
        {
            if ( obj == null || obj.Count == 0 )
                throw new BaseException("SDB_INVALIDARG");
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_AGGREGATE;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.CollectionFullName = collectionFullName;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.Flags = 0;
            sdbMessage.Insertor = obj[0];

            byte[] request = SDBMessageHelper.BuildAggrRequest(sdbMessage, isBigEndian);

            for (int count = 1; count < obj.Count; count++)
            {
                request = SDBMessageHelper.AppendAggrMsg(request, obj[count], isBigEndian);
            }
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                if (flags == SequoiadbConstants.SDB_DMS_EOC)
                    return null;
                else
                {
                    throw new BaseException(flags);
                }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return new DBCursor(rtnSDBMessage, this);
        }

        /** \fn DBCursor GetQueryMeta(BsonDocument query, BsonDocument orderBy, BsonDocument hint,
         *                            long skipRows, long returnRows)
         *  \brief Get the index blocks' or data blocks' infomations for concurrent query
         *  \param query
         *            the matching rule, return all the meta information if null
         *  \param orderBy
         *            the ordered rule, never sort if null
         *  \param hint
         *            Specified the index used to scan data. e.g. {"":"ageIndex"} means
         *            using index "ageIndex" to scan data(index scan); {"":null} means not using
         *            any index to scan data(table scan). when hint is null,
         *            database automatically match the optimal index to scan data.
         *  \param skipRows
         *            The rows to be skipped
         *  \param returnRows
         *            return the specified amount of documents,
         *            when returnRows is 0, return nothing,
         *            when returnRows is -1, return all the documents
         *  \return The DBCursor of matching infomations or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \deprecated this API only support in java
         */
        public DBCursor GetQueryMeta(BsonDocument query, BsonDocument orderBy, BsonDocument hint,
                                     long skipRows, long returnRows)
        {
            BsonDocument dummyObj = new BsonDocument();
            if (query == null)
                query = dummyObj;
            if (orderBy == null)
                orderBy = dummyObj;
            if (returnRows < 0)
                returnRows = -1;
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.GET_QUERYMETA;

            BsonDocument newHint = new BsonDocument();
            newHint.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            if ( null == hint )
            {
                newHint.Add(SequoiadbConstants.FIELD_HINT, dummyObj);
            }
            else
            {
                newHint.Add(SequoiadbConstants.FIELD_HINT, hint);
            }
            SDBMessage rtnSDBMessage = AdminCommand(commandString, query, null, orderBy,
                                                     newHint, skipRows, returnRows, 0);
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                if (flags == SequoiadbConstants.SDB_DMS_EOC)
                    return null;
                else
                {
                    throw new BaseException(flags);
                }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return new DBCursor(rtnSDBMessage, this);
        }

        /** \fn void AttachCollection (string subClFullName, BsonDocument options)
         * \brief Attach the specified collection.
         * \param subClFullName The name of the subcollection
         * \param options The low boudary and up boudary
         *       eg: {"LowBound":{a:1},"UpBound":{a:100}}
         * \retval void
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void AttachCollection(string subClFullName, BsonDocument options)
        {
            // check argument
            if (subClFullName == null || subClFullName.Equals("") ||
                subClFullName.Length > SequoiadbConstants.COLLECTION_MAX_SZ ||
                options == null || options.ElementCount == 0)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // build a bson to send
            BsonDocument attObj = new BsonDocument();
            attObj.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            attObj.Add(SequoiadbConstants.FIELD_SUBCLNAME, subClFullName);
            foreach (string key in options.Names)
            {
                attObj.Add(options.GetElement(key));
            }
            // build commond
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.LINK_CL;
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtnSDBMessage = AdminCommand(commandString, attObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check the return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn void DetachCollection(string subClFullName)
         * \brief Detach the specified collection.
         * \param subClFullName The name of the subcollection
         * \retval void
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void DetachCollection(string subClFullName)
        {
            // check argument
            if (subClFullName == null || subClFullName.Equals("") ||
                subClFullName.Length > SequoiadbConstants.COLLECTION_MAX_SZ)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // build a bson to send
            BsonDocument detObj = new BsonDocument();
            detObj.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            detObj.Add(SequoiadbConstants.FIELD_SUBCLNAME, subClFullName);
            // build command
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.UNLINK_CL;
            BsonDocument dummyObj = new BsonDocument();
            // run command
            SDBMessage rtnSDBMessage = AdminCommand(commandString, detObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check the return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        private void _Alter1(BsonDocument options)
        {
            // check argument
            if (null == options)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // build a bson to send
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            newObj.Add(SequoiadbConstants.FIELD_OPTIONS, options);
            // build command
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ALTER_COLLECTION;
            BsonDocument dummyObj = new BsonDocument();
            // run command
            SDBMessage rtnSDBMessage = AdminCommand(commandString, newObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check the return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        private void _Alter2(BsonDocument options)
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
            newObj.Add(SequoiadbConstants.FIELD_NAME_ALTER_TYPE, SequoiadbConstants.SDB_ALTER_CL );
            newObj.Add(SequoiadbConstants.FIELD_NAME_VERSION, SequoiadbConstants.SDB_ALTER_VERSION);
            newObj.Add(SequoiadbConstants.FIELD_NAME, collectionFullName);
            // append alters
            flag = options.TryGetElement(SequoiadbConstants.FIELD_NAME_ALTER, out elem);
            if (true == flag && elem.Value.IsBsonDocument)
            {
                newObj.Add(elem);
            }
            else
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // append options
            flag = false ;
            flag = options.TryGetElement(SequoiadbConstants.FIELD_OPTIONS, out elem);
            if ( true == flag )
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

            // build command
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ALTER_COLLECTION;
            BsonDocument dummyObj = new BsonDocument();
            // run command
            SDBMessage rtnSDBMessage = AdminCommand(commandString, newObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check the return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn void Alter(BsonDocument options)
         * \brief Alter the attributes of current collection
         * \param options The options for altering current collection:
         *
         *     ReplSize     : Assign how many replica nodes need to be synchronized when a write request(insert, update, etc) is executed
         *     ShardingKey  : Assign the sharding key
         *     ShardingType : Assign the sharding type
         *     Partition    : When the ShardingType is "hash", need to assign Partition, it's the bucket number for hash, the range is [2^3,2^20]
         *                    e.g. {RepliSize:0, ShardingKey:{a:1}, ShardingType:"hash", Partition:1024}
         * \note Can't alter attributes about split in partition collection; After altering a collection to
         *       be a partition collection, need to split this collection manually
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
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
                _Alter2(options);
            }
            else
            {
                _Alter1(options);
            }
        }

        /** \fn DBCursor ListLobs()
         * \brief List all of the lobs in current collection
         * \retval DBCursor of lobs
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public DBCursor ListLobs()
        {
            DBCursor cursor = null;
            // build command
            string command = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.LIST_LOBS_CMD;
            // build a bson to send
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            // run command
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtnSDBMessage = AdminCommand(command, newObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check the return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
            {
                int errCode = new BaseException("SDB_DMS_EOC").ErrorCode;
                if (errCode == flags)
                {
                    return cursor;
                }
                else
                {
                    throw new BaseException(flags);
                }
            }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            cursor = new DBCursor(rtnSDBMessage, this);
            return cursor;
        }

        /** \fn DBLob CreateLob()
         * \brief Create a large object
         * \return The newly created lob object
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public DBLob CreateLob()
        {
            return CreateLob(ObjectId.Empty);
        }

        /** \fn DBLob CreateLob(ObjectId id)
         * \brief Create a large object with specified oid
         * \param id The oid for the creating lob
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public DBLob CreateLob(ObjectId id)
        {
            DBLob lob = new DBLob(this);
            lob.Open(id, DBLob.SDB_LOB_CREATEONLY);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return lob;
        }

        /** \fn DBLob OpenLob(ObjectId id, int mode)
         * \brief Open an existing lob with the speceifed oid
         * \param id The oid of the existing lob
         * \param mode Open mode:
         *              DBLob.SDB_LOB_READ for reading,
         *              DBLob.SDB_LOB_WRITE for writing.
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public DBLob OpenLob(ObjectId id, int mode)
        {
            if (mode != DBLob.SDB_LOB_READ && mode != DBLob.SDB_LOB_WRITE)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "mode is unsupported: " + mode);
            }
            DBLob lob = new DBLob(this);
            lob.Open(id, mode);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
            return lob;
        }

        /** \fn DBLob OpenLob(ObjectId id)
         * \brief Open an existing lob with the speceifed oid
         * \param id The oid of the existing lob
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public DBLob OpenLob(ObjectId id)
        {
            return OpenLob(id, DBLob.SDB_LOB_READ);
        }

        /** \fn DBLob RemoveLob(ObjectId id)
         * \brief Remove an existing lob with the speceifed oid
         * \param id The oid of the existing lob
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void RemoveLob(ObjectId id)
        {
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            newObj.Add(SequoiadbConstants.FIELD_LOB_OID, id);

            SDBMessage sdbMessage = new SDBMessage();
            // MsgHeader
            sdbMessage.OperationCode = Operation.MSG_BS_LOB_REMOVE_REQ;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            // the rest part of _MsgOpLOb
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = (short)0;
            sdbMessage.Flags = SequoiadbConstants.DEFAULT_FLAGS;
            sdbMessage.ContextIDList = new List<long>();
            sdbMessage.ContextIDList.Add(SequoiadbConstants.DEFAULT_CONTEXTID);
            sdbMessage.Matcher = newObj;


            byte[] request = SDBMessageHelper.BuildRemoveLobRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags);
            }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn DBLob TruncateLob(ObjectId id, long length)
         * \brief Truncate an exist lob.
         * \param id The oid of the existing lob.
         * \param length The truncate length.
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void TruncateLob(ObjectId id, long length)
        {
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            newObj.Add(SequoiadbConstants.FIELD_LOB_OID, id);
            newObj.Add(SequoiadbConstants.FIELD_LOB_LENGTH, length);

            SDBMessage sdbMessage = new SDBMessage();
            // MsgHeader
            sdbMessage.OperationCode = Operation.MSG_BS_LOB_TRUNCATE_REQ;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            // the rest part of _MsgOpLOb
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = (short)0;
            sdbMessage.Flags = SequoiadbConstants.DEFAULT_FLAGS;
            sdbMessage.ContextIDList = new List<long>();
            sdbMessage.ContextIDList.Add(SequoiadbConstants.DEFAULT_CONTEXTID);
            sdbMessage.Matcher = newObj;


            byte[] request = SDBMessageHelper.BuildTruncateLobRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
            {
                throw new BaseException(flags);
            }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn void Truncate()
         * \brief Truncate the collection
         * \return void
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void Truncate()
        {
            // build a bson to send
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            // build command
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.TRUNCATE;
            BsonDocument dummyObj = new BsonDocument();
            // run command
            SDBMessage rtnSDBMessage = AdminCommand(commandString, newObj, dummyObj, dummyObj, dummyObj, 0, -1, 0);
            // check the return flag
            int flags = rtnSDBMessage.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        /** \fn void CreateIdIndex(BsonDocument options)
         * \brief Create $id index in collection
         * \param options Options for create id index, or null for no options, see SequoiaDB Information Center "SequoiaDB Shell Methods" for more detail.
         *         e.g.: {"Offline":true}
         *         <ul>
         *          <li>Offline   : Use offline mode to create index or not, default to be false
         *         </ul>
         * \return void
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void CreateIdIndex(BsonDocument options)
        {
            BsonDocument newObj = new BsonDocument();
            BsonDocument subObj = new BsonDocument();

            subObj.Add(SequoiadbConstants.FIELD_NAME, SequoiadbConstants.SDB_ALTER_CRT_ID_INDEX);
            if (null == options)
            {
                subObj.Add(SequoiadbConstants.FIELD_NAME_ARGS, null);
            }
            else
            {
                subObj.Add(SequoiadbConstants.FIELD_NAME_ARGS, options);
            }

            newObj.Add(SequoiadbConstants.FIELD_NAME_ALTER, subObj);

            Alter(newObj);
        }

        /** \fn void DropIdIndex()
         * \brief Drop $id index in collection
         * \return void
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        public void DropIdIndex()
        {
            BsonDocument newObj = new BsonDocument();
            BsonDocument subObj = new BsonDocument();

            subObj.Add(SequoiadbConstants.FIELD_NAME, SequoiadbConstants.SDB_ALTER_DROP_ID_INDEX);
            subObj.Add(SequoiadbConstants.FIELD_NAME_ARGS, BsonNull.Value);

            newObj.Add(SequoiadbConstants.FIELD_NAME_ALTER, subObj);

            Alter(newObj);
        }

        private BsonDocument _TryGenOID(BsonDocument obj, bool ensureOID)
        {
            if (true == ensureOID)
            {
                ObjectId objId;
                BsonValue tmp;
                if (!obj.TryGetValue(SequoiadbConstants.OID, out tmp))
                {
                    objId = ObjectId.GenerateNewId();
                    obj.Add(SequoiadbConstants.OID, objId);
                }
            }
            return obj;
        }

        private void _Update(int flag, BsonDocument matcher, BsonDocument modifier, BsonDocument hint)
        {
            if (modifier == null)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            BsonDocument dummyObj = new BsonDocument();
            if (matcher == null)
            {
                matcher = dummyObj;
            }
            if (hint == null)
            {
                hint = dummyObj;
            }
            SDBMessage sdbMessage = new SDBMessage();

            sdbMessage.OperationCode = Operation.OP_UPDATE;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.Flags = flag;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.CollectionFullName = collectionFullName;
            sdbMessage.RequestID = 0;
            sdbMessage.Matcher = matcher;
            sdbMessage.Modifier = modifier;
            sdbMessage.Hint = hint;

            byte[] request = SDBMessageHelper.BuildUpdateRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            int errorCode = rtnSDBMessage.Flags;
            if (errorCode != 0)
            {
                throw new BaseException(errorCode);
            }
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        private void _CreateIndex(string name, BsonDocument key, bool isUnique, bool isEnforced, int sortBufferSize)
        {
            if (sortBufferSize < 0)
                throw new BaseException("SDB_INVALIDARG");
            string commandString = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CREATE_INX;
            BsonDocument obj = new BsonDocument();
            BsonDocument dummyObj = new BsonDocument();
            BsonDocument createObj = new BsonDocument();
            BsonDocument hintObj = new BsonDocument();
            obj.Add(SequoiadbConstants.IXM_NAME, name);
            obj.Add(SequoiadbConstants.IXM_KEY, key);
            obj.Add(SequoiadbConstants.IXM_UNIQUE, isUnique);
            obj.Add(SequoiadbConstants.IXM_ENFORCED, isEnforced);
            createObj.Add(SequoiadbConstants.FIELD_COLLECTION, collectionFullName);
            createObj.Add(SequoiadbConstants.FIELD_INDEX, obj);

            hintObj.Add(SequoiadbConstants.IXM_SORT_BUFFER_SIZE, sortBufferSize);

            SDBMessage rtn = AdminCommand(commandString, createObj, dummyObj, dummyObj, hintObj, -1, -1, 0);

            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            // upsert cache
            sdb.UpsertCache(collectionFullName);
        }

        private SDBMessage AdminCommand(string command, BsonDocument query, BsonDocument selector, BsonDocument orderBy,
            BsonDocument hint, long skipRows, long returnRows, int flag)
        {
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_QUERY;
            sdbMessage.CollectionFullName = command;
            sdbMessage.Version = SequoiadbConstants.DEFAULT_VERSION;
            sdbMessage.W = SequoiadbConstants.DEFAULT_W;
            sdbMessage.Padding = 0;
            sdbMessage.Flags = flag;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.SkipRowsCount = skipRows;
            sdbMessage.ReturnRowsCount = returnRows;
            // matcher
            if (query == null)
            {
                sdbMessage.Matcher = dummyObj;
            }
            else
            {
                sdbMessage.Matcher = query;
            }
            // selector
            if (selector == null)
            {
                sdbMessage.Selector = dummyObj;
            }
            else
            {
                sdbMessage.Selector = selector;
            }
            // orderBy
            if (orderBy == null)
            {
                sdbMessage.OrderBy = dummyObj;
            }
            else
            {
                sdbMessage.OrderBy = orderBy;
            }
            // hint
            if (hint == null)
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

        private List<BsonDocument> GetMoreCommand(SDBMessage rtnSDBMessage)
        {
            ulong requestID = rtnSDBMessage.RequestID;
            List<long> contextIDs = rtnSDBMessage.ContextIDList;
            List<BsonDocument> fullList = new List<BsonDocument>();
            bool hasMore = true;
            while (hasMore)
            {
                SDBMessage sdbMessage = new SDBMessage();
                sdbMessage.OperationCode = Operation.OP_GETMORE;
                sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
                sdbMessage.ContextIDList = contextIDs;
                sdbMessage.RequestID = requestID;
                sdbMessage.NumReturned = -1;

                byte[] request = SDBMessageHelper.BuildGetMoreRequest(sdbMessage, isBigEndian);
                connection.SendMessage(request);
                rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
                rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
                int flags = rtnSDBMessage.Flags;
                if (flags != 0)
                    if (flags == SequoiadbConstants.SDB_DMS_EOC)
                        hasMore = false;
                    else
                    {
                        throw new BaseException(flags);
                    }
                else
                {
                    requestID = rtnSDBMessage.RequestID;
                    List<BsonDocument> objList = rtnSDBMessage.ObjectList;
                    fullList.AddRange(objList);
                }
            }
            return fullList;
        }
   }
}
