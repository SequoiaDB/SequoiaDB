package org.springframework.data.sequoiadb.assist;

import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.logging.Logger;

import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;

/**
 * A database connection with internal connection pooling. For most applications, you should have one Sdb instance
 * for the entire JVM.
 * <p>
 * The following are equivalent, and all connect to the local database running on the default port:
 * <pre>
 * Sdb sequoiadb1 = new Sdb();
 * Sdb sequoiadb1 = new Sdb("localhost");
 * Sdb sequoiadb2 = new Sdb("localhost", 11810);
 * Sdb sequoiadb4 = new Sdb(new ServerAddress("localhost"));
 * </pre>
 * <p>
 * You can connect to a
 * <a href="http://www.sequoiadb.org/display/DOCS/Replica+Sets">replica set</a> using the Java driver by passing
 * a ServerAddress list to the Sdb constructor. For example:
 * <pre>
 * Sdb sdb = new Sdb(Arrays.asList(
 *   new ServerAddress("localhost", 11810),
 *   new ServerAddress("localhost", 27018),
 *   new ServerAddress("localhost", 27019)));
 * </pre>
 * You can connect to a sharded cluster using the same constructor.  Sdb will auto-detect whether the servers are
 * a list of replica set members or a list of sdb servers.
 * <p>
 * By default, all read and write operations will be made on the primary,
 * but it's possible to read from secondaries by changing the read preference:
 * <p>
 * <pre>
 * sdb.setReadPreference(ReadPreference.secondary());
 * </pre>
 * By default, write operations will not throw exceptions on failure, but that is easily changed too:
 * <p>
 * <pre>
 * sdb.setWriteConcern(WriteConcern.SAFE);
 * </pre>
 *
 * Note: This class has been superseded by {@code SdbClient}, and may be deprecated in a future release.
 *
 * @see SdbClient
 * @see ReadPreference
 * @see WriteConcern
 */
public class Sdb {

    static Logger logger = Logger.getLogger(Bytes.LOGGER.getName() + ".Sdb");

    /**
     * @deprecated Replaced by <code>Sdb.getMajorVersion()</code>
     */
    @Deprecated
    public static final int MAJOR_VERSION = 2;

    /**
     * @deprecated Replaced by <code>Sdb.getMinorVersion()</code>
     */
    @Deprecated
    public static final int MINOR_VERSION = 12;

    private static final String FULL_VERSION = "2.12.3";

    static int cleanerIntervalMS;

    private static final String ADMIN_DATABASE_NAME = "admin";

    static {
        cleanerIntervalMS = Integer.parseInt(System.getProperty("org.springframework.data.sequoiadb.assist.cleanerIntervalMS", "1000"));
    }

    private ConnectionManager _cm;
    private List<String> _connStrings;


    /**
     * Gets the major version of this library
     * @return the major version, e.g. 2
     *
     * @deprecated Please use {@link #getVersion()} instead.
     */
    @Deprecated
    public static int getMajorVersion() {
        return MAJOR_VERSION;
    }

    /**
     * Gets the minor version of this library
     * @return the minor version, e.g. 8
     *
     * @deprecated Please use {@link #getVersion()} instead.
     */
    @Deprecated
    public static int getMinorVersion() {
        return MINOR_VERSION;
    }

