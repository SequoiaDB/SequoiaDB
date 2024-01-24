package org.springframework.data.mongodb.assist;

// DB.java

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;

import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * A thread-safe client view of a logical database in a MongoDB cluster. A DB instance can be achieved from a {@link MongoClient} instance
 * using code like:
 * <pre>
 * {@code
 * MongoClient mongoClient = new MongoClient();
 * DB db = mongoClient.getDB("<db name>");
 * }</pre>
 *
 * @mongodb.driver.manual reference/glossary/#term-database Database
 * @see MongoClient
 */
public abstract class DB {

    private final Mongo _mongo;
    private final String _csName;
    private final ConnectionManager _cm;

    /**
     * Constructs a new instance of the {@code DB}.
     *
     * @param mongo the mongo instance
     * @param name  the database name
     */
    public DB( Mongo mongo , String name ){
        if(name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Invalid database name format. Database name is either empty or it contains spaces.");
        }
        _mongo = mongo;
        _csName = name;
        _cm = _mongo.getConnectionManager();
    }

//    /**
//     * Determines the read preference that should be used for the given command.
//     *
//     * @param command             the {@link DBObject} representing the command
//     * @param requestedPreference the preference requested by the client.
//     * @return the read preference to use for the given command.  It will never return {@code null}.
//     * @see com.mongodb.ReadPreference
//     */
//    ReadPreference getCommandReadPreference(DBObject command, ReadPreference requestedPreference){
//        throw new UnsupportedOperationException("not supported!");
//    }

    /**
     * Starts a new 'consistent request'.
     * <p/>
     * Following this call and until {@link com.mongodb.DB#requestDone()} is called,
     * all db operations will use the same underlying connection.
     * <p/>
     * This is useful to ensure that operations happen in a certain order with predictable results.
     */
    public abstract void requestStart();

    /**
     * Ends the current 'consistent request'.
     */
    public abstract void requestDone();

    /**
     * Ensure that a connection is assigned to the current 'consistent request'
     * (from primary pool, if connected to a replica set)
     */
    public abstract void requestEnsureConnection();

    /**
     * Gets a collection with a given name.
     * If the collection does not exist, a new collection is created.
     * <p/>
     * This class is NOT part of the public API.  Be prepared for non-binary compatible changes in minor releases.
     *
     * @param name the name of the collection
     * @return the collection
     */
    protected abstract DBCollection doGetCollection( String name );

    /**
     * Gets a collection with a given name.
     * If the collection does not exist, a new collection is created.
     *
     * @param name the name of the collection to return
     * @return the collection
     */
    public DBCollection getCollection( String name ){
        DBCollection c = doGetCollection( name );
        return c;
    }

    /**
     * Creates a collection with a given name and options.
     * If the collection does not exist, a new collection is created.
     * <p/>
     * Possible options:
     * <ul>
     * <li>
     * <b>capped</b> ({@code boolean}) - Enables a collection cap.
     * False by default. If enabled, you must specify a size parameter.
     * </li>
     * <li>
     * <b>size</b> ({@code int}) - If capped is true, size specifies a maximum size in bytes for the capped collection.
     * When capped is false, you may use size to preallocate space.
     * </li>
     * <li>
     * <b>max</b> ({@code int}) -   Optional. Specifies a maximum "cap" in number of documents for capped collections.
     * You must also specify size when specifying max.
     * </li>
     * <p/>
     * </ul>
     * <p/>
     * Note that if the {@code options} parameter is {@code null},
     * the creation will be deferred to when the collection is written to.
     *
     * @param name    the name of the collection to return
     * @param options options
     * @return the collection
     * @throws MongoException
     */
    public DBCollection createCollection( String name, DBObject options ){
        final String myName = name;
        final BSONObject myOptions = options;

        return _cm.execute(_csName, new CSCallback<DBCollection>(){
            @Override
            public DBCollection doInCS(CollectionSpace cs) throws BaseException {
                cs.createCollection(myName, myOptions);
                return new DBCollectionImpl(new DBApiLayer(_mongo, _csName, null), myName);
            }
        });
    }

