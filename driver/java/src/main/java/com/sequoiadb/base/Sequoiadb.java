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
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.message.ResultSet;
import com.sequoiadb.message.request.*;
import com.sequoiadb.message.response.CommonResponse;
import com.sequoiadb.message.response.SdbReply;
import com.sequoiadb.message.response.SdbResponse;
import com.sequoiadb.message.response.SysInfoResponse;
import com.sequoiadb.net.IConnection;
import com.sequoiadb.net.ServerAddress;
import com.sequoiadb.net.TCPConnection;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.Code;
import org.bson.util.JSON;

import java.io.Closeable;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;

/**
 * The connection with SequoiaDB server.
 */
public class Sequoiadb implements Closeable {
    private InetSocketAddress socketAddress;
    private IConnection connection;
    private String userName;
    private String password;
    private ByteOrder byteOrder = ByteOrder.BIG_ENDIAN;
    private long requestId;
    private long lastUseTime;

    private Map<String, Long> nameCache = new HashMap<String, Long>();
    private static boolean enableCache = true;
    private static long cacheInterval = 300 * 1000;
    private BSONObject attributeCache = null;

    private final static String DEFAULT_HOST = "127.0.0.1";
    private final static int DEFAULT_PORT = 11810;

    /**
     * specified the package size of the collections in current collection space to be 4K
     */
    public final static int SDB_PAGESIZE_4K = 4096;
    /**
     * specified the package size of the collections in current collection space to be 8K
     */
    public final static int SDB_PAGESIZE_8K = 8192;
    /**
     * specified the package size of the collections in current collection space to be 16K
     */
    public final static int SDB_PAGESIZE_16K = 16384;
    /**
     * specified the package size of the collections in current collection space to be 32K
     */
    public final static int SDB_PAGESIZE_32K = 32768;
    /**
     * specified the package size of the collections in current collection space to be 64K
     */
    public final static int SDB_PAGESIZE_64K = 65536;
    /**
     * 0 means using database's default pagesize, it 64k now
     */
    public final static int SDB_PAGESIZE_DEFAULT = 0;

    public final static int SDB_LIST_CONTEXTS = 0;
    public final static int SDB_LIST_CONTEXTS_CURRENT = 1;
    public final static int SDB_LIST_SESSIONS = 2;
    public final static int SDB_LIST_SESSIONS_CURRENT = 3;
    public final static int SDB_LIST_COLLECTIONS = 4;
    public final static int SDB_LIST_COLLECTIONSPACES = 5;
    public final static int SDB_LIST_STORAGEUNITS = 6;
    public final static int SDB_LIST_GROUPS = 7;
    public final static int SDB_LIST_STOREPROCEDURES = 8;
    public final static int SDB_LIST_DOMAINS = 9;
    public final static int SDB_LIST_TASKS = 10;
    public final static int SDB_LIST_TRANSACTIONS = 11;
    public final static int SDB_LIST_TRANSACTIONS_CURRENT = 12;
    public final static int SDB_LIST_CL_IN_DOMAIN = 129;
    public final static int SDB_LIST_CS_IN_DOMAIN = 130;

    public final static int SDB_SNAP_CONTEXTS = 0;
    public final static int SDB_SNAP_CONTEXTS_CURRENT = 1;
    public final static int SDB_SNAP_SESSIONS = 2;
    public final static int SDB_SNAP_SESSIONS_CURRENT = 3;
    public final static int SDB_SNAP_COLLECTIONS = 4;
    public final static int SDB_SNAP_COLLECTIONSPACES = 5;
    public final static int SDB_SNAP_DATABASE = 6;
    public final static int SDB_SNAP_SYSTEM = 7;
    public final static int SDB_SNAP_CATALOG = 8;
    public final static int SDB_SNAP_TRANSACTIONS = 9;
    public final static int SDB_SNAP_TRANSACTIONS_CURRENT = 10;
    public final static int SDB_SNAP_ACCESSPLANS = 11;
    public final static int SDB_SNAP_HEALTH = 12;

    public final static int FMP_FUNC_TYPE_INVALID = -1;
    public final static int FMP_FUNC_TYPE_JS = 0;
    public final static int FMP_FUNC_TYPE_C = 1;
    public final static int FMP_FUNC_TYPE_JAVA = 2;

    public final static String CATALOG_GROUP_NAME = "SYSCatalogGroup";

    void upsertCache(String name) {
        if (name == null)
            return;
        if (enableCache) {
            long current = System.currentTimeMillis();
            nameCache.put(name, current);
            String[] arr = name.split("\\.");
            if (arr.length > 1) {
                nameCache.put(arr[0], current);
            }
        }
    }

    void removeCache(String name) {
        if (name == null)
            return;
        String[] arr = name.split("\\.");
        if (arr.length == 1) {

            nameCache.remove(name);
            Set<String> keySet = nameCache.keySet();
            List<String> list = new ArrayList<String>();
            for (String str : keySet) {
                String[] nameArr = str.split("\\.");
                if (nameArr.length > 1 && nameArr[0].equals(name))
                    list.add(str);
            }
            if (list.size() != 0) {
                for (String str : list)
                    nameCache.remove(str);
            }
        } else {
            nameCache.remove(name);
        }
    }

