/*
 * Copyright 2017 SequoiaDB Inc.
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
    private String hostName;
    private int port;
    private String nodeName;
    private int id;
    private ReplicaGroup rg;
    private Sequoiadb sequoiadb;

    Node(String hostName, int port, int nodeId, ReplicaGroup rg) {
        this.rg = rg;
        this.hostName = hostName;
        this.port = port;
        this.nodeName = hostName + ":" + port;
        this.id = nodeId;
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
     * Connect to current node with the same username and password.
     *
     * @return The Sequoiadb instance of current node.
     * @throws BaseException If error happens.
     */
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
     */
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
     * @return The Sequoiadb object of current node.
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
        obj.put(SdbConstants.FIELD_NAME_NODEID, id);

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