    /**
     * Returns a collection matching a given string.
     *
     * @param s the name of the collection
     * @return the collection
     */
    public DBCollection getCollectionFromString( String s ){
        return getCollection( s );
    }

    /**
     * Executes a database command.
     * This method calls {@link DB#command(DBObject, int)} } with 0 as query option.
     *
     * @param cmd {@code DBObject} representation of the command to be executed
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     */
    public CommandResult command( DBObject cmd ){
        throw new UnsupportedOperationException("not supported!");
    }


    /**
     * Executes DQL sql command in the database.
     *
     * @param selectSqlCommand the DQL sql command, e.g. "select * from foo.bar".
     * @return result set.
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     */
    public DBCursor executeSelectSql(final String selectSqlCommand) {
         IQueryResult result = _cm.execute(new QueryInSessionCallback<IQueryResult>() {
            @Override
            public IQueryResult doQuery(Sequoiadb db) throws BaseException {
                com.sequoiadb.base.DBCursor cursor = db.exec(selectSqlCommand);
                return new DBQueryResult(cursor, db);
            }
        });
        return new DBCursor(new QueryResultIterator(_cm, result));
    }

    /**
     * Executes DML and DDL sql command in the database.
     *
     * @param otherSqlCommand the DML and DDL sql command, e.g. ""create collectionspace foo" or "".
     * @return result set.
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     */
    public void executeOtherSql(final String otherSqlCommand) {
        _cm.execute(new DBCallback<Void>(){
            @Override
            public Void doInDB(Sequoiadb db) throws BaseException {
                db.execUpdate(otherSqlCommand);
                return null;
            }
        });
    }

