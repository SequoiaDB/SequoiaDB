package org.springframework.data.mongodb.assist;

import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.logging.Logger;

import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * A database connection with internal connection pooling. For most applications, you should have one Mongo instance
 * for the entire JVM.
 * <p>
 * The following are equivalent, and all connect to the local database running on the default port:
 * <pre>
 * Mongo mongo1 = new Mongo();
 * Mongo mongo1 = new Mongo("localhost");
 * Mongo mongo2 = new Mongo("localhost", 27017);
 * Mongo mongo4 = new Mongo(new ServerAddress("localhost"));
 * </pre>
 * <p>
 * You can connect to a
 * <a href="http://www.mongodb.org/display/DOCS/Replica+Sets">replica set</a> using the Java driver by passing
 * a ServerAddress list to the Mongo constructor. For example:
 * <pre>
 * Mongo mongo = new Mongo(Arrays.asList(
 *   new ServerAddress("localhost", 27017),
 *   new ServerAddress("localhost", 27018),
 *   new ServerAddress("localhost", 27019)));
 * </pre>
 * You can connect to a sharded cluster using the same constructor.  Mongo will auto-detect whether the servers are
 * a list of replica set members or a list of mongos servers.
 * <p>
 * By default, all read and write operations will be made on the primary,
 * but it's possible to read from secondaries by changing the read preference:
 * <p>
 * <pre>
 * mongo.setReadPreference(ReadPreference.secondary());
 * </pre>
 * By default, write operations will not throw exceptions on failure, but that is easily changed too:
 * <p>
 * <pre>
 * mongo.setWriteConcern(WriteConcern.SAFE);
 * </pre>
 * <p>
 * Note: This class has been superseded by {@code MongoClient}, and may be deprecated in a future release.
 *
 * @see MongoClient
 * @see ReadPreference
 * @see WriteConcern
 */
public class Mongo {

    static Logger logger = Logger.getLogger(Bytes.LOGGER.getName() + ".Mongo");

    static final String DEFAULT_HOST = "127.0.0.1";
    static final int DEFAULT_PORT = 11810;

    /**
     * @deprecated Replaced by <code>Mongo.getMajorVersion()</code>
     */
    @Deprecated
    public static final int MAJOR_VERSION = 2;

    /**
     * @deprecated Replaced by <code>Mongo.getMinorVersion()</code>
     */
    @Deprecated
    public static final int MINOR_VERSION = 12;

    private static final String FULL_VERSION = "2.12.3";

    static int cleanerIntervalMS;

    private static final String ADMIN_DATABASE_NAME = "admin";

    static {
        cleanerIntervalMS = Integer.parseInt(System.getProperty("org.springframework.data.mongodb.assist.cleanerIntervalMS", "1000"));
    }

    private ConnectionManager _cm;
    private List<String> _coordList;
    private MongoOptions options;


    /**
     * Gets the major version of this library
     *
     * @return the major version, e.g. 2
     * @deprecated Please use {@link #getVersion()} instead.
     */
    @Deprecated
    public static int getMajorVersion() {
        return MAJOR_VERSION;
    }

    /**
     * Gets the minor version of this library
     *
     * @return the minor version, e.g. 8
     * @deprecated Please use {@link #getVersion()} instead.
     */
    @Deprecated
    public static int getMinorVersion() {
        return MINOR_VERSION;
    }

    /**
     * Connect to the MongoDB instance at the given address, select and return the {@code DB} specified in the {@code DBAddress} parameter.
     *
     * @param addr The details of the server and database to connect to
     * @return the DB requested in the addr parameter.
     * @deprecated
     */
    @Deprecated
    public static DB connect(DBAddress addr) {
        return new Mongo(addr).getDB(addr.getDBName());
    }

    /**
     * Creates a Mongo instance based on a (single) mongodb node (localhost, default port)
     *
     * @throws UnknownHostException
     * @throws MongoException
     */
//    @Deprecated
    public Mongo() throws UnknownHostException {
        this(new ServerAddress());
    }

    /**
     * Creates a Mongo instance based on a (single) mongodb node
     *
     * @param host the database's host address
     * @param port the port on which the database is running
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws MongoException
     */
//    @Deprecated
    public Mongo(String host, int port) throws UnknownHostException {
        this(new ServerAddress(host, port));
    }

    public Mongo(String host, int port, String userName, String password,
                 MongoOptions options) throws UnknownHostException {
        this(new ServerAddress(host, port), userName, password, options);
    }

    /**
     * Creates a Mongo instance based on a (single) mongodb node
     *
     * @param addr the database address
     */
//    @Deprecated
    public Mongo(ServerAddress addr) {
        this(Arrays.asList(addr));
    }

    /**
     * Creates a Mongo instance based on the given ServerAddress
     *
     * @param coords address of coord nodes
     */
//    @Deprecated
    public Mongo(List<ServerAddress> coords) {
        this(coords, null);
    }

