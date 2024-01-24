package org.springframework.data.sequoiadb.assist;


import org.bson.BSONObject;
import org.bson.util.annotations.NotThreadSafe;
import java.util.concurrent.TimeUnit;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;


/** An iterator over database results.
 * Doing a <code>find()</code> query on a collection returns a
 * <code>DBCursor</code> thus
 *
 * <blockquote><pre>
 * DBCursor cursor = collection.find( query );
 * if( cursor.hasNext() )
 *     BSONObject obj = cursor.next();
 * </pre></blockquote>
 *
 * <p><b>Warning:</b> Calling <code>toArray</code> or <code>length</code> on
 * a DBCursor will irrevocably turn it into an array.  This
 * means that, if the cursor was iterating over ten million results
 * (which it was lazily fetching from the database), suddenly there will
 * be a ten-million element array in memory.  Before converting to an array,
 * make sure that there are a reasonable number of results using
 * <code>skip()</code> and <code>limit()</code>.
 * <p>For example, to get an array of the 1000-1100th elements of a cursor, use
 *
 * <blockquote><pre>
 * List<BSONObject> obj = collection.find( query ).skip( 1000 ).limit( 100 ).toArray();
 * </pre></blockquote>
 *
 *
 * @dochub cursors
 */
@NotThreadSafe
public class DBCursor implements Cursor, Iterable<BSONObject> {

    private int _num = 0;
    private final ArrayList<BSONObject> _all = new ArrayList<BSONObject>();
    private QueryResultIterator _resultIterator;

//    /**
//     * Initializes a new database cursor
//     * @param collection collection to use
//     * @param q query to perform
//     * @param k keys to return from the query
//     * @param preference the Read Preference for this query
//     */
//    public DBCursor( DBCollection collection , BSONObject q , BSONObject k, ReadPreference preference ){
//        throw new UnsupportedOperationException("not supported!");
//    }

    DBCursor(QueryResultIterator resultIterator) {
        _resultIterator = resultIterator;
    }

    /**
     * Adds a comment to the query to identify queries in the database profiler output.
     *
     * @since 2.12
     */
    public DBCursor comment(final String comment) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Limits the number of documents a cursor will return for a query.
     *
     * @see #limit(int)
     * @since 2.12
     */
    public DBCursor maxScan(final int max) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Specifies an <em>exclusive</em> upper limit for the index to use in a query.
     *
     * @since 2.12
     */
    public DBCursor max(final BSONObject max) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Specifies an <em>inclusive</em> lower limit for the index to use in a query.
     *
     * @since 2.12
     */
    public DBCursor min(final BSONObject min) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Forces the cursor to only return fields included in the index.
     * @since 2.12
     */
    public DBCursor returnKey() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Modifies the documents returned to include references to the on-disk location of each document.  The location will be returned
     * in a property named {@code $diskLoc}
     * @since 2.12
     */
    public DBCursor showDiskLoc() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Types of cursors: iterator or array.
     */
    static enum CursorType { ITERATOR , ARRAY }

    /**
     * Creates a copy of an existing database cursor.
     * The new cursor is an iterator, even if the original
     * was an array.
     *
     * @return the new cursor
     */
    public DBCursor copy() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * creates a copy of this cursor object that can be iterated.
     * Note:
     * - you can iterate the DBCursor itself without calling this method
     * - no actual data is getting copied.
     * Note:
     * This method is not supported in package org.springframework.data.sequoiadb.assist
     * @return
     */
    public Iterator<BSONObject> iterator(){
        throw new UnsupportedOperationException("not supported!");
    }

    // ---- querty modifiers --------

