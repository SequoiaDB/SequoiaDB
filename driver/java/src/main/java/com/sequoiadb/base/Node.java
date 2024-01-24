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

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.request.AdminRequest;
import com.sequoiadb.message.response.SdbReply;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

/**
 * Node of SequoiaDB.
 */
public class Node {
    private final String hostName;
    private final int port;
    private final String nodeName;
    private final int id;
    private final ReplicaGroup rg;
    private Sequoiadb sequoiadb;

    Node(String hostName, int port, int nodeId, ReplicaGroup rg) {
        this.rg = rg;
        this.hostName = hostName;
        this.port = port;
        this.nodeName = hostName + ":" + port;
        this.id = nodeId;
    }

    Node(String hostName, int port, ReplicaGroup rg) {
        this(hostName, port, rg.getNodeId(hostName, port), rg);
    }

    /**
     * Node status.
     */
    public enum NodeStatus {
        SDB_NODE_ALL(1),
        SDB_NODE_ACTIVE(2),
        SDB_NODE_INACTIVE(3),
        SDB_NODE_UNKNOWN(4);
        private final int key;

        NodeStatus(int key) {
            this.key = key;
        }

        public int getKey() {
            return key;
        }

        public static NodeStatus getByKey(int key) {
            NodeStatus nodeStatus = NodeStatus.SDB_NODE_ALL;
            for (NodeStatus status : NodeStatus.values()) {
                if (status.getKey() == key) {
                    nodeStatus = status;
                    break;
                }
            }
            return nodeStatus;
        }
    }

    /**
     * @return Current node's id.
     */
    public int getNodeId() {
        return id;
    }

    /**
     * @return Current node's parent replica group.
     */
    public ReplicaGroup getReplicaGroup() {
        return rg;
    }

    /**
     * Disconnect from current node.
     *
     * @throws BaseException If error happens.
     */
    public void disconnect() throws BaseException {
        sequoiadb.disconnect();
    }

    /**
     * Connect to current node with the same username and password of coordination node.
     *
     * @return The Sequoiadb instance of current node.
     * @throws BaseException If error happens.
     * @deprecated This function is deprecated
     */
    @Deprecated
    public Sequoiadb connect() throws BaseException {
        if (sequoiadb != null && !sequoiadb.isClosed()) {
            sequoiadb.close();
        }
        sequoiadb = new Sequoiadb(hostName, port, rg.getSequoiadb().getUserName(),
                rg.getSequoiadb().getPassword());
        return sequoiadb;
    }

    /**
     * Connect to current node with username and password.
     *
     * @param username user name
     * @param password pass word
     * @return The Sequoiadb instance of current node.
     * @throws BaseException If error happens.
     * @deprecated This function is deprecated
     */
    @Deprecated
    public Sequoiadb connect(String username, String password) throws BaseException {
        if (sequoiadb != null && !sequoiadb.isClosed()) {
            sequoiadb.close();
        }
        sequoiadb = new Sequoiadb(hostName, port, username, password);
        return sequoiadb;
    }

    /**
     * Get the Sequoiadb of current node.
     *
     * @return The Sequoiadb object of current node or null for having not
     * connected to the current node yet.
     * @see Node#connect()
     * @see Node#connect(String, String)
     */
    public Sequoiadb getSdb() {
        return sequoiadb;
    }

    /**
     * Get the hostname of current node.
     *
     * @return Hostname of current node.
     */
    public String getHostName() {
        return hostName;
    }

    /**
     * Get the port of current node.
     *
     * @return The port of current node.
     */
    public int getPort() {
        return port;
    }

    /**
     * Get the name of current node.
     *
     * @return The name of current node.
     */
    public String getNodeName() {
        return nodeName;
    }

    /**
     * Get the status of current node.
     *
     * @return The status of current node.
     * @throws BaseException If error happens.
     * @deprecated The status of node are invalid, never use this api again.
     */
    public NodeStatus getStatus() throws BaseException {
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_GROUPID, rg.getId());
        obj.put(SdbConstants.FIELD_NAME_NODENAME, nodeName);

        AdminRequest request = new AdminRequest(AdminCommand.SNAP_DATABASE, obj);
        SdbReply response = rg.getSequoiadb().requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_NET_CANNOT_CONNECT.getErrorCode()) {
                return NodeStatus.SDB_NODE_INACTIVE;
            } else {
                rg.getSequoiadb().throwIfError(response);
            }
        }
        return NodeStatus.SDB_NODE_ACTIVE;
    }

    /**
     * Start current node.
     *
     * @throws BaseException If error happens.
     */
    public void start() throws BaseException {
        startStop(true);
    }

    /**
     * Stop current node.
     *
     * @throws BaseException If error happens.
     */
    public void stop() throws BaseException {
        startStop(false);
    }

    /**
     * Alter node to set Location.
     * @param location The location name of node. If it is "", it means to remove the location setting of node.
     * @throws BaseException If error happens.
     * @since 3.6.1
     */
    public void setLocation(String location) throws BaseException {
        if (location == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The location name is null");
        }

        BSONObject option = new BasicBSONObject(SdbConstants.NODE_LOCATION, location);
        alterInternal(SdbConstants.NODE_SET_LOCATION, option);
    }

    private void alterInternal(String taskName, BSONObject options) throws BaseException {
        if (options == null || options.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The option is null or empty");
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_ACTION, taskName);
        matcher.put(SdbConstants.FIELD_NAME_OPTIONS, options);

        BSONObject hint = new BasicBSONObject();
        hint.put(SdbConstants.FIELD_NAME_GROUPID, rg.getId());
        hint.put(SdbConstants.FIELD_NAME_NODEID, id);

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_NODE, matcher, hint);
        SdbReply response = rg.getSequoiadb().requestAndResponse(request);
        rg.getSequoiadb().throwIfError(response);
    }

    private void startStop(boolean start) {
        BSONObject config = new BasicBSONObject();
        config.put(SdbConstants.FIELD_NAME_HOST, hostName);
        config.put(SdbConstants.PMD_OPTION_SVCNAME, Integer.toString(port));

        String cmd = start ? AdminCommand.STARTUP_NODE : AdminCommand.SHUTDOWN_NODE;
        AdminRequest request = new AdminRequest(cmd, config);
        SdbReply response = rg.getSequoiadb().requestAndResponse(request);
        String msg = "node = " + hostName + ":" + port;
        rg.getSequoiadb().throwIfError(response, msg);
    }
}
