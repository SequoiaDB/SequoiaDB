/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.base;

import java.io.Closeable;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.Arrays;

import com.sequoiadb.message.*;
import com.sequoiadb.message.request.*;
import com.sequoiadb.message.response.*;
import com.sequoiadb.util.AuthAlgorithmSHA256;
import com.sequoiadb.util.Helper;
import com.sequoiadb.util.SdbSecureUtil;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.Code;
import org.bson.util.JSON;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.net.IConnection;
import com.sequoiadb.net.ServerAddress;
import com.sequoiadb.net.TCPConnection;
import com.sequoiadb.net.ConnectionProxy;

/**
 * The connection with SequoiaDB server.
 */
public class Sequoiadb implements Closeable {
    private InetSocketAddress socketAddress;
    private IConnection connection;
    private ConnectionProxy connProxy;
    private String userName;
    private String password;
    private ByteOrder byteOrder = ByteOrder.BIG_ENDIAN;
    private long requestId;
    private long lastUseTime;
    private int currentCacheSize = 0;
    private boolean isOldVersionLobServer = false;

    // cache cs/cl name
    private Map<String, Long> nameCache = new HashMap<String, Long>();
    private static ClientOptions globalClientConf = new ClientOptions();

    private BSONObject attributeCache = null;

    private final static String DEFAULT_HOST = "127.0.0.1";
    private final static int DEFAULT_PORT = 11810;

    private final static int DEFAULT_BUFF_LENGTH = 512;
    private ByteBuffer requestBuffer = null;
    private SdbProtocolVersion protocolVersion = SdbProtocolVersion.SDB_PROTOCOL_VERSION_INVALID;
    private int closeAllCursorMark = 0;
    private SdbAuthVersion authVersion = SdbAuthVersion.SDB_AUTH_MD5;

    private final int MAX_USERNAME_LENGTH = 256;
    private final int MAX_PASSWORD_LENGTH = 256;

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

    /**
     * List of all the contexts of all the sessions
     */
    public final static int SDB_LIST_CONTEXTS = 0;
    /**
     * List of the contexts of current session
     */
    public final static int SDB_LIST_CONTEXTS_CURRENT = 1;
    /**
     * List of all the sessions
     */
    public final static int SDB_LIST_SESSIONS = 2;
    /**
     * List of current session
     */
    public final static int SDB_LIST_SESSIONS_CURRENT = 3;
    /**
     * List of collections
     */
    public final static int SDB_LIST_COLLECTIONS = 4;
    /**
     * List of collection spaces
     */
    public final static int SDB_LIST_COLLECTIONSPACES = 5;
    /**
     * List of strorage units
     */
    public final static int SDB_LIST_STORAGEUNITS = 6;
    /**
     * List of all the groups
     */
    public final static int SDB_LIST_GROUPS = 7;
    /**
     * List of store procedures
     */
    public final static int SDB_LIST_STOREPROCEDURES = 8;
    /**
     * List of domains
     */
    public final static int SDB_LIST_DOMAINS = 9;
    /**
     * List of tasks
     */
    public final static int SDB_LIST_TASKS = 10;
    /**
     * List of all the transactions of all the sessions
     */
    public final static int SDB_LIST_TRANSACTIONS = 11;
    /**
     * List of all transactions of current session
     */
    public final static int SDB_LIST_TRANSACTIONS_CURRENT = 12;
    /**
     * List of service tasks
     */
    public final static int SDB_LIST_SVCTASKS = 14;
    /**
     * List of sequences
     */
    public final static int SDB_LIST_SEQUENCES = 15;
    /**
     * List of users
     */
    public final static int SDB_LIST_USERS = 16;
    /**
     * List of backups
     */
    public final static int SDB_LIST_BACKUPS = 17 ;
    //public final static int SDB_LIST_RESERVED1 = 18 ;
    //public final static int SDB_LIST_RESERVED2 = 19 ;
    //public final static int SDB_LIST_RESERVED3 = 20 ;
    //public final static int SDB_LIST_RESERVED4 = 21 ;
    /**
     * List of data source
     */
    public final static int SDB_LIST_DATASOURCES = 22;
    //public final static int SDB_LIST_RESERVED7 = 24 ;
    /**
     * List of recycle bin
     */
    public final static int SDB_LIST_RECYCLEBIN = 27;
    /**
     * list group mode
     */
    public final static int SDB_LIST_GROUPMODES = 28;
    // reserved
    public final static int SDB_LIST_CL_IN_DOMAIN = 129;
    // reserved
    public final static int SDB_LIST_CS_IN_DOMAIN = 130;

    /**
     * Snapshot of all the contexts of all the sessions
     */
    public final static int SDB_SNAP_CONTEXTS = 0;
    /**
     * Snapshot of the contexts of current session
     */
    public final static int SDB_SNAP_CONTEXTS_CURRENT = 1;
    /**
     * Snapshot of all the sessions
     */
    public final static int SDB_SNAP_SESSIONS = 2;
    /**
     * Snapshot of current session
     */
    public final static int SDB_SNAP_SESSIONS_CURRENT = 3;
    /**
     * Snapshot of collections
     */
    public final static int SDB_SNAP_COLLECTIONS = 4;
    /**
     * Snapshot of collection spaces
     */
    public final static int SDB_SNAP_COLLECTIONSPACES = 5;
    /**
     * Snapshot of database
     */
    public final static int SDB_SNAP_DATABASE = 6;
    /**
     * Snapshot of system
     */
    public final static int SDB_SNAP_SYSTEM = 7;
    /**
     * Snapshot of catalog
     */
    public final static int SDB_SNAP_CATALOG = 8;
    /**
     * Snapshot of all the transactions of all the sessions
     */
    public final static int SDB_SNAP_TRANSACTIONS = 9;
    /**
     * Snapshot of all transactions of current session
     */
    public final static int SDB_SNAP_TRANSACTIONS_CURRENT = 10;
    /**
     * Snapshot of access plans
     */
    public final static int SDB_SNAP_ACCESSPLANS = 11;
    /**
     * Snapshot of health
     */
    public final static int SDB_SNAP_HEALTH = 12;
    /**
     * Snapshot of configs
     */
    public final static int SDB_SNAP_CONFIGS = 13;
    /**
     * Snapshot of service tasks
     */
    public final static int SDB_SNAP_SVCTASKS = 14;
    /**
     * Snapshot of sequences
     */
    public final static int SDB_SNAP_SEQUENCES = 15;
    //public final static int SDB_SNAP_RESERVED1 = 16;
    //public final static int SDB_SNAP_RESERVED2 = 17;
    /**
     * Snapshot of queries
     */
    public final static int SDB_SNAP_QUERIES = 18;
    /**
     * Snapshot of latch waits
     */
    public final static int SDB_SNAP_LATCHWAITS = 19;
    /**
     * Snapshot of lock waits
     */
    public final static int SDB_SNAP_LOCKWAITS = 20;
    /**
     * Snapshot of index statistics
     */
    public final static int SDB_SNAP_INDEXSTATS = 21;
    //public final static int SDB_SNAP_RESERVED3 = 22;
    /**
     * Snapshot of tasks
     */
    public final static int SDB_SNAP_TASKS = 23;
    /**
     * Snapshot of indexes
     */
    public final static int SDB_SNAP_INDEXES = 24;
    /**
     * Snapshot of transaction waits
     */
    public final static int SDB_SNAP_TRANSWAITS = 25;
    /**
     * Snapshot of transaction deadlock
     */
    public final static int SDB_SNAP_TRANSDEADLOCK = 26;
    /**
     * Snapshot of recycle bin
     */
    public final static int SDB_SNAP_RECYCLEBIN = 27;

    public final static int FMP_FUNC_TYPE_INVALID = -1;
    public final static int FMP_FUNC_TYPE_JS = 0;
    public final static int FMP_FUNC_TYPE_C = 1;
    public final static int FMP_FUNC_TYPE_JAVA = 2;

    public final static String CATALOG_GROUP_NAME = "SYSCatalogGroup";

    void upsertCache(String name) {
        if (name == null) {
            return;
        }
        if (globalClientConf.getEnableCache()) {
            long current = System.currentTimeMillis();
            nameCache.put(name, current);
            String[] arr = name.split("\\.");
            if (arr.length > 1) {
                // extract cs name from cl full name and that
                // upsert cs name
                nameCache.put(arr[0], current);
            }
        }
    }

    void setIsOldVersionLobServer(boolean isOldVersionLobServer) {
        this.isOldVersionLobServer = isOldVersionLobServer;
    }

    boolean getIsOldVersionLobServer() {
        return isOldVersionLobServer;
    }

    void removeCache(String name) {
        if (name == null) {
            return;
        }
        String[] arr = name.split("\\.");
        if (arr.length == 1) {
            // when we come here, "name" is a cs name, so
            // we are going to remove the cache of the cs
            // and the cache of the cls

            // remove cs cache
            // name may be "foo.", it's a invalid name,
            // we don't want to remove anything,
            // so we use "name" but not "arr[0]" here
            nameCache.remove(name);
            Set<String> keySet = nameCache.keySet();
            List<String> list = new ArrayList<String>();
            for (String str : keySet) {
                String[] nameArr = str.split("\\.");
                if (nameArr.length > 1 && nameArr[0].equals(name)) {
                    list.add(str);
                }
            }
            if (list.size() != 0) {
                for (String str : list) {
                    nameCache.remove(str);
                }
            }
        } else {
            // we are going to remove the cache of the cl
            nameCache.remove(name);
        }
    }