    /**
     * Sorts this cursor's elements.
     * This method must be called before getting any object from the cursor.
     * @param orderBy the fields by which to sort
     * @return a cursor pointing to the first element of the sorted results
     */
    public DBCursor sort( BSONObject orderBy ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * adds a special operator like $maxScan or $returnKey
     * e.g. addSpecial( "$returnKey" , 1 )
     * e.g. addSpecial( "$maxScan" , 100 )
     * @param name
     * @param o
     * @return
     * @dochub specialOperators
     */
    public DBCursor addSpecial( String name , Object o ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Informs the database of indexed fields of the collection in order to improve performance.
     * @param indexKeys a <code>BSONObject</code> with fields and direction
     * @return same DBCursor for chaining operations
     */
    public DBCursor hint( BSONObject indexKeys ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     *  Informs the database of an indexed field of the collection in order to improve performance.
     * @param indexName the name of an index
     * @return same DBCursor for chaining operations
     */
    public DBCursor hint( String indexName ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Set the maximum execution time for operations on this cursor.
     *
     * @param maxTime  the maximum time that the server will allow the query to run, before killing the operation. A non-zero value
     *                 requires a server version >= 2.6
     * @param timeUnit the time unit
     * @return same DBCursor for chaining operations
     * @since 2.12.0
     *
     * @sequoiadb.server.release 2.6
     */
    public DBCursor maxTime(final long maxTime, final TimeUnit timeUnit) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Use snapshot mode for the query. Snapshot mode assures no duplicates are
     * returned, or objects missed, which were present at both the start and end
     * of the query's execution (if an object is new during the query, or deleted
     * during the query, it may or may not be returned, even with snapshot mode).
     * Note that short query responses (less than 1MB) are always effectively snapshotted.
     * Currently, snapshot mode may not be used with sorting or explicit hints.
     * @return same DBCursor for chaining operations
     */
    public DBCursor snapshot() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns an object containing basic information about the
     * execution of the query that created this cursor
     * This creates a <code>BSONObject</code> with the key/value pairs:
     * "cursor" : cursor type
     * "nScanned" : number of records examined by the database for this query
     * "n" : the number of records that the database returned
     * "millis" : how long it took the database to execute the query
     * @return a <code>BSONObject</code>
     * @throws BaseException
     * @dochub explain
     */
    public BSONObject explain(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Limits the number of elements returned.
     * Note: parameter <tt>n</tt> should be positive, although a negative value is supported for legacy reason.
     * Passing a negative value will call {@link DBCursor#batchSize(int)} which is the preferred method.
     * @param n the number of elements to return
     * @return a cursor to iterate the results
     * @dochub limit
     */
    public DBCursor limit( int n ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Limits the number of elements returned in one batch.
     * A cursor typically fetches a batch of result objects and store them locally.
     *
     * If <tt>batchSize</tt> is positive, it represents the size of each batch of objects retrieved.
     * It can be adjusted to optimize performance and limit data transfer.
     *
     * If <tt>batchSize</tt> is negative, it will limit of number objects returned, that fit within the max batch size limit (usually 4MB), and cursor will be closed.
     * For example if <tt>batchSize</tt> is -10, then the server will return a maximum of 10 documents and as many as can fit in 4MB, then close the cursor.
     * Note that this feature is different from limit() in that documents must fit within a maximum size, and it removes the need to send a request to close the cursor server-side.
     *
     * The batch size can be changed even after a cursor is iterated, in which case the setting will apply on the next batch retrieval.
     *
     * @param n the number of elements to return in a batch
     * @return
     */
    public DBCursor batchSize( int n ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Discards a given number of elements at the beginning of the cursor.
     * @param n the number of elements to skip
     * @return a cursor pointing to the new first element of the results
     * @throws IllegalStateException if the cursor has started to be iterated through
     */
    public DBCursor skip( int n ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets the cursor id.
     * @return the cursor id, or 0 if there is no active cursor.
     */
    public long getCursorId() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * kills the current cursor on the server.
     */
    public void close() {
        if (_resultIterator != null) {
            _resultIterator.close();
        }
    }

    /**
     * makes this query ok to run on a slave node
     *
     * @return a copy of the same cursor (for chaining)
     *
     * @deprecated Replaced with {@code ReadPreference.secondaryPreferred()}
     * @see ReadPreference#secondaryPreferred()
     */
    @Deprecated
    public DBCursor slaveOk(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * adds a query option - see Bytes.QUERYOPTION_* for list
     * @param option
     * @return
     */
    public DBCursor addOption( int option ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * sets the query option - see Bytes.QUERYOPTION_* for list
     * @param options
     */
    public DBCursor setOptions( int options ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * resets the query options
     */
    public DBCursor resetOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets the query options
     * @return
     */
    public int getOptions(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets the number of times, so far, that the cursor retrieved a batch from the database
     * @return The number of times OP_GET_MORE has been called
     * @deprecated there is no replacement for this method
     */
    @Deprecated
    public int numGetMores() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets a list containing the number of items received in each batch
     * @return a list containing the number of items received in each batch
     * @deprecated there is no replacement for this method
     */
    @Deprecated
    public List<Integer> getSizes() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Returns the number of objects through which the cursor has iterated.
     * @return the number of objects seen
     */
    public int numSeen(){
        return _num;
    }

    // ----- iterator api -----

    /**
     * Checks if there is another object available
     * @return
     * @throws BaseException
     */
    public boolean hasNext() {
        boolean hasNext =  _resultIterator.hasNext();
        if (!hasNext) {
            try {
                close();
            } catch (Exception e) {
            }
        }
        return hasNext;
    }

    /**
     * Returns the object the cursor is at and moves the cursor ahead by one.
     * @return the next element
     * @throws BaseException
     */
    public BSONObject next() {
        _num++;
        return (BSONObject)_resultIterator.next();
    }

    /**
     * Returns the element the cursor is at.
     * @return the current element
     */
    public BSONObject curr(){
        return (BSONObject)_resultIterator.current();
    }

    /**
     * Not implemented.
     */
    public void remove(){
        throw new UnsupportedOperationException( "can't remove from a cursor" );
    }

    /**
     * pulls back all items into an array and returns the number of objects.
     * Note: this can be resource intensive
     * @see #count()
     * @see #size()
     * @return the number of elements in the array
     * @throws BaseException
     */
    public int length() {
        toArray();
        return _all.size();
    }

    /**
     * Converts this cursor to an array.
     * @return an array of elements
     * @throws BaseException
     */
    public List<BSONObject> toArray(){
        return toArray( Integer.MAX_VALUE );
    }

    /**
     * Converts this cursor to an array.
     * @param max the maximum number of objects to return
     * @return an array of objects
     * @throws BaseException
     */
    public List<BSONObject> toArray( int max ) {
        while(max > _all.size() && _resultIterator.hasNext()) {
            _all.add((BSONObject)_resultIterator.next());
        }
        return _all;
    }

    /**
     * for testing only!
     * Iterates cursor and counts objects
     * @see #count()
     * @return num objects
     * @throws BaseException
     */
    public int itcount(){
        int n = 0;
        while ( this.hasNext() ){
            this.next();
            n++;
        }
        return n;
    }

    /**
     * Counts the number of objects matching the query
     * This does not take limit/skip into consideration
     * @see #size()
     * @return the number of objects
     * @throws BaseException
     */
    public int count() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * @return the first matching document
     *
     * @since 2.12
     */
    public BSONObject one() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Counts the number of objects matching the query
     * this does take limit/skip into consideration
     * @see #count()
     * @return the number of objects
     * @throws BaseException
     */
    public int size() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets the fields to be returned
     * @return
     */
    public BSONObject getKeysWanted(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets the query
     * @return
     */
    public BSONObject getQuery(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * gets the collection
     * @return
     */
    public DBCollection getCollection(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the Server Address of the server that data is pulled from.
     * Note that this information may not be available until hasNext() or next() is called.
     * @return
     */
    public ServerAddress getServerAddress() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the read preference for this cursor.
     * See the * documentation for {@link ReadPreference}
     * for more information.
     *
     * @param preference Read Preference to use
     */
    public DBCursor setReadPreference( ReadPreference preference ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default read preference
     * @return
     */
    public ReadPreference getReadPreference(){
        throw new UnsupportedOperationException("not supported!");
    }

    public DBCursor setDecoderFactory(DBDecoderFactory fact){
        throw new UnsupportedOperationException("not supported!");
    }

    public DBDecoderFactory getDecoderFactory(){
        throw new UnsupportedOperationException("not supported!");
    }

    @Override
    public String toString() {
        return _resultIterator.toString();
    }

}