    /**
     * Creates a Mongo instance based on a (single) mongo node using a given ServerAddress
     *
     * @param addr    the database address
     * @param options options for creating connections
     * @throws MongoException
     */
//    @Deprecated
    public Mongo(ServerAddress addr, MongoOptions options) {
        this(Arrays.asList(addr), "", "", options);
    }

    public Mongo(ServerAddress addr, String userName, String password, MongoOptions options) {
        this(Arrays.asList(addr), userName, password, options);
    }

    /**
     * Creates a Mongo instance based on a (single) mongo node using a given ServerAddress
     *
     * @param coords  address of coord nodes
     * @param options options for creating connections
     */
//    @Deprecated
    public Mongo(List<ServerAddress> coords, MongoOptions options) {
        this(coords, "", "", options);
    }

    /**
     * Specified the address of coords.
     *
     * @param coords   the coord node list
     * @param options  the options for creating connections
     * @param userName the authentication user
     * @param password the password of the authentication user
     * @throws MongoException
     */
//    @Deprecated
    public Mongo(List<ServerAddress> coords, String userName, String password,
                 MongoOptions options) {
        List<String> list = _getAddresses(coords);
        ConfigOptions configOptions = options == null ? null : options.getNetworkOptions();
        DatasourceOptions datasourceOptions = options == null ? null : options.getDatasourceOptions();
        _init(list, userName, password, configOptions, datasourceOptions);
    }

    List<String> _getAddresses(List<ServerAddress> args) {
        List<String> list = new ArrayList<String>();
        String address;

        if (args == null) return list;
        for (ServerAddress sa : args) {
            address = sa.getHost() + ":" + sa.getPort();
            list.add(address);
        }
        return list;
    }

    List<ServerAddress> _getServerAddress(List<String> args) {
        List<ServerAddress> list = new ArrayList<ServerAddress>();
        String address;

        if (args == null) return list;
        for (String addr : args) {
            String[] tmp = addr.split(":");
            try {
                list.add(new ServerAddress(tmp[0], Integer.parseInt(tmp[1])));
            } catch (UnknownHostException e) {
                // TODO:
                continue;
            }
        }
        return list;
    }

    void _init(List<String> coords, String username, String password,
               ConfigOptions configOptions, DatasourceOptions datasourceOptions) {
        _coordList = coords;
        _cm = new ConnectionManager(coords, username, password, configOptions, datasourceOptions);
    }

    ConnectionManager getConnectionManager() {
        return _cm;
    }

    public int getIdleConnCount() {
        return _cm.getIdleConnCount();
    }

    public int getUsedConnCount() {
        return _cm.getUsedConnCount();
    }

    /**
     * Gets a database object from this MongoDB instance.
     *
     * @param dbname the name of the database to retrieve
     * @return a DB representing the specified database
     */
    public DB getDB(String dbname) {
        return new DBApiLayer(this, dbname, null);
    }

    /**
     * Returns the list of databases used by the driver since this Mongo instance was created. This may include DBs that exist in the client
     * but not yet on the server.
     *
     * @return a collection of database objects
     */
    public Collection<DB> getUsedDatabases() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets a list of the names of all databases on the connected server.
     *
     * @return list of database names
     * @throws MongoException
     */
    public List<String> getDatabaseNames() {
        return _cm.execute(new DBCallback<List<String>>() {
            @Override
            public List<String> doInDB(Sequoiadb db) throws BaseException {
                List<String> list = db.getCollectionSpaceNames();
                Collections.sort(list);
                return list;
            }
        });
    }

    /**
     * Drops the database if it exists.
     *
     * @param dbName name of database to drop
     * @throws MongoException
     */
    public void dropDatabase(String dbName) {
        getDB(dbName).dropDatabase();
    }

    /**
     * gets this driver version
     *
     * @return the full version string of this driver, e.g. "2.8.0"
     */
    public String getVersion() {
        return FULL_VERSION;
    }

    /**
     * Get a String for debug purposes.
     *
     * @return a string representing the hosts used in this Mongo instance
     * @deprecated This method is NOT a part of public API and will be dropped in 3.x versions.
     */
    @Deprecated
    public String debugString() {
        return toString();
    }

    /**
     * Gets a {@code String} representation of current connection point, i.e. master.
     *
     * @return server address in a host:port form
     */
    public String getConnectPoint() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Get the status of the replica set cluster.
     *
     * @return replica set status information
     */
    public ReplicaSetStatus getReplicaSetStatus() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the address of the current master
     *
     * @return the address
     */
    public ServerAddress getAddress() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets a list of all server addresses used when this Mongo was created
     *
     * @return list of server addresses
     * @throws MongoException
     */
    public List<ServerAddress> getAllAddress() {
        List<ServerAddress> result = _getServerAddress(_coordList);
        return result;
    }