    boolean fetchCache(String name) {
        if (globalClientConf.getEnableCache()) {
            if (nameCache.containsKey(name)) {
                long lastUpdatedTime = nameCache.get(name);
                if ((System.currentTimeMillis() - lastUpdatedTime) >= globalClientConf.getCacheInterval()) {
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
     * Initialize the global configuration of SequoiaDB driver.
     *
     * @param options The global configuration of SequoiaDB driver
     */
    public static void initClient(ClientOptions options) {
        globalClientConf = options != null ? options : new ClientOptions();
        BSON.setExactlyDate( globalClientConf.getExactlyDate() );
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
     * @return IP address of SequoiaDB server.
     */
    public String getIP() {
        return socketAddress.getAddress().getHostAddress();
    }

    /**
     * @return Service port of SequoiaDB server.
     */
    public int getPort() {
        return socketAddress.getPort();
    }

    /**
     * @return the node name of current coord node in format of "ip:port".
     */
    public String getNodeName() {
        return getIP() + ":" + getPort();
    }

    /**
     * @return the socket address of remote host.
     */
    public String getRemoteAddress() {
        if (connection == null) {
            return null;
        }
        return connection.getRemoteAddress();
    }

    /**
     * @return the socket address of localhost.
     */
    public String getLocalAddress() {
        if (connection == null) {
            return null;
        }
        return connection.getLocalAddress();
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
     * Reserve.
     */
    public int getCurrentCacheSize() {
        return currentCacheSize;
    }

    /**
     * Get a builder to create Sequoiadb instance.
     *
     * @return A builder of Sequoiadb
     */
    public static Builder builder() {
        return new Builder();
    }

    private Sequoiadb( Builder builder ) {
        init( builder.addressList, builder.userConfig.getUserName(),
                builder.userConfig.getPassword(), builder.configOptions );
    }

    /**
     * Use server address "127.0.0.1:11810".
     *
     * @param username the user's name of the account
     * @param password the password of the account
     * @throws BaseException SDB_NETWORK means network error, SDB_INVALIDARG means wrong address or the
     *                       address don't map to the hosts table.
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
     * @throws BaseException SDB_NETWORK means network error, SDB_INVALIDARG means wrong address or the
     *                       address don't map to the hosts table
     */
    public Sequoiadb(String connString, String username, String password) throws BaseException {
        this(connString, username, password, (ConfigOptions) null);
    }

    /**
     * @param connString remote server address "Host:Port"
     * @param username   the user's name of the account
     * @param password   the password of the account
     * @param options    the options for connection
     * @throws BaseException SDB_NETWORK means network error, SDB_INVALIDARG means wrong address or the
     *                       address don't map to the hosts table.
     */
    public Sequoiadb(String connString, String username, String password, ConfigOptions options)
            throws BaseException {
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
        init( connStrings, username, password, options );
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
     * @throws BaseException SDB_NETWORK means network error, SDB_INVALIDARG means wrong address or the
     *                       address don't map to the hosts table.
     */
    public Sequoiadb(String host, int port, String username, String password) throws BaseException {
        this(host, port, username, password, null);
    }

    /**
     * @param host     the address of coord
     * @param port     the port of coord
     * @param username the user's name of the account
     * @param password the password of the account
     * @throws BaseException SDB_NETWORK means network error, SDB_INVALIDARG means wrong address or the
     *                       address don't map to the hosts table.
     */
    public Sequoiadb(String host, int port, String username, String password, ConfigOptions options)
            throws BaseException {
        init(host, port, username, password, options);
    }

    /**
     * @deprecated Use com.sequoiadb.base.ConfigOptions instead of com.sequoiadb.net.ConfigOptions.
     */
    @Deprecated
    public Sequoiadb(String host, int port, String username, String password,
                     com.sequoiadb.net.ConfigOptions options) throws BaseException {
        this(host, port, username, password, (ConfigOptions) options);
    }

    private void init(String host, int port, String username, String password,
                      ConfigOptions options) throws BaseException {
        if (host == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "host is null");
        }
        if (username == null && password == null) {
            username = "";
            password = "";
        }
        if (username == null || password == null) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "User name or password is null" );
        }

        if (options == null) {
            options = new ConfigOptions();
        }

        socketAddress = new InetSocketAddress(host, port);
        connection = new TCPConnection(socketAddress, options);
        connection.connect();

        connProxy = new ConnectionProxy(connection);

        SysInfoResponse sysInfoResponse = getSysInfo();
        byteOrder = sysInfoResponse.byteOrder();
        protocolVersion = sysInfoResponse.getPeerProtocolVersion();
        authVersion = sysInfoResponse.getAuthVersion();

        authenticate(username, password, authVersion);
        this.userName = username;
        this.password = password;
    }

    private void init(String connString, String username, String password, ConfigOptions options) {
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

    private void init(List<String> addressList, String username, String password, ConfigOptions options) {
        if (addressList == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Server address list is null");
        }

        List<String> list = new ArrayList<String>();
        for (String str : addressList) {
            if (str != null && !str.isEmpty()) {
                list.add(str);
            }
        }

        if (list.size() == 0) {
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

    private void authenticate(String userName, String password, SdbAuthVersion authVersion) {
        if (authVersion == SdbAuthVersion.SDB_AUTH_SCRAM_SHA256) {
            try {
                authenticateSHA256(userName, password);
            } catch (BaseException e) {
                // During rolling upgrade, the old version of catalog node
                // does not support the SAH256 algorithm.
                if (e.getErrorCode() == SDBError.SDB_UNKNOWN_MESSAGE.getErrorCode()) {
                    authenticateMD5(userName, password);
                } else {
                    close();
                    throw e;
                }
            }
        } else {
            authenticateMD5(userName, password);
        }
    }

    private void authenticateMD5(String userName, String password) {
        AuthRequest request = new AuthVerifyMD5Request(userName, password);
        SdbReply response = requestAndResponse(request);
        if (response.getFlag() != 0) {
            close();
        }
        throwIfError(response, "Failed to authenticate " + userName);
    }

    private void authenticateSHA256(String userName, String password) {
        AuthAlgorithmSHA256 sha256 = new AuthAlgorithmSHA256();
        String md5Pwd = Helper.md5(password);

        // 1. Get salt, CombineNonce, IterCount from engine
        AuthRequest request1 = AuthVerifySHA256Request.step1(userName, sha256.generateClientNonce());
        AuthVerifySHA256Response response1 = requestAndResponse(request1, AuthVerifySHA256Response.class);
        throwIfError(response1, "Failed to authenticate " + userName);
        AuthVerifySHA256Response.Step1Data data1 = response1.getStep1Data();

        if (data1 == null) {
            // When "auth" is false in catalog node configure file sdb.conf, or
            // there is no user in SYSAUTH.SYSUSRS, the result is empty. So we
            // don't need to authenticate.
            return;
        }

        // 2. Calculate proof
        AuthAlgorithmSHA256.AuthProof authProof = sha256.calculateProof(userName,
                md5Pwd,
                data1.getCombineNonceBase64(),
                data1.getSaltBase64(),
                data1.getIterCount());

        String clientProofBase64 = Helper.Base64Encode(authProof.getClientProof());

        // 3. Get server proof from engine
        AuthRequest request2 = AuthVerifySHA256Request.step2(userName, data1.getCombineNonceBase64(),
                clientProofBase64);
        AuthVerifySHA256Response response2 = requestAndResponse(request2, AuthVerifySHA256Response.class);
        throwIfError(response2, "Failed to authenticate " + userName);
        AuthVerifySHA256Response.Step2Data data2 = response2.getStep2Data();

        // 4. Verify server proof
        byte[] actualServerProof = Helper.Base64Decode(data2.getServerProofBase64());
        byte[] expectServerProof = authProof.getServerProof();
        if (!Arrays.equals(actualServerProof, expectServerProof)) {
            throw new BaseException(SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN);
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
        createUser(username, password, null);
    }

    /**
     * Create an user in current database.
     *
     * @param username The connection user name
     * @param password The connection password
     * @param options  The options for user
     *                 <ul>
     *                 <li>AuditMask: User audit log mask, value list:
     *                             ACCESS,CLUSTER,SYSTEM,DML,DDL,DCL,DQL,INSERT,DELETE,UPDATE,OTHER.
     *                             You can combine multiple values with '|'. 'ALL' means that all mask items
     *                             are turned on, and 'NONE' means that no mask items are turned on.
     *                             If an item in the user audit log is not configured, the configuration of the
     *                             corresponding mask item on the node is inherited. You can also use '!' to
     *                             disable inheritance of this mask( e.g. "!DDL|DML" ).
     *                   <li>Roles: Customize user role array.
     *                   </ul>
     * @throws BaseException If error happens.
     */
    public void createUser(String username, String password, BSONObject options) throws BaseException {
        if (username == null || username.length() == 0 || password == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        if (username.length() > MAX_USERNAME_LENGTH || password.length() > MAX_PASSWORD_LENGTH) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Exceeds username or password maximum length");
        }
        AuthRequest request = new CreateUserRequest(username, password, authVersion, options);
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
        if (username == null || username.length() == 0 || password == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        AuthRequest request = new RemoveUserRequest(username, password);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, username);
    }

    /**
     * Get information for a specific user
     *
     * @param username The username of the user.
     * @param options  Additional options for outputting user information.
     *                 <ul>
     *                 <li>ShowPrivileges(Boolean): Whether to display the privileges of the user in the output result.
     *                 </ul>
     * @return User information.
     * @throws BaseException If error happens
     */
    public BSONObject getUser(String username, BSONObject options) throws BaseException {
        if (username == null || username.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Username can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_USER, username);
        if (options != null) {
            matcher.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.GET_USER, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);

        try (DBCursor cursor = new DBCursor(response, this)) {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_AUTH_ROLE_NOT_EXIST);
            }
            return cursor.getNext();
        }
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
        // let the receive buffer shrink to default value
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
        // client not connect to database or client
        // disconnect from database
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
     * @deprecated Create a new Sequoiadb instance instead.
     */
    @Deprecated
    public void changeConnectionOptions(ConfigOptions options) throws BaseException {
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
    public CollectionSpace createCollectionSpace(String csName) throws BaseException {
        return createCollectionSpace(csName, SDB_PAGESIZE_DEFAULT);
    }

    /**
     * Create collection space.
     *
     * @param csName   The name of collection space
     * @param pageSize The Page Size as below:
     *                 <ul>
     *                 <li>{@link Sequoiadb#SDB_PAGESIZE_4K}
     *                 <li>{@link Sequoiadb#SDB_PAGESIZE_8K}
     *                 <li>{@link Sequoiadb#SDB_PAGESIZE_16K}
     *                 <li>{@link Sequoiadb#SDB_PAGESIZE_32K}
     *                 <li>{@link Sequoiadb#SDB_PAGESIZE_64K}
     *                 <li>{@link Sequoiadb#SDB_PAGESIZE_DEFAULT}
     *                 </ul>
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public CollectionSpace createCollectionSpace(String csName, int pageSize) throws BaseException {
        BSONObject options = new BasicBSONObject();
        options.put("PageSize", pageSize);
        return createCollectionSpace(csName, options);
    }

    /**
     * Create collection space.
     *
     * @param csName  The name of collection space
     * @param options Contains configuration information for create collection space. The options are as
     *                below:
     *                <ul>
     *                <li>PageSize(int) : Assign how large the page size is for the collection created in
     *                this collection space, default to be 64K
     *                <li>Domain(String) : Assign which domain does current collection space belong to, it will
     *                belongs to the system domain if not assign this option
     *                <li>LobPageSize(int) : The Lob data page size, default value is 262144
     *                and the unit is byte
     *                <li>DataSource(String) : Assign which data source does current collection space belong to
     *                <li>Mapping(String) : The name of the collection space mapped by the current collection space
     *                </ul>
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public CollectionSpace createCollectionSpace(String csName, BSONObject options)
            throws BaseException {
        if (csName == null || csName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
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
        dropCollectionSpace(csName, null);
    }

    /**
     * Remove the named collection space.
     *
     * @param csName The collection space name
     * @param options Contains configuration information for drop collection space. The options are as
     *                below:
     *                <ul>
     *                <li>EnsureEmpty(boolean) : check whether the collection space is empty when drop,
     *                false means drop directly, true means only empty can drop, default value is false
     *                <li>SkipRecycleBin(boolean) : Indicates whether to skip recycle bin, default is false.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void dropCollectionSpace(String csName, BSONObject options) throws BaseException {
        if (csName == null || csName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                                    "cs name can not be null or empty");
        }

        BSONObject innerOptions = new BasicBSONObject();
        innerOptions.put(SdbConstants.FIELD_NAME_NAME, csName);
        if (options != null) {
            innerOptions.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.DROP_CS, innerOptions);
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
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "The old name of collection space is null or empty");
        }
        if (newName == null || newName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "The new name of collection space is null or empty");
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
     *                <li>Deep:int Flush with deep mode or not. 1 in default. 0 for non-deep mode,1 for
     *                deep mode,-1 means use the configuration with server</li>
     *                <li>Block:boolean Flush with block mode or not. false in default.</li>
     *                <li>CollectionSpace:String Specify the collectionspace to sync. If not set, will
     *                sync all the collection spaces and logs, otherwise, will only sync the collection
     *                space specified.</li>
     *                <li>Some of other options are as below:(only take effect in coordinate nodes,
     *                please visit the official website to search "sync" or "Location Elements" for more
     *                detail.) GroupID:int, GroupName:String, NodeID:int, HostName:String,
     *                svcname:String ...</li>
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
     *                <li>CollectionSpace: (String) Specify the collection space to be analyzed.</li>
     *                <li>Collection: (String) Specify the collection to be analyzed.</li>
     *                <li>Index: (String) Specify the index to be analyzed.</li>
     *                <li>Mode: (Int32) Specify the analyze mode (default is 1):
     *                <ul>
     *                <li>Mode 1 will analyze with data samples.</li>
     *                <li>Mode 2 will analyze with full data.</li>
     *                <li>Mode 3 will generate default statistics.</li>
     *                <li>Mode 4 will reload statistics into memory cache.</li>
     *                <li>Mode 5 will clear statistics from memory cache.</li>
     *                </ul>
     *                </li>
     *                <li>Other options: Some of other options are as below:(only take effect in
     *                coordinate nodes, please visit the official website to search "analyze" or
     *                "Location Elements" for more detail.) GroupID:int, GroupName:String, NodeID:int,
     *                HostName:String, svcname:String, ...</li>
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
     * Get the named collection space. If the collection space not exit, throw BaseException with
     * errcode SDB_DMS_CS_NOTEXIST.
     *
     * @param csName The collection space name.
     * @return the object of the specified collection space, or an exception when the collection
     * space does not exist.
     * @throws BaseException If error happens.
     */
    public CollectionSpace getCollectionSpace(String csName) throws BaseException {
        if (csName == null || csName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "cs name can not be null or empty");
        }
        // get cs object from cache
        if (fetchCache(csName)) {
            return new CollectionSpace(this, csName);
        }

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_NAME, csName);

        AdminRequest request = new AdminRequest(AdminCommand.TEST_CS, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, csName);
        upsertCache(csName);
        return new CollectionSpace(this, csName);
    }

    /**
     * Verify the existence of collection space.
     *
     * @param csName The collection space name.
     * @return True if existed or false if not existed.
     * @throws BaseException If error happens.
     */
    public boolean isCollectionSpaceExist(String csName) throws BaseException {
        if (csName == null || csName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "cs name can not be null or empty");
        }

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
     *                <li>Type: (String) Specify the snapshot type to be reset (default is "all"):
     *                <ul>
     *                <li>"sessions"</li>
     *                <li>"sessions current"</li>
     *                <li>"database"</li>
     *                <li>"health"</li>
     *                <li>"all"</li>
     *                </ul>
     *                </li>
     *                <li>SessionID: (Int32) Specify the session ID to be reset.</li>
     *                <li>Other options: Some of other options are as below:(please visit the official
     *                website to search "Location Elements" for more detail.)
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
     * @param listType   The list type as below:
     *                   <ul>
     *                   <li>{@link Sequoiadb#SDB_LIST_CONTEXTS}
     *                   <li>{@link Sequoiadb#SDB_LIST_CONTEXTS_CURRENT}
     *                   <li>{@link Sequoiadb#SDB_LIST_SESSIONS}
     *                   <li>{@link Sequoiadb#SDB_LIST_SESSIONS_CURRENT}
     *                   <li>{@link Sequoiadb#SDB_LIST_COLLECTIONS}
     *                   <li>{@link Sequoiadb#SDB_LIST_COLLECTIONSPACES}
     *                   <li>{@link Sequoiadb#SDB_LIST_STORAGEUNITS}
     *                   <li>{@link Sequoiadb#SDB_LIST_GROUPS}
     *                   <li>{@link Sequoiadb#SDB_LIST_STOREPROCEDURES}
     *                   <li>{@link Sequoiadb#SDB_LIST_DOMAINS}
     *                   <li>{@link Sequoiadb#SDB_LIST_TASKS}
     *                   <li>{@link Sequoiadb#SDB_LIST_TRANSACTIONS}
     *                   <li>{@link Sequoiadb#SDB_LIST_TRANSACTIONS_CURRENT}
     *                   <li>{@link Sequoiadb#SDB_LIST_SVCTASKS}
     *                   <li>{@link Sequoiadb#SDB_LIST_SEQUENCES}
     *                   <li>{@link Sequoiadb#SDB_LIST_USERS}
     *                   <li>{@link Sequoiadb#SDB_LIST_BACKUPS}
     *                   <li>{@link Sequoiadb#SDB_LIST_DATASOURCES}
     *                   <li>{@link Sequoiadb#SDB_LIST_RECYCLEBIN}
     *                   <li>{@link Sequoiadb#SDB_LIST_GROUPMODES}
     *                   </ul>
     * @param query      The matching rule, match all the documents if null.
     * @param selector   The selective rule, return the whole document if null.
     * @param orderBy    The ordered rule, never sort if null.
     * @param hint       The options provided for specific list type. Reserved.
     * @param skipRows   Skip the first skipRows documents.
     * @param returnRows Only return returnRows documents. -1 means return all matched results.
     * @return The target information by cursor.
     * @throws BaseException If error happens.
     */
    public DBCursor getList(int listType, BSONObject query, BSONObject selector, BSONObject orderBy,
                            BSONObject hint, long skipRows, long returnRows) throws BaseException {
        String command = getListCommand(listType);
        int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
        flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
        AdminRequest request = new AdminRequest(command, query, selector, orderBy, hint, skipRows,
                returnRows, flag);
        SdbReply response = requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0 && flags != SDBError.SDB_DMS_EOC.getErrorCode()) {
            String msg = "query = " + query + ", selector = " + selector + ", orderBy = " + orderBy;
            throwIfError(response, msg);
        }

        return new DBCursor(response, this);
    }

    /**
     * Get the information of specified type.
     *
     * @param listType   The list type as below:
     *                   <ul>
     *                   <li>{@link Sequoiadb#SDB_LIST_CONTEXTS}
     *                   <li>{@link Sequoiadb#SDB_LIST_CONTEXTS_CURRENT}
     *                   <li>{@link Sequoiadb#SDB_LIST_SESSIONS}
     *                   <li>{@link Sequoiadb#SDB_LIST_SESSIONS_CURRENT}
     *                   <li>{@link Sequoiadb#SDB_LIST_COLLECTIONS}
     *                   <li>{@link Sequoiadb#SDB_LIST_COLLECTIONSPACES}
     *                   <li>{@link Sequoiadb#SDB_LIST_STORAGEUNITS}
     *                   <li>{@link Sequoiadb#SDB_LIST_GROUPS}
     *                   <li>{@link Sequoiadb#SDB_LIST_STOREPROCEDURES}
     *                   <li>{@link Sequoiadb#SDB_LIST_DOMAINS}
     *                   <li>{@link Sequoiadb#SDB_LIST_TASKS}
     *                   <li>{@link Sequoiadb#SDB_LIST_TRANSACTIONS}
     *                   <li>{@link Sequoiadb#SDB_LIST_TRANSACTIONS_CURRENT}
     *                   <li>{@link Sequoiadb#SDB_LIST_SVCTASKS}
     *                   <li>{@link Sequoiadb#SDB_LIST_SEQUENCES}
     *                   <li>{@link Sequoiadb#SDB_LIST_USERS}
     *                   <li>{@link Sequoiadb#SDB_LIST_BACKUPS}
     *                   <li>{@link Sequoiadb#SDB_LIST_DATASOURCES}
     *                   <li>{@link Sequoiadb#SDB_LIST_RECYCLEBIN}
     *                   <li>{@link Sequoiadb#SDB_LIST_GROUPMODES}
     *                   </ul>
     * @param query    The matching rule, match all the documents if null.
     * @param selector The selective rule, return the whole document if null.
     * @param orderBy  The ordered rule, never sort if null.
     * @return The target information by cursor.
     * @throws BaseException If error happens.
     */
    public DBCursor getList(int listType, BSONObject query, BSONObject selector, BSONObject orderBy)
            throws BaseException {
        return getList(listType, query, selector, orderBy, null, 0, -1);
    }

    /**
     * Flush the options to configuration file.
     *
     * @param options The param of flush, pass {"Global":true} or {"Global":false} In cluster
     *                environment, passing {"Global":true} will flush data's and catalog's configuration
     *                file, while passing {"Global":false} will flush coord's configuration file In
     *                stand-alone environment, both them have the same behaviour
     * @throws BaseException If error happens.
     */
    public void flushConfigure(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.EXPORT_CONFIG, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Force the node to update configs online.
     *
     * @param configs The specific configuration parameters to update
     * @param options Options The control options:(Only take effect in coordinate nodes) GroupID:INT32,
     *                GroupName:String, NodeID:INT32, HostName:String, svcname:String, ...
     * @throws BaseException If error happens.
     */
    public void updateConfig(BSONObject configs, BSONObject options) throws BaseException {
        if (options == null) {
            options = new BasicBSONObject();
        }
        BSONObject newObj = new BasicBSONObject();
        newObj.putAll(options);
        newObj.put("Configs", configs);

        AdminRequest request = new AdminRequest(AdminCommand.UPDATE_CONFIG, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Force the node to update configs online.
     *
     * @param configs The specific configuration parameters to update
     * @throws BaseException If error happens.
     */
    public void updateConfig(BSONObject configs) throws BaseException {
        updateConfig(configs, new BasicBSONObject());
    }

    /**
     * Force the node to delete configs online.
     *
     * @param configs The specific configuration parameters to delete
     * @param options Options The control options:(Only take effect in coordinate nodes) GroupID:INT32,
     *                GroupName:String, NodeID:INT32, HostName:String, svcname:String, ...
     * @throws BaseException If error happens.
     */
    public void deleteConfig(BSONObject configs, BSONObject options) throws BaseException {
        BSONObject newObj = new BasicBSONObject();
        newObj.putAll(options);
        newObj.put("Configs", configs);

        AdminRequest request = new AdminRequest(AdminCommand.DELETE_CONFIG, newObj);
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
        if (sql == null || sql.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "sql can not be null or empty");
        }
        SQLRequest request = new SQLRequest(sql);
        SdbReply response = requestAndResponse(request);
        String securityInfo = SdbSecureUtil.toSecurityStr(sql, getInfoEncryption());
        throwIfError(response, "sql: " + securityInfo);
    }

    /**
     * Execute sql in database.
     *
     * @param sql the SQL command
     * @return the DBCursor of the result
     * @throws BaseException If error happens.
     */
    public DBCursor exec(String sql) throws BaseException {
        if (sql == null || sql.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "sql can not be null or empty");
        }
        SQLRequest request = new SQLRequest(sql);
        SdbReply response = requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String securityInfo = SdbSecureUtil.toSecurityStr(sql, getInfoEncryption());
                throwIfError(response, "sql: " + securityInfo);
            }
        }

        return new DBCursor(response, this);
    }

    /**
     * Get snapshot of the database.
     *
     * @param snapType The snapshot type as below:
     *                  <ul>
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONTEXTS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONTEXTS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SESSIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SESSIONS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_COLLECTIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_COLLECTIONSPACES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_DATABASE}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SYSTEM}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CATALOG}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSACTIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSACTIONS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_ACCESSPLANS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_HEALTH}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONFIGS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SVCTASKS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SEQUENCES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_QUERIES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_LATCHWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_LOCKWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_INDEXSTATS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TASKS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_INDEXES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSDEADLOCK}
     *                  <li>{@link Sequoiadb#SDB_SNAP_RECYCLEBIN}
     *                  </ul>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @return the DBCursor of the result
     * @throws BaseException If error happens.
     * @deprecated Use {@link Sequoiadb#getSnapshot(int, BSONObject, BSONObject, BSONObject)} instead.
     */
    @Deprecated
    public DBCursor getSnapshot(int snapType, String matcher, String selector, String orderBy)
            throws BaseException {
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
     * @param snapType The snapshot type as below:
     *                  <ul>
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONTEXTS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONTEXTS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SESSIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SESSIONS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_COLLECTIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_COLLECTIONSPACES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_DATABASE}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SYSTEM}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CATALOG}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSACTIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSACTIONS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_ACCESSPLANS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_HEALTH}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONFIGS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SVCTASKS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SEQUENCES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_QUERIES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_LATCHWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_LOCKWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_INDEXSTATS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TASKS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_INDEXES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSDEADLOCK}
     *                  <li>{@link Sequoiadb#SDB_SNAP_RECYCLEBIN}
     *                  </ul>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @return the DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor getSnapshot(int snapType, BSONObject matcher, BSONObject selector,
                                BSONObject orderBy) throws BaseException {
        return getSnapshot(snapType, matcher, selector, orderBy, null, 0, -1);
    }

    /**
     * Get snapshot of the database.
     *
     * @param snapType The snapshot type as below:
     *                  <ul>
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONTEXTS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONTEXTS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SESSIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SESSIONS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_COLLECTIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_COLLECTIONSPACES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_DATABASE}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SYSTEM}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CATALOG}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSACTIONS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSACTIONS_CURRENT}
     *                  <li>{@link Sequoiadb#SDB_SNAP_ACCESSPLANS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_HEALTH}
     *                  <li>{@link Sequoiadb#SDB_SNAP_CONFIGS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SVCTASKS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_SEQUENCES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_QUERIES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_LATCHWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_LOCKWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_INDEXSTATS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TASKS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_INDEXES}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSWAITS}
     *                  <li>{@link Sequoiadb#SDB_SNAP_TRANSDEADLOCK}
     *                  </ul>
     * @param matcher    the matching rule, match all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       the hint rule, the options provided for specific snapshot type format:{
     *                   '$Options': { <options> } }
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents.
     * @return the DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor getSnapshot(int snapType, BSONObject matcher, BSONObject selector,
                                BSONObject orderBy, BSONObject hint, long skipRows, long returnRows)
            throws BaseException {
        String command = getSnapshotCommand(snapType);
        int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
        flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
        QueryRequest request = new QueryRequest(command, matcher, selector, orderBy, hint, skipRows,
                returnRows, flag);
        SdbReply response = requestAndResponse(request);

        if (response.getFlag() == SDBError.SDB_DMS_EOC.getErrorCode()) {
            return null;
        } else if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher + ", selector = " + selector + ", orderBy = "
                    + orderBy + ", hint = " + hint + ", skipRows = " + skipRows
                    + ", returnRows = " + returnRows;
            throwIfError(response, msg);
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
            case SDB_SNAP_CONFIGS:
                return AdminCommand.SNAP_CONFIGS;
            case SDB_SNAP_SVCTASKS:
                return AdminCommand.SNAP_SVCTASKS;
            case SDB_SNAP_SEQUENCES:
                return AdminCommand.SNAP_SEQUENCES;
            case SDB_SNAP_QUERIES:
                return AdminCommand.SNAP_QUERIES;
            case SDB_SNAP_LATCHWAITS:
                return AdminCommand.SNAP_LATCHWAITS;
            case SDB_SNAP_LOCKWAITS:
                return AdminCommand.SNAP_LOCKWAITS;
            case SDB_SNAP_INDEXSTATS:
                return AdminCommand.SNAP_INDEXSTATS;
            case SDB_SNAP_TASKS:
                return AdminCommand.SNAP_TASKS;
            case SDB_SNAP_INDEXES:
                return AdminCommand.SNAP_INDEXES;
            case SDB_SNAP_TRANSWAITS:
                return AdminCommand.SNAP_TRANSWAITS;
            case SDB_SNAP_TRANSDEADLOCK:
                return AdminCommand.SNAP_TRANSDEADLOCK;
            case SDB_SNAP_RECYCLEBIN:
                return AdminCommand.SNAP_RECYCLEBIN;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG,
                        String.format("Invalid snapshot type: %d", snapType));
        }
    }

    /**
     * Begin the transaction.
     *
     * @throws BaseException If error happens.
     */
    public void beginTransaction() throws BaseException {
        TransactionRequest request = new TransactionRequest(
                TransactionRequest.TransactionType.Begin);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Commit the transaction.
     *
     * @throws BaseException If error happens.
     */
    public void commit() throws BaseException {
        TransactionRequest request = new TransactionRequest(
                TransactionRequest.TransactionType.Commit);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Rollback the transaction.
     *
     * @throws BaseException If error happens.
     */
    public void rollback() throws BaseException {
        TransactionRequest request = new TransactionRequest(
                TransactionRequest.TransactionType.Rollback);
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
     * @return The result of the eval operation, including the return value type, the return data
     * and the error message. If succeed to eval, error message is null, and we can extract
     * the eval result from the return cursor and return type, if not, the return cursor and
     * the return type are null, we can extract the error message for more detail.
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
        // if something wrong with the eval operation, not throws exception here
        if (flag != 0) {
            if (response.getErrorObj() != null) {
                evalResult.errmsg = response.getErrorObj();
            }
            return evalResult;
        } else {
            // get the return type of eval result
            if (response.getReturnedNum() > 0) {
                BSONObject obj = response.getResultSet().getNext();
                int typeValue = (Integer) obj.get(SdbConstants.FIELD_NAME_RETYE);
                evalResult.returnType = Sequoiadb.SptReturnType.getTypeByValue(typeValue);
            }
            // set the return cursor
            evalResult.cursor = new DBCursor(response, this);
            return evalResult;
        }
    }

    /**
     * Backup database.
     *
     * @param options Contains a series of backup configuration infomations. Backup the whole cluster if
     *                null. The "options" contains 5 options as below. All the elements in options are
     *                optional. eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup",
     *                "Name":"backupName", "Description":description, "EnsureInc":true,
     *                "OverWrite":true}
     *                <ul>
     *                <li>GroupID : The id(s) of replica group(s) which to be backuped
     *                <li>GroupName : The name(s) of replica group(s) which to be backuped
     *                <li>Name : The name for the backup
     *                <li>Path : The backup path, if not assign, use the backup path assigned in the
     *                configuration file, the path support to use wildcard(%g/%G:group name, %h/%H:host
     *                name, %s/%S:service name). e.g. {Path:"/opt/sequoiadb/backup/%g"}
     *                <li>isSubDir : Whether the path specified by paramer "Path" is a subdirectory of
     *                the path specified in the configuration file, default to be false
     *                <li>Prefix : The prefix of name for the backup, default to be null. e.g.
     *                {Prefix:"%g_bk_"}
     *                <li>EnableDateDir : Whether turn on the feature which will create subdirectory
     *                named to current date like "YYYY-MM-DD" automatically, default to be false
     *                <li>Description : The description for the backup
     *                <li>EnsureInc : Whether turn on increment synchronization, default to be false
     *                <li>OverWrite : Whether overwrite the old backup file with the same name, default
     *                to be false
     *                </ul>
     * @throws BaseException If error happens.
     * @deprecated Rename to "backup".
     */
    @Deprecated
    public void backupOffline(BSONObject options) throws BaseException {
        backup(options);
    }

    /**
     * Backup database.
     *
     * @param options Contains a series of backup configuration infomations. Backup the whole cluster if
     *                null. The "options" contains 5 options as below. All the elements in options are
     *                optional. eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup",
     *                "Name":"backupName", "Description":description, "EnsureInc":true,
     *                "OverWrite":true}
     *                <ul>
     *                <li>GroupID : The id(s) of replica group(s) which to be backuped
     *                <li>GroupName : The name(s) of replica group(s) which to be backuped
     *                <li>Name : The name for the backup
     *                <li>Path : The backup path, if not assign, use the backup path assigned in the
     *                configuration file, the path support to use wildcard(%g/%G:group name, %h/%H:host
     *                name, %s/%S:service name). e.g. {Path:"/opt/sequoiadb/backup/%g"}
     *                <li>isSubDir : Whether the path specified by paramer "Path" is a subdirectory of
     *                the path specified in the configuration file, default to be false
     *                <li>Prefix : The prefix of name for the backup, default to be null. e.g.
     *                {Prefix:"%g_bk_"}
     *                <li>EnableDateDir : Whether turn on the feature which will create subdirectory
     *                named to current date like "YYYY-MM-DD" automatically, default to be false
     *                <li>Description : The description for the backup
     *                <li>EnsureInc : Whether turn on increment synchronization, default to be false
     *                <li>OverWrite : Whether overwrite the old backup file with the same name, default
     *                to be false
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void backup(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.BACKUP_OFFLINE, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * List the backups.
     *
     * @param options  Contains configuration information for listing backups, list all the backups in
     *                 the default backup path if null. The "options" contains several options as below.
     *                 All the elements in options are optional. eg: {"GroupName":["rgName1", "rgName2"],
     *                 "Path":"/opt/sequoiadb/backup", "Name":"backupName"}
     *                 <ul>
     *                 <li>GroupID : Specified the group id of the backups, default to list all the
     *                 backups of all the groups.
     *                 <li>GroupName : Specified the group name of the backups, default to list all the
     *                 backups of all the groups.
     *                 <li>Path : Specified the path of the backups, default to use the backup path
     *                 asigned in the configuration file.
     *                 <li>Name : Specified the name of backup, default to list all the backups.
     *                 <li>IsSubDir : Specified the "Path" is a subdirectory of the backup path asigned
     *                 in the configuration file or not, default to be false.
     *                 <li>Prefix : Specified the prefix name of the backups, support for using
     *                 wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not
     *                 using wildcards.
     *                 <li>Detail : Display the detail of the backups or not, default to be false.
     *                 </ul>
     * @param matcher  The matching rule, return all the documents if null
     * @param selector The selective rule, return the whole document if null
     * @param orderBy  The ordered rule, never sort if null
     * @return the DBCursor of the backup or null while having no backup information.
     * @throws BaseException If error happens.
     */
    public DBCursor listBackup(BSONObject options, BSONObject matcher, BSONObject selector,
                               BSONObject orderBy) throws BaseException {
        int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
        flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
        AdminRequest request = new AdminRequest(AdminCommand.LIST_BACKUP, matcher, selector,
                orderBy, options, 0, -1, flag );
        SdbReply response = requestAndResponse(request);

        if (response.getFlag() == SDBError.SDB_DMS_EOC.getErrorCode()) {
            return null;
        } else if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher + ", selector = " + selector + ", orderBy = "
                    + orderBy + ", options = " + options;
            throwIfError(response, msg);
        }
        return new DBCursor(response, this);
    }

    /**
     * Remove the backups.
     *
     * @param options Contains configuration information for removing backups, remove all the backups in
     *                the default backup path if null. The "options" contains several options as below.
     *                All the elements in options are optional. eg: {"GroupName":["rgName1", "rgName2"],
     *                "Path":"/opt/sequoiadb/backup", "Name":"backupName"}
     *                <ul>
     *                <li>GroupID : Specified the group id of the backups, default to list all the
     *                backups of all the groups.
     *                <li>GroupName : Specified the group name of the backups, default to list all the
     *                backups of all the groups.
     *                <li>Path : Specified the path of the backups, default to use the backup path
     *                asigned in the configuration file.
     *                <li>Name : Specified the name of backup, default to list all the backups.
     *                <li>IsSubDir : Specified the "Path" is a subdirectory of the backup path assigned
     *                in the configuration file or not, default to be false.
     *                <li>Prefix : Specified the prefix name of the backups, support for using
     *                wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not
     *                using wildcards.
     *                <li>Detail : Display the detail of the backups or not, default to be false.
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
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public DBCursor listTasks(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                              BSONObject hint) throws BaseException {
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

        // append argument:{ "TaskID": { "$in": [ 1, 2, 3 ] } }
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
     * @param isAsync The operation "cancel task" is async or not, "true" for async, "false" for sync.
     *                Default sync.
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
     * @param options The options for setting session attributes. Can not be null. While it's a empty
     *                options, the local session attributes cache will be cleanup. Please reference
     *                {@see <a
     *                href=http://doc.sequoiadb.com/cn/SequoiaDB-cat_id-1432190808-edition_id-@SDB_SYMBOL_VERSION>here</a>}
     *                for more detail.
     * @throws BaseException If error happens.
     */
    public void setSessionAttr(BSONObject options) throws BaseException {
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options can not be null");
        }
        if (options.isEmpty()) {
            clearSessionAttrCache();
            return;
        }
        BSONObject newObj = new BasicBSONObject();

        for (String key: options.keySet()) {
            Object value = options.get(key);
            if (key.equalsIgnoreCase(SdbConstants.FIELD_NAME_PREFERRED_INSTANCE_LEGACY) ||
                    key.equalsIgnoreCase(SdbConstants.FIELD_NAME_PREFERRED_INSTANCE)) {
                if (value instanceof String) {
                    String valueStr = (String)value;
                    int v ;
                    if (valueStr.equalsIgnoreCase("M") ||valueStr.equalsIgnoreCase("-M")) {
                        v = PreferInstance.MASTER;
                    } else if (valueStr.equalsIgnoreCase("S") ||valueStr.equalsIgnoreCase("-S")) {
                        v = PreferInstance.SLAVE;
                    } else if (valueStr.equalsIgnoreCase("A") ||valueStr.equalsIgnoreCase("-A")) {
                        v = PreferInstance.ANYONE;
                    } else {
                        throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
                    }
                    newObj.put(key, v);
                } else if (value instanceof Integer) {
                    newObj.put(key, value);
                }
                // Add new version of preferred instance
                newObj.put(SdbConstants.FIELD_NAME_PREFERRED_INSTANCE_V1_LEGACY, value);
            } else {
                newObj.put(key, value);
            }
        }

        clearSessionAttrCache();
        AdminRequest request = new AdminRequest(AdminCommand.SET_SESSION_ATTRIBUTE, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Get the attributes of the current session from the local cache if possible.
     *
     * @return the BSONObject of the session attribute.
     * @throws BaseException If error happens.
     * @since 2.8.5
     */
    public BSONObject getSessionAttr() throws BaseException {
        return getSessionAttr(true);
    }

    /**
     * Get the attributes of the current session.
     *
     * @param useCache use the local cache or not.
     * @return the BSONObject of the session attribute.
     * @throws BaseException If error happens.
     * @since 3.2
     */
    public BSONObject getSessionAttr(boolean useCache) throws BaseException {
        BSONObject result = null;
        if (useCache) {
            result = getSessionAttrCache();
            if (result != null) {
                return result;
            }
        }
        AdminRequest request = new AdminRequest(AdminCommand.GET_SESSION_ATTRIBUTE);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        ResultSet resultSet = response.getResultSet();
        if (resultSet != null && resultSet.hasNext()) {
            result = resultSet.getNext();
            if (result == null) {
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
     * Close all the cursors created in current connection, we can't use those cursors to get data
     * again.
     *
     * @throws BaseException If error happens.
     */
    public void closeAllCursors() throws BaseException {
        closeAllCursorMark++;
        interrupt();
    }

    /**
     * Send an interrupt message to engine.
     *
     * @throws BaseException If error happens.
     */
    public void interrupt() throws BaseException {
        if (isClosed()) {
            return;
        }
        InterruptRequest request = new InterruptRequest();
        sendRequest(request);
    }

    /**
     * Send "INTERRUPT_SELF" message to engine to stop the current operation. When the current operation had finish,
     * nothing happened, Otherwise, the current operation will be stop, and return error.
     *
     * @throws BaseException If error happens.
     */
    public void interruptOperation() throws BaseException {
        if (isClosed()) {
            return;
        }
        InterruptRequest request = new InterruptRequest(true);
        sendRequest(request);
    }

    /**
     * List all the replica group.
     *
     * @return information of all replica groups.
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
            throw new BaseException(SDBError.SDB_INVALIDARG, "Domain name can't be null or empty");
        }
        return _checkIsExistByList(SDB_LIST_DOMAINS, domainName);
    }

    /**
     * Create a domain.
     *
     * @param domainName The name of the creating domain
     * @param options    The options for the domain. The options are as below:
     *                   <ul>
     *                   <li>Groups : the list of the replica groups' names which the domain is going to
     *                   contain. eg: { "Groups": [ "group1", "group2", "group3" ] } If this argument is
     *                   not included, the domain will contain all replica groups in the cluster.
     *                   <li>AutoSplit : If this option is set to be true, while creating
     *                   collection(ShardingType is "hash") in this domain, the data of this collection
     *                   will be split(hash split) into all the groups in this domain automatically.
     *                   However, it won't automatically split data into those groups which were add into
     *                   this domain later. eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit:
     *                   true" }
     *                   </ul>
     * @return the newly created collection space object
     * @throws BaseException If error happens.
     */
    public Domain createDomain(String domainName, BSONObject options) throws BaseException {
        if (domainName == null || domainName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "domain name is empty or null");
        }

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
     * @throws BaseException If the domain not exit, throw BaseException with the error
     *                       SDB_CAT_DOMAIN_NOT_EXIST.
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
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *                 "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *                 null, database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public DBCursor listDomains(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                                BSONObject hint) throws BaseException {
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
     * @deprecated Use isReplicaGroupExist(String rgName) instead.
     */
    @Deprecated
    public boolean isRelicaGroupExist(String rgName) {
        return isReplicaGroupExist(rgName);
    }

    /**
     * whether the replica group exists in the database or not
     *
     * @param rgName replica group's name
     * @return true or false
     */
    public boolean isReplicaGroupExist(String rgName) {
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
    public ReplicaGroup getReplicaGroup(String rgName) throws BaseException {
        BSONObject rg = getDetailByName(rgName);
        if (rg == null) {
            throw new BaseException(SDBError.SDB_CLS_GRP_NOT_EXIST,
                    String.format("Group with the name[%s] does not exist", rgName));
        }
        Object groupId = rg.get(SdbConstants.FIELD_NAME_GROUPID);
        if (groupId instanceof Integer) {
            return new ReplicaGroup(this, (Integer) groupId, rgName);
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
        String groupName = rg.get(SdbConstants.FIELD_NAME_GROUPNAME).toString();
        return new ReplicaGroup(this, rgId, groupName);
    }

    /**
     * Create replica group by name.
     *
     * @param rgName replica group's name
     * @return A replica group object.
     * @throws BaseException If error happens.
     */
    public ReplicaGroup createReplicaGroup(String rgName) throws BaseException {
        if (rgName == null || rgName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The name of replica group is null or empty");
        }
        BSONObject rg = new BasicBSONObject();
        rg.put(SdbConstants.FIELD_NAME_GROUPNAME, rgName);

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_GROUP, rg);
        SdbReply response = requestAndResponse(request);
        throwIfError(response, rgName);

        // This operation is for compatibility with older versions of sdb. Newer versions of sdb will return metadata
        // information after a successful operation.
        BSONObject result = response.getReturnData();
        if (result != null) {
            Object groupId = result.get(SdbConstants.FIELD_NAME_GROUPID);
            if (groupId instanceof Integer) {
                return new ReplicaGroup(this, (Integer) groupId, rgName);
            }
            throw new BaseException(SDBError.SDB_NET_BROKEN_MSG);
        }
        return new ReplicaGroup(this, rgName);
    }

    /**
     * Remove replica group by name.
     *
     * @param rgName replica group's name
     * @throws BaseException If error happens.
     */
    public void removeReplicaGroup(String rgName) throws BaseException {
        if (rgName == null || rgName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The name of replica group is null or empty");
        }
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
        if (rgName == null || rgName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The name of replica group is null or empty");
        }
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
     * @deprecated Use "void createReplicaCataGroup(String hostName, int port, String dbPath, final
     * BSONObject options)" instead.
     */
    @Deprecated
    public void createReplicaCataGroup(String hostName, int port, String dbPath,
                                       Map<String, String> configure) {
        BSONObject obj = new BasicBSONObject();
        if (configure != null) {
            for (String key : configure.keySet()) {
                obj.put(key, configure.get(key));
            }
        }
        createReplicaCataGroup(hostName, port, dbPath, obj);
    }

    /**
     * Stop the specified session's current operation and terminate it.
     *
     * @param sessionID The ID of the session.
     */
    public void forceSession(long sessionID) {
        forceSession(sessionID, null);
    }

    /**
     * Stop the specified session's current operation and terminate it.
     *
     * @param sessionID The ID of the session.
     * @param option    The control options, Please reference
     *                  {@see <a
     *                  href=http://doc.sequoiadb.com/cn/SequoiaDB-cat_id-1482314609-edition_id-@SDB_SYMBOL_VERSION>here</a>}
     *                  for more detail.
     */
    public void forceSession(long sessionID, BSONObject option) {

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_SESSION_ID, sessionID);
        if (option != null) {
            matcher.putAllUnique(option);
        }
        AdminRequest request = new AdminRequest(AdminCommand.FORCE_SESSION, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Send message to server.
     *
     * @param message The message to send to server.
     */
    public void msg(String message) {
        MessageRequest request = new MessageRequest(message);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Clear the cache of the nodes (data/coord node).
     *
     * @param options The control options:(Only take effect in coordinate nodes). About the parameter
     *                'options', please reference to the official website(www.sequoiadb.com) and then
     *                search "命令位置参数" for more details. Some of its optional parameters are as
     *                bellow:
     *                <p>
     *                <ul>
     *                <li>Global(Bool) : execute this command in global or not. While 'options' is null,
     *                it's equals to {Glocal: true}.
     *                <li>GroupID(INT32 or INT32 Array) : specified one or several groups by their group
     *                IDs. e.g. {GroupID:[1001, 1002]}.
     *                <li>GroupName(String or String Array) : specified one or several groups by their
     *                group names. e.g. {GroupID:"group1"}.
     *                <li>...
     *                </ul>
     * @return void
     */
    public void invalidateCache(BSONObject options) {
        AdminRequest request = new AdminRequest(AdminCommand.INVALIDATE_CACHE, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Create a sequence.
     *
     * @param seqName The name of sequence
     * @return A sequence object of creation
     */
    public DBSequence createSequence(String seqName){
        return createSequence(seqName, null);
    }

    /**
     * Create a sequence with the specified options.
     *
     * @param seqName The name of sequence
     * @param options The options specified by user, details as bellow:
     *                <ul>
     *                  <li>StartValue(long) : The start value of sequence
     *                  <li>MinValue(long)   : The minimum value of sequence
     *                  <li>MaxValue(long)   : The maxmun value of sequence
     *                  <li>Increment(int)   : The increment value of sequence
     *                  <li>CacheSize(int)   : The cache size of sequence
     *                  <li>AcquireSize(int) : The acquire size of sequence
     *                  <li>Cycled(boolean)  : The cycled flag of sequence
     *                </ul>
     * @return A sequence object of creation
     */
    public DBSequence createSequence(String seqName, BSONObject options){
        if (seqName == null || seqName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Sequence name can't be null or empty");
        }
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, seqName);
        if (options != null) {
            obj.putAll(options);
        }
        AdminRequest request = new AdminRequest(AdminCommand.CREATE_SEQUENCE, obj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        return new DBSequence(seqName,this);
    }

    /**
     * Drop the specified sequence.
     *
     * @param seqName The name of sequence
     */
    public void dropSequence(String seqName){
        if (seqName == null || seqName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Sequence name can't be null or empty");
        }
        BSONObject newObj = new BasicBSONObject();
        newObj.put(SdbConstants.FIELD_NAME_NAME, seqName);

        AdminRequest request = new AdminRequest(AdminCommand.DROP_SEQUENCE, newObj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Get the specified sequence.
     *
     * @param seqName The name of sequence
     * @return The specified sequence object
     */
    public DBSequence getSequence(String seqName){
        if (seqName == null || seqName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Sequence name can't be null or empty");
        }
        if (isSequenceExist(seqName)){
            return new DBSequence(seqName, this);
        } else {
            throw new BaseException(SDBError.SDB_SEQUENCE_NOT_EXIST, "Sequence does not exist");
        }
    }

    private boolean isSequenceExist(String seqName) throws BaseException {
        if (seqName == null || seqName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Sequence name can't be null or empty");
        }
        return _checkIsExistByList(SDB_LIST_SEQUENCES, seqName);
    }

    /**
     * Rename sequence.
     *
     * @param oldName    The old name of sequence
     * @param newName The new name of sequence
     */
    public void renameSequence(String oldName, String newName){
        if (oldName == null || oldName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "oldName can't be null or empty");
        }
        if (newName == null || newName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "newName can't be null or empty");
        }

        BSONObject option = new BasicBSONObject();
        option.put(SdbConstants.FIELD_NAME_NAME, oldName);
        option.put(SdbConstants.FIELD_NAME_NEWNAME, newName);

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_ACTION, SdbConstants.SEQ_OPT_RENAME);
        obj.put(SdbConstants.FIELD_NAME_OPTIONS, option);

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_SEQUENCE, obj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Whether the data source exists or not.
     *
     * @param dataSourceName The data source name
     * @throws BaseException If error happens.
     */
    public boolean isDataSourceExist(String dataSourceName) throws BaseException {
        if (null == dataSourceName || dataSourceName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "data source name cannot be empty or null");
        }
        return _checkIsExistByList(SDB_LIST_DATASOURCES, dataSourceName);
    }

    /**
     * Create data source.
     * @param dataSourceName The data source name
     * @param addresses The list of coord addresses for the target sequoiadb cluster, spearated by ','
     * @param user User name of the data source
     * @param password User password of the data source
     * @param type Data source type, default is "SequoiaDB"
     * @param option Optional configuration option for create data source, as follows:
     *                <ul>
     *                  <li>AccessMode(String) : Configure access permissions for the data source, default is "ALL",
     *                  the values are as follows:
     *                  <ul>
     *                    <li>"READ" : Allow read-only operation
     *                    <li>"WRITE" : Allow write-only operation
     *                    <li>"ALL" or "READ|WRITE" : Allow all operations
     *                    <li>"NONE" : Neither read nor write operation is allowed
     *                  </ul>
     *                  <li>ErrorFilterMask(String) : Configure error filtering for data operations on data sources,
     *                  default is "NONE", the values are as follows:
     *                  <ul>
     *                    <li>"READ" : Filter data read errors
     *                    <li>"WRITE" : Filter data write errors
     *                    <li>"ALL" or "READ|WRITE" : Filter both data read and write errors
     *                    <li>"NONE" : Do not filter any errors
     *                  </ul>
     *                  <li>ErrorControlLevel(String) : Configure the error control level when performing unsupported data
     *                  operations(such as DDL) on the mapping collection or collection space, default is "low",
     *                  the values are as follows:
     *                  <ul>
     *                    <li>"high" : Report an error and output an error message
     *                    <li>"low" : Ignore unsupported data operations and do not execute on data source
     *                  </ul>
     *                  <li>TransPropagateMode(String) : Configure the transaction propagation mode on data source,
     *                  default is "never", the values are as follows:
     *                  <ul>
     *                    <li>"never": Transaction operation is forbidden. Report an error and output and error message.
     *                    <li>"notsupport": Transaction operation is not supported on data source. The operation
     *                    will be converted to non-transactional and send to data source.
     *                  </ul>
     *                  <li>InheritSessionAttr(Bool): Configure whether the session between the coordination node and the
     *                  data source inherits properties of the local session, default is true. The supported attributes
                        include PreferedInstance, PreferedInstanceMode, PreferedStrict, PreferedPeriod and Timeout.
     *                </ul>
     * @return A data source object
     * @throws BaseException If error happens.
     */
    public DataSource createDataSource(String dataSourceName, String addresses,
                                       String user, String password,
                                       String type, BSONObject option) throws BaseException {
        if (dataSourceName == null || dataSourceName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "data source name is empty or null");
        }
        if (addresses == null || addresses.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "data source address list is empty or null");
        }
        if ( type == null || type.equals("")){
            type = "SequoiaDB";
        }

        BSONObject obj = new BasicBSONObject();
        if ( option != null ) {
            obj.putAllUnique(option);
        }
        obj.put(SdbConstants.FIELD_NAME_NAME, dataSourceName);
        obj.put(SdbConstants.FIELD_NAME_ADDRESS, addresses);
        obj.put(SdbConstants.FIELD_NAME_USER, user);
        obj.put(SdbConstants.FIELD_NAME_PASSWD, Helper.md5(password));
        obj.put(SdbConstants.FIELD_NAME_TYPE, type);

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_DATASOURCE, obj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        return new DataSource(this, dataSourceName);
    }

    /**
     * Drop data source.
     *
     * @param dataSourceName The data source name
     * @throws BaseException If error happens.
     */
    public void dropDataSource(String dataSourceName) throws BaseException {
        if (dataSourceName == null || dataSourceName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "data source name is empty or null");
        }

        BSONObject obj = new BasicBSONObject(SdbConstants.FIELD_NAME_NAME, dataSourceName);
        AdminRequest request = new AdminRequest(AdminCommand.DROP_DATASOURCE, obj);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * List data source.
     *
     * @param matcher  The matching rule, return all the records if null
     * @param selector The selective rule, return the whole records if null
     * @param orderBy  The ordered rule, never sort if null
     * @param hint  Reserved, please specify null
     * @return Cursor of data source information
     * @throws BaseException If error happens.
     */
    public DBCursor listDataSources(BSONObject matcher, BSONObject selector, BSONObject orderBy,
                                    BSONObject hint) throws BaseException {
        return getList(SDB_LIST_DATASOURCES, matcher, selector, orderBy);
    }

    /**
     * Get data source.
     *
     * @param dataSourceName The data source name
     * @return The data source object
     * @throws BaseException If error happens.
     */
    public DataSource getDataSource(String dataSourceName) throws BaseException {
        if (isDataSourceExist(dataSourceName)) {
            return new DataSource(this, dataSourceName);
        } else {
            throw new BaseException(SDBError.SDB_CAT_DATASOURCE_NOTEXIST, dataSourceName);
        }
    }

    /**
     * Get recycle bin.
     *
     * @return The recycle bin object
     */
    public DBRecycleBin getRecycleBin(){
        return new DBRecycleBin(this);
    }

    /**
     * Create a role
     *
     * @param role Some detailed information about the role.
     *             e.g. {Role: "my_role", Privileges: [{Resource: {cs: "my_cs", cl: "my_cl"}, Actions: ["find"]},
     *             Roles: ["other_role"]]}
     *             <ul>
     *             <li>Role(string): The name of the role.
     *             <li>Privileges: List of privileges, containing the following fields:
     *             <ul>
     *             <li>Resource: The resource to which the privilege applies.
     *             <ul>
     *             <li>cs(String): Collection space
     *             <li>cl(String): Collection
     *             </ul>
     *             </li>
     *             <li>Actions(List&lt;String&gt;): List of actions included in the privilege.
     *             </ul>
     *             <li>Roles(List&lt;String&gt;): List of inherited roles. Inheriting a role will grant all the
     *             privileges of that role.
     *             </ul>
     * @throws BaseException If error happens
     */
    public void createRole(BSONObject role) throws BaseException {
        if (role == null || !role.containsField(SdbConstants.FIELD_NAME_ROLE)) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid role");
        }

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_ROLE, role);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Delete a role
     *
     * @param roleName The name of the role to be deleted.
     * @throws BaseException If error happens
     */
    public void dropRole(String roleName) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        AdminRequest request = new AdminRequest(AdminCommand.DROP_ROLE, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Get information for a role
     *
     * @param roleName The name of the role.
     * @param options  Additional options for outputting role information.
     *                 <ul>
     *                 <li>ShowPrivileges(Boolean): Whether to display the privileges information.
     *                 </ul>
     * @return The detailed information of the role.
     * @throws BaseException If error happens
     */
    public BSONObject getRole(String roleName, BSONObject options) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        if (options != null) {
            matcher.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.GET_ROLE, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);

        try (DBCursor cursor = new DBCursor(response, this)) {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_AUTH_ROLE_NOT_EXIST);
            }
            return cursor.getNext();
        }
    }

    /**
     * List all role information
     *
     * @param options The optional information for the output result.
     *                <ul>
     *                <li>ShowPrivileges(Boolean): Whether to display the privileges information.
     *                <li>ShowBuiltinRoles(Boolean): Whether to display built-in roles.
     *                </ul>
     * @return Role information cursor.
     * @throws BaseException If error happens
     */
    public DBCursor listRoles(BSONObject options) throws BaseException {
        AdminRequest request = new AdminRequest(AdminCommand.LIST_ROLES, options);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
        return new DBCursor(response, this);
    }

    /**
     * Update role information
     *
     * @param roleName The name of the role.
     * @param role     The new role information.
     *                 e.g. {Role: "my_role", Privileges: [{Resource: {cs: "my_cs", cl: "my_cl"}, Actions: ["find"]}],
     *                 Roles: ["other_role"]}
     *                 <ul>
     *                 <li>Role(string): The name of the role.
     *                 <li>Privileges: List of privileges, containing the following fields:
     *                 <ul>
     *                 <li>Resource: The resource to which the privilege applies.
     *                 <ul>
     *                 <li>cs(String): Collection space
     *                 <li>cl(String): Collection
     *                 </ul>
     *                 </li>
     *                 <li>Actions(List&lt;String&gt;): List of actions included in the privilege.
     *                 </ul>
     *                 <li>Roles(List&lt;String&gt;): List of inherited roles. Inheriting a role will grant all the
     *                 privileges of that role.
     *                 </ul>
     * @throws BaseException If error happens
     */
    public void updateRole(String roleName, BSONObject role) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        matcher.putAll(role);

        AdminRequest request = new AdminRequest(AdminCommand.UPDATE_ROLE, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Grant some privileges to a specific role
     *
     * @param roleName   The name of the role.
     * @param privileges The privileges to be granted.
     *                   e.g. [{Resource: {cs: "my_cs", cl: "my_cl"}, Actions: ["find"]}]
     *                   <ul>
     *                   <li>Resource: The resource to which the privilege applies.
     *                   <ul>
     *                   <li>cs(String): Collection space
     *                   <li>cl(String): Collection
     *                   </ul>
     *                   </li>
     *                   <li>Actions(List&lt;String&gt;): List of actions included in the privilege.
     *                   </ul>
     * @throws BaseException If error happens
     */
    public void grantPrivilegesToRole(String roleName, BSONObject privileges) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        matcher.put(SdbConstants.FIELD_NAME_PRIVILEGES, privileges);

        AdminRequest request = new AdminRequest(AdminCommand.GRANT_PRIVILEGES, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Revoke some privileges from a specific role
     *
     * @param roleName   The name of the role.
     * @param privileges List privileges that need to be revoked.
     *                   e.g. [{Resource: {cs: "my_cs", cl: "my_cl"}, Actions: ["find"]}]
     *                   <ul>
     *                   <li>Resource: The resource to which the privilege applies.
     *                   <ul>
     *                   <li>cs(String): Collection space
     *                   <li>cl(String): Collection
     *                   </ul>
     *                   </li>
     *                   <li>Actions(List&lt;String&gt;): List of actions included in the privilege.
     *                   </ul>
     * @throws BaseException If error happens
     */
    public void revokePrivilegesFromRole(String roleName, BSONObject privileges) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        matcher.put(SdbConstants.FIELD_NAME_PRIVILEGES, privileges);

        AdminRequest request = new AdminRequest(AdminCommand.REVOKE_PRIVILEGES, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Make a certain role inherit from another role
     *
     * @param roleName The name of the role.
     * @param roles    List role names that needs to be inherited from. e.g. ["my_role_1", "my_role_2"]
     * @throws BaseException If error happens
     */
    public void grantRolesToRole(String roleName, BSONObject roles) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        matcher.put(SdbConstants.FIELD_NAME_ROLES, roles);

        AdminRequest request = new AdminRequest(AdminCommand.GRANT_ROLES_TO_ROLE, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Revoke some roles that are inherited by a specific role.
     *
     * @param roleName The target role.
     * @param roles    List of role names to be revoked. e.g. ["my_role_1", "my_role_2"]
     * @throws BaseException If error happens
     */
    public void revokeRolesFromRole(String roleName, BSONObject roles) throws BaseException {
        if (roleName == null || roleName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Role name can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_ROLE, roleName);
        matcher.put(SdbConstants.FIELD_NAME_ROLES, roles);

        AdminRequest request = new AdminRequest(AdminCommand.REVOKE_ROLES_FROM_ROLE, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Grant some roles to a specific user
     *
     * @param username The username of the user.
     * @param roles    List of role names to be granted. e.g. ["my_role_1", "my_role_2"]
     *
     * @throws BaseException If error happens
     */
    public void grantRolesToUser(String username, BSONObject roles) throws BaseException {
        if (username == null || username.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Username can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_USER, username);
        matcher.put(SdbConstants.FIELD_NAME_ROLES, roles);

        AdminRequest request = new AdminRequest(AdminCommand.GRANT_ROLES_TO_USER, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Revoke some roles from a specific user
     *
     * @param username The username of the user.
     * @param roles    List of role names to be revoked. e.g. ["my_role_1", "my_role_2"]
     * @throws BaseException If error happens
     */
    public void revokeRolesFromUser(String username, BSONObject roles) throws BaseException {
        if (username == null || username.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Username can't be null or empty");
        }

        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_USER, username);
        matcher.put(SdbConstants.FIELD_NAME_ROLES, roles);

        AdminRequest request = new AdminRequest(AdminCommand.REVOKE_ROLES_FROM_USER, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    /**
     * Clear all user caches on all nodes
     *
     * @throws BaseException If error happens
     */
    public void invalidateUserCache() throws BaseException {
        invalidateUserCache(null, null);
    }

    /**
     * Invalidate user cache
     *
     * @param username The name of user to be invalidated, if null, invalidate all users
     * @param options  The control options (Only take effect in coordinate nodes)
     *                 <ul>
     *                 <li>GroupID(int)
     *                 <li>GroupName(String)
     *                 <li>NodeID(int)
     *                 <li>HostName(String)
     *                 <li>svcname(String)
     *                 <li>...
     *                 </ul>
     * @throws BaseException If error happens
     */
    public void invalidateUserCache(String username, BSONObject options) throws BaseException {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_USER, username);
        if (options != null) {
            matcher.putAll(options);
        }
        AdminRequest request = new AdminRequest(AdminCommand.INVALIDATE_USER_CACHE, matcher);
        SdbReply response = requestAndResponse(request);
        throwIfError(response);
    }

    private boolean _checkIsExistByList(int listType, String targetName) throws BaseException {
        if (null == targetName || targetName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, targetName);
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_NAME, targetName);
        DBCursor cursor = getList(listType, matcher, null, null);
        try {
            if (cursor != null && cursor.hasNext()) {
                return true;
            } else {
                return false;
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
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
            case SDB_LIST_SVCTASKS:
                return AdminCommand.LIST_SVCTASKS;
            case SDB_LIST_SEQUENCES:
                return AdminCommand.LIST_SEQUENCES;
            case SDB_LIST_USERS:
                return AdminCommand.LIST_USERS;
            case SDB_LIST_BACKUPS:
                return AdminCommand.LIST_BACKUPS;
            case SDB_LIST_DATASOURCES:
                return AdminCommand.LIST_DATASOURCES;
            case SDB_LIST_RECYCLEBIN:
                return AdminCommand.LIST_RECYCLEBIN;
            case SDB_LIST_CL_IN_DOMAIN:
                return AdminCommand.LIST_CL_IN_DOMAIN;
            case SDB_LIST_CS_IN_DOMAIN:
                return AdminCommand.LIST_CS_IN_DOMAIN;
            case SDB_LIST_GROUPMODES:
                return AdminCommand.LIST_GROUPMODES;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG,
                        String.format("Invalid list type: %d", listType));
        }
    }

    String getUserName() {
        return userName;
    }

    String getPassword() {
        return password;
    }

    BSONObject getDetailByName(String name) throws BaseException {
        if (name == null || name.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "name is null or empty");
        }
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

    int getCloseAllCursorMark() {
        return closeAllCursorMark;
    }

    private SysInfoResponse receiveSysInfoResponse() {
        SysInfoResponse response = new SysInfoResponse();
        byte[] lengthBytes = connection.receive(response.length());
        ByteBuffer buffer = ByteBuffer.wrap(lengthBytes);
        response.decode(buffer, null);
        return response;
    }

    private ByteBuffer receiveSdbResponse(ByteBuffer buffer) {
        try {
            byte[] lengthBytes = connection.receive(4);
            int length = ByteBuffer.wrap(lengthBytes).order(byteOrder).getInt();
            buffer = Helper.resetBuff(buffer, length, byteOrder);
            System.arraycopy(lengthBytes, 0, buffer.array(), 0, lengthBytes.length);
            connection.receive(buffer.array(), 4, length - 4);
        }catch (Exception e){
            connection.close();
            throw new BaseException(SDBError.SDB_NETWORK, "Failed to receive message.", e);
        }
        return buffer;
    }

    private void validateResponse(SdbRequest request, SdbResponse response) {
        if ((request.opCode() | MsgOpCode.RESP_MASK) != response.opCode()) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    ("request=" + request.opCode() + " response=" + response.opCode()));
        }
    }

    private void resetRequestBuff(int len) {
        requestBuffer = Helper.resetBuff(requestBuffer, len, byteOrder);
    }

    protected void cleanRequestBuff(){
        requestBuffer = null;
    }

    private ByteBuffer encodeRequest(Request request) {
        resetRequestBuff(request.length());
        request.setRequestId(getNextRequestId());
        request.encode(requestBuffer, protocolVersion);
        return requestBuffer;
    }

    private void sendRequest(Request request) {
        ByteBuffer buffer = encodeRequest(request);
        if (!isClosed()) {
            // no need to set currentCacheSize here, for only command message use sendRequest
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
        response.decode(buffer, protocolVersion);
        return response;
    }

    private ByteBuffer sendAndReceive(ByteBuffer request, ByteBuffer buff) {
        if (!isClosed()) {
            connection.send(request);
            lastUseTime = System.currentTimeMillis();
            ByteBuffer response = receiveSdbResponse(buff);
            if (request.limit() > currentCacheSize) {
                currentCacheSize = request.limit();
            }
            if (response.limit() > currentCacheSize) {
                currentCacheSize = response.limit();
            }
            return response;
        } else {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
    }

    SdbReply requestAndResponse(SdbRequest request) {
        return requestAndResponse(request, SdbReply.class);
    }

    <T extends SdbResponse> T requestAndResponse(SdbRequest request, Class<T> tClass) {
        return requestAndResponse(request, tClass, null);
    }

    <T extends SdbResponse> T requestAndResponse(SdbRequest request, Class<T> tClass, ByteBuffer buff) {
        ByteBuffer out = encodeRequest(request);
        ByteBuffer in = sendAndReceive(out, buff);
        T response = decodeResponse(in, tClass);
        validateResponse(request, response);
        return response;
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

    // errorMsg Object to avoid unnecessary toString() invoke when no error happened
    void throwIfError(CommonResponse response, Object errorMsg) throws BaseException {
        if (response.getFlag() != 0) {
            String remoteAddress = "remote address[" + getNodeName() + "]";
            BSONObject errorObj = response.getErrorObj();
            String detail = getErrorDetail(errorObj, errorMsg);
            if (detail != null && !detail.isEmpty()) {
                throw new BaseException(response.getFlag(), remoteAddress + ", " + detail,
                        errorObj);
            } else {
                throw new BaseException(response.getFlag(), remoteAddress, errorObj);
            }
        }
    }

    // dependent implementation but not invoke throwIfError(r,o) to avoid duplicate stack trace
    void throwIfError(CommonResponse response) throws BaseException {
        if (response.getFlag() != 0) {
            String remoteAddress = "remote address[" + getNodeName() + "]";
            BSONObject errorObj = response.getErrorObj();
            String detail = getErrorDetail(errorObj, null);
            if (detail != null && !detail.isEmpty()) {
                throw new BaseException(response.getFlag(), remoteAddress + ", " + detail,
                        errorObj);
            } else {
                throw new BaseException(response.getFlag(), remoteAddress, errorObj);
            }
        }
    }

    boolean getInfoEncryption() {
        return globalClientConf.getInfoEncryption();
    }

    private SysInfoResponse getSysInfo() {
        sendRequest(new SysInfoRequest());
        return receiveSysInfoResponse();
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
            cleanRequestBuff();
            connection.close();
        }
    }

    protected Object getObjectFromResp(SdbReply response, String targetField){
        BSONObject result;
        DBCursor cursor = new DBCursor(response, this);
        try {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT);
            }
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        boolean flag = result.containsField(targetField);
        if (!flag) {
            throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT);
        }
        return result.get(targetField);
    }

    /**
     * Get the connection proxy object, which can be used to update the configuration
     * of the connection object.
     *
     * @return The connection proxy object
     */
    public ConnectionProxy getConnProxy(){
        return connProxy;
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
    /**
     * The builder of Sequoiadb.
     *
     * </p>
     * Usage example:
     * <pre>
     * Sequoiadb db = Sequoiadb.builder()
     *         .serverAddress( "sdbserver:11810" )
     *         .userConfig( new UserConfig( "admin", "admin" ) )
     *         .build();
     * </pre>
     */
    public static final class Builder {
        private List<String> addressList = null;
        private UserConfig userConfig = null;
        private ConfigOptions configOptions = null;

        private Builder() {
        }

        /**
         * Set an address of SequoiaDB node, format: "Host:Port", eg: "sdbserver:11810"
         *
         * @param address The address of SequoiaDB node
         */
        public Builder serverAddress( String address ) {
            if ( address == null || address.isEmpty() ){
                throw new BaseException( SDBError.SDB_INVALIDARG, "The server address is null or empty" );
            }
            this.addressList = new ArrayList<>();
            this.addressList.add( address );
            return this;
        }

        /**
         * Set an address list of SequoiaDB node.
         *
         * @param addressList The address list of SequoiaDB node.
         */
        public Builder serverAddress( List<String> addressList ) {
            if ( addressList == null || addressList.isEmpty() ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The server address list is null or empty" );
            }
            this.addressList = addressList;
            return this;
        }

        /**
         * Set the user config.
         *
         * @param userConfig The user config.
         */
        public Builder userConfig( UserConfig userConfig ) {
            if ( userConfig == null ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The user config is null" );
            }
            this.userConfig = userConfig;
            return this;
        }

        /**
         * Set the options for connection.
         *
         * @param option The options for connection
         */
        public Builder configOptions( ConfigOptions option ) {
            if ( option == null ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The connection options is null" );
            }
            this.configOptions = option;
            return this;
        }

        /**
         * Create a Sequoiadb instance.
         *
         * @return The Sequoiadb instance
         */
        public Sequoiadb build() {
            if ( userConfig == null ) {
                userConfig = new UserConfig();
            }
            if ( configOptions == null ) {
                configOptions = new ConfigOptions();
            }
            return new Sequoiadb( this );
        }
    }
}
