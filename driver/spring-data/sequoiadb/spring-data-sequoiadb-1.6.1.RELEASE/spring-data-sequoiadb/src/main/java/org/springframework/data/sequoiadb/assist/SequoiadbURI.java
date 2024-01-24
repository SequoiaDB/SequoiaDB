package org.springframework.data.sequoiadb.assist;

import java.net.UnknownHostException;
import java.util.List;

/**
 * Represents a <a href="http://www.sequoiadb.org/display/DOCS/Connections">URI</a>
 * which can be used to create a Sdb instance. The URI describes the hosts to
 * be used and options.
 * <p>
 * This class has been superseded by <{@code SdbClientURI}, and may be deprecated in a future release.
 * <p>The format of the URI is:
 * <pre>
 *   sequoiadb://[username:password@]host1[:port1][,host2[:port2],...[,hostN[:portN]]][/[database][?options]]
 * </pre>
 * <ul>
 *   <li>{@code sequoiadb://} is a required prefix to identify that this is a string in the standard connection format.</li>
 *   <li>{@code username:password@} are optional.  If given, the driver will attempt to login to a database after
 *       connecting to a database server.</li>
 *   <li>{@code host1} is the only required part of the URI.  It identifies a server address to connect to.</li>
 *   <li>{@code :portX} is optional and defaults to :11810 if not provided.</li>
 *   <li>{@code /database} is the name of the database to login to and thus is only relevant if the
 *       {@code username:password@} syntax is used. If not specified the "admin" database will be used by default.</li>
 *   <li>{@code ?options} are connection options. Note that if {@code database} is absent there is still a {@code /}
 *       required between the last host and the {@code ?} introducing the options. Options are name=value pairs and the pairs
 *       are separated by "&amp;". For backwards compatibility, ";" is accepted as a separator in addition to "&amp;",
 *       but should be considered as deprecated.</li>
 * </ul>
 * <p>
 *     The Java driver supports the following options (case insensitive):
 * <p>
 *     Replica set configuration:
 * </p>
 * <ul>
 *   <li>{@code replicaSet=name}: Implies that the hosts given are a seed list, and the driver will attempt to find
 *        all members of the set.</li>
 * </ul>
 * <p>Connection Configuration:</p>
 * <ul>
 *   <li>{@code connectTimeoutMS=ms}: How long a connection can take to be opened before timing out.</li>
 *   <li>{@code socketTimeoutMS=ms}: How long a send or receive on a socket can take before timing out.</li>
 * </ul>
 * <p>Connection pool configuration:</p>
 * <ul>
 *   <li>{@code maxPoolSize=n}: The maximum number of connections in the connection pool.</li>
 *   <li>{@code waitQueueMultiple=n} : this multiplier, multiplied with the maxPoolSize setting, gives the maximum number of
 *       threads that may be waiting for a connection to become available from the pool.  All further threads will get an
 *       exception right away.</li>
 *   <li>{@code waitQueueTimeoutMS=ms}: The maximum wait time in milliseconds that a thread may wait for a connection to
 *       become available.</li>
 * </ul>
 * <p>Write concern configuration:</p>
 * <ul>
 *   <li>{@code safe=true|false}
 *     <ul>
 *       <li>{@code true}: the driver sends a getLastError command after every update to ensure that the update succeeded
 *           (see also {@code w} and {@code wtimeoutMS}).</li>
 *       <li>{@code false}: the driver does not send a getLastError command after every update.</li>
 *     </ul>
 *   </li>
 *   <li>{@code w=wValue}
 *     <ul>
 *       <li>The driver adds { w : wValue } to the getLastError command. Implies {@code safe=true}.</li>
 *       <li>wValue is typically a number, but can be any string in order to allow for specifications like
 *           {@code "majority"}</li>
 *     </ul>
 *   </li>
 *   <li>{@code wtimeoutMS=ms}
 *     <ul>
 *       <li>The driver adds { wtimeout : ms } to the getlasterror command. Implies {@code safe=true}.</li>
 *       <li>Used in combination with {@code w}</li>
 *     </ul>
 *   </li>
 * </ul>
 * <p>Read preference configuration:</p>
 * <ul>
 *   <li>{@code slaveOk=true|false}: Whether a driver connected to a replica set will send reads to slaves/secondaries.</li>
 *   <li>{@code readPreference=enum}: The read preference for this connection.  If set, it overrides any slaveOk value.
 *     <ul>
 *       <li>Enumerated values:
 *         <ul>
 *           <li>{@code primary}</li>
 *           <li>{@code primaryPreferred}</li>
 *           <li>{@code secondary}</li>
 *           <li>{@code secondaryPreferred}</li>
 *           <li>{@code nearest}</li>
 *         </ul>
 *       </li>
 *     </ul>
 *   </li>
 *   <li>{@code readPreferenceTags=string}.  A representation of a tag set as a comma-separated list of colon-separated
 *       key-value pairs, e.g. {@code "dc:ny,rack:1}".  Spaces are stripped from beginning and end of all keys and values.
 *       To specify a list of tag sets, using multiple readPreferenceTags,
 *       e.g. {@code readPreferenceTags=dc:ny,rack:1;readPreferenceTags=dc:ny;readPreferenceTags=}
 *     <ul>
 *        <li>Note the empty value for the last one, which means match any secondary as a last resort.</li>
 *        <li>Order matters when using multiple readPreferenceTags.</li>
 *     </ul>
 *   </li>
 * </ul>
 * @see SdbClientURI
 * @see SequoiadbOptions for the default values for all options
 */
