package org.springframework.data.sequoiadb.assist;

import java.net.UnknownHostException;
import java.util.List;

/**
 * A SequoiaDB client with internal connection pooling. For most applications, you should have one SdbClient instance
 * for the entire JVM.
 * <p>
 * The following are equivalent, and all connect to the local database running on the default port:
 * <pre>
 * SdbClient sdbClient1 = new SdbClient();
 * SdbClient sdbClient1 = new SdbClient("localhost");
 * SdbClient sdbClient2 = new SdbClient("localhost", 11810);
 * SdbClient sdbClient4 = new SdbClient(new ServerAddress("localhost"));
 * SdbClient sdbClient5 = new SdbClient(new ServerAddress("localhost"), new SdbClientOptions.Builder().build());
 * </pre>
 * <p>
 * You can connect to a
 * <a href="http://www.sequoiadb.org/display/DOCS/Replica+Sets">replica set</a> using the Java driver by passing
 * a ServerAddress list to the SdbClient constructor. For example:
 * <pre>
 * SdbClient sdbClient = new SdbClient(Arrays.asList(
 *   new ServerAddress("localhost", 11810),
 *   new ServerAddress("localhost", 27018),
 *   new ServerAddress("localhost", 27019)));
 * </pre>
 * You can connect to a sharded cluster using the same constructor.  SdbClient will auto-detect whether the servers are
 * a list of replica set members or a list of sdb servers.
 * <p>
 * By default, all read and write operations will be made on the primary, but it's possible to read from secondaries
 * by changing the read preference:
 * <pre>
 * sdbClient.setReadPreference(ReadPreference.secondaryPreferred());
 * </pre>
 * By default, all write operations will wait for acknowledgment by the server, as the default write concern is
 * {@code WriteConcern.ACKNOWLEDGED}.
 * <p>
 * Note: This class supersedes the {@code Sdb} class.  While it extends {@code Sdb}, it differs from it in that
 * the default write concern is to wait for acknowledgment from the server of all write operations.  In addition, its
 * constructors accept instances of {@code SdbClientOptions} and {@code SdbClientURI}, which both also
 * set the same default write concern.
 * <p>
 * In general, users of this class will pick up all of the default options specified in {@code SdbClientOptions}.  In
 * particular, note that the default value of the connectionsPerHost option has been increased to 100 from the old
 * default value of 10 used by the superseded {@code Sdb} class.
 *
 * @see ReadPreference#primary()
 * @see com.sequoiadb.WriteConcern#ACKNOWLEDGED
 * @see SdbClientOptions
 * @see SdbClientURI
 * @since 2.10.0
 */
public class SdbClient extends Sdb {

    private final SdbClientOptions options;