    boolean fetchCache(String name) {
        if (enableCache) {
            if (nameCache.containsKey(name)) {
                long lastUpdatedTime = nameCache.get(name);
                if ((System.currentTimeMillis() - lastUpdatedTime) > cacheInterval) {
                    nameCache.remove(name);
                    return false;
                } else {
                    return true;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    /**
     * Initialize the configuration options for client.
     *
     * @param options the configuration options for client
     */
    public static void initClient(ClientOptions options) {
        enableCache = (options != null) ? options.getEnableCache() : true;
        cacheInterval = (options != null && options.getCacheInterval() >= 0) ? options.getCacheInterval() : 300 * 1000;
    }

    /**
     * Get address of the remote server.
     *
     * @return ServerAddress
     * @deprecated Use Sequoiadb.getHost() and Sequoiadb.getPort() instead.
     */
    @Deprecated
    public ServerAddress getServerAddress() {
        return new ServerAddress(socketAddress);
    }

    /**
     * @return Host name of SequoiaDB server.
     */
    public String getHost() {
        return socketAddress.getHostName();
    }

    /**
     * @return Service port of SequoiaDB server.
     */
    public int getPort() {
        return socketAddress.getPort();
    }

    @Override
    public String toString() {
        return String.format("%s:%d", getHost(), getPort());
    }

    /**
     * Judge the endian of the physical computer
     *
     * @return Big-Endian is true while Little-Endian is false
     * @deprecated Use getByteOrder() instead.
     */
    @Deprecated
    public boolean isEndianConvert() {
        return byteOrder == ByteOrder.BIG_ENDIAN;
    }

    /**
     * @return ByteOrder of SequoiaDB server.
     * @since 2.9
     */
    public ByteOrder getByteOrder() {
        return byteOrder;
    }

    /**
     * @return The last used time of this connection.
     * @since 2.9
     */
    public long getLastUseTime() {
        return lastUseTime;
    }

    /**
     * Use server address "127.0.0.1:11810".
     *
     * @param username the user's name of the account
     * @param password the password of the account
     * @throws BaseException SDB_NETWORK means network error,
     *                       SDB_INVALIDARG means wrong address or the address don't map to the hosts table.
     * @deprecated do not use this Constructor, should provide server address explicitly
     */
    @Deprecated
    public Sequoiadb(String username, String password) throws BaseException {
        this(DEFAULT_HOST, DEFAULT_PORT, username, password, null);
    }

    /**
     * @param connString remote server address "Host:Port"
     * @param username   the user's name of the account
     * @param password   the password of the account
     * @throws BaseException SDB_NETWORK means network error,
     *                       SDB_INVALIDARG means wrong address or the address don't map to the hosts table
     */
    public Sequoiadb(String connString, String username, String password)
            throws BaseException {
        this(connString, username, password, (ConfigOptions) null);
    }

    /**
     * @param connString remote server address "Host:Port"
     * @param username   the user's name of the account
     * @param password   the password of the account
     * @param options    the options for connection
     * @throws BaseException SDB_NETWORK means network error,
     *                       SDB_INVALIDARG means wrong address or the address don't map to the hosts table.
     */
    public Sequoiadb(String connString, String username, String password,
                     ConfigOptions options) throws BaseException {
        init(connString, username, password, options);
    }

    /**
     * @deprecated Use com.sequoiadb.base.ConfigOptions instead of com.sequoiadb.net.ConfigOptions.
     */
    @Deprecated
    public Sequoiadb(String connString, String username, String password,
                     com.sequoiadb.net.ConfigOptions options) throws BaseException {
        this(connString, username, password, (ConfigOptions) options);
    }

    /**
     * Use a random valid address to connect to database.
     *
     * @param connStrings The array of the coord's address.
     * @param username    The user's name of the account.
     * @param password    The password of the account.
     * @param options     The options for connection.
     * @throws BaseException If error happens.
     */
    public Sequoiadb(List<String> connStrings, String username, String password,
                     ConfigOptions options) throws BaseException {
        if (connStrings == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "connStrings is null");
        }

        List<String> list = new ArrayList<String>();
        for (String str : connStrings) {
            if (str != null && !str.isEmpty()) {
                list.add(str);
            }
        }

        if (0 == list.size()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Address list has no valid address");
        }

        if (options == null) {
            options = new ConfigOptions();
        }

        Random random = new Random();
        while (list.size() > 0) {
            int index = random.nextInt(list.size());
            String str = list.get(index);
            try {
                init(str, username, password, options);
                return;
            } catch (BaseException e) {
                if (e.getErrorCode() == SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode()) {
                    throw e;
                }
                list.remove(index);
            }
        }

        throw new BaseException(SDBError.SDB_NET_CANNOT_CONNECT, "No valid address");
    }

    /**
     * @deprecated Use com.sequoiadb.base.ConfigOptions instead of com.sequoiadb.net.ConfigOptions.
     */
    @Deprecated
    public Sequoiadb(List<String> connStrings, String username, String password,
                     com.sequoiadb.net.ConfigOptions options) throws BaseException {
        this(connStrings, username, password, (ConfigOptions) options);
    }

    /**
     * @param host     the address of coord
     * @param port     the port of coord
     * @param username the user's name of the account
     * @param password the password of the account
     * @throws BaseException SDB_NETWORK means network error,
     *                       SDB_INVALIDARG means wrong address or the address don't map to the hosts table.
     */
    public Sequoiadb(String host, int port, String username, String password)
            throws BaseException {
        this(host, port, username, password, null);
    }

    /**
     * @param host     the address of coord
     * @param port     the port of coord
     * @param username the user's name of the account
     * @param password the password of the account
     * @throws BaseException SDB_NETWORK means network error,
     *                       SDB_INVALIDARG means wrong address or the address don't map to the hosts table.
     */
    public Sequoiadb(String host, int port,
                     String username, String password,
                     ConfigOptions options) throws BaseException {
        init(host, port, username, password, options);
    }

    /**
     * @deprecated Use com.sequoiadb.base.ConfigOptions instead of com.sequoiadb.net.ConfigOptions.
     */
    @Deprecated
    public Sequoiadb(String host, int port,
                     String username, String password,
                     com.sequoiadb.net.ConfigOptions options) throws BaseException {
        this(host, port, username, password, (ConfigOptions) options);
    }

    private void init(String host, int port,
                      String username, String password,
                      ConfigOptions options) throws BaseException {
        if (host == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "host is null");
        }

        if (options == null) {
            options = new ConfigOptions();
        }

        InetSocketAddress socketAddress = new InetSocketAddress(host, port);
        connection = new TCPConnection(socketAddress, options);
        connection.connect();

        byteOrder = getSysInfo();
        authenticate(username, password);

        this.socketAddress = socketAddress;
        this.userName = username;
        this.password = password;
    }

    private void init(String connString, String username, String password,
                      ConfigOptions options) {
        if (connString == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "connString is null");
        }

        String host;
        int port;

        if (connString.indexOf(":") > 0) {
            String[] tmp = connString.split(":");
            if (tmp.length != 2) {
                throw new BaseException(SDBError.SDB_INVALIDARG,
                        String.format("Invalid connString: %s", connString));
            }
            host = tmp[0].trim();
            port = Integer.parseInt(tmp[1].trim());
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    String.format("Invalid connString: %s", connString));
        }

        init(host, port, username, password, options);
    }

    private void authenticate(String username, String password) {
        AuthRequest request = new AuthRequest(username, password, AuthRequest.AuthType.Verify);
        SdbReply response = requestAndResponse(request);

        try {
            throwIfError(response, "failed to authenticate " + userName);
        } catch (BaseException e) {
            close();
            throw e;
        }
    }

    /**
     * Create an user in current database.
     *
     * @param username The connection user name
     * @param password The connection password
     * @throws BaseException If error happens.
     */
    public void createUser(String username, String password) throws BaseException {
        if (username == null || username.length() == 0 || password == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        AuthRequest request = new AuthRequest(username, password, AuthRequest.AuthType.CreateUser);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, username);
    }

