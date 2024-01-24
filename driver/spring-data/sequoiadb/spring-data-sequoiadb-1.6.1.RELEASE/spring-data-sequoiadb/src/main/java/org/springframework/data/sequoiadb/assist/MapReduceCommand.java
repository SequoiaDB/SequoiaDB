package org.springframework.data.sequoiadb.assist;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Map;
import java.util.concurrent.TimeUnit;

import static java.util.concurrent.TimeUnit.MILLISECONDS;

/**
 * This class groups the argument for a map/reduce operation and can build the underlying command object
 *
 * @dochub mapreduce
 */
public class MapReduceCommand {

    /**
     * Represents the different options available for outputting the results of a map-reduce operation.
     *
     */
    public static enum OutputType {
        /**
         * Save the job output to a collection, replacing its previous content
         */
        REPLACE,
        /**
         * Merge the job output with the existing contents of outputTarget collection
         */
        MERGE,
        /**
         * Reduce the job output with the existing contents of outputTarget collection
         */
        REDUCE,
        /**
         * Return results inline, no result is written to the DB server
         */
        INLINE
    }

    /**
     * Represents the command for a map reduce operation Runs the command in REPLACE output type to a named collection
     *
     * @param inputCollection  the collection to read from
     * @param map              a JavaScript function that associates or "maps" a value with a key and emits the key and value pair.
     * @param reduce           a JavaScript function that "reduces" to a single object all the values associated with a particular key.
     * @param outputCollection specifies the location of the result of the map-reduce operation (optional) - leave null if want to get the
     *                         result inline
     * @param type             specifies the type of job output
     * @param query            specifies the selection criteria using query operators for determining the documents input to the map
     *                         function.
     * @dochub mapreduce
     */
    public MapReduceCommand(DBCollection inputCollection, String map, String reduce, String outputCollection, OutputType type,
                            BSONObject query) {
        _input = inputCollection.getName();
        _map = map;
        _reduce = reduce;
        _outputTarget = outputCollection;
        _outputType = type;
        _query = query;
    }

    /**
     * Sets the verbosity of the MapReduce job,
     * defaults to 'true'
     *
     * @param verbose
     *            The verbosity level.
     */
    public void setVerbose( Boolean verbose ){
        _verbose = verbose;
    }

    /**
     * Gets the verbosity of the MapReduce job.
     *
     * @return the verbosity level.
     */
    public Boolean isVerbose(){
        return _verbose;
    }

    /**
     * Get the name of the collection the MapReduce will read from
     *
     * @return name of the collection the MapReduce will read from
     */
    public String getInput(){
        return _input;
    }


    /**
     * Get the map function, as a JS String
     *
     * @return the map function (as a JS String)
     */
    public String getMap(){
        return _map;
    }

    /**
     * Gets the reduce function, as a JS String
     *
     * @return the reduce function (as a JS String)
     */
    public String getReduce(){
        return _reduce;
    }

    /**
     * Gets the output target (name of collection to save to)
     * This value is nullable only if OutputType is set to INLINE
     *
     * @return The outputTarget
     */
    public String getOutputTarget(){
        return _outputTarget;
    }


    /**
     * Gets the OutputType for this instance.
     * @return The outputType.
     */
    public OutputType getOutputType(){
        return _outputType;
    }


    /**
     * Gets the Finalize JS Function
     *
     * @return The finalize function (as a JS String).
     */
    public String getFinalize(){
        return _finalize;
    }

    /**
     * Sets the Finalize JS Function
     *
     * @param finalize
     *            The finalize function (as a JS String)
     */
    public void setFinalize( String finalize ){
        _finalize = finalize;
    }

    /**
     * Gets the query to run for this MapReduce job
     *
     * @return The query object
     */
    public BSONObject getQuery(){
        return _query;
    }

    /**
     * Gets the (optional) sort specification object
     *
     * @return the Sort BSONObject
     */
    public BSONObject getSort(){
        return _sort;
    }

    /**
     * Sets the (optional) sort specification object
     *
     * @param sort
     *            The sort specification object
     */
    public void setSort( BSONObject sort ){
        _sort = sort;
    }