    /**
     * Creates an instance based on a (single) sequoiadb node (localhost, default port).
     *
     * @throws UnknownHostException
     * @throws BaseException
     */
    public SdbClient() throws UnknownHostException {
        this(new ServerAddress());
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node.
     *
     * @param host server to connect to in format host[:port]
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws BaseException
     */
    public SdbClient(String host) throws UnknownHostException {
        this(new ServerAddress(host));
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node (default port).
     *
     * @param host    server to connect to in format host[:port]
     * @param options default query options
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws BaseException
     */
    public SdbClient(String host, SdbClientOptions options) throws UnknownHostException {
        this(new ServerAddress(host), options);
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node.
     *
     * @param host the database's host address
     * @param port the port on which the database is running
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws BaseException
     */
    public SdbClient(String host, int port) throws UnknownHostException {
        this(new ServerAddress(host, port));
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node
     *
     * @param addr the database address
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     */
    public SdbClient(ServerAddress addr) {
        this(addr, new SdbClientOptions.Builder().build());
    }

    /**
     * Creates a Sdb instance based on a (single) sequoiadb node and a list of credentials
     *
     * @param addr the database address
     * @param credentialsList the list of credentials used to authenticate all connections
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     * @since 2.11.0
     */
    public SdbClient(ServerAddress addr, List<SequoiadbCredential> credentialsList) {
        this(addr, credentialsList, new SdbClientOptions.Builder().build());
    }

    /**
     * Creates a Sdb instance based on a (single) sdb node using a given ServerAddress and default options.
     *
     * @param addr    the database address
     * @param options default options
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     */
    public SdbClient(ServerAddress addr, SdbClientOptions options) {
        this(addr, null, options);
    }

    /**
     * Creates a Sdb instance based on a (single) sdb node using a given ServerAddress and default options.
     *
     * @param addr    the database address
     * @param credentialsList the list of credentials used to authenticate all connections
     * @param options default options
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     * @since 2.11.0
     */
    @SuppressWarnings("deprecation")
    public SdbClient(ServerAddress addr, List<SequoiadbCredential> credentialsList, SdbClientOptions options) {
        super(addr);
        this.options = options;
    }

    /**
     * Creates a Sdb based on a list of replica set members or a list of sdb.
     * It will find all members (the master will be used by default). If you pass in a single server in the list,
     * the driver will still function as if it is a replica set. If you have a standalone server,
     * use the Sdb(ServerAddress) constructor.
     * <p/>
     * If this is a list of sdb servers, it will pick the closest (lowest ping time) one to send all requests to,
     * and automatically fail over to the next server if the closest is down.
     *
     * @param seeds Put as many servers as you can in the list and the system will figure out the rest.  This can
     *              either be a list of sdb servers in the same replica set or a list of sdb servers in the same
     *              sharded cluster.
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     */
    public SdbClient(List<ServerAddress> seeds) {
        this(seeds, null, new SdbClientOptions.Builder().build());
    }

    /**
     * Creates a Sdb based on a list of replica set members or a list of sdb.
     * It will find all members (the master will be used by default). If you pass in a single server in the list,
     * the driver will still function as if it is a replica set. If you have a standalone server,
     * use the Sdb(ServerAddress) constructor.
     * <p/>
     * If this is a list of sdb servers, it will pick the closest (lowest ping time) one to send all requests to,
     * and automatically fail over to the next server if the closest is down.
     *
     * @param seeds Put as many servers as you can in the list and the system will figure out the rest.  This can
     *              either be a list of sdb servers in the same replica set or a list of sdb servers in the same
     *              sharded cluster. \
     * @param credentialsList the list of credentials used to authenticate all connections
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     * @since 2.11.0
     */
    public SdbClient(List<ServerAddress> seeds, List<SequoiadbCredential> credentialsList) {
        this(seeds, credentialsList, new SdbClientOptions.Builder().build());
    }


    /**
     * Creates a Sdb based on a list of replica set members or a list of sdb.
     * It will find all members (the master will be used by default). If you pass in a single server in the list,
     * the driver will still function as if it is a replica set. If you have a standalone server,
     * use the Sdb(ServerAddress) constructor.
     * <p/>
     * If this is a list of sdb servers, it will pick the closest (lowest ping time) one to send all requests to,
     * and automatically fail over to the next server if the closest is down.
     *
     * @param seeds   Put as many servers as you can in the list and the system will figure out the rest.  This can
     *                either be a list of sdb servers in the same replica set or a list of sdb servers in the same
     *                sharded cluster.
     * @param options default options
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     */
    public SdbClient(List<ServerAddress> seeds, SdbClientOptions options) {
        this(seeds, null, options);
    }

    /**
     * Creates a Sdb based on a list of replica set members or a list of sdb.
     * It will find all members (the master will be used by default). If you pass in a single server in the list,
     * the driver will still function as if it is a replica set. If you have a standalone server,
     * use the Sdb(ServerAddress) constructor.
     * <p/>
     * If this is a list of sdb servers, it will pick the closest (lowest ping time) one to send all requests to,
     * and automatically fail over to the next server if the closest is down.
     *
     * @param seeds   Put as many servers as you can in the list and the system will figure out the rest.  This can
     *                either be a list of sdb servers in the same replica set or a list of sdb servers in the same
     *                sharded cluster.
     * @param credentialsList the list of credentials used to authenticate all connections
     * @param options default options
     * @throws BaseException
     * @see com.sequoiadb.ServerAddress
     * @since 2.11.0
     */
    @SuppressWarnings("deprecation")
    public SdbClient(List<ServerAddress> seeds, List<SequoiadbCredential> credentialsList, SdbClientOptions options) {
        super(seeds);
        this.options = options;
    }


    /**
     * Creates a Sdb described by a URI.
     * If only one address is used it will only connect to that node, otherwise it will discover all nodes.
     * @param uri the URI
     * @throws BaseException
     * @throws UnknownHostException
     * @see SequoiadbURI
     * @dochub connections
     */
    @SuppressWarnings("deprecation")
    public SdbClient(SdbClientURI uri) throws UnknownHostException {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the list of credentials that this client authenticates all connections with
     *
     * @return the list of credentials
     * @since 2.11.0
     */
    public List<SequoiadbCredential> getCredentialsList() {
        throw new UnsupportedOperationException("not supported!");
    }

    public SdbClientOptions getSdbClientOptions() {
        throw new UnsupportedOperationException("not supported!");
    }
}