    /**
     * Remove the specified user from current database.
     *
     * @param username The connection user name
     * @param password The connection password
     * @throws BaseException If error happens.
     */
    public void removeUser(String username, String password) throws BaseException {
        AuthRequest request = new AuthRequest(username, password, AuthRequest.AuthType.DeleteUser);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, username);
    }

    /**
     * Disconnect from the server.
     *
     * @throws BaseException If error happens.
     * @deprecated Use close() instead.
     */
    @Deprecated
    public void disconnect() throws BaseException {
        close();
    }

    /**
     * Release the resource of the connection.
     *
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void releaseResource() throws BaseException {
        closeAllCursors();
        attributeCache = null;
    }

    /**
     * Whether the connection has been closed or not.
     *
     * @return return true when the connection has been closed
     * @since 2.2
     */
    public boolean isClosed() {
        if (connection == null) {
            return true;
        }
        return connection.isClosed();
    }

    /**
     * Send a test message to database to test whether the connection is valid or not.
     *
     * @return if the connection is valid, return true
     * @throws BaseException If error happens.
     */
    public boolean isValid() throws BaseException {
        if (isClosed()) {
            return false;
        }

        try {
            killContext();
        } catch (BaseException e) {
            return false;
        }
        return true;
    }

    /**
     * Change the connection options.
     *
     * @param options The connection options
     * @throws BaseException If error happens.
     * @deprecated Create a new Sequoiadb instance instead..
     */
    @Deprecated
    public void changeConnectionOptions(ConfigOptions options)
            throws BaseException {
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }
        close();
        init(getHost(), getPort(), userName, password, options);
    }

    /**
     * Create the named collection space with default SDB_PAGESIZE_64K.
     *
     * @param csName The collection space name
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public CollectionSpace createCollectionSpace(String csName)
            throws BaseException {
        return createCollectionSpace(csName, SDB_PAGESIZE_DEFAULT);
    }

    /**
     * Create collection space.
     *
     * @param csName   The name of collection space
     * @param pageSize The Page Size as below:
     *                 <ul>
     *                 <li> SDB_PAGESIZE_4K
     *                 <li> SDB_PAGESIZE_8K
     *                 <li> SDB_PAGESIZE_16K
     *                 <li> SDB_PAGESIZE_32K
     *                 <li> SDB_PAGESIZE_64K
     *                 <li> SDB_PAGESIZE_DEFAULT
     *                 </ul>
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public CollectionSpace createCollectionSpace(String csName, int pageSize)
            throws BaseException {
        BSONObject options = new BasicBSONObject();
        options.put("PageSize", pageSize);
        return createCollectionSpace(csName, options);
    }

    /**
     * Create collection space.
     *
     * @param csName  The name of collection space
     * @param options Contains configuration information for create collection space. The options are as below:
     *                <ul>
     *                <li>PageSize    : Assign how large the page size is for the collection created in this collection space, default to be 64K
     *                <li>Domain    : Assign which domain does current collection space belong to, it will belongs to the system domain if not assign this option
     *                </ul>
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public CollectionSpace createCollectionSpace(String csName, BSONObject options)
            throws BaseException {
        if (csName == null || csName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
        }
        if (isCollectionSpaceExist(csName)) {
            throw new BaseException(SDBError.SDB_DMS_CS_EXIST, csName);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, csName);
        if (null != options) {
            obj.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_CS, obj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        upsertCache(csName);
        return new CollectionSpace(this, csName);
    }

    /**
     * Remove the named collection space.
     *
     * @param csName The collection space name
     * @throws BaseException If error happens.
     */
    public void dropCollectionSpace(String csName) throws BaseException {
        if (!isCollectionSpaceExist(csName)) {
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, csName);
        }

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_NAME, csName);

        AdminRequest request = new AdminRequest(AdminCommand.DROP_CS, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        removeCache(csName);
    }

    /**
     * @param csName  The collection space name
     * @param options The control options:(Only take effect in coordinate nodes, can be null)
     *                <ul>
     *                <li>GroupID:int</li>
     *                <li>GroupName:String</li>
     *                <li>NodeID:int</li>
     *                <li>HostName:String</li>
     *                <li>svcname:String</li>
     *                <li>...</li>
     *                </ul>
     * @throws BaseException If error happens.
     * @since 2.8
     */
    public void loadCollectionSpace(String csName, BSONObject options) throws BaseException {
        if (csName == null || csName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
        }
        if (isCollectionSpaceExist(csName)) {
            throw new BaseException(SDBError.SDB_DMS_CS_EXIST, csName);
        }

        BSONObject newOptions = new BasicBSONObject();
        newOptions.put(SdbConstants.FIELD_NAME_NAME, csName);
        if (options != null) {
            newOptions.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.LOAD_CS, newOptions);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        upsertCache(csName);
    }

    /**
     * @param csName  The collection space name
     * @param options The control options:(Only take effect in coordinate nodes, can be null)
     *                <ul>
     *                <li>GroupID:int</li>
     *                <li>GroupName:String</li>
     *                <li>NodeID:int</li>
     *                <li>HostName:String</li>
     *                <li>svcname:String</li>
     *                <li>...</li>
     *                </ul>
     * @throws BaseException If error happens.
     * @since 2.8
     */
    public void unloadCollectionSpace(String csName, BSONObject options) throws BaseException {
        if (csName == null || csName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
        }
        if (!isCollectionSpaceExist(csName)) {
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, csName);
        }

        BSONObject newOptions = new BasicBSONObject();
        newOptions.put(SdbConstants.FIELD_NAME_NAME, csName);
        if (options != null) {
            newOptions.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.UNLOAD_CS, newOptions);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        removeCache(csName);
    }

    /**
     * @param oldName The old collection space name
     * @param newName The new collection space name
     * @throws BaseException If error happens.
     * @since 2.8
     */
    public void renameCollectionSpace(String oldName, String newName) throws BaseException {
        if (oldName == null || oldName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, oldName);
        }
        if (newName == null || newName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, newName);
        }
        if (!isCollectionSpaceExist(oldName)) {
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, oldName);
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_OLDNAME, oldName);
        matcher.put(SdbConstants.FIELD_NAME_NEWNAME, newName);

        AdminRequest request = new AdminRequest(AdminCommand.RENAME_CS, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        removeCache(oldName);
        upsertCache(newName);
    }

    /**
     * Sync the database to disk.
     *
     * @param options The control options:(can be null)
     *                <ul>
     *                <li>
     *                Deep:int
     *                Flush with deep mode or not. 1 in default.
     *                0 for non-deep mode,1 for deep mode,-1 means use the configuration with server
     *                </li>
     *                <li>
     *                Block:boolean
     *                Flush with block mode or not. false in default.
     *                </li>
     *                <li>
     *                CollectionSpace:String
     *                Specify the collectionspace to sync.
     *                If not set, will sync all the collection spaces and logs,
     *                otherwise, will only sync the collection space specified.
     *                </li>
     *                <li>
     *                Some of other options are as below:(only take effect in coordinate nodes,
     *                please visit the official website to search "sync" or
     *                "Location Elements" for more detail.)
     *                GroupID:int,
     *                GroupName:String,
     *                NodeID:int,
     *                HostName:String,
     *                svcname:String
     *                ...
     *                </li>
     *                </ul>
     * @throws BaseException If error happens.
     * @since 2.8
     */
    public void sync(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.SYNC_DB, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Sync the whole database to disk.
     *
     * @throws BaseException If error happens.
     * @since 2.8
     */
    public void sync() throws BaseException {
        sync(null);
    }

    /**
     * Analyze collection or index to collect statistics information
     *
     * @param options The control options:(can be null)
     *                <ul>
     *                <li>
     *                CollectionSpace: (String) Specify the collection space to be analyzed.
     *                </li>
     *                <li>
     *                Collection: (String) Specify the collection to be analyzed.
     *                </li>
     *                <li>
     *                Index: (String) Specify the index to be analyzed.
     *                </li>
     *                <li>
     *                Mode: (Int32) Specify the analyze mode (default is 1):
     *                <ul>
     *                <li>Mode 1 will analyze with data samples.</li>
     *                <li>Mode 2 will analyze with full data.</li>
     *                <li>Mode 3 will generate default statistics.</li>
     *                <li>Mode 4 will reload statistics into memory cache.</li>
     *                <li>Mode 5 will clear statistics from memory cache.</li>
     *                </ul>
     *                </li>
     *                <li>
     *                Other options: Some of other options are as below:(only take effect in coordinate nodes,
     *                please visit the official website to search "analyze" or "Location Elements" for more detail.)
     *                GroupID:int, GroupName:String, NodeID:int, HostName:String, svcname:String, ...
     *                </li>
     *                </ul>
     * @throws BaseException If error happens.
     * @since 2.9
     */
    public void analyze(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.ANALYZE, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Analyze all collections and indexes to collect statistics information
     *
     * @throws BaseException If error happens.
     * @since 2.9
     */
    public void analyze() throws BaseException {
        analyze(null);
    }

    /**
     * Get the named collection space.
     * If the collection space not exit, throw BaseException with errcode SDB_DMS_CS_NOTEXIST.
     *
     * @param csName The collection space name.
     * @return the object of the specified collection space, or an exception when the collection space does not exist.
     * @throws BaseException If error happens.
     */
    public CollectionSpace getCollectionSpace(String csName)
            throws BaseException {
        if (fetchCache(csName)) {
            return new CollectionSpace(this, csName);
        }
        if (isCollectionSpaceExist(csName)) {
            return new CollectionSpace(this, csName);
        } else {
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, csName);
        }
    }

    /**
     * Verify the existence of collection space.
     *
     * @param csName The collection space name.
     * @return True if existed or false if not existed.
     * @throws BaseException If error happens.
     */
    public boolean isCollectionSpaceExist(String csName) throws BaseException {
        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_NAME, csName);

        AdminRequest request = new AdminRequest(AdminCommand.TEST_CS, options);
        SdbReply response = requestAndResponse(request);

        int flag = response.getFlag();
        if (flag == 0) {
            upsertCache(csName);
            return true;
        } else if (flag == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
            removeCache(csName);
            return false;
        } else {
            throwIfError(response, csName);
            return false; // make compiler happy
        }
    }

    /**
     * Get all the collection spaces.
     *
     * @return Cursor of all collection space names.
     * @throws BaseException If error happens.
     */
    public DBCursor listCollectionSpaces() throws BaseException {
        return getList(SDB_LIST_COLLECTIONSPACES, null, null, null);
    }

    /**
     * Get all the collection space names.
     *
     * @return A list of all collection space names
     * @throws BaseException If error happens.
     */
    public ArrayList<String> getCollectionSpaceNames() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_COLLECTIONSPACES, null, null, null);
        if (cursor == null) {
            return null;
        }
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * Get all the collections.
     *
     * @return Cursor of all collections
     * @throws BaseException If error happens.
     */
    public DBCursor listCollections() throws BaseException {
        return getList(SDB_LIST_COLLECTIONS, null, null, null);
    }

    /**
     * Get all the collection names.
     *
     * @return A list of all collection names
     * @throws BaseException If error happens.
     */
    public ArrayList<String> getCollectionNames() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_COLLECTIONS, null, null, null);
        if (cursor == null) {
            return null;
        }
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * Get all the storage units.
     *
     * @return A list of all storage units
     * @throws BaseException If error happens.
     */
    public ArrayList<String> getStorageUnits() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_STORAGEUNITS, null, null, null);
        if (cursor == null) {
            return null;
        }
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * Reset the snapshot.
     *
     * @throws BaseException If error happens.
     */
    public void resetSnapshot() throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.RESET_SNAPSHOT);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Reset the snapshot.
     *
     * @param options The control options:(can be null)
     *                <ul>
     *                <li>
     *                Type: (String) Specify the snapshot type to be reset (default is "all"):
     *                <ul>
     *                <li>"sessions"</li>
     *                <li>"sessions current"</li>
     *                <li>"database"</li>
     *                <li>"health"</li>
     *                <li>"all"</li>
     *                </ul>
     *                </li>
     *                <li>
     *                SessionID: (Int32) Specify the session ID to be reset.
     *                </li>
     *                <li>
     *                Other options: Some of other options are as below:(please visit the official website to
     *                search "Location Elements" for more detail.)
     *                <ul>
     *                <li>GroupID:int,</li>
     *                <li>GroupName:String,</li>
     *                <li>NodeID:int,</li>
     *                <li>HostName:String,</li>
     *                <li>svcname:String,</li>
     *                <li>...</li>
     *                </ul>
     *                </li>
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void resetSnapshot(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.RESET_SNAPSHOT, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Get the information of specified type.
     *
     * @param listType The list type as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_LIST_CONTEXTS             : Get all contexts list
     *                 <dt>Sequoiadb.SDB_LIST_CONTEXTS_CURRENT     : Get contexts list for the current session
     *                 <dt>Sequoiadb.SDB_LIST_SESSIONS             : Get all sessions list
     *                 <dt>Sequoiadb.SDB_LIST_SESSIONS_CURRENT     : Get the current session
     *                 <dt>Sequoiadb.SDB_LIST_COLLECTIONS          : Get all collections list
     *                 <dt>Sequoiadb.SDB_LIST_COLLECTIONSPACES     : Get all collection spaces list
     *                 <dt>Sequoiadb.SDB_LIST_STORAGEUNITS         : Get storage units list
     *                 <dt>Sequoiadb.SDB_LIST_GROUPS               : Get replica group list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_STOREPROCEDURES      : Get stored procedure list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_DOMAINS              : Get all the domains list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_TASKS                : Get all the running split tasks ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_TRANSACTIONS         : Get all the transactions information.
     *                 <dt>Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT : Get the transactions information of current session.
     *                 </dl>
     * @param query    The matching rule, match all the documents if null.
     * @param selector The selective rule, return the whole document if null.
     * @param orderBy  The ordered rule, never sort if null.
     * @throws BaseException If error happens.
     */
    public DBCursor getList(int listType, BSONObject query, BSONObject selector, BSONObject orderBy) throws BaseException {
        String command = getListCommand(listType);
        AdminRequest request = new AdminRequest(command, query, selector, orderBy, null);
        SdbReply response = requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "query = " + query +
                        ", selector = " + selector +
                        ", orderBy = " + orderBy;
                throwIfError(response, msg);
            }
        }

        return new DBCursor(response, this);
    }

    /**
     * Flush the options to configuration file.
     *
     * @param options The param of flush, pass {"Global":true} or {"Global":false}
     *                In cluster environment, passing {"Global":true} will flush data's and catalog's configuration file,
     *                while passing {"Global":false} will flush coord's configuration file
     *                In stand-alone environment, both them have the same behaviour
     * @throws BaseException If error happens.
     */
    public void flushConfigure(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.EXPORT_CONFIG, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Execute sql in database.
     *
     * @param sql the SQL command.
     * @throws BaseException If error happens.
     */
    public void execUpdate(String sql) throws BaseException {
        SQLRequest request = new SQLRequest(sql);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, sql);
    }

    /**
     * Execute sql in database.
     *
     * @param sql the SQL command
     * @return the DBCursor of the result
     * @throws BaseException If error happens.
     */
    public DBCursor exec(String sql) throws BaseException {
        SQLRequest request = new SQLRequest(sql);
        SdbReply response = requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                throwIfError(response, sql);
            }
        }

        return new DBCursor(response, this);
    }

    /**
     * Get snapshot of the database.
     *
     * @param snapType The snapshot types are as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS             : Get all contexts' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT     : Get the current context's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS             : Get all sessions' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS_CURRENT     : Get the current session's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONS          : Get the collections' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONSPACES     : Get the collection spaces' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_DATABASE             : Get database's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SYSTEM               : Get system's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CATALOG              : Get catalog's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS         : Get the snapshot of all the transactions
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT : Get the snapshot of current transactions
     *                 <dt>Sequoiadb.SDB_SNAP_ACCESSPLANS          : Get the snapshot of cached access plans
     *                 <dt>Sequoiadb.SDB_SNAP_HEALTH               : Get the snapshot of node health detection
     *                 </dl>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @return the DBCursor of the result
     * @throws BaseException If error happens.
     */
    public DBCursor getSnapshot(int snapType, String matcher, String selector,
                                String orderBy) throws BaseException {
        BSONObject ma = null;
        BSONObject se = null;
        BSONObject or = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (selector != null) {
            se = (BSONObject) JSON.parse(selector);
        }
        if (orderBy != null) {
            or = (BSONObject) JSON.parse(orderBy);
        }

        return getSnapshot(snapType, ma, se, or);
    }

    /**
     * Get snapshot of the database.
     *
     * @param snapType The snapshot types are as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS             : Get all contexts' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT     : Get the current context's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS             : Get all sessions' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS_CURRENT     : Get the current session's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONS          : Get the collections' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONSPACES     : Get the collection spaces' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_DATABASE             : Get database's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SYSTEM               : Get system's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CATALOG              : Get catalog's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS         : Get snapshot of transactions in current session
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT : Get snapshot of all the transactions
     *                 <dt>SequoiaDB.SDB_SNAP_ACCESSPLANS          : Get the snapshot of cached access plans
     *                 <dt>Sequoiadb.SDB_SNAP_HEALTH               : Get the snapshot of node health detection
     *                 </dl>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @return the DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor getSnapshot(int snapType, BSONObject matcher,
                                BSONObject selector, BSONObject orderBy) throws BaseException {
        String command = getSnapshotCommand(snapType);

        AdminRequest request = new AdminRequest(command, matcher, selector, orderBy, null);
        SdbReply response = requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "matcher = " + matcher +
                        ", selector = " + selector +
                        ", orderBy = " + orderBy;
                throwIfError(response, msg);
            }
        }

        return new DBCursor(response, this);
    }

    private String getSnapshotCommand(int snapType) {
        switch (snapType) {
            case SDB_SNAP_CONTEXTS:
                return AdminCommand.SNAP_CONTEXTS;
            case SDB_SNAP_CONTEXTS_CURRENT:
                return AdminCommand.SNAP_CONTEXTS_CURRENT;
            case SDB_SNAP_SESSIONS:
                return AdminCommand.SNAP_SESSIONS;
            case SDB_SNAP_SESSIONS_CURRENT:
                return AdminCommand.SNAP_SESSIONS_CURRENT;
            case SDB_SNAP_COLLECTIONS:
                return AdminCommand.SNAP_COLLECTIONS;
            case SDB_SNAP_COLLECTIONSPACES:
                return AdminCommand.SNAP_COLLECTIONSPACES;
            case SDB_SNAP_DATABASE:
                return AdminCommand.SNAP_DATABASE;
            case SDB_SNAP_SYSTEM:
                return AdminCommand.SNAP_SYSTEM;
            case SDB_SNAP_CATALOG:
                return AdminCommand.SNAP_CATALOG;
            case SDB_SNAP_TRANSACTIONS:
                return AdminCommand.SNAP_TRANSACTIONS;
            case SDB_SNAP_TRANSACTIONS_CURRENT:
                return AdminCommand.SNAP_TRANSACTIONS_CURRENT;
            case SDB_SNAP_ACCESSPLANS:
                return AdminCommand.SNAP_ACCESSPLANS;
            case SDB_SNAP_HEALTH:
                return AdminCommand.SNAP_HEALTH;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG, String.format("Invalid snapshot type: %d", snapType));
        }
    }

    /**
     * Begin the transaction.
     *
     * @throws BaseException If error happens.
     */
    public void beginTransaction() throws BaseException {
        TransactionRequest request = new TransactionRequest(TransactionRequest.TransactionType.Begin);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Commit the transaction.
     *
     * @throws BaseException If error happens.
     */
    public void commit() throws BaseException {
        TransactionRequest request = new TransactionRequest(TransactionRequest.TransactionType.Commit);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Rollback the transaction.
     *
     * @throws BaseException If error happens.
     */
    public void rollback() throws BaseException {
        TransactionRequest request = new TransactionRequest(TransactionRequest.TransactionType.Rollback);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Create a storage procedure.
     *
     * @param code The code of storage procedure
     * @throws BaseException If error happens.
     */
    public void crtJSProcedure(String code) throws BaseException {
        if (null == code || code.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, code);
        }

        BSONObject options = new BasicBSONObject();
        Code codeObj = new Code(code);
        options.put(SdbConstants.FIELD_NAME_FUNC, codeObj);
        options.put(SdbConstants.FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS);

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_PROCEDURE, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Remove a store procedure.
     *
     * @param name The name of store procedure to be removed
     * @throws BaseException If error happens.
     */
    public void rmProcedure(String name) throws BaseException {
        if (null == name || name.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, name);
        }

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_FUNC, name);

        AdminRequest request = new AdminRequest(AdminCommand.REMOVE_PROCEDURE, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * List the storage procedures.
     *
     * @param condition The condition of list eg: {"name":"sum"}. return all if null
     * @throws BaseException If error happens.
     */
    public DBCursor listProcedures(BSONObject condition) throws BaseException {
        return getList(SDB_LIST_STOREPROCEDURES, condition, null, null);
    }

    /**
     * Eval javascript code.
     *
     * @param code The javascript code
     * @return The result of the eval operation, including the return value type,
     * the return data and the error message. If succeed to eval, error message is null,
     * and we can extract the eval result from the return cursor and return type,
     * if not, the return cursor and the return type are null, we can extract
     * the error message for more detail.
     * @throws BaseException If error happens.
     */
    public Sequoiadb.SptEvalResult evalJS(String code) throws BaseException {
        if (code == null || code.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        SptEvalResult evalResult = new Sequoiadb.SptEvalResult();

        BSONObject newObj = new BasicBSONObject();
        Code codeObj = new Code(code);
        newObj.put(SdbConstants.FIELD_NAME_FUNC, codeObj);
        newObj.put(SdbConstants.FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS);

        AdminRequest request = new AdminRequest(AdminCommand.EVAL, newObj);
        SdbReply response = requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (response.getErrorObj() != null) {
                evalResult.errmsg = response.getErrorObj();
            }
            return evalResult;
        } else {
            if (response.getReturnedNum() > 0) {
                BSONObject obj = response.getResultSet().getNext();
                int typeValue = (Integer) obj.get(SdbConstants.FIELD_NAME_RETYE);
                evalResult.returnType = Sequoiadb.SptReturnType.getTypeByValue(typeValue);
            }
            evalResult.cursor = new DBCursor(response, this);
            return evalResult;
        }
    }

    /**
     * Backup the whole database or specified replica group.
     *
     * @param options Contains a series of backup configuration infomations.
     *                Backup the whole cluster if null. The "options" contains 5 options as below.
     *                All the elements in options are optional.
     *                eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup",
     *                "Name":"backupName", "Description":description, "EnsureInc":true, "OverWrite":true}
     *                <ul>
     *                <li>GroupID     : The id(s) of replica group(s) which to be backuped
     *                <li>GroupName   : The name(s) of replica group(s) which to be backuped
     *                <li>Name        : The name for the backup
     *                <li>Path        : The backup path, if not assign, use the backup path assigned in the configuration file,
     *                the path support to use wildcard(%g/%G:group name, %h/%H:host name, %s/%S:service name).
     *                e.g.  {Path:"/opt/sequoiadb/backup/%g"}
     *                <li>isSubDir    : Whether the path specified by paramer "Path" is a subdirectory of
     *                the path specified in the configuration file, default to be false
     *                <li>Prefix      : The prefix of name for the backup, default to be null. e.g. {Prefix:"%g_bk_"}
     *                <li>EnableDateDir : Whether turn on the feature which will create subdirectory named to
     *                current date like "YYYY-MM-DD" automatically, default to be false
     *                <li>Description : The description for the backup
     *                <li>EnsureInc   : Whether turn on increment synchronization, default to be false
     *                <li>OverWrite   : Whether overwrite the old backup file with the same name, default to be false
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void backupOffline(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.BACKUP_OFFLINE, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * List the backups.
     *
     * @param options  Contains configuration information for listing backups, list all the backups in the default backup path if null.
     *                 The "options" contains several options as below. All the elements in options are optional.
     *                 eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName"}
     *                 <ul>
     *                 <li>GroupID     : Specified the group id of the backups, default to list all the backups of all the groups.
     *                 <li>GroupName   : Specified the group name of the backups, default to list all the backups of all the groups.
     *                 <li>Path        : Specified the path of the backups, default to use the backup path asigned in the configuration file.
     *                 <li>Name        : Specified the name of backup, default to list all the backups.
     *                 <li>IsSubDir    : Specified the "Path" is a subdirectory of the backup path asigned in the configuration file or not, default to be false.
     *                 <li>Prefix      : Specified the prefix name of the backups, support for using wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not using wildcards.
     *                 <li>Detail      : Display the detail of the backups or not, default to be false.
     *                 </ul>
     * @param matcher  The matching rule, return all the documents if null
     * @param selector The selective rule, return the whole document if null
     * @param orderBy  The ordered rule, never sort if null
     * @return the DBCursor of the backup or null while having no backup information.
     * @throws BaseException If error happens.
     */
    public DBCursor listBackup(BSONObject options, BSONObject matcher,
                               BSONObject selector, BSONObject orderBy) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.LIST_BACKUP, matcher, selector, orderBy, options);
        SdbReply response = requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "matcher = " + matcher +
                        ", selector = " + selector +
                        ", orderBy = " + orderBy +
                        ", options = " + options;
                throwIfError(response, msg);
            }
        }

        DBCursor cursor = new DBCursor(response, this);
        return cursor;
    }

    /**
     * Remove the backups.
     *
     * @param options Contains configuration information for removing backups, remove all the backups in the default backup path if null.
     *                The "options" contains several options as below. All the elements in options are optional.
     *                eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName"}
     *                <ul>
     *                <li>GroupID     : Specified the group id of the backups, default to list all the backups of all the groups.
     *                <li>GroupName   : Specified the group name of the backups, default to list all the backups of all the groups.
     *                <li>Path        : Specified the path of the backups, default to use the backup path asigned in the configuration file.
     *                <li>Name        : Specified the name of backup, default to list all the backups.
     *                <li>IsSubDir    : Specified the "Path" is a subdirectory of the backup path assigned in the configuration file or not, default to be false.
     *                <li>Prefix      : Specified the prefix name of the backups, support for using wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not using wildcards.
     *                <li>Detail      : Display the detail of the backups or not, default to be false.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void removeBackup(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.REMOVE_BACKUP, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * List the tasks.
     *
     * @param matcher  The matching rule, return all the documents if null
     * @param selector The selective rule, return the whole document if null
     * @param orderBy  The ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public DBCursor listTasks(BSONObject matcher, BSONObject selector,
                              BSONObject orderBy, BSONObject hint) throws BaseException {
        return getList(SDB_LIST_TASKS, matcher, selector, orderBy);
    }

    /**
     * Wait the tasks to finish.
     *
     * @param taskIDs The array of task id
     * @throws BaseException If error happens.
     */
    public void waitTasks(long[] taskIDs) throws BaseException {
        if (taskIDs == null || taskIDs.length == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "taskIDs is empty or null");
        }

        BSONObject newObj = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        BSONObject list = new BasicBSONList();
        for (int i = 0; i < taskIDs.length; i++) {
            list.put(Integer.toString(i), taskIDs[i]);
        }
        subObj.put("$in", list);
        newObj.put(SdbConstants.FIELD_NAME_TASKID, subObj);

        AdminRequest request = new AdminRequest(AdminCommand.WAIT_TASK, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Cancel the specified task.
     *
     * @param taskID  The task id
     * @param isAsync The operation "cancel task" is async or not,
     *                "true" for async, "false" for sync. Default sync.
     * @throws BaseException If error happens.
     */
    public void cancelTask(long taskID, boolean isAsync) throws BaseException {
        if (taskID <= 0) {
            String msg = "taskID = " + taskID + ", isAsync = " + isAsync;
            throw new BaseException(SDBError.SDB_INVALIDARG, msg);
        }

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SdbConstants.FIELD_NAME_TASKID, taskID);
        newObj.put(SdbConstants.FIELD_NAME_ASYNC, isAsync);

        AdminRequest request = new AdminRequest(AdminCommand.CANCEL_TASK, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    private void clearSessionAttrCache() {
        attributeCache = null;
    }

    private BSONObject getSessionAttrCache() {
        return attributeCache;
    }

    private void setSessionAttrCache(BSONObject attribute) {
        attributeCache = attribute;
    }

    /**
     * Set the attributes of the current session.
     *
     * @param options The configuration options for the current session.The options are as below:
     *                <ul>
     *                <li>PreferedInstance : Preferred instance for read request in the current session. Could be single value in "M", "m", "S", "s", "A", "a", 1-255, or BSON Array to include multiple values. e.g. { "PreferedInstance" : [ 1, 7 ] }.
     *                <ul>
     *                <li>"M", "m": read and write instance( master instance ). If multiple numeric instances are given with "M", matched master instance will be chosen in higher priority. If multiple numeric instances are given with "M" or "m", master instance will be chosen if no numeric instance is matched.</li>
     *                <li>"S", "s": read only instance( slave instance ). If multiple numeric instances are given with "S", matched slave instances will be chosen in higher priority. If multiple numeric instances are given with "S" or "s", slave instance will be chosen if no numeric instance is matched.</li>
     *                <li>"A", "a": any instance.</li>
     *                <li>1-255: the instance with specified instance ID.</li>
     *                <li>If multiple alphabet instances are given, only first one will be used.</li>
     *                <li>If matched instance is not found, will choose instance by random.</li>
     *                </ul>
     *                </li>
     *                <li>PreferedInstanceMode : The mode to choose query instance when multiple preferred instances are found in the current session. e.g. { "PreferedInstanceMode : "random" }.
     *                <ul>
     *                <li>"random": choose the instance from matched instances by random.</li>
     *                <li>"ordered": choose the instance from matched instances by the order of "PreferedInstance".</li>
     *                </ul>
     *                </li>
     *                <li>Timeout : The timeout (in ms) for operations in the current session. -1 means no timeout for operations. e.g. { "Timeout" : 10000 }.
     *                </li>
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void setSessionAttr(BSONObject options) throws BaseException {
        if (null == options || options.isEmpty()) {
            return;
        }

        BSONObject newObj = new BasicBSONObject();

        newObj.putAll(options);

        if (options.containsField(SdbConstants.FIELD_NAME_PREFERED_INSTANCE)) {
            Object value = options.get(SdbConstants.FIELD_NAME_PREFERED_INSTANCE);
            if (value instanceof String) {
                int v = PreferInstance.MASTER;
                if (value.equals("M") || value.equals("m")) {
                    v = PreferInstance.MASTER;
                } else if (value.equals("S") || value.equals("s")) {
                    v = PreferInstance.SLAVE;
                } else if (value.equals("A") || value.equals("a")) {
                    v = PreferInstance.ANYONE;
                } else {
                    throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
                }
                newObj.put(SdbConstants.FIELD_NAME_PREFERED_INSTANCE, v);
            } else if (value instanceof Integer) {
                newObj.put(SdbConstants.FIELD_NAME_PREFERED_INSTANCE, value);
            }
            newObj.put(SdbConstants.FIELD_NAME_PREFERED_INSTANCE_V1, value);
        }

        clearSessionAttrCache();

        AdminRequest request = new AdminRequest(AdminCommand.SET_SESSION_ATTRIBUTE, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Get the attributes of the current session.
     *
     * @return the BSONObject of the session attribute.
     * @throws BaseException If error happens.
     * @since 2.8.5
     */
    public BSONObject getSessionAttr() throws BaseException {
        BSONObject result = getSessionAttrCache();
        if (null != result) {
            return result;
        }
        AdminRequest request = new AdminRequest(AdminCommand.GET_SESSION_ATTRIBUTE);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        ResultSet resultSet = response.getResultSet();
        if (null != resultSet && resultSet.hasNext()) {
            result = resultSet.getNext();
            if (null == result) {
                clearSessionAttrCache();
            } else {
                setSessionAttrCache(result);
            }
        } else {
            clearSessionAttrCache();
        }
        return result;
    }

    /**
     * Close all the cursors created in current connection, we can't use those cursors to get
     * data again.
     *
     * @throws BaseException If error happens.
     */
    public void closeAllCursors() throws BaseException {
        if (isClosed()) {
            return;
        }
        InterruptRequest request = new InterruptRequest();
        sendRequest(request);
    }

    /**
     * List all the replica group.
     *
     * @return cursor of all collection space names
     * @throws BaseException If error happens.
     */
    public DBCursor listReplicaGroups() throws BaseException {
        return getList(SDB_LIST_GROUPS, null, null, null);
    }

    /**
     * Verify the existence of domain.
     *
     * @param domainName the name of domain
     * @return True if existed or False if not existed
     * @throws BaseException If error happens.
     */
    public boolean isDomainExist(String domainName) throws BaseException {
        if (null == domainName || domainName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, domainName);
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_NAME, domainName);

        DBCursor cursor = getList(SDB_LIST_DOMAINS, matcher, null, null);
        try {
            if (cursor != null && cursor.hasNext())
                return true;
            else
                return false;
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    /**
     * Create a domain.
     *
     * @param domainName The name of the creating domain
     * @param options    The options for the domain. The options are as below:
     *                   <ul>
     *                   <li>Groups    : the list of the replica groups' names which the domain is going to contain.
     *                   eg: { "Groups": [ "group1", "group2", "group3" ] }
     *                   If this argument is not included, the domain will contain all replica groups in the cluster.
     *                   <li>AutoSplit    : If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
     *                   the data of this collection will be split(hash split) into all the groups in this domain automatically.
     *                   However, it won't automatically split data into those groups which were add into this domain later.
     *                   eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
     *                   </ul>
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public Domain createDomain(String domainName, BSONObject options) throws BaseException {
        if (null == domainName || domainName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "domain name is empty or null");
        }
        if (isDomainExist(domainName))
            throw new BaseException(SDBError.SDB_CAT_DOMAIN_EXIST, domainName);

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SdbConstants.FIELD_NAME_NAME, domainName);
        if (null != options) {
            newObj.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_DOMAIN, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        return new Domain(this, domainName);
    }

    /**
     * Drop a domain.
     *
     * @param domainName the name of the domain
     * @throws BaseException If error happens.
     */
    public void dropDomain(String domainName) throws BaseException {
        if (null == domainName || domainName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "domain name is empty or null");
        }

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SdbConstants.FIELD_NAME_NAME, domainName);

        AdminRequest request = new AdminRequest(AdminCommand.DROP_DOMAIN, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Get the specified domain.
     *
     * @param domainName the name of the domain
     * @return the Domain instance
     * @throws BaseException If the domain not exit, throw BaseException with the error SDB_CAT_DOMAIN_NOT_EXIST.
     */
    public Domain getDomain(String domainName) throws BaseException {
        if (isDomainExist(domainName)) {
            return new Domain(this, domainName);
        } else {
            throw new BaseException(SDBError.SDB_CAT_DOMAIN_NOT_EXIST, domainName);
        }
    }

    /**
     * List domains.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public DBCursor listDomains(BSONObject matcher, BSONObject selector,
                                BSONObject orderBy, BSONObject hint) throws BaseException {
        return getList(SDB_LIST_DOMAINS, matcher, selector, orderBy);
    }

    /**
     * Get all the replica groups' name.
     *
     * @return A list of all the replica groups' names.
     * @throws BaseException If error happens.
     */
    public ArrayList<String> getReplicaGroupNames() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_GROUPS, null, null, null);
        if (cursor == null) {
            return null;
        }
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("GroupName").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * Get the information of the replica groups.
     *
     * @return A list of information of the replica groups.
     * @throws BaseException If error happens.
     */
    public ArrayList<String> getReplicaGroupsInfo() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_GROUPS, null, null, null);
        if (cursor == null) {
            return null;
        }
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * whether the replica group exists in the database or not
     *
     * @param rgName replica group's name
     * @return true or false
     */
    public boolean isRelicaGroupExist(String rgName) {
        BSONObject rg = getDetailByName(rgName);
        if (rg == null) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * whether the replica group exists in the database or not
     *
     * @param rgId id of replica group
     * @return true or false
     */
    public boolean isReplicaGroupExist(int rgId) {
        BSONObject rg = getDetailById(rgId);
        if (rg == null) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * Get replica group by name.
     *
     * @param rgName replica group's name
     * @return A replica group object or null for not exit.
     * @throws BaseException If error happens.
     */
    public ReplicaGroup getReplicaGroup(String rgName)
            throws BaseException {
        BSONObject rg = getDetailByName(rgName);
        if (rg == null) {
            throw new BaseException(SDBError.SDB_CLS_GRP_NOT_EXIST,
                    String.format("Group with the name[%s] does not exist", rgName));
        }
        return new ReplicaGroup(this, rgName);
    }

    /**
     * Get replica group by id.
     *
     * @param rgId replica group id
     * @return A replica group object or null for not exit.
     * @throws BaseException If error happens.
     */
    public ReplicaGroup getReplicaGroup(int rgId) throws BaseException {
        BSONObject rg = getDetailById(rgId);
        if (rg == null) {
            throw new BaseException(SDBError.SDB_CLS_GRP_NOT_EXIST,
                    String.format("Group with the name[%d] does not exist", rgId));
        }
        return new ReplicaGroup(this, rgId);
    }

    /**
     * Create replica group by name.
     *
     * @param rgName replica group's name
     * @return A replica group object.
     * @throws BaseException If error happens.
     */
    public ReplicaGroup createReplicaGroup(String rgName) throws BaseException {
        BSONObject rg = new BasicBSONObject();
        rg.put(SdbConstants.FIELD_NAME_GROUPNAME, rgName);

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_GROUP, rg);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, rgName);
        return new ReplicaGroup(this, rgName);
    }

    /**
     * Remove replica group by name.
     *
     * @param rgName replica group's name
     * @throws BaseException If error happens.
     */
    public void removeReplicaGroup(String rgName) throws BaseException {
        BSONObject rg = new BasicBSONObject();
        rg.put(SdbConstants.FIELD_NAME_GROUPNAME, rgName);

        AdminRequest request = new AdminRequest(AdminCommand.REMOVE_GROUP, rg);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, rgName);
    }

    private long getNextRequestId() {
        return requestId++;
    }

    /**
     * Active replica group by name.
     *
     * @param rgName replica group name
     * @throws BaseException If error happens.
     */
    public void activateReplicaGroup(String rgName) throws BaseException {
        BSONObject rg = new BasicBSONObject();
        rg.put(SdbConstants.FIELD_NAME_GROUPNAME, rgName);

        AdminRequest request = new AdminRequest(AdminCommand.ACTIVE_GROUP, rg);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, rgName);
    }

    /**
     * Create the replica Catalog group with the given options.
     *
     * @param hostName The host name
     * @param port     The port
     * @param dbPath   The database path
     * @param options  The configure options
     * @throws BaseException If error happens.
     */
    public void createReplicaCataGroup(String hostName, int port, String dbPath,
                                       BSONObject options) {
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_HOST, hostName);
        obj.put(SdbConstants.PMD_OPTION_SVCNAME, Integer.toString(port));
        obj.put(SdbConstants.PMD_OPTION_DBPATH, dbPath);
        if (options != null) {
            for (String key : options.keySet()) {
                if (key.equals(SdbConstants.FIELD_NAME_HOST)
                        || key.equals(SdbConstants.PMD_OPTION_SVCNAME)
                        || key.equals(SdbConstants.PMD_OPTION_DBPATH)) {
                    continue;
                }
                obj.put(key, options.get(key));
            }
        }

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_CATALOG_GROUP, obj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Create the replica Catalog group with the given options.
     *
     * @param hostName  The host name
     * @param port      The port
     * @param dbPath    The database path
     * @param configure The configure options
     * @throws BaseException If error happens.
     * @deprecated Use "void createReplicaCataGroup(String hostName, int port, String dbPath,
     * final BSONObject options)" instead.
     */
    public void createReplicaCataGroup(String hostName, int port,
                                       String dbPath, Map<String, String> configure) {
        BSONObject obj = new BasicBSONObject();
        if (configure != null) {
            for (String key : configure.keySet()) {
                obj.put(key, configure.get(key));
            }
        }
        createReplicaCataGroup(hostName, port, dbPath, obj);
    }

    private String getListCommand(int listType) {
        switch (listType) {
            case SDB_LIST_CONTEXTS:
                return AdminCommand.LIST_CONTEXTS;
            case SDB_LIST_CONTEXTS_CURRENT:
                return AdminCommand.LIST_CONTEXTS_CURRENT;
            case SDB_LIST_SESSIONS:
                return AdminCommand.LIST_SESSIONS;
            case SDB_LIST_SESSIONS_CURRENT:
                return AdminCommand.LIST_SESSIONS_CURRENT;
            case SDB_LIST_COLLECTIONS:
                return AdminCommand.LIST_COLLECTIONS;
            case SDB_LIST_COLLECTIONSPACES:
                return AdminCommand.LIST_COLLECTIONSPACES;
            case SDB_LIST_STORAGEUNITS:
                return AdminCommand.LIST_STORAGEUNITS;
            case SDB_LIST_GROUPS:
                return AdminCommand.LIST_GROUPS;
            case SDB_LIST_STOREPROCEDURES:
                return AdminCommand.LIST_PROCEDURES;
            case SDB_LIST_DOMAINS:
                return AdminCommand.LIST_DOMAINS;
            case SDB_LIST_TASKS:
                return AdminCommand.LIST_TASKS;
            case SDB_LIST_TRANSACTIONS:
                return AdminCommand.LIST_TRANSACTIONS;
            case SDB_LIST_TRANSACTIONS_CURRENT:
                return AdminCommand.LIST_TRANSACTIONS_CURRENT;
            case SDB_LIST_CL_IN_DOMAIN:
                return AdminCommand.LIST_CL_IN_DOMAIN;
            case SDB_LIST_CS_IN_DOMAIN:
                return AdminCommand.LIST_CS_IN_DOMAIN;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG, String.format("Invalid list type: %d", listType));
        }
    }

    String getUserName() {
        return userName;
    }

    String getPassword() {
        return password;
    }

    BSONObject getDetailByName(String name) throws BaseException {
        BSONObject condition = new BasicBSONObject();
        condition.put(SdbConstants.FIELD_NAME_GROUPNAME, name);

        BSONObject result;
        DBCursor cursor = getList(Sequoiadb.SDB_LIST_GROUPS, condition, null, null);
        try {
            if (cursor == null || !cursor.hasNext()) {
                return null;
            }
            result = cursor.getNext();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return result;
    }

    BSONObject getDetailById(int id) throws BaseException {
        BSONObject condition = new BasicBSONObject();
        condition.put(SdbConstants.FIELD_NAME_GROUPID, id);

        BSONObject result;
        DBCursor cursor = getList(Sequoiadb.SDB_LIST_GROUPS, condition, null, null);
        try {
            if (cursor == null || !cursor.hasNext()) {
                return null;
            }
            result = cursor.getNext();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return result;
    }

    private SysInfoResponse receiveSysInfoResponse() {
        SysInfoResponse response = new SysInfoResponse();
        byte[] lengthBytes = connection.receive(response.length());
        ByteBuffer buffer = ByteBuffer.wrap(lengthBytes);
        response.decode(buffer);
        return response;
    }

    private ByteBuffer receiveSdbResponse() {
        byte[] lengthBytes = connection.receive(4);
        int length = ByteBuffer.wrap(lengthBytes).order(byteOrder).getInt();

        byte[] bytes = new byte[length];
        System.arraycopy(lengthBytes, 0, bytes, 0, lengthBytes.length);
        connection.receive(bytes, 4, length - 4);
        ByteBuffer buffer = ByteBuffer.wrap(bytes).order(byteOrder);
        return buffer;
    }

    private void validateResponse(SdbRequest request, SdbResponse response) {
        if ((request.opCode() | MsgOpCode.RESP_MASK) != response.opCode()) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    ("request=" + request.opCode() + " response=" + response.opCode()));
        }
    }

    private ByteBuffer encodeRequest(Request request) {
        ByteBuffer buffer = ByteBuffer.allocate(request.length());
        buffer.order(byteOrder);
        request.setRequestId(getNextRequestId());
        request.encode(buffer);
        return buffer;
    }

    private void sendRequest(Request request) {
        ByteBuffer buffer = encodeRequest(request);
        if (!isClosed()) {
            connection.send(buffer);
        } else {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
    }

    private <T extends SdbResponse> T decodeResponse(ByteBuffer buffer, Class<T> tClass) {
        T response;
        try {
            response = tClass.newInstance();
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
        response.decode(buffer);
        return response;
    }

    private ByteBuffer sendAndReceive(ByteBuffer request) {
        if (!isClosed()) {
            connection.send(request);
            lastUseTime = System.currentTimeMillis();
            return receiveSdbResponse();
        } else {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
    }

    <T extends SdbResponse> T requestAndResponse(SdbRequest request, Class<T> tClass) {
        ByteBuffer out = encodeRequest(request);
        ByteBuffer in = sendAndReceive(out);
        T response = decodeResponse(in, tClass);
        validateResponse(request, response);
        return response;
    }

    SdbReply requestAndResponse(SdbRequest request) {
        return requestAndResponse(request, SdbReply.class);
    }

    private String getErrorDetail(BSONObject errorObj, Object errorMsg) {
        String detail = null;

        if (errorObj != null) {
            String serverDetail = (String) errorObj.get("detail");
            if (serverDetail != null && !serverDetail.isEmpty()) {
                detail = serverDetail;
            }
        }

        if (errorMsg != null) {
            if (detail != null && !detail.isEmpty()) {
                detail += ", " + errorMsg.toString();
            } else {
                detail = errorMsg.toString();
            }
        }

        return detail;
    }

    void throwIfError(CommonResponse response, Object errorMsg) throws BaseException {
        if (response.getFlag() != 0) {
            BSONObject errorObj = response.getErrorObj();
            String detail = getErrorDetail(errorObj, errorMsg);
            if (detail != null && !detail.isEmpty()) {
                throw new BaseException(response.getFlag(), detail);
            } else {
                throw new BaseException(response.getFlag());
            }
        }
    }

    void throwIfError(CommonResponse response) throws BaseException {
        if (response.getFlag() != 0) {
            BSONObject errorObj = response.getErrorObj();
            String detail = getErrorDetail(errorObj, null);
            if (detail != null && !detail.isEmpty()) {
                throw new BaseException(response.getFlag(), detail);
            } else {
                throw new BaseException(response.getFlag());
            }
        }
    }

    private ByteOrder getSysInfo() {
        SysInfoRequest request = new SysInfoRequest();
        sendRequest(request);

        SysInfoResponse response = receiveSysInfoResponse();

        return response.byteOrder();
    }

    private void killContext() {
        if (isClosed()) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }

        long[] contextIds = new long[]{-1};
        KillContextRequest request = new KillContextRequest(contextIds);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Close the connection.
     *
     * @throws BaseException If error happens.
     */
    @Override
    public void close() throws BaseException {
        if (isClosed()) {
            return;
        }
        try {
            releaseResource();
            DisconnectRequest request = new DisconnectRequest();
            sendRequest(request);
        } finally {
            connection.close();
        }
    }

    /**
     * Class for executing stored procedure result.
     */
    public static class SptEvalResult {
        private SptReturnType returnType;
        private BSONObject errmsg;
        private DBCursor cursor;

        public SptEvalResult() {
            returnType = null;
            errmsg = null;
            cursor = null;
        }

        /**
         * Set return type.
         */
        public void setReturnType(SptReturnType returnType) {
            this.returnType = returnType;
        }

        /**
         * Get return type.
         */
        public SptReturnType getReturnType() {
            return returnType;
        }

        /**
         * Set error type.
         */
        public void setErrMsg(BSONObject errmsg) {
            this.errmsg = errmsg;
        }

        /**
         * Get error type.
         */
        public BSONObject getErrMsg() {
            return errmsg;
        }

        /**
         * Set result cursor.
         */
        public void setCursor(DBCursor cursor) {
            if (this.cursor != null) {
                this.cursor.close();
            }
            this.cursor = cursor;
        }

        /**
         * Get result cursor.
         */
        public DBCursor getCursor() {
            return cursor;
        }
    }

    public enum SptReturnType {
        TYPE_VOID(0),
        TYPE_STR(1),
        TYPE_NUMBER(2),
        TYPE_OBJ(3),
        TYPE_BOOL(4),
        TYPE_RECORDSET(5),
        TYPE_CS(6),
        TYPE_CL(7),
        TYPE_RG(8),
        TYPE_RN(9);

        private int typeValue;

        SptReturnType(int typeValue) {
            this.typeValue = typeValue;
        }

        public int getTypeValue() {
            return typeValue;
        }

        public static SptReturnType getTypeByValue(int typeValue) {
            SptReturnType retType = null;
            for (SptReturnType rt : values()) {
                if (rt.getTypeValue() == typeValue) {
                    retType = rt;
                    break;
                }
            }
            return retType;
        }
    }
}