    /**
     * Executes a database command.
     * This method calls {@link DB#command(com.mongodb.DBObject, int, com.mongodb.DBEncoder) } with 0 as query option.
     *
     * @param cmd     {@code DBObject} representation of the command to be executed
     * @param encoder {@link DBEncoder} to be used for command encoding
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     */
    public CommandResult command( DBObject cmd, DBEncoder encoder ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command. This method calls
     * {@link DB#command(com.mongodb.DBObject, int, com.mongodb.ReadPreference, com.mongodb.DBEncoder) } with the database default read
     * preference.  The only option used by this method was "slave ok", therefore this method has been replaced with
     * {@link com.mongodb.DB#command(DBObject, ReadPreference, DBEncoder)}.
     *
     * @param cmd     {@code DBObject} representation the command to be executed
     * @param options query options to use
     * @param encoder {@link DBEncoder} to be used for command encoding
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @deprecated Use {@link com.mongodb.DB#command(DBObject, ReadPreference, DBEncoder)} instead.  This method will be removed in 3.0.
     */
    @Deprecated
    public CommandResult command( DBObject cmd , int options, DBEncoder encoder ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command. This method calls
     * {@link DB#command(com.mongodb.DBObject, int, com.mongodb.ReadPreference, com.mongodb.DBEncoder) } with a default encoder.  The only
     * option used by this method was "slave ok", therefore this method has been replaced
     * with {@link com.mongodb.DB#command(DBObject, ReadPreference)}.
     *
     * @param cmd            A {@code DBObject} representation the command to be executed
     * @param options        The query options to use
     * @param readPreference The {@link ReadPreference} for this command (nodes selection is the biggest part of this)
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @deprecated Use {@link com.mongodb.DB#command(DBObject, ReadPreference)} instead.  This method will be removed in 3.0.
     */
    @Deprecated
    public CommandResult command( DBObject cmd , int options, ReadPreference readPreference ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command.  The only option used by this method was "slave ok", therefore this method has been replaced with {@link
     * com.mongodb.DB#command(DBObject, ReadPreference, DBEncoder)}.
     *
     * @param cmd            A {@code DBObject} representation the command to be executed
     * @param options        The query options to use
     * @param readPreference The {@link ReadPreference} for this command (nodes selection is the biggest part of this)
     * @param encoder        A {@link DBEncoder} to be used for command encoding
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @deprecated Use {@link com.mongodb.DB#command(DBObject, ReadPreference, DBEncoder)} instead.  This method will be removed in 3.0.
     */
    @Deprecated
    public CommandResult command( DBObject cmd , int options, ReadPreference readPreference, DBEncoder encoder ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command with the selected readPreference, and encodes the command using the given encoder.
     *
     * @param cmd            The {@code DBObject} representation the command to be executed
     * @param readPreference Where to execute the command - this will only be applied for a subset of commands
     * @param encoder        The DBEncoder that knows how to serialise the cmd
     * @return The result of executing the command, success or failure
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @since 2.12
     */
    public CommandResult command( final DBObject cmd , final ReadPreference readPreference, final DBEncoder encoder ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command with the given query options.  The only option used by this method was "slave ok", therefore this method
     * has been replaced with {@link com.mongodb.DB#command(DBObject, ReadPreference)}.
     *
     * @param cmd     The {@code DBObject} representation the command to be executed
     * @param options The query options to use
     * @return The result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @deprecated Use {@link com.mongodb.DB#command(DBObject, ReadPreference)} instead.  This method will be removed in 3.0.
     */
    @Deprecated
    public CommandResult command(DBObject cmd, int options) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes the command against the database with the given read preference.  This method is the preferred way of setting read
     * preference, use this instead of {@link DB#command(com.mongodb.DBObject, int) }
     *
     * @param cmd            The {@code DBObject} representation the command to be executed
     * @param readPreference Where to execute the command - this will only be applied for a subset of commands
     * @return The result of executing the command, success or failure
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @since 2.12
     */
    public CommandResult command(final DBObject cmd, final ReadPreference readPreference) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command. This method constructs a simple dbobject and calls {@link DB#command(com.mongodb.DBObject) }
     *
     * @param cmd name of the command to be executed
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     */
    public CommandResult command( String cmd ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command. This method constructs a simple dbobject and calls {@link DB#command(com.mongodb.DBObject, int)  }
     *
     * @param cmd     name of the command to be executed
     * @param options query options to use
     * @return result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @deprecated Use {@link com.mongodb.DB#command(String, ReadPreference)} instead.  This method will be removed in 3.0.
     */
    @Deprecated
    public CommandResult command( String cmd, int options  ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Executes a database command. This method constructs a simple dbobject and calls {@link DB#command(com.mongodb.DBObject, int,
     * com.mongodb.ReadPreference)  }. The only option used by this method was "slave ok", therefore this method has been replaced with
     * {@link com.mongodb.DB#command(DBObject, ReadPreference)}.
     *
     * @param cmd            The name of the command to be executed
     * @param readPreference Where to execute the command - this will only be applied for a subset of commands
     * @return The result of the command execution
     * @throws MongoException
     * @mongodb.driver.manual tutorial/use-database-commands Commands
     * @since 2.12
     */
    public CommandResult command(final String cmd, final ReadPreference readPreference) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Evaluates JavaScript functions on the database server.
     * This is useful if you need to touch a lot of data lightly, in which case network transfer could be a bottleneck.
     *
     * @param code @{code String} representation of JavaScript function
     * @param args arguments to pass to the JavaScript function
     * @return result of the command execution
     * @throws MongoException
     */
    public CommandResult doEval( String code , Object ... args ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Calls {@link DB#doEval(java.lang.String, java.lang.Object[]) }.
     * If the command is successful, the "retval" field is extracted and returned.
     * Otherwise an exception is thrown.
     *
     * @param code @{code String} representation of JavaScript function
     * @param args arguments to pass to the JavaScript function
     * @return result of the execution
     * @throws MongoException
     */
    public Object eval( String code , Object ... args ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Helper method for calling a 'dbStats' command.
     * It returns storage statistics for a given database.
     *
     * @return result of the execution
     * @throws MongoException
     */
    public CommandResult getStats() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the name of this database.
     *
     * @return the name
     */
    public String getName(){
        return _csName;
    }

    /**
     * Makes this database read-only.
     * Important note: this is a convenience setting that is only known on the client side and not persisted.
     *
     * @param b if the database should be read-only
     * @deprecated Avoid making database read-only via this method.
     *             Connect with a user credentials that has a read-only access to a server instead.
     */
    @Deprecated
    public void setReadOnly(Boolean b) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns a set containing all collections in the existing database.
     *
     * @return an set of names
     * @throws MongoException
     */
    public Set<String> getCollectionNames(){

        return _cm.execute(_csName, new CSCallback<Set<String>>() {
            @Override
            public Set<String> doInCS(CollectionSpace cs) throws BaseException {
                List<String> names = cs.getCollectionNames();
                Collections.sort(names);

                return new LinkedHashSet<String>(names);
            }
        });
    }

    /**
     * Checks to see if a collection with a given name exists on a server.
     *
     * @param collectionName a name of the collection to test for existence
     * @return {@code false} if no collection by that name exists, {@code true} if a match to an existing collection was found
     * @throws MongoException
     */
    public boolean collectionExists(String collectionName)
    {
        if (collectionName == null || "".equals(collectionName))
            return false;

        String collectionFullName = _csName + "." + collectionName;
        Set<String> collections = getCollectionNames();
        if (collections.isEmpty())
            return false;

        for (String collection : collections)
        {
            if (collectionFullName.equalsIgnoreCase(collection))
                return true;
        }

        return false;
    }


    /**
     * Returns the name of this database.
     *
     * @return the name
     */
    @Override
    public String toString(){
        return _csName;
    }

    /**
     * Returns the error status of the last operation on the current connection. The result of this command will look like:
     * <pre>
     * {@code
     * { "err" :  errorMessage  , "ok" : 1.0 }
     * }</pre>
     * The value for errorMessage will be null if no error occurred, or a description otherwise.
     * <p> Important note: when calling this method directly, it is undefined which connection "getLastError" is called on. You may need
     * to explicitly use a "consistent Request", see {@link DB#requestStart()} It is better not to call this method directly but instead
     * use {@link WriteConcern} </p>
     *
     * @return {@code DBObject} with error and status information
     * @throws MongoException
     * @see WriteConcern#ACKNOWLEDGED
     * @deprecated The getlasterror command will not be supported in future versions of MongoDB.  Use acknowledged writes instead.
     */
    @Deprecated
    public CommandResult getLastError(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the error status of the last operation on the current connection.
     *
     * @param concern a {@link WriteConcern} to be used while checking for the error status.
     * @return {@code DBObject} with error and status information
     * @throws MongoException
     * @deprecated The getlasterror command will not be supported in future versions of MongoDB.  Use acknowledged writes instead.
     * @see WriteConcern#ACKNOWLEDGED
     */
    @Deprecated
    public CommandResult getLastError( WriteConcern concern ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the error status of the last operation on the current connection.
     *
     * @param w        when running with replication, this is the number of servers to replicate to before returning. A <b>w</b> value of <b>1</b> indicates the primary only. A <b>w</b> value of <b>2</b> includes the primary and at least one secondary, etc. In place of a number, you may also set <b>w</b> to majority to indicate that the command should wait until the latest write propagates to a majority of replica set members. If using <b>w</b>, you should also use <b>wtimeout</b>. Specifying a value for <b>w</b> without also providing a <b>wtimeout</b> may cause {@code getLastError} to block indefinitely.
     * @param wtimeout a value in milliseconds that controls how long to wait for write propagation to complete. If replication does not complete in the given timeframe, the getLastError command will return with an error status.
     * @param fsync    if <b>true</b>, wait for {@code mongod} to write this data to disk before returning. Defaults to <b>false</b>.
     * @return {@code DBObject} with error and status information
     * @throws MongoException
     * @deprecated The getlasterror command will not be supported in future versions of MongoDB.  Use acknowledged writes instead.
     * @see WriteConcern#ACKNOWLEDGED
     */
    @Deprecated
    public CommandResult getLastError( int w , int wtimeout , boolean fsync ){
        throw new UnsupportedOperationException("not supported!");
    }


    /**
     * Sets the write concern for this database. It will be used for
     * write operations to any collection in this database. See the
     * documentation for {@link WriteConcern} for more information.
     *
     * @param concern {@code WriteConcern} to use
     */
    public void setWriteConcern( WriteConcern concern ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the write concern for this database.
     *
     * @return {@code WriteConcern} to be used for write operations, if not specified explicitly
     */
    public WriteConcern getWriteConcern(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the read preference for this database. Will be used as default for
     * read operations from any collection in this database. See the
     * documentation for {@link ReadPreference} for more information.
     *
     * @param preference {@code ReadPreference} to use
     */
    public void setReadPreference( ReadPreference preference ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the read preference for this database.
     *
     * @return {@code ReadPreference} to be used for read operations, if not specified explicitly
     */
    public ReadPreference getReadPreference(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Drops this database, deleting the associated data files. Use with caution.
     *
     * @throws MongoException
     */
    public void dropDatabase(){
        _cm.execute(new DBCallback<Void>() {
            @Override
            public Void doInDB(Sequoiadb db) throws BaseException {
                db.dropCollectionSpace(_csName);
                return null;
            }
        });
    }

    /**
     * Returns {@code true} if a user has been authenticated on this database.
     *
     * @return {@code true} if authenticated, {@code false} otherwise
     * @dochub authenticate
     * @deprecated Please use {@link MongoClient#MongoClient(java.util.List, java.util.List)} to create a client, which
     *             will authenticate all connections to server
     */
    @Deprecated
    public boolean isAuthenticated() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Authenticates to db with the given credentials.  If this method (or {@code authenticateCommand}) has already been
     * called with the same credentials and the authentication test succeeded, this method will return {@code true}.  If this method
     * has already been called with different credentials and the authentication test succeeded,
     * this method will throw an {@code IllegalStateException}.  If this method has already been called with any credentials
     * and the authentication test failed, this method will re-try the authentication test with the
     * given credentials.
     *
     * @param username name of user for this database
     * @param password password of user for this database
     * @return true if authenticated, false otherwise
     * @throws MongoException        if authentication failed due to invalid user/pass, or other exceptions like I/O
     * @throws IllegalStateException if authentication test has already succeeded with different credentials
     * @dochub authenticate
     * @see #authenticateCommand(String, char[])
     * @deprecated Please use {@link MongoClient#MongoClient(java.util.List, java.util.List)} to create a client, which
     *             will authenticate all connections to server
     */
    @Deprecated
    public boolean authenticate(String username, char[] password) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Authenticates to db with the given credentials.  If this method (or {@code authenticate}) has already been
     * called with the same credentials and the authentication test succeeded, this method will return true.  If this method
     * has already been called with different credentials and the authentication test succeeded,
     * this method will throw an {@code IllegalStateException}.  If this method has already been called with any credentials
     * and the authentication test failed, this method will re-try the authentication test with the
     * given credentials.
     *
     * @param username name of user for this database
     * @param password password of user for this database
     * @return the CommandResult from authenticate command
     * @throws MongoException        if authentication failed due to invalid user/pass, or other exceptions like I/O
     * @throws IllegalStateException if authentication test has already succeeded with different credentials
     * @dochub authenticate
     * @see #authenticate(String, char[])
     * @deprecated Please use {@link MongoClient#MongoClient(java.util.List, java.util.List)} to create a client, which
     *             will authenticate all connections to server
     */
    @Deprecated
    public synchronized CommandResult authenticateCommand(String username, char[] password) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Adds or updates a user for this database
     *
     * @param username the user name
     * @param passwd   the password
     * @return the result of executing this operation
     * @throws MongoException
     * @mongodb.driver.manual administration/security-access-control/  Access Control
     * @deprecated Use {@code DB.command} to call either the addUser or updateUser command
     */
    @Deprecated
    public WriteResult addUser( String username , char[] passwd ){
        throw new UnsupportedOperationException("not supported!");
//        final String myUserName = username;
//        final String myPasswd = new String(passwd);
//
//        return _cm.execute(new DBCallback<WriteResult>() {
//            @Override
//            public WriteResult doInDB(Sequoiadb db) throws BaseException {
//                WriteResult result = new WriteResult();
//                try {
//                    db.createUser(myUserName, myPasswd);
//                } catch(BaseException e) {
//                    result.setExpInfo(e, "failed to create db user");
//                }
//                return result;
//            }
//        });
    }

    /**
     * Adds or updates a user for this database
     *
     * @param username the user name
     * @param passwd the password
     * @param readOnly if true, user will only be able to read
     * @return the result of executing this operation
     * @throws MongoException
     * @mongodb.driver.manual administration/security-access-control/  Access Control
     * @deprecated Use {@code DB.command} to call either the addUser or updateUser command
     */
    @Deprecated
    public WriteResult addUser( String username , char[] passwd, boolean readOnly ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Removes the specified user from the database.
     *
     * @param username user to be removed
     * @return the result of executing this operation
     * @throws MongoException
     * @mongodb.driver.manual administration/security-access-control/  Access Control
     * @deprecated Use {@code DB.command} to call the dropUser command
     */
    @Deprecated
    public WriteResult removeUser( String username ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the last error that occurred since start of database or a call to {@link com.mongodb.DB#resetError()} The return object
     * will look like:
     * <pre>
     * {@code
     * { err : errorMessage, nPrev : countOpsBack, ok : 1 }
     * }</pre>
     * The value for errorMessage will be null of no error has occurred, otherwise the error message.
     * The value of countOpsBack will be the number of operations since the error occurred.
     * <p> Care must be taken to ensure that calls to getPreviousError go to the same connection as that
     * of the previous operation. See {@link DB#requestStart()} for more information.</p>
     *
     * @return {@code DBObject} with error and status information
     * @throws MongoException
     * @deprecated The getlasterror command will not be supported in future versions of MongoDB.  Use acknowledged writes instead.
     * @see WriteConcern#ACKNOWLEDGED
     */
    @Deprecated
    public CommandResult getPreviousError(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Resets the error memory for this database.
     * Used to clear all errors such that {@link DB#getPreviousError()} will return no error.
     *
     * @throws MongoException
     * @deprecated The getlasterror command will not be supported in future versions of MongoDB.  Use acknowledged writes instead.
     * @see WriteConcern#ACKNOWLEDGED
     */
    @Deprecated
    public void resetError(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * For testing purposes only - this method forces an error to help test error handling
     *
     * @throws MongoException
     * @deprecated The getlasterror command will not be supported in future versions of MongoDB.  Use acknowledged writes instead.
     * @see WriteConcern#ACKNOWLEDGED
     */
    @Deprecated
    public void forceError(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the {@link Mongo} instance
     *
     * @return the instance of {@link Mongo} this database belongs to
     */
    public Mongo getMongo(){
        return _mongo;
    }

    /**
     * Gets another database on same server
     *
     * @param name name of the database
     * @return the database
     */
    public DB getSisterDB( String name ){
        return _mongo.getDB( name );
    }

    /**
     * Makes it possible to execute "read" queries on a slave node
     *
     * @see ReadPreference#secondaryPreferred()
     * @deprecated Replaced with {@code ReadPreference.secondaryPreferred()}
     */
    @Deprecated
    public void slaveOk(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Adds the given flag to the default query options.
     *
     * @param option value to be added
     */
    public void addOption( int option ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the default query options, overwriting previous value.
     *
     * @param options bit vector of query options
     */
    public void setOptions( int options ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Resets the query options.
     */
    public void resetOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default query options
     *
     * @return bit vector of query options
     */
    public int getOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Forcefully kills any cursors leaked by neglecting to call {@code DBCursor.close}
     *
     * @param force true if should clean regardless of number of dead cursors
     * @see com.mongodb.DBCursor#close()
     * @deprecated Clients should ensure that {@link DBCursor#close()} is called.
     */
    @Deprecated
    public abstract void cleanCursors( boolean force );

}


