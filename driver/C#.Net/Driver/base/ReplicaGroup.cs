using System;
using SequoiaDB.Bson;
using System.Collections.Generic;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class ReplicaGroup
     *  \brief Database operation interfaces of replica group.
     */
    public class ReplicaGroup
	{
        private int groupID = -1;
        private string groupName = null;
        private bool isCatalog = false;
        internal bool isBigEndian = false;
        private Sequoiadb sdb = null;
        internal ReplicaGroup(Sequoiadb sdb, string groupName, int groupID)
        {
            this.sdb = sdb;
            this.groupName = groupName;
            this.groupID = groupID;
            isCatalog = groupName.Equals(SequoiadbConstants.CATALOG_GROUP);
            isBigEndian = sdb.isBigEndian;
        }

        /** \property SequoiaDB
         *  \brief Return the sequoiadb handle of current group 
         *  \return The Sequoiadb object
         */
        public Sequoiadb SequoiaDB
        {
            get { return sdb; }
        }

        /** \property GroupName
         *  \brief Return the name of current group
         *  \return The group name
         */
        public string GroupName
        {
            get { return groupName; }
        }

        /** \property GroupID
         *  \brief Return the group ID of current group
         *  \return The group ID
         */
        public int GroupID
        {
            get { return groupID; }
        }

        /** \property IsCatalog
         *  \brief Verify the role of current group
         *  \return True if is catalog group or False if not
         */
        public bool IsCatalog
        {
            get { return isCatalog; }
        }

        /** \fn bool Stop()
         *  \brief Stop the current node
         *  \return True if succeed or False if fail
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public bool Stop()
        {
            bool start = false;
            return StopStart(start);
        }

        /** \fn bool Start()
         *  \brief Start the current node
         *  \return True if succeed or False if fail
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public bool Start()
        {
            bool start = true;
            return StopStart(start);
        }

        /** \fn int GetNodeNum( SDBConst.NodeStatus status)
         *  \brief Get the count of node with given status
         *  \param status The specified status as below:
         *  
         *      SDB_NODE_ALL
         *      SDB_NODE_ACTIVE
         *      SDB_NODE_INACTIVE
         *      SDB_NODE_UNKNOWN
         *  \return The count of node
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \deprecated Since v2.8, the status of node are invalid, never use this api again.
         */
        public int GetNodeNum( SDBConst.NodeStatus status)
        {
            try
            {
                int total = 0;
                BsonDocument detail = GetDetail();
                if (detail[SequoiadbConstants.FIELD_GROUP].IsBsonArray)
                {
                    BsonArray nodes = detail[SequoiadbConstants.FIELD_GROUP].AsBsonArray;
                    total = nodes.Count;
                    //foreach (BsonDocument node in nodes)
                    //{
                    //    Node rnode = ExtractNode(node);
                    //    SDBConst.NodeStatus sta = rnode.GetStatus();
                    //    if (SDBConst.NodeStatus.SDB_NODE_ALL == status || rnode.GetStatus() == status)
                    //        ++total;
                    //}
                }
                return total;
            }
            catch (KeyNotFoundException)
            {
                throw new BaseException("SDB_CLS_NODE_NOT_EXIST");
            }
            catch (FormatException)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
        }

        /** \fn BsonDocument GetDetail()
         *  \brief Get the detail information of current group
         *  \return The detail information in BsonDocument object
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public BsonDocument GetDetail()
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument dummyobj = new BsonDocument();
            matcher.Add(SequoiadbConstants.FIELD_GROUPNAME, groupName);
            matcher.Add(SequoiadbConstants.FIELD_GROUPID, groupID);
            DBCursor cursor = sdb.GetList(SDBConst.SDB_LIST_GROUPS, matcher, dummyobj, dummyobj);
            if (cursor != null)
            {
                BsonDocument detail = cursor.Next();
                if (detail != null)
                    return detail;
                else
                    throw new BaseException("SDB_CLS_GRP_NOT_EXIST");
            }
            else
            {
                throw new BaseException("SDB_SYS");
            }
        }

        /** \fn Node CreateNode(string hostName, int port, string dbpath,
                               Dictionary<string, string> map)
         *  \brief Create the replica node
         *  \param hostName The host name of node
         *  \param port The port of node
         *  \param dbpath The database path of node
         *  \param map The other configure information of node
         *  \return The Node object
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         *  \deprecated we override this api by passing a "BsonDocument" instead of a "Dictionary"
         */
        public Node CreateNode(string hostName, int port, string dbpath,
                               Dictionary<string, string> map)
        {
            BsonDocument obj = new BsonDocument();
            Dictionary<string, string>.Enumerator it = map.GetEnumerator();
            while (it.MoveNext())
                obj.Add(it.Current.Key, it.Current.Value);
            return CreateNode(hostName, port, dbpath, obj);
        }

        /** \fn Node CreateNode(string hostName, int port, string dbpath,
                                BsonDocument configure)
         *  \brief Create the replica node
         *  \param hostName The host name of node
         *  \param port The port of node
         *  \param dbpath The database path of node
         *  \param configure The other configure information of node
         *  \return The Node object
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public Node CreateNode(string hostName, int port, string dbpath,
                               BsonDocument configure)
        {
            if (hostName == null || port < 0 || port > 65535 || dbpath == null)
                throw new BaseException("SDB_INVALIDARG");
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CREATE_CMD + " "
                             + SequoiadbConstants.NODE;
            BsonDocument configuration = new BsonDocument();
            configuration.Add(SequoiadbConstants.FIELD_GROUPNAME, groupName);
            configuration.Add(SequoiadbConstants.FIELD_HOSTNAME, hostName);
            configuration.Add(SequoiadbConstants.SVCNAME, port.ToString());
            configuration.Add(SequoiadbConstants.DBPATH, dbpath);
            if (null != configure)
            {
                configure.Remove(SequoiadbConstants.FIELD_GROUPNAME);
                configure.Remove(SequoiadbConstants.FIELD_HOSTNAME);
                configure.Remove(SequoiadbConstants.SVCNAME);
                configure.Remove(SequoiadbConstants.DBPATH);
                configuration.Add(configure);
            }
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, configuration, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
            else
                return GetNode(hostName, port);
        }

        /** \fn void RemoveNode(string hostName, int port,
                       BsonDocument configure)
         *  \brief Remove the specified replica node
         *  \param hostName The host name of node
         *  \param port The port of node
         *  \param configure The configurations for the replica node
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void RemoveNode(string hostName, int port,
                               BsonDocument configure)
        {
            if (hostName == null || port < 0 || port < 0 || port > 65535)
                throw new BaseException("SDB_INVALIDARG");
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.REMOVE_CMD + " "
                 + SequoiadbConstants.NODE;
            BsonDocument config = new BsonDocument();
            config.Add(SequoiadbConstants.FIELD_GROUPNAME, groupName);
            config.Add(SequoiadbConstants.FIELD_HOSTNAME, hostName);
            config.Add(SequoiadbConstants.SVCNAME, Convert.ToString(port));
            if ( configure != null )
            {
                foreach (string key in configure.Names)
                {
                    if (key.Equals(SequoiadbConstants.FIELD_GROUPNAME) ||
                        key.Equals(SequoiadbConstants.FIELD_HOSTNAME) ||
                        key.Equals(SequoiadbConstants.SVCNAME))
                    {
                        continue;
                    }
                    config.Add(configure.GetElement(key));
                }
            }
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, config, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
        }

        /** \fn Node GetMaster()
         *  \brief Get the master node of current group
         *  \return The fitted node or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public Node GetMaster()
        {
            int primaryNodeId = -1;
            try
            {
                // get information of nodes from catalog
                BsonDocument detail = GetDetail();
                // check the nodes in current group
                
                BsonValue groupValue = detail.Contains(SequoiadbConstants.FIELD_GROUP) ? detail[SequoiadbConstants.FIELD_GROUP] : null;
                if (groupValue == null || !groupValue.IsBsonArray)
                {
                    throw new BaseException((int)Errors.errors.SDB_SYS);
                }
                BsonArray nodes = groupValue.AsBsonArray;
                if (nodes.Count == 0)
                {
                    throw new BaseException((int)Errors.errors.SDB_CLS_EMPTY_GROUP);
                }
                // get primary node id
                BsonValue primaryValue = detail.Contains(SequoiadbConstants.FIELD_PRIMARYNODE) ? detail[SequoiadbConstants.FIELD_PRIMARYNODE] : null;
                if (primaryValue == null)
                {
                    throw new BaseException((int)Errors.errors.SDB_RTN_NO_PRIMARY_FOUND);
                }
                else if (!primaryValue.IsInt32)
                {
                    throw new BaseException((int)Errors.errors.SDB_SYS, "primary node id should be a int32 value");
                }
                else if (primaryValue.AsInt32 == -1) // TODO: test it
                {
                    throw new BaseException((int)Errors.errors.SDB_RTN_NO_PRIMARY_FOUND);
                }
                // get the master from the node list
                primaryNodeId = primaryValue.AsInt32;
                foreach (BsonDocument node in nodes)
                {
                    BsonValue nodeIdValue = node.Contains(SequoiadbConstants.FIELD_NODEID) ? node[SequoiadbConstants.FIELD_NODEID] : null;
                    if (nodeIdValue == null || !nodeIdValue.IsInt32)
                    {
                        throw new BaseException("SDB_SYS");
                    }
                    int nodeID = node[SequoiadbConstants.FIELD_NODEID].AsInt32;
                    if (nodeID == primaryNodeId)
                    {
                        return ExtractNode(node);
                    }
                }
                throw new BaseException((int)Errors.errors.SDB_SYS, "no information about the primary node in node array");
            }
            catch (KeyNotFoundException)
            {
                throw new BaseException("SDB_SYS");
            }
            catch (FormatException)
            {
                throw new BaseException("SDB_SYS");
            }
        }

        /** \fn Node GetSlave()
         *  \brief Get the slave node of current group
         *  \return The fitted node or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public Node GetSlave()
        {
            return GetSlave(null);
        }

        /** \fn Node GetSlave(params int[] positions)
         *  \brief Get the slave node of current group
         *  \param positions The positions of nodes
         *  \return The fitted node or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public Node GetSlave(params int[] positions)
        {
            bool needGeneratePosition = false;
            List<int> validPositions = new List<int>();
            // check arguements 
            if (positions == null || positions.Length == 0)
            {
                needGeneratePosition = true;
            }
            else
            {
                foreach (int pos in positions)
                {
                    if (pos < 1 || pos > 7)
                    {
                        throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "the valid position of node should be [1, 7]");
                    }
                    if (!validPositions.Contains(pos))
                    {
                        validPositions.Add(pos);
                    }
                }
                if (validPositions.Count < 1 || validPositions.Count > 7)
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "the amount of valid positions should be [1, 7]");
                }
            }
            // get group details
            try
            {
                int primaryId = -1;
                bool hasPrimary = true;
                BsonDocument detail = GetDetail();
                BsonValue groupValue = detail.Contains(SequoiadbConstants.FIELD_GROUP) ? detail[SequoiadbConstants.FIELD_GROUP] : null;
                if (groupValue == null || !groupValue.IsBsonArray)
                {
                    throw new BaseException("SDB_SYS");
                }
                BsonArray nodes = groupValue.AsBsonArray;
                if (nodes.Count == 0)
                {
                    throw new BaseException((int)Errors.errors.SDB_CLS_EMPTY_GROUP);
                }
                BsonValue primaryIdValue = detail.Contains(SequoiadbConstants.FIELD_PRIMARYNODE) ? detail[SequoiadbConstants.FIELD_PRIMARYNODE] : null;
                if (primaryIdValue == null)
                {
                    hasPrimary = false;
                }
                else if (!primaryIdValue.IsInt32)
                {
                    throw new BaseException((int)Errors.errors.SDB_SYS);
                }
                else if ((primaryId = primaryIdValue.AsInt32) == -1)
                {
                    hasPrimary = false;
                }
                // try to mark the position of primary node in the nodes list,
                // the value of position is [1, 7]
                int primaryNodePosition = 0;
                int counter = 0;
                List<BsonDocument> nodeList = new List<BsonDocument>();
                foreach (BsonDocument node in nodes)
                {
                    counter++;
                    BsonValue nodeIdValue = node.Contains(SequoiadbConstants.FIELD_NODEID) ? node[SequoiadbConstants.FIELD_NODEID] : null;
                    if (nodeIdValue == null || !nodeIdValue.IsInt32)
                    {
                        throw new BaseException((int)Errors.errors.SDB_SYS, "invalid node id in node list");
                    }
                    int nodeId = nodeIdValue.AsInt32;
                    if (hasPrimary && primaryId == nodeId)
                    {
                        primaryNodePosition = counter;
                    }
                    nodeList.Add(node);
                }
                // check
                if (hasPrimary && primaryNodePosition == 0)
                {
                    throw new BaseException((int)Errors.errors.SDB_SYS, "have no primary node in nodes list");
                }
                // try to generate slave node's positions
                int nodeCount = nodeList.Count;
                if (needGeneratePosition)
                {
                    for (int i = 0; i < nodeCount; i++)
                    {
                        if (hasPrimary && primaryNodePosition == i + 1)
                        {
                            continue;
                        }
                        validPositions.Add(i + 1);
                    }
                }
                // select a node to return
                int nodeIndex = -1;
                if (nodeCount == 1)
                {
                    return ExtractNode(nodeList[0]);
                }
                else if (validPositions.Count == 1)
                {
                    nodeIndex = (validPositions[0] - 1) % nodeCount;
                    return ExtractNode(nodeList[nodeIndex]);
                }
                else
                {
                    int position = 0;
                    Random rand = new Random();
                    int[] flags = new int[7];
                    List<int> includePrimaryPositions = new List<int>();
                    List<int> excludePrimaryPositions = new List<int>();

                    foreach (int pos in validPositions)
                    {
                        if (pos <= nodeCount)
                        {
                            nodeIndex = pos - 1;
                            if (flags[nodeIndex] == 0)
                            {
                                flags[nodeIndex] = 1;
                                includePrimaryPositions.Add(pos);
                                if (hasPrimary && primaryNodePosition != pos)
                                {
                                    excludePrimaryPositions.Add(pos);
                                }
                            }
                        }
                        else
                        {
                            nodeIndex = (pos - 1) % nodeCount;
                            if (flags[nodeIndex] == 0)
                            {
                                flags[nodeIndex] = 1;
                                includePrimaryPositions.Add(pos);
                                if (hasPrimary && primaryNodePosition != nodeIndex + 1)
                                {
                                    excludePrimaryPositions.Add(pos);
                                }
                            }
                        }
                    }

                    if (excludePrimaryPositions.Count > 0)
                    {
                        position = rand.Next(excludePrimaryPositions.Count);
                        position = excludePrimaryPositions[position];
                    }
                    else
                    {
                        position = rand.Next(includePrimaryPositions.Count);
                        Console.WriteLine("position is: " + position);
                        position = includePrimaryPositions[position];
                        if (needGeneratePosition)
                        {
                            position += 1;
                        }
                    }
                    nodeIndex = (position - 1) % nodeCount;
                    return ExtractNode(nodeList[nodeIndex]);
                }
            }
            catch (KeyNotFoundException)
            {
                throw new BaseException("SDB_SYS");
            }
            catch (FormatException)
            {
                throw new BaseException("SDB_SYS");
            }
        }

        /** \fn Node GetNode(string nodeName)
         *  \brief Get the node by node name
         *  \param nodeName The node name
         *  \return The fitted node or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public Node GetNode(string nodeName)
        {
            // check input argument
            if (nodeName == null || 
                !nodeName.Contains(SequoiadbConstants.NODE_NAME_SERVICE_SEP))
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // extract hostname and port
            string[] hostname = null;
            string targetHostName = null;
            int targetHostPort = -1;
            hostname = nodeName.Split(SequoiadbConstants.NODE_NAME_SERVICE_SEP[0]);
            targetHostName = hostname[0].Trim();
            if (targetHostName.Equals(string.Empty))
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            try
            {
                targetHostPort = int.Parse(hostname[1].Trim());
            }
            catch (FormatException)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
            // get node
            return GetNode(targetHostName, targetHostPort);
        }

        /** \fn Node GetNode(string hostName, int port)
         *  \brief Get the node by host name and port
         *  \param hostName The host name
         *  \param port The port
         *  \return The fitted node or null
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public Node GetNode(string hostName, int port)
        {
            try
            {
                BsonDocument detail = GetDetail();
                if (!detail[SequoiadbConstants.FIELD_GROUP].IsBsonArray)
                {
                    throw new BaseException("SDB_SYS");
                }
                BsonArray nodes = detail[SequoiadbConstants.FIELD_GROUP].AsBsonArray;
                foreach (BsonDocument node in nodes)
                {
                    if (!node[SequoiadbConstants.FIELD_HOSTNAME].IsString)
                    {
                        throw new BaseException("SDB_SYS");
                    }
                    string hostname = node[SequoiadbConstants.FIELD_HOSTNAME].AsString;
                    if (hostname.Equals(hostName))
                    {
                        Node rn = ExtractNode(node);
                        if (rn.Port == port)
                        {
                            return rn;
                        }
                    }
                }
                throw new BaseException("SDB_CLS_NODE_NOT_EXIST");
            }
            catch (KeyNotFoundException)
            {
                throw new BaseException("SDB_SYS");
            }
        }

        /** \fn void AttachNode( string hostName, 
         *                       int port, 
         *                       BsonDocument options )
         *  \brief Attach a node to the group
         *  \param [in] hostName The host name of node.
         *  \param [in] port The port for the node.
         *  \param [in] optoins The options of attach.
         *  \retval SDB_OK Operation Success
         *  \retval Others Operation Fail
         */
        public void AttachNode(string hostName, int port, BsonDocument options)
        {
            if (hostName == null || port < 0 || port > 65535 )
                throw new BaseException("SDB_INVALIDARG");
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.CREATE_CMD + " "
                             + SequoiadbConstants.NODE;
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_GROUPNAME, groupName);
            newObj.Add(SequoiadbConstants.FIELD_HOSTNAME, hostName);
            newObj.Add(SequoiadbConstants.SVCNAME, port.ToString());
            newObj.Add(SequoiadbConstants.FIELD_NAME_ONLY_ATTACH, true);

            if (options != null && options.ElementCount != 0)
            {
                foreach (string key in options.Names)
                {
                    if (SequoiadbConstants.FIELD_NAME_ONLY_ATTACH != key )
                        newObj.Add(options.GetElement(key));
                }
            }

            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, newObj, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
        }

        /** \fn void DetachNode( string hostName,
         *                       int port,
         *                       BsonDocument options )
         *  \brief Detach a node from the group
         *  \param [in] pHostName The host name of node.
         *  \param [in] port The port for the node.
         *  \param [in] optoins The options of detach.
         *  \retval SDB_OK Operation Success
         *  \retval Others Operation Fail
         */
        public void DetachNode(string hostName, int port, BsonDocument options)
        {
            if (hostName == null || port < 0 || port > 65535)
                throw new BaseException("SDB_INVALIDARG");
            string command = SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.REMOVE_CMD + " "
                             + SequoiadbConstants.NODE;
            BsonDocument newObj = new BsonDocument();
            newObj.Add(SequoiadbConstants.FIELD_GROUPNAME, groupName);
            newObj.Add(SequoiadbConstants.FIELD_HOSTNAME, hostName);
            newObj.Add(SequoiadbConstants.SVCNAME, port.ToString());
            newObj.Add(SequoiadbConstants.FIELD_NAME_ONLY_DETACH, true);

            if (options != null && options.ElementCount != 0)
            {
                foreach (string key in options.Names)
                {
                    if (SequoiadbConstants.FIELD_NAME_ONLY_DETACH != key)
                        newObj.Add(options.GetElement(key));
                }
            }

            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, newObj, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
                throw new BaseException(flags);
        }

        private SDBMessage AdminCommand(string command, BsonDocument arg1, BsonDocument arg2,
                                        BsonDocument arg3, BsonDocument arg4)
        {
            IConnection connection = sdb.Connection;
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage sdbMessage = new SDBMessage();
            sdbMessage.OperationCode = Operation.OP_QUERY;

            // arg1
            if (null == arg1)
            {
                sdbMessage.Matcher = dummyObj;
            }
            else
            {
                sdbMessage.Matcher = arg1;
            }
            // arg2
            if (null == arg2)
            {
                sdbMessage.Selector = dummyObj;
            }
            else
            {
                sdbMessage.Selector = arg2;
            }
            // arg3
            if (null == arg3)
            {
                sdbMessage.OrderBy = dummyObj;
            }
            else
            {
                sdbMessage.OrderBy = arg3;
            }
            // arg4
            if (null == arg4)
            {
                sdbMessage.Hint = dummyObj;
            }
            else
            {
                sdbMessage.Hint = arg4;
            }
            sdbMessage.CollectionFullName = command;
            sdbMessage.Flags = 0;
            sdbMessage.NodeID = SequoiadbConstants.ZERO_NODEID;
            sdbMessage.RequestID = 0;
            sdbMessage.SkipRowsCount = -1;
            sdbMessage.ReturnRowsCount = -1;

            byte[] request = SDBMessageHelper.BuildQueryRequest(sdbMessage, isBigEndian);
            connection.SendMessage(request);
            SDBMessage rtnSDBMessage = SDBMessageHelper.MsgExtractReply(connection.ReceiveMessage(isBigEndian), isBigEndian);
            rtnSDBMessage = SDBMessageHelper.CheckRetMsgHeader(sdbMessage, rtnSDBMessage);
            return rtnSDBMessage;
        }

        private Node ExtractNode(BsonDocument node)
        {
            try
            {
                if (!node[SequoiadbConstants.FIELD_HOSTNAME].IsString)
                {
                    throw new BaseException("SDB_SYS");
                }
                string hostName = node[SequoiadbConstants.FIELD_HOSTNAME].AsString;
                if (!node[SequoiadbConstants.FIELD_NODEID].IsInt32)
                {
                    throw new BaseException("SDB_SYS");
                }
                int nodeID = node[SequoiadbConstants.FIELD_NODEID].AsInt32;
                if (!node[SequoiadbConstants.FIELD_SERVICE].IsBsonArray)
                {
                    throw new BaseException("SDB_SYS");
                }
                BsonArray svcs = node[SequoiadbConstants.FIELD_SERVICE].AsBsonArray;
                foreach (BsonDocument svc in svcs)
                {
                    if (!svc[SequoiadbConstants.FIELD_SERVICE_TYPE].IsInt32)
                    {
                        throw new BaseException("SDB_SYS");
                    }
                    int type = svc[SequoiadbConstants.FIELD_SERVICE_TYPE].AsInt32;
                    if (type == 0)
                    {
                        if (!svc[SequoiadbConstants.FIELD_NAME].IsString)
                        {
                            throw new BaseException("SDB_SYS");
                        }
                        string svcname = svc[SequoiadbConstants.FIELD_NAME].AsString;
                        return new Node(this, hostName, int.Parse(svcname), nodeID);
                    }
                }
                throw new BaseException("SDB_SYS");
            }
            catch(KeyNotFoundException)
            {
                throw new BaseException("SDB_SYS");
            }
            catch (FormatException)
            {
                throw new BaseException("SDB_SYS");
            }
        }

        private bool StopStart(bool start)
        {
            string command = start ? SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.ACTIVE_CMD + " "
                                    + SequoiadbConstants.GROUP :
                                   SequoiadbConstants.ADMIN_PROMPT + SequoiadbConstants.SHUTDOWN_CMD + " "
                                    + SequoiadbConstants.GROUP;
            BsonDocument configuration = new BsonDocument();
            configuration.Add(SequoiadbConstants.FIELD_GROUPNAME, groupName);
            configuration.Add(SequoiadbConstants.FIELD_GROUPID, groupID);
            BsonDocument dummyObj = new BsonDocument();
            SDBMessage rtn = AdminCommand(command, configuration, dummyObj, dummyObj, dummyObj);
            int flags = rtn.Flags;
            if (flags != 0)
                return false;
            else
                return true;
        }
    }
}