    /**
     * Connect to the SequoiaDB instance at the given address, select and return the {@code DB} specified in the {@code DBAddress} parameter.
     *
     * @param addr The details of the server and database to connect to
     * @return the DB requested in the addr parameter.
     * @throws BaseException
     * @deprecated Please use {@link SdbClient#getDB(String)} instead.
     */
    @Deprecated
    public static DB connect( DBAddress addr ){
        return new Sdb( addr ).getDB( addr.getDBName() );
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node (localhost, default port)
     * @throws UnknownHostException
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient()})
     *
     */
    @Deprecated
    public Sdb()
            throws UnknownHostException {
        this( new ServerAddress() );
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node (default port)
     * @param host server to connect to
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(String)}
     *
     */
    @Deprecated
    public Sdb(String host )
            throws UnknownHostException{
        this( new ServerAddress( host ) );
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node (default port)
     * @param host server to connect to
     * @param options default query options
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(String, SdbClientOptions)}
     *
     */
    @Deprecated
    public Sdb(String host , SequoiadbOptions options )
            throws UnknownHostException {
        this( new ServerAddress( host ) , options );
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node
     * @param host the database's host address
     * @param port the port on which the database is running
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(String, int)}
     *
     */
    @Deprecated
    public Sdb(String host , int port )
            throws UnknownHostException {
        this( new ServerAddress( host , port ) );
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node
     * @see com.sequoiadb.ServerAddress
     * @param addr the database address
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(ServerAddress)}
     *
     */
    @Deprecated
    public Sdb(ServerAddress addr ) {
        this(addr, new SequoiadbOptions());
    }

    /**
     * Creates a Sdb instance based on a (single) sdb node using a given ServerAddress
     * @see com.sequoiadb.ServerAddress
     * @param addr the database address
     * @param options default query options
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(ServerAddress, SdbClientOptions)}
     *
     */
    @Deprecated
    public Sdb(ServerAddress addr , SequoiadbOptions options ) {
        String host1 = addr.getHost() + ":" + addr.getPort();
        List<String> list = new ArrayList<String>();
        list.add(host1);
        _init(list, "", "" , null);
    }

    /**
     * <p>Creates a Sdb in paired mode. <br/> This will also work for
     * a replica set and will find all members (the master will be used by
     * default).</p>
     *
     * @see com.sequoiadb.ServerAddress
     * @param left left side of the pair
     * @param right right side of the pair
     * @throws BaseException
     */
    @Deprecated
    public Sdb(ServerAddress left , ServerAddress right ) {
        String host1 = left.getHost() + ":" + left.getPort();
        String host2 = right.getHost() + ":" + right.getPort();
        List<String> list = new ArrayList<String>();
        list.add(host1);
        list.add(host2);
        _init(list, "", "", null);
    }

    /**
     * <p>Creates a Sdb connection in paired mode. <br/> This will also work for
     * a replica set and will find all members (the master will be used by
     * default).</p>
     *
     * @see com.sequoiadb.ServerAddress
     * @param left left side of the pair
     * @param right right side of the pair
     * @param options the optional settings for the Sdb instance
     * @throws BaseException
     * @deprecated Please use {@link SdbClient#SdbClient(java.util.List, SdbClientOptions)} instead.
     */
    @Deprecated
    public Sdb(ServerAddress left , ServerAddress right , SequoiadbOptions options ) {
        this(left, right);
    }

    /**
     * Creates a Sdb based on a list of replica set members or a list of sdb.
     * It will find all members (the master will be used by default). If you pass in a single server in the list,
     * the driver will still function as if it is a replica set. If you have a standalone server,
     * use the Sdb(ServerAddress) constructor.
     * <p>
     * If this is a list of sdb servers, it will pick the closest (lowest ping time) one to send all requests to,
     * and automatically fail over to the next server if the closest is down.
     *
     * @see com.sequoiadb.ServerAddress
     * @param seeds Put as many servers as you can in the list and the system will figure out the rest.  This can
     *              either be a list of sdb servers in the same replica set or a list of sdb servers in the same
     *              sharded cluster.
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(java.util.List)}
     *
     */
    @Deprecated
    public Sdb(List<ServerAddress> seeds ) {
        this( seeds , null );
    }

    /**
     * Creates a Sdb based on a list of replica set members or a list of sdb.
     * It will find all members (the master will be used by default). If you pass in a single server in the list,
     * the driver will still function as if it is a replica set. If you have a standalone server,
     * use the Sdb(ServerAddress) constructor.
     * <p>
     * If this is a list of sdb servers, it will pick the closest (lowest ping time) one to send all requests to,
     * and automatically fail over to the next server if the closest is down.
     *
     * @see com.sequoiadb.ServerAddress
     * @param seeds Put as many servers as you can in the list and the system will figure out the rest.  This can
     *              either be a list of sdb servers in the same replica set or a list of sdb servers in the same
     *              sharded cluster.
     * @param options for configuring this Sdb instance
     * @throws BaseException
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(java.util.List, SdbClientOptions)}
     *
     */
    @Deprecated
    public Sdb(List<ServerAddress> seeds , SequoiadbOptions options ) {
        List<String> list = _getAddresses(seeds);
        _init(list, "", "", null);
    }

    /**
     * Creates a Sdb described by a URI.
     * If only one address is used it will only connect to that node, otherwise it will discover all nodes.
     * If the URI contains database credentials, the database will be authenticated lazily on first use
     * with those credentials.
     * @param uri the URI to connect to.
     * <p>examples:<ul>
     *   <li>sequoiadb://localhost</li>
     *   <li>sequoiadb://fred:foobar@localhost/</li>
     * </ul></p>
     * @throws BaseException
     * @throws UnknownHostException
     * @dochub connections
     *
     * @deprecated Replaced by {@link SdbClient#SdbClient(SdbClientURI)}
     *
     */
    @Deprecated
    public Sdb(SequoiadbURI uri ) throws UnknownHostException {
        throw new UnsupportedOperationException("not support to use URI");
    }

    List<String> _getAddresses(List<ServerAddress> args) {
        List<String> list = new ArrayList<String>();
        String address;

        if (args == null) return list;
        for(ServerAddress sa : args) {
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
            } catch(UnknownHostException e) {
                // TODO:
                continue;
            }
        }
        return list;
    }

    void _init(List<String>  connStrings, String username, String password, ConfigOptions options) {
        _connStrings = connStrings;
        _cm = new ConnectionManager(connStrings, username, password, options, null);
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
     * Gets a database object from this SequoiaDB instance.
     *
     * @param dbname the name of the database to retrieve
     * @return a DB representing the specified database
     */
    public DB getDB( String dbname ){
        return new DBApiLayer(this, dbname, null);
    }

    /**
     * Returns the list of databases used by the driver since this Sdb instance was created. This may include DBs that exist in the client
     * but not yet on the server.
     *
     * @return a collection of database objects
     */
    public Collection<DB> getUsedDatabases(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets a list of the names of all databases on the connected server.
     *
     * @return list of database names
     * @throws BaseException
     */
    public List<String> getDatabaseNames(){
       return _cm.execute(new DBCallback<List<String>>() {
           @Override
           public List<String> doInDB(Sequoiadb db) throws com.sequoiadb.exception.BaseException {
               List<String > list = db.getCollectionSpaceNames();
               Collections.sort(list);
               return list;
           }
       });
    }

    /**
     * Drops the database if it exists.
     * @param dbName name of database to drop
     * @throws BaseException
     */
    public void dropDatabase(String dbName){
        getDB( dbName ).dropDatabase();
    }

    /**
     * gets this driver version
     * @return the full version string of this driver, e.g. "2.8.0"
     */
    public String getVersion(){
        return FULL_VERSION;
    }

    /**
     * Get a String for debug purposes.
     *
     * @return a string representing the hosts used in this Sdb instance
     * @deprecated This method is NOT a part of public API and will be dropped in 3.x versions.
     */
    @Deprecated
    public String debugString(){
        return toString();
    }

    /**
     * Gets a {@code String} representation of current connection point, i.e. master.
     *
     * @return server address in a host:port form
     */
    public String getConnectPoint(){
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
     * @return the address
     */
    public ServerAddress getAddress(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets a list of all server addresses used when this Sdb was created
     *
     * @return list of server addresses
     * @throws BaseException
     */
    public List<ServerAddress> getAllAddress() {
        List<ServerAddress> result = _getServerAddress(_connStrings);
        return result;
    }

    /**
     * Gets the list of server addresses currently seen by this client. This includes addresses auto-discovered from a replica set.
     *
     * @return list of server addresses
     * @throws BaseException
     */
    public List<ServerAddress> getServerAddressList() {
        return getAllAddress();
    }

    /**
     * Closes the underlying connector, which in turn closes all open connections.
     * Once called, this Sdb instance can no longer be used.
     */
    public void close(){
        _cm.close();
    }

    /**
     * Sets the write concern for this database. Will be used as default for
     * writes to any collection in any database. See the
     * documentation for {@link WriteConcern} for more information.
     *
     * @param concern write concern to use
     */
    public void setWriteConcern( WriteConcern concern ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default write concern
     *
     * @return the default write concern
     */
    public WriteConcern getWriteConcern(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the read preference for this database. Will be used as default for
     * reads from any collection in any database. See the
     * documentation for {@link ReadPreference} for more information.
     *
     * @param preference Read Preference to use
     */
    public void setReadPreference( ReadPreference preference ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default read preference
     *
     * @return the default read preference
     */
    public ReadPreference getReadPreference(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * makes it possible to run read queries on secondary nodes
     *
     * @deprecated Replaced with {@code ReadPreference.secondaryPreferred()}
     * @see ReadPreference#secondaryPreferred()
     */
    @Deprecated
    public void slaveOk(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Add a default query option keeping any previously added options.
     *
     * @param option value to be added to current options
     */
    public void addOption( int option ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Set the default query options.  Overrides any existing options.
     *
     * @param options value to be set
     */
    public void setOptions( int options ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Reset the default query options
     */
    public void resetOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default query options
     *
     * @return an int representing the options to be used by queries
     */
    public int getOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the sdb options.
     *
     * @deprecated Please use {@link SdbClient}
     *             and corresponding {@link com.sequoiadb.SdbClient#getSdbClientOptions()}
     * @return A {@link com.sequoiadb.SequoiadbOptions} containing the settings for this SequoiaDB instance.
     */
    @Deprecated
    public SequoiadbOptions getSequoiadbOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the maximum size for a BSON object supported by the current master server.
     * Note that this value may change over time depending on which server is master.
     * If the size is not known yet, a request may be sent to the master server
     * @return the maximum size
     * @throws BaseException
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
     * @throws BaseException
     */
    public CommandResult fsync(boolean async) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Forces the master server to fsync the RAM data to disk, then lock all writes. The database will be read-only after this command
     * returns.
     *
     * @return result of the command execution
     * @throws BaseException
     */
    public CommandResult fsyncAndLock() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Unlocks the database, allowing the write operations to go through. This command may be asynchronous on the server, which means there
     * may be a small delay before the database becomes writable.
     *
     * @return {@code BSONObject} in the following form {@code {"ok": 1,"info": "unlock completed"}}
     * @throws BaseException
     */
    public BSONObject unlock() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns true if the database is locked (read-only), false otherwise.
     *
     * @return result of the command execution
     * @throws BaseException
     */
    public boolean isLocked() {
        throw new UnsupportedOperationException("not supported!");
    }

    // -------


    /**
     * Sdb.Holder can be used as a static place to hold several instances of Sdb.
     * Security is not enforced at this level, and needs to be done on the application side.
     */
    public static class Holder {

        /**
         * Attempts to find an existing SdbClient instance matching that URI in the holder, and returns it if exists.
         * Otherwise creates a new Sdb instance based on this URI and adds it to the holder.
         *
         * @param uri the Sdb URI
         * @return the client
         * @throws BaseException
         * @throws UnknownHostException
         *
         * @deprecated Please use {@link #connect(SdbClientURI)} instead.
         */
        @Deprecated
        public Sdb connect(final SequoiadbURI uri) throws UnknownHostException {
            throw new UnsupportedOperationException("not supported!");
        }

        /**
         * Attempts to find an existing SdbClient instance matching that URI in the holder, and returns it if exists.
         * Otherwise creates a new Sdb instance based on this URI and adds it to the holder.
         *
         * @param uri the Sdb URI
         * @return the client
         * @throws BaseException
         * @throws UnknownHostException
         */
        public Sdb connect(final SdbClientURI uri) throws UnknownHostException {
            throw new UnsupportedOperationException("not supported!");
        }

        private String toKey(final SdbClientURI uri) {
            return uri.toString();
        }

        public static Holder singleton() { return _default; }

        private static Holder _default = new Holder();
        private final ConcurrentMap<String,Sdb> _sequoiadbs = new ConcurrentHashMap<String,Sdb>();

    }



}