    /**
     * Gets the (optional) limit on input
     *
     * @return The limit specification object
     */
    public int getLimit(){
        return _limit;
    }

    /**
     * Sets the (optional) limit on input
     *
     * @param limit
     *            The limit specification object
     */
    public void setLimit( int limit ){
        _limit = limit;
    }

    /**
     * Gets the max execution time for this command, in the given time unit.
     *
     * @param timeUnit the time unit to return the value in.
     * @return the maximum execution time
     * @since 2.12.0
     *
     * @sequoiadb.server.release 2.6
     */
    public long getMaxTime(final TimeUnit timeUnit) {
        return timeUnit.convert(_maxTimeMS, MILLISECONDS);
    }

    /**
     * Sets the max execution time for this command, in the given time unit.
     *
     * @param maxTime  the maximum execution time. A non-zero value requires a server version >= 2.6
     * @param timeUnit the time unit that maxTime is specified in
     * @since 2.12.0
     *
     * @sequoiadb.server.release 2.6
     */
    public void setMaxTime(final long maxTime, final TimeUnit timeUnit) {
        this._maxTimeMS = MILLISECONDS.convert(maxTime, timeUnit);
    }

    /**
     * Gets the (optional) JavaScript  scope
     *
     * @return The JavaScript scope
     */
    public Map<String, Object> getScope(){
        return _scope;
    }

    /**
     * Sets the (optional) JavaScript scope
     *
     * @param scope
     *            The JavaScript scope
     */
    public void setScope( Map<String, Object> scope ){
        _scope = scope;
    }

    /**
     * Sets the (optional) database name where the output collection should reside
     * @param outputDB
     */
    public void setOutputDB(String outputDB) {
        this._outputDB = outputDB;
    }



    public BSONObject toDBObject() {
        BasicBSONObject cmd = new BasicBSONObject();

        cmd.put("mapreduce", _input);
        cmd.put("map", _map);
        cmd.put("reduce", _reduce);
        cmd.put("verbose", _verbose);

        BasicBSONObject out = new BasicBSONObject();
        switch(_outputType) {
            case INLINE:
                out.put("inline", 1);
                break;
            case REPLACE:
                out.put("replace", _outputTarget);
                break;
            case MERGE:
                out.put("merge", _outputTarget);
                break;
            case REDUCE:
                out.put("reduce", _outputTarget);
                break;
        }
        if (_outputDB != null)
            out.put("db", _outputDB);
        cmd.put("out", out);

        if (_query != null)
            cmd.put("query", _query);

        if (_finalize != null)
            cmd.put( "finalize", _finalize );

        if (_sort != null)
            cmd.put("sort", _sort);

        if (_limit > 0)
            cmd.put("limit", _limit);

        if (_scope != null)
            cmd.put("scope", _scope);

        if (_extra != null) {
            cmd.putAll(_extra);
        }

        if (_maxTimeMS != 0) {
            cmd.put("maxTimeMS", _maxTimeMS);
        }

        return cmd;
    }

    public void addExtraOption(String name, Object value) {
        if (_extra == null)
            _extra = new BasicBSONObject();
        _extra.put(name, value);
    }

    public BSONObject getExtraOptions() {
        return _extra;
    }

    /**
     * Sets the read preference for this command.
     * See the * documentation for {@link ReadPreference}
     * for more information.
     *
     * @param preference Read Preference to use
     */
    public void setReadPreference( ReadPreference preference ){
        _readPref = preference;
    }

    /**
     * Gets the read preference
     * @return
     */
    public ReadPreference getReadPreference(){
        return _readPref;
    }


    public String toString() {
        return toDBObject().toString();
    }

    final String _input;
    final String _map;
    final String _reduce;
    final String _outputTarget;
    ReadPreference _readPref;
    String _outputDB = null;
    final OutputType _outputType;
    final BSONObject _query;
    String _finalize;
    BSONObject _sort;
    int _limit;
    Map<String, Object> _scope;
    Boolean _verbose = true;
    BSONObject _extra;
    private long _maxTimeMS;
}