    /**
     * Gets the list of server addresses currently seen by this client. This includes addresses auto-discovered from a replica set.
     *
     * @return list of server addresses
     * @throws MongoException
     */
    public List<ServerAddress> getServerAddressList() {
        return getAllAddress();
    }

    /**
     * Closes the underlying connector, which in turn closes all open connections.
     * Once called, this Mongo instance can no longer be used.
     */
    public void close() {
        _cm.close();
    }

    /**
     * Sets the write concern for this database. Will be used as default for
     * writes to any collection in any database. See the
     * documentation for {@link WriteConcern} for more information.
     *
     * @param concern write concern to use
     */
    public void setWriteConcern(WriteConcern concern) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default write concern
     *
     * @return the default write concern
     */
    public WriteConcern getWriteConcern() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the read preference for this database. Will be used as default for
     * reads from any collection in any database. See the
     * documentation for {@link ReadPreference} for more information.
     *
     * @param preference Read Preference to use
     */
    public void setReadPreference(ReadPreference preference) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default read preference
     *
     * @return the default read preference
     */
    public ReadPreference getReadPreference() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * makes it possible to run read queries on secondary nodes
     *
     * @see ReadPreference#secondaryPreferred()
     * @deprecated Replaced with {@code ReadPreference.secondaryPreferred()}
     */
    @Deprecated
    public void slaveOk() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Add a default query option keeping any previously added options.
     *
     * @param option value to be added to current options
     */
    public void addOption(int option) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Set the default query options.  Overrides any existing options.
     *
     * @param options value to be set
     */
    public void setOptions(int options) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Reset the default query options
     */
    public void resetOptions() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default query options
     *
     * @return an int representing the options to be used by queries
     */
    public int getOptions() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the mongo options.
     *
     * @return A {@link com.mongodb.MongoOptions} containing the settings for this MongoDB instance.
     * @deprecated Please use {@link MongoClient}
     * and corresponding {@link com.mongodb.MongoClient#getMongoClientOptions()}
     */
    @Deprecated
    public MongoOptions getMongoOptions() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the maximum size for a BSON object supported by the current master server.
     * Note that this value may change over time depending on which server is master.
     * If the size is not known yet, a request may be sent to the master server
     *
     * @return the maximum size
     * @throws MongoException
     */
    public int getMaxBsonObjectSize() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Forces the master server to fsync the RAM data to disk This is done automatically by the server at intervals, but can be forced for
     * better reliability.
     *
     * @param async if true, the fsync will be done asynchronously on the server.
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual reference/command/fsync/ fsync command
     */
    public CommandResult fsync(boolean async) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Forces the master server to fsync the RAM data to disk, then lock all writes. The database will be read-only after this command
     * returns.
     *
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual reference/command/fsync/ fsync command
     */
    public CommandResult fsyncAndLock() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Unlocks the database, allowing the write operations to go through. This command may be asynchronous on the server, which means there
     * may be a small delay before the database becomes writable.
     *
     * @return {@code DBObject} in the following form {@code {"ok": 1,"info": "unlock completed"}}
     * @throws MongoException
     * @mongodb.driver.manual reference/command/fsync/ fsync command
     */
    public DBObject unlock() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns true if the database is locked (read-only), false otherwise.
     *
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual reference/command/fsync/ fsync command
     */
    public boolean isLocked() {
        throw new UnsupportedOperationException("not supported!");
    }

    // -------


    /**
     * Mongo.Holder can be used as a static place to hold several instances of Mongo.
     * Security is not enforced at this level, and needs to be done on the application side.
     */
    public static class Holder {

        /**
         * Attempts to find an existing MongoClient instance matching that URI in the holder, and returns it if exists.
         * Otherwise creates a new Mongo instance based on this URI and adds it to the holder.
         *
         * @param uri the Mongo URI
         * @return the client
         * @throws MongoException
         * @throws UnknownHostException
         * @deprecated Please use {@link #connect(MongoClientURI)} instead.
         */
        @Deprecated
        public Mongo connect(final MongoURI uri) throws UnknownHostException {
            throw new UnsupportedOperationException("not supported!");
        }

        /**
         * Attempts to find an existing MongoClient instance matching that URI in the holder, and returns it if exists.
         * Otherwise creates a new Mongo instance based on this URI and adds it to the holder.
         *
         * @param uri the Mongo URI
         * @return the client
         * @throws MongoException
         * @throws UnknownHostException
         */
        public Mongo connect(final MongoClientURI uri) throws UnknownHostException {
            throw new UnsupportedOperationException("not supported!");
        }

        private String toKey(final MongoClientURI uri) {
            return uri.toString();
        }

        public static Holder singleton() {
            return _default;
        }

        private static Holder _default = new Holder();
        private final ConcurrentMap<String, Mongo> _mongos = new ConcurrentHashMap<String, Mongo>();

    }


}