public class SequoiadbURI {

    /**
     * The prefix for sequoiadb URIs.
     */
    public static final String SEQUOIADB_PREFIX = "sequoiadb://";

    private final SdbClientURI sdbClientURI;
    private final SequoiadbOptions sequoiadbOptions;

    /**
     * Creates a SequoiadbURI from a string.
     * @param uri the URI
     * @dochub connections
     *
     * @deprecated Replaced by {@link SdbClientURI#SdbClientURI(String)}
     *
     */
    @Deprecated
    public SequoiadbURI(String uri ) {
        throw new UnsupportedOperationException("not supported!");
    }

    @Deprecated
    public SequoiadbURI(final SdbClientURI sdbClientURI) {
        throw new UnsupportedOperationException("not supported!");
    }

    // ---------------------------------

    /**
     * Gets the username
     * @return
     */
    public String getUsername(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the password
     * @return
     */
    public char[] getPassword(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the list of hosts
     * @return
     */
    public List<String> getHosts(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the database name
     * @return
     */
    public String getDatabase(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the collection name
     * @return
     */
    public String getCollection(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the credentials
     *
     * @since 2.11.0
     */
    public SequoiadbCredential getCredentials() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the options.  This method will return the same instance of {@code SequoiadbOptions} for every call, so it's
     * possible to mutate the returned instance to change the defaults.
     * @return the sdb options
     */
    public SequoiadbOptions getOptions(){
        return sequoiadbOptions;
    }

    /**
     * creates a Sdb instance based on the URI
     * @return a new Sdb instance.  There is no caching, so each call will create a new instance, each of which
     * must be closed manually.
     * @throws BaseException
     * @throws UnknownHostException
     */
    @SuppressWarnings("deprecation")
    public Sdb connect()
            throws UnknownHostException {
        // TODO caching?
        // Note: we can't change this to new SdbClient(this) as that would silently change the default write concern.
        return new Sdb(this);
    }

    /**
     * returns the DB object from a newly created Sdb instance based on this URI
     * @return the database specified in the URI.  This will implicitly create a new Sdb instance,
     * which must be closed manually.
     * @throws BaseException
     * @throws UnknownHostException
     */
    public DB connectDB() throws UnknownHostException {
        return connect().getDB(getDatabase());
    }

    /**
     * returns the URI's DB object from a given Sdb instance
     * @param sdb the Sdb instance to get the database from.
     * @return the database specified in this URI
     */
    public DB connectDB( Sdb sdb){
        return sdb.getDB( getDatabase() );
    }

    /**
     * returns the URI's Collection from a given DB object
     * @param db the database to get the collection from
     * @return
     */
    public DBCollection connectCollection( DB db ){
        return db.getCollection( getCollection() );
    }

    /**
     * returns the URI's Collection from a given Sdb instance
     * @param sdb the sdb instance to get the collection from
     * @return the collection specified in this URI
     */
    public DBCollection connectCollection( Sdb sdb){
        return connectDB(sdb).getCollection( getCollection() );
    }

    // ---------------------------------

    @Override
    public String toString() {
        return sdbClientURI.toString();
    }

    SdbClientURI toClientURI() {
        return sdbClientURI;
    }
}

