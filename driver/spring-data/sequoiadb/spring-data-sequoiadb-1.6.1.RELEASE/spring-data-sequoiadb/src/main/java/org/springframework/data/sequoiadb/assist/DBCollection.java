package org.springframework.data.sequoiadb.assist;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import java.util.*;
import java.util.concurrent.TimeUnit;

import static java.util.concurrent.TimeUnit.MILLISECONDS;

/**
 * This class provides a skeleton implementation of a database collection. <p>A typical invocation sequence is thus
 * <pre>
 * {@code
 * SdbClient sequoiadbClient = new SdbClient(new ServerAddress("localhost", 11810));
 * DB db = sdb.getDB("mydb");
 * DBCollection collection = db.getCollection("test"); }
 * </pre>
 * To get a collection to use, just specify the name of the collection to the getCollection(String collectionName) method:
 * <pre>
 * {@code
 * DBCollection coll = db.getCollection("testCollection"); }
 * </pre>
 * Once you have the collection object, you can insert documents into the collection:
 * <pre>
 * {@code
 * BasicBSONObject doc = new BasicBSONObject("name", "SequoiaDB").append("type", "database")
 *                                                         .append("count", 1)
 *                                                         .append("info", new BasicBSONObject("x", 203).append("y", 102));
 * coll.insert(doc); }
 * </pre>
 * To show that the document we inserted in the previous step is there, we can do a simple findOne() operation to get the first document in
 * the collection:
 * <pre>
 * {@code
 * BSONObject myDoc = coll.findOne();
 * System.out.println(myDoc); }
 * </pre>
 */
@SuppressWarnings("unchecked")
public abstract class DBCollection {

    /**
     * @deprecated Please use {@link #getName()} instead.
     */
    @Deprecated
    final protected String _name;

    /**
     * @deprecated Please use {@link #getFullName()} instead.
     */
    @Deprecated
    final protected String _fullName;

    /**
     * @deprecated Please use {@link #setHintFields(java.util.List)} and {@link #getHintFields()} instead.
     */
    @Deprecated
    protected List<BSONObject> _hintFields = new ArrayList<BSONObject>();

    /**
     * @deprecated Please use {@link #getObjectClass()} and {@link #setObjectClass(Class)} instead.
     */
    @Deprecated
    protected Class _objectClass = null;

    final protected  DB _db;
    final protected ConnectionManager _cm;
    final protected String _csName;
    final protected String _clName;

    protected  <T> T execute(CLCallback<T> callback) {
        return _cm.execute(_csName, _clName, callback);
    }

    protected List<BSONObject> convertBson(List<BSONObject> list, boolean appendOID) {
        List<BSONObject> myList = new ArrayList<BSONObject>(list.size());
        for(BSONObject obj : list) {
            apply(obj, appendOID);
            myList.add(obj);
        }
        return myList;
    }

    /**
     * Initializes a new collection. No operation is actually performed on the database.
     * @param base database in which to create the collection
     * @param name the name of the collection
     */
    protected DBCollection( DB base , String name ){
        throw new UnsupportedOperationException("not supported!");
    }

    DBCollection( ConnectionManager cm, DB base , String name){
        _cm = cm;
        _db = base;
        _csName = _db.getName();
        _clName = name;
        _name = name;
        _fullName = _db.getName() + "." + name;
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param arr     {@code BSONObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException if the operation fails
     * @dochub insert Insert
     */
    public WriteResult insert(BSONObject[] arr , WriteConcern concern ){
        return insert( arr, concern, null);
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param arr     {@code BSONObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder {@code DBEncoder} to be used
     * @return the result of the operation
     * @throws BaseException if the operation fails
     * @dochub insert Insert
     */
    public WriteResult insert(BSONObject[] arr , WriteConcern concern, DBEncoder encoder) {
        return insert(Arrays.asList(arr), concern, encoder);
    }

    /**
     * Insert a document into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param o       {@code BSONObject} to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException if the operation fails
     * @dochub insert Insert
     */
    public WriteResult insert(BSONObject o , WriteConcern concern ){
        return insert( Arrays.asList(o) , concern );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added. Collection wide {@code WriteConcern} will be used.
     *
     * @param arr {@code BSONObject}'s to be inserted
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public WriteResult insert(BSONObject ... arr){
        return insert( arr , getWriteConcern() );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param arr     {@code BSONObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public WriteResult insert(WriteConcern concern, BSONObject ... arr){
        return insert( arr, concern );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param list list of {@code BSONObject} to be inserted
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public WriteResult insert(List<BSONObject> list ){
        return insert( list, getWriteConcern() );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param list    list of {@code BSONObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public WriteResult insert(List<BSONObject> list, WriteConcern concern ){
        return insert(list, concern, null );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param list    a list of {@code BSONObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder {@code DBEncoder} to use to serialise the documents
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public abstract WriteResult insert(List<BSONObject> list, WriteConcern concern, DBEncoder encoder);

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.
     *
     * @param q       the selection criteria for the update
     * @param o       the modifications to apply
     * @param upsert  when true, inserts a document if no document matches the update query criteria
     * @param multi   when true, updates all documents in the collection that match the update query criteria, otherwise only updates one
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException
     */
    public WriteResult update( BSONObject q , BSONObject o , boolean upsert , boolean multi , WriteConcern concern ){
        return update( q, o, upsert, multi, concern, null);
    }

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.
     *
     * @param matcher       the selection criteria for the update
     * @param rule       the modifications to apply
     * @param upsert  when true, inserts a document if no document matches the update query criteria
     * @param multi   when true, updates all documents in the collection that match the update query criteria, otherwise only updates one
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder the DBEncoder to use
     * @return the result of the operation
     * @throws BaseException
     */
    public abstract WriteResult update(BSONObject matcher , BSONObject rule , boolean upsert , boolean multi , WriteConcern concern, DBEncoder encoder );

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.
     *
     * @param matcher       the selection criteria for the update
     * @param rule       the modifications to apply
     * @param hint       the index to apply
     * @param upsert  when true, inserts a document if no document matches the update query criteria
     * @return the result of the operation
     * @throws BaseException
     */
    public abstract WriteResult update(BSONObject matcher, BSONObject rule, BSONObject hint, boolean upsert);

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.  Calls {@link DBCollection#update(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, boolean, boolean, com.sequoiadb.WriteConcern)} with default WriteConcern.
     *
     * @param matcher      the selection criteria for the update
     * @param rule      the modifications to apply
     * @param upsert when true, inserts a document if no document matches the update query criteria
     * @param multi  when true, updates all documents in the collection that match the update query criteria, otherwise only updates one
     * @return the result of the operation
     * @throws BaseException
     */
    public WriteResult update(BSONObject matcher, BSONObject rule, boolean upsert , boolean multi ){
        return update( matcher , rule , upsert , multi , getWriteConcern() );
    }

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.  Calls {@link DBCollection#update(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, boolean, boolean)} with upsert=false and multi=false
     *
     * @param matcher the selection criteria for the update
     * @param rule the modifications to apply
     * @return the result of the operation
     * @throws BaseException
     */
    public WriteResult update(BSONObject matcher , BSONObject rule){
        return update( matcher , rule , false , false );
    }

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find()}.  Calls {@link DBCollection#update(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, boolean, boolean)} with upsert=false and multi=true
     *
     * @param matcher the selection criteria for the update
     * @param rule the modifications to apply
     * @return the result of the operation
     * @throws BaseException
     */
    public WriteResult updateMulti(BSONObject matcher, BSONObject rule ){
        return update( matcher , rule , false , true );
    }

    /**
     * Adds any necessary fields to a given object before saving it to the collection.
     * @param o object to which to add the fields
     */
    protected abstract void doapply( BSONObject o );

    /**
     * Remove documents from a collection.
     *
     * @param o       the deletion criteria using query operators. Omit the query parameter or pass an empty document to delete all
     *                documents in the collection.
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException
     */
    public WriteResult remove( BSONObject o , WriteConcern concern ){
        return remove(  o, concern, null);
    }

    /**
     * Remove documents from a collection.
     *
     * @param o       the deletion criteria using query operators. Omit the query parameter or pass an empty document to delete all
     *                documents in the collection.
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder {@code DBEncoder} to be used
     * @return the result of the operation
     * @throws BaseException
     */
    public abstract WriteResult remove( BSONObject o , WriteConcern concern, DBEncoder encoder );

    /**
     * Remove documents from a collection. Calls {@link DBCollection#remove(com.sequoiadb.BSONObject, com.sequoiadb.WriteConcern)} with the
     * default WriteConcern
     *
     * @param o the deletion criteria using query operators. Omit the query parameter or pass an empty document to delete all documents in
     *          the collection.
     * @return the result of the operation
     * @throws BaseException
     */
    public WriteResult remove( BSONObject o ){
        return remove( o , getWriteConcern() );
    }


    /**
     * Finds objects
     */
    abstract QueryResultIterator find(BSONObject ref, BSONObject fields, int numToSkip, int batchSize, int limit, int options,
                                      ReadPreference readPref, DBDecoder decoder);

    abstract QueryResultIterator find(BSONObject ref, BSONObject fields, int numToSkip, int batchSize, int limit, int options,
                                      ReadPreference readPref, DBDecoder decoder, DBEncoder encoder);

    abstract QueryResultIterator findInternal(final BSONObject matcher, final BSONObject selector,
                                              final BSONObject orderBy, final BSONObject hint,
                                              final long skipRows, final long returnRows, final int flags);
    /**
     * Calls {@link DBCollection#find(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, int, int)} and applies the query options
     *
     * @param query     query used to search
     * @param fields    the fields of matching objects to return
     * @param numToSkip number of objects to skip
     * @param batchSize the batch size. This option has a complex behavior, see {@link DBCursor#batchSize(int) }
     * @param options   see {@link com.sequoiadb.Bytes} QUERYOPTION_*
     * @return the cursor
     * @throws BaseException
     * @deprecated use {@link com.sequoiadb.DBCursor#skip(int)}, {@link com.sequoiadb.DBCursor#batchSize(int)} and {@link
     * com.sequoiadb.DBCursor#setOptions(int)} on the {@code DBCursor} returned from {@link com.sequoiadb.DBCollection#find(BSONObject,
     * BSONObject)}
     */
    @Deprecated
    public DBCursor find( BSONObject query , BSONObject fields , int numToSkip , int batchSize , int options ){
        QueryResultIterator resultIterator =  find(query, fields, numToSkip, batchSize, -1, options,
                null, null, null);
        return new DBCursor(resultIterator);
    }

    /**
     * Finds objects from the database that match a query. A DBCursor object is returned, that can be iterated to go through the results.
     *
     * @param query     query used to search
     * @param fields    the fields of matching objects to return
     * @param numToSkip number of objects to skip
     * @param batchSize the batch size. This option has a complex behavior, see {@link DBCursor#batchSize(int) }
     * @return the cursor
     * @throws BaseException
     * @deprecated use {@link com.sequoiadb.DBCursor#skip(int)} and {@link com.sequoiadb.DBCursor#batchSize(int)} on the {@code DBCursor}
     * returned from {@link com.sequoiadb.DBCollection#find(BSONObject, BSONObject)}
     */
    @Deprecated
    public DBCursor find( BSONObject query , BSONObject fields , int numToSkip , int batchSize ) {
        DBCursor cursor = find(query, fields, numToSkip, batchSize, 0);
        return cursor;
    }

    /**
     * Finds objects from the database that match a query. A DBCursor object is returned, that can be iterated to go through the results.
     *
     * @param query     query used to search
     * @param fields    the fields of matching objects to return
     * @param orderBy   the order to return
     * @param hint      specify the indexes to be used
     * @param skipRows  number of objects to skip
     * @param returnRows number of objects to return
     * @param flags the query flags
     * @return the cursor
     * @throws BaseException
     */
    public DBCursor find(BSONObject matcher, BSONObject selector, BSONObject orderBy, BSONObject hint,
                         long skipRows, long returnRows, int flags) {
        QueryResultIterator resultIterator =  findInternal(matcher, selector, orderBy, hint, skipRows, returnRows, flags);
        return new DBCursor(resultIterator);
    }


    /**
     * Finds an object by its id.
     * This compares the passed in value to the _id field of the document
     *
     * @param obj any valid object
     * @return the object, if found, otherwise null
     * @throws BaseException
     */
    public BSONObject findOne( Object obj ){
        return findOne(obj, null);
    }


    /**
     * Finds an object by its id.
     * This compares the passed in value to the _id field of the document
     *
     * @param obj any valid object
     * @param fields fields to return
     * @return the object, if found, otherwise null
     * @throws BaseException
     */
    public BSONObject findOne( Object obj, BSONObject fields ){
        Iterator<BSONObject> iterator = find(new BasicBSONObject("_id", obj), fields, 0, -1,
                -1, 0, null, null);
        return (iterator.hasNext() ? iterator.next() : null);
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.
     *
     * @param query     specifies the selection criteria for the modification
     * @param fields    a subset of fields to return
     * @param sort      determines which document the operation will modify if the query selects multiple documents
     * @param remove    when true, removes the selected document
     * @param returnNew when true, returns the modified document rather than the original
     * @param update    the modifications to apply
     * @param upsert    when true, operation creates a new document if the query returns no documents
     * @return the document as it was before the modifications, unless {@code returnNew} is true, in which case it returns the document
     * after the changes were made
     * @throws BaseException
     */
    public BSONObject findAndModify(BSONObject query, BSONObject fields, BSONObject sort, boolean remove, BSONObject update, boolean returnNew, boolean upsert){
        return findAndModify(query, fields, sort, remove, update, returnNew, upsert, 0L, MILLISECONDS);
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.
     *
     * @param query       specifies the selection criteria for the modification
     * @param fields      a subset of fields to return
     * @param sort        determines which document the operation will modify if the query selects multiple documents
     * @param remove      when {@code true}, removes the selected document
     * @param returnNew   when true, returns the modified document rather than the original
     * @param update      performs an update of the selected document
     * @param upsert      when true, operation creates a new document if the query returns no documents
     * @param maxTime     the maximum time that the server will allow this operation to execute before killing it. A non-zero value requires
     *                    a server version >= 2.6
     * @param maxTimeUnit the unit that maxTime is specified in
     * @return the document as it was before the modifications, unless {@code returnNew} is true, in which case it returns the document
     * after the changes were made
     * @since 2.12.0
     */
    public BSONObject findAndModify(final BSONObject query, final BSONObject fields, final BSONObject sort,
                                  final boolean remove, final BSONObject update,
                                  final boolean returnNew, final boolean upsert,
                                  final long maxTime, final TimeUnit maxTimeUnit) {

        return _cm.execute(_csName, _clName, new CLCallback<BSONObject>() {
            @Override
            public BSONObject doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                BSONObject ret = null;
                com.sequoiadb.base.DBCursor cursor = null;
                BSONObject myHint = (_hintFields.size() > 0) ? (BSONObject)_hintFields.get(0) : null;
                try {
                    if (remove) {
                        cursor = cl.queryAndRemove(query, fields, sort, myHint, 0, 1, 0);
                    } else if (!upsert) {
                        cursor = cl.queryAndUpdate(query, fields, sort, myHint, update,
                                0, 1, 0, returnNew);
                    } else {
                        throw new UnsupportedOperationException("not supported find and insert!");
                    }
                    if (cursor.hasNext()) {
                        ret = cursor.getNext();
                    }
                } finally {
                    if (cursor != null) {
                        cursor.close();
                    }
                }
                return ret;
            }
        });
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.  Calls {@link DBCollection#findAndModify(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, boolean,
     * com.sequoiadb.BSONObject, boolean, boolean)} with fields=null, remove=false, returnNew=false, upsert=false
     *
     * @param query  specifies the selection criteria for the modification
     * @param sort   determines which document the operation will modify if the query selects multiple documents
     * @param update the modifications to apply
     * @return the document as it was before the modifications.
     * @throws BaseException
     */
    public BSONObject findAndModify( BSONObject query , BSONObject sort , BSONObject update) {
        return findAndModify( query, null, sort, false, update, false, false);
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.  Calls {@link DBCollection#findAndModify(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, boolean,
     * com.sequoiadb.BSONObject, boolean, boolean)} with fields=null, sort=null, remove=false, returnNew=false, upsert=false
     *
     * @param query  specifies the selection criteria for the modification
     * @param update the modifications to apply
     * @return the document as it was before the modifications.
     * @throws BaseException
     */
    public BSONObject findAndModify( BSONObject query , BSONObject update ){
        return findAndModify( query, null, null, false, update, false, false );
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.  Ccalls {@link DBCollection#findAndModify(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, boolean,
     * com.sequoiadb.BSONObject, boolean, boolean)} with fields=null, sort=null, remove=true, returnNew=false, upsert=false
     *
     * @param query specifies the selection criteria for the modification
     * @return the document as it was before it was removed
     * @throws BaseException
     */
    public BSONObject findAndRemove( BSONObject query ) {
        return findAndModify( query, null, null, true, null, false, false );
    }


    /**
     * Calls {@link DBCollection#createIndex(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject)} with default index options
     *
     * @param keys a document that contains pairs with the name of the field or fields to index and order of the index
     * @throws BaseException
     */
    public void createIndex( final BSONObject keys ){
        createIndex( keys , null );
    }

    /**
     * Forces creation of an index on a set of fields, if one does not already exist.
     *
     * @param keys    a document that contains pairs with the name of the field or fields to index and order of the index
     * @param options a document that controls the creation of the index.
     * @throws BaseException
     */
    public void createIndex( BSONObject keys , BSONObject options ){
        createIndex( keys, options, null);
    }

    /**
     * Forces creation of an index on a set of fields, if one does not already exist.
     *
     * @param keys    a document that contains pairs with the name of the field or fields to index and order of the index
     * @param options a document that controls the creation of the index.
     * @param encoder specifies the encoder that used during operation
     * @throws BaseException
     * @deprecated use {@link #createIndex(BSONObject, com.sequoiadb.BSONObject)} the encoder is not used.
     */
    @Deprecated
    public abstract void createIndex(BSONObject keys, BSONObject options, DBEncoder encoder);

    /**
     * Creates an ascending index on a field with default options, if one does not already exist.
     *
     * @param name name of field to index on
     * @throws BaseException
     * @deprecated use {@link DBCollection#createIndex(BSONObject)} instead
     */
    @Deprecated
    public void ensureIndex( final String name ){
        ensureIndex( new BasicBSONObject( name , 1 ) );
    }

    /**
     * Calls {@link DBCollection#ensureIndex(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject)} with default options
     * @param keys an object with a key set of the fields desired for the index
     * @throws BaseException
     *
     * @deprecated use {@link DBCollection#createIndex(BSONObject)} instead
     */
    @Deprecated
    public void ensureIndex( final BSONObject keys ){
        ensureIndex( keys , "" );
    }

    /**
     * Calls {@link DBCollection#ensureIndex(com.sequoiadb.BSONObject, java.lang.String, boolean)} with unique=false
     *
     * @param keys fields to use for index
     * @param name an identifier for the index
     * @throws BaseException
     * @deprecated use {@link DBCollection#createIndex(BSONObject, BSONObject)} instead
     */
    @Deprecated
    public void ensureIndex( BSONObject keys , String name ){
        ensureIndex( keys , name , false );
    }

    /**
     * Ensures an index on this collection (that is, the index will be created if it does not exist).
     *
     * @param keys   fields to use for index
     * @param name   an identifier for the index. If null or empty, the default name will be used.
     * @param unique if the index should be unique
     * @throws BaseException
     * @deprecated use {@link DBCollection#createIndex(BSONObject, BSONObject)} instead
     */
    @Deprecated
    public void ensureIndex( BSONObject keys , String name , boolean unique ){

        final BSONObject myKeys = keys;
        final boolean myUnique = unique;
        final String indexName = (name == null || name.isEmpty()) ? genIndexName(keys) : name;

        execute(new CLCallback<Void>() {
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                cl.createIndex(indexName, myKeys, myUnique, false, 64);
                return null;
            }
        });
    }

    /**
     * Creates an index on a set of fields, if one does not already exist.
     *
     * @param keys      an object with a key set of the fields desired for the index
     * @param optionsIN options for the index (name, unique, etc)
     * @throws BaseException
     * @deprecated use {@link DBCollection#createIndex(BSONObject, BSONObject)} instead
     */
    @Deprecated
    public void ensureIndex( final BSONObject keys , final BSONObject optionsIN ){
        createIndex( keys, optionsIN );
    }

    /**
     * Clears all indices that have not yet been applied to this collection.
     * @deprecated This will be removed in 3.0
     */
    @Deprecated
    public void resetIndexCache(){
        throw new UnsupportedOperationException("not supported!");
    }


    /**
     * Convenience method to generate an index name from the set of fields it is over.
     * @param keys the names of the fields used in this index
     * @return a string representation of this index's fields
     *
     * @deprecated This method is NOT a part of public API and will be dropped in 3.x versions.
     */
    @Deprecated
    public static String genIndexName( BSONObject keys ){
        StringBuilder name = new StringBuilder();
        for ( String s : keys.keySet() ){
            if ( name.length() > 0 )
                name.append( '_' );
            name.append( s ).append( '_' );
            Object val = keys.get( s );
            if ( val instanceof Number || val instanceof String )
                name.append( val.toString().replace( ' ', '_' ) );
        }
        return name.toString();
    }


    /**
     * Set hint fields for this collection (to optimize queries).
     * @param lst a list of {@code BSONObject}s to be used as hints
     */
    public void setHintFields( List<BSONObject> lst ){
        _hintFields = lst;
    }

    /**
     * Get hint fields for this collection (used to optimize queries).
     * @return a list of {@code BSONObject} to be used as hints.
     */
    protected List<BSONObject> getHintFields() {
        return _hintFields;
    }

    /**
     * Queries for an object in this collection.
     *
     * @param ref A document outlining the search query
     * @return an iterator over the results
     */
    public DBCursor find( BSONObject ref ){
        return find( ref, null, 0, -1 );
    }

    /**
     * Queries for an object in this collection.
     * <p>
     * An empty BSONObject will match every document in the collection.
     * Regardless of fields specified, the _id fields are always returned.
     * </p>
     * <p>
     * An example that returns the "x" and "_id" fields for every document
     * in the collection that has an "x" field:
     * </p>
     * <pre>
     * {@code
     * BasicBSONObject keys = new BasicBSONObject();
     * keys.put("x", 1);
     *
     * DBCursor cursor = collection.find(new BasicBSONObject(), keys);}
     * </pre>
     *
     * @param ref object for which to search
     * @param keys fields to return
     * @return a cursor to iterate over results
     */
    public DBCursor find( BSONObject ref , BSONObject keys ){
        return find( ref, keys, 0, -1 );
    }


    /**
     * Queries for all objects in this collection.
     *
     * @return a cursor which will iterate over every object
     */
    public DBCursor find(){
        return find( null, null, 0, -1 );
    }

    /**
     * Returns a single object from this collection.
     *
     * @return the object found, or {@code null} if the collection is empty
     * @throws BaseException
     */
    public BSONObject findOne(){
        return findOne( new BasicBSONObject() );
    }

    /**
     * Returns a single object from this collection matching the query.
     * @param o the query object
     * @return the object found, or {@code null} if no such object exists
     * @throws BaseException
     */
    public BSONObject findOne( BSONObject o ){
        return findOne( o, null, null, null);
    }

    /**
     * Returns a single object from this collection matching the query.
     * @param o the query object
     * @param fields fields to return
     * @return the object found, or {@code null} if no such object exists
     * @throws BaseException
     */
    public BSONObject findOne( BSONObject o, BSONObject fields ) {
        return findOne( o, fields, null, null);
    }

    /**
     * Returns a single object from this collection matching the query.
     * @param o the query object
     * @param fields fields to return
     * @param orderBy fields to order by
     * @return the object found, or {@code null} if no such object exists
     * @throws BaseException
     */
    public BSONObject findOne( BSONObject o, BSONObject fields, BSONObject orderBy){
        return findOne(o, fields, orderBy, null);
    }

    /**
     * Get a single document from collection.
     *
     * @param o        the selection criteria using query operators.
     * @param fields   specifies which fields SequoiaDB will return from the documents in the result set.
     * @param readPref {@link ReadPreference} to be used for this operation
     * @return A document that satisfies the query specified as the argument to this method, or {@code null} if no such object exists
     * @throws BaseException
     */
    public BSONObject findOne( BSONObject o, BSONObject fields, ReadPreference readPref ){
        return findOne(o, fields, null, null);
    }

    /**
     * Get a single document from collection.
     *
     * @param o        the selection criteria using query operators.
     * @param fields   specifies which projection SequoiaDB will return from the documents in the result set.
     * @param orderBy  A document whose fields specify the attributes on which to sort the result set.
     * @param readPref {@code ReadPreference} to be used for this operation
     * @return A document that satisfies the query specified as the argument to this method, or {@code null} if no such object exists
     * @throws BaseException
     */
    public BSONObject findOne(BSONObject o, BSONObject fields, BSONObject orderBy, ReadPreference readPref) {
        return findOne(o, fields, orderBy, null, 0, MILLISECONDS);
    }

    /**
     * Get a single document from collection.
     *
     * @param o           the selection criteria using query operators.
     * @param fields      specifies which projection SequoiaDB will return from the documents in the result set.
     * @param orderBy     A document whose fields specify the attributes on which to sort the result set.
     * @param readPref    {@code ReadPreference} to be used for this operation
     * @param maxTime     the maximum time that the server will allow this operation to execute before killing it
     * @param maxTimeUnit the unit that maxTime is specified in
     * @return A document that satisfies the query specified as the argument to this method.
     * @since 2.12.0
     */
    BSONObject findOne(BSONObject o, BSONObject fields, BSONObject orderBy, ReadPreference readPref,
                     long maxTime, TimeUnit maxTimeUnit) {
        final BSONObject myMatcher = o;
        final BSONObject myFields = fields;
        final BSONObject myOrderBy = orderBy;
        final BSONObject myHint = (_hintFields.size() > 0) ? _hintFields.get(0) : null;

        BSONObject obj =
                _cm.execute(_csName, _clName, new CLCallback<BSONObject>(){
                    public BSONObject doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                         BSONObject retObj = cl.queryOne(myMatcher, myFields, myOrderBy, myHint, 0);
                         return retObj;
                    }
                });
        return obj;
    }


    /**
     * calls {@link DBCollection#apply(com.sequoiadb.BSONObject, boolean)} with ensureID=true
     * @param o {@code BSONObject} to which to add fields
     * @return the modified parameter object
     */
    public Object apply( BSONObject o ){
        return apply( o , true );
    }

    /**
     * calls {@link DBCollection#doapply(com.sequoiadb.BSONObject)}, optionally adding an automatic _id field
     * @param jo object to add fields to
     * @param ensureID whether to add an {@code _id} field
     * @return the modified object {@code o}
     */
    public Object apply( BSONObject jo , boolean ensureID ){
        Object id = jo.get("_id");
        if (ensureID && id == null) {
            id = ObjectId.get();
            jo.put("_id", id);
        }

        doapply(jo);

        return id;
    }

    /**
     * Update an existing document or insert a document depending on the parameter. If the document does not contain an '_id' field, then
     * the method performs an insert with the specified fields in the document as well as an '_id' field with a unique objectid value. If
     * the document contains an '_id' field, then the method performs an upsert querying the collection on the '_id' field: <ul> <li>If a
     * document does not exist with the specified '_id' value, the method performs an insert with the specified fields in the document.</li>
     * <li>If a document exists with the specified '_id' value, the method performs an update, replacing all field in the existing record
     * with the fields from the document.</li> </ul>. Calls {@link DBCollection#save(com.sequoiadb.BSONObject, com.sequoiadb.WriteConcern)} with
     * default WriteConcern
     *
     * @param jo {@link BSONObject} to save to the collection.
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public WriteResult save( BSONObject jo ){
        return save(jo, null);
    }

    /**
     * Update an existing document or insert a document depending on the parameter. If the document does not contain an '_id' field, then
     * the method performs an insert with the specified fields in the document as well as an '_id' field with a unique objectid value. If
     * the document contains an '_id' field, then the method performs an upsert querying the collection on the '_id' field: <ul> <li>If a
     * document does not exist with the specified '_id' value, the method performs an insert with the specified fields in the document.</li>
     * <li>If a document exists with the specified '_id' value, the method performs an update, replacing all field in the existing record
     * with the fields from the document.</li> </ul>
     *
     * @param jo      {@link BSONObject} to save to the collection.
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws BaseException if the operation fails
     */
    public WriteResult save( BSONObject jo, WriteConcern concern ){

        Object id = jo.get( "_id" );

        if (id == null || (id instanceof ObjectId && ((ObjectId) id).isNew())) {
            if (id != null) {
                ((ObjectId) id).notNew();
            }
            return insert(jo);
        }

        BSONObject q = new BasicBSONObject();
        q.put("_id", id);
        BSONObject m = new BasicBSONObject();
        m.put("$replace", jo);
        return update(q, m, true, false, concern);
    }

    /**
     * Drops all indices from this collection
     * @throws BaseException
     */
    public void dropIndexes(){
        execute(new CLCallback<Void>(){
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                List<String> indexNames = new ArrayList<String>();
                com.sequoiadb.base.DBCursor cursor = cl.getIndexes();
                while(cursor.hasNext()) {
                    BSONObject obj = cursor.getNext();
                    BSONObject indexDef = (BSONObject)obj.get("IndexDef");
                    String name = (String)indexDef.get("name");
                    indexNames.add(name);
                }
                cursor.close();
                for(String name : indexNames) {
                    cl.dropIndex(name);
                }
                return null;
            }
        });
    }


    /**
     * Drops an index from this collection
     * @param name the index name
     * @throws BaseException
     */
    public void dropIndexes( String name ){
        final String myName = name;
        execute(new CLCallback<Void>() {
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                cl.dropIndex(myName);
                return null;
            }
        });
    }

    /**
     * Drops (deletes) this collection. Use with care.
     * @throws BaseException
     */
    public void drop(){
        _cm.execute(_csName, new CSCallback<Void>(){
            @Override
            public Void doInCS(CollectionSpace cs) throws com.sequoiadb.exception.BaseException {
                cs.dropCollection(_clName);
                return null;
            }
        });
    }

    /**
     * Get the number of documents in the collection.
     *
     * @return the number of documents
     * @throws BaseException
     */
    public long count(){
        return getCount(new BasicBSONObject(), null);
    }

    /**
     * Get the count of documents in collection that would match a criteria.
     *
     * @param query specifies the selection criteria
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long count(BSONObject query){
        return getCount(query, null);
    }

    /**
     * Get the count of documents in collection that would match a criteria.
     *
     * @param query     specifies the selection criteria
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long count(BSONObject query, ReadPreference readPrefs ){
        return getCount(query, null, readPrefs);
    }


    /**
     * Get the count of documents in a collection.  Calls {@link DBCollection#getCount(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject)} with an
     * empty query and null fields.
     *
     * @return the number of documents in the collection
     * @throws BaseException
     */
    public long getCount(){
        return getCount(new BasicBSONObject(), null);
    }

    /**
     * Get the count of documents in a collection. Calls {@link DBCollection#getCount(com.sequoiadb.BSONObject, com.sequoiadb.BSONObject,
     * com.sequoiadb.ReadPreference)} with empty query and null fields.
     *
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long getCount(ReadPreference readPrefs){
        return getCount(new BasicBSONObject(), null, readPrefs);
    }

    /**
     * Get the count of documents in collection that would match a criteria. Calls {@link DBCollection#getCount(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject)} with null fields.
     *
     * @param query specifies the selection criteria
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long getCount(BSONObject query){
        return getCount(query, null);
    }


    /**
     * Get the count of documents in collection that would match a criteria. Calls {@link DBCollection#getCount(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, long, long)} with limit=0 and skip=0
     *
     * @param query  specifies the selection criteria
     * @param fields this is ignored
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long getCount(BSONObject query, BSONObject fields){
        return getCount( query , fields , 0 , 0 );
    }

    /**
     * Get the count of documents in collection that would match a criteria.  Calls {@link DBCollection#getCount(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, long, long, com.sequoiadb.ReadPreference)} with limit=0 and skip=0
     *
     * @param query     specifies the selection criteria
     * @param fields    this is ignored
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long getCount(BSONObject query, BSONObject fields, ReadPreference readPrefs){
        return getCount( query , fields , 0 , 0, readPrefs );
    }

    /**
     * Get the count of documents in collection that would match a criteria.  Calls {@link DBCollection#getCount(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, long, long, com.sequoiadb.ReadPreference)} with the DBCollection's ReadPreference
     *
     * @param query          specifies the selection criteria
     * @param fields     this is ignored
     * @param limit          limit the count to this value
     * @param skip           number of documents to skip
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long getCount(BSONObject query, BSONObject fields, long limit, long skip){
        return getCount(query, fields, limit, skip, getReadPreference());
    }

    /**
     * Get the count of documents in collection that would match a criteria.
     *
     * @param query     specifies the selection criteria
     * @param fields    this is ignored
     * @param limit     limit the count to this value
     * @param skip      number of documents to skip
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     */
    public long getCount(BSONObject query, BSONObject fields, long limit, long skip, ReadPreference readPrefs ){
        return getCount(query, fields, limit, skip, readPrefs, 0, MILLISECONDS);
    }

    /**
     * Get the count of documents in collection that would match a criteria.
     *
     * @param query       specifies the selection criteria
     * @param fields      this is ignored
     * @param limit       limit the count to this value
     * @param skip        number of documents to skip
     * @param readPrefs   {@link ReadPreference} to be used for this operation
     * @param maxTime     the maximum time that the server will allow this operation to execute before killing it
     * @param maxTimeUnit the unit that maxTime is specified in
     * @return the number of documents that matches selection criteria
     * @throws BaseException
     * @since 2.12
     */
    long getCount(final BSONObject query, final BSONObject fields, final long limit, final long skip,
                  final ReadPreference readPrefs, final long maxTime, final TimeUnit maxTimeUnit) {
        final BSONObject myHint = (_hintFields.size() > 0) ? _hintFields.get(0) : null;
        return execute(new CLCallback<Long>() {
            @Override
            public Long doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                return cl.getCount(query, myHint);
            }
        });
    }

    /**
     * Calls {@link DBCollection#rename(java.lang.String, boolean)} with dropTarget=false
     * @param newName new collection name (not a full namespace)
     * @return the new collection
     * @throws BaseException
     */
    public DBCollection rename( String newName ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Renames of this collection to newName
     * @param newName new collection name (not a full namespace)
     * @param dropTarget if a collection with the new name exists, whether or not to drop it
     * @return the new collection
     * @throws BaseException
     */
    public DBCollection rename( String newName, boolean dropTarget ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL. Calls {@link DBCollection#group(com.sequoiadb.BSONObject,
     * com.sequoiadb.BSONObject, com.sequoiadb.BSONObject, java.lang.String, java.lang.String)} with finalize=null
     *
     * @param key     specifies one or more document fields to group
     * @param cond    specifies the selection criteria to determine which documents in the collection to process
     * @param initial initializes the aggregation result document
     * @param reduce  specifies an $reduce Javascript function, that operates on the documents during the grouping operation
     * @return a document with the grouped records as well as the command meta-data
     * @throws BaseException
     */
    public BSONObject group( BSONObject key , BSONObject cond , BSONObject initial , String reduce ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param key      specifies one or more document fields to group
     * @param cond     specifies the selection criteria to determine which documents in the collection to process
     * @param initial  initializes the aggregation result document
     * @param reduce   specifies an $reduce Javascript function, that operates on the documents during the grouping operation
     * @param finalize specifies a Javascript function that runs each item in the result set before final value will be returned
     * @return a document with the grouped records as well as the command meta-data
     * @throws BaseException
     */
    public BSONObject group( BSONObject key , BSONObject cond , BSONObject initial , String reduce , String finalize ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param key       specifies one or more document fields to group
     * @param cond      specifies the selection criteria to determine which documents in the collection to process
     * @param initial   initializes the aggregation result document
     * @param reduce    specifies an $reduce Javascript function, that operates on the documents during the grouping operation
     * @param finalize  specifies a Javascript function that runs each item in the result set before final value will be returned
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return a document with the grouped records as well as the command meta-data
     * @throws BaseException
     */
    public BSONObject group( BSONObject key , BSONObject cond , BSONObject initial , String reduce , String finalize, ReadPreference readPrefs ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param cmd the group command containing the details of how to perform the operation.
     * @return a document with the grouped records as well as the command meta-data
     * @throws BaseException
     */
    public BSONObject group( GroupCommand cmd ) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param cmd       the group command containing the details of how to perform the operation.
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return a document with the grouped records as well as the command meta-data
     * @throws BaseException
     */
    public BSONObject group( GroupCommand cmd, ReadPreference readPrefs ) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param args object representing the arguments to the group function
     * @return a document with the grouped records as well as the command meta-data
     * @throws BaseException
     * @deprecated use {@link DBCollection#group(com.sequoiadb.GroupCommand)} instead.  This method will be removed in 3.0
     */
    @Deprecated
    public BSONObject group( BSONObject args ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find the distinct values for a specified field across a collection and returns the results in an array.
     *
     * @param key Specifies the field for which to return the distinct values
     * @return A {@code List} of the distinct values
     * @throws BaseException
     */
    public List distinct( String key ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find the distinct values for a specified field across a collection and returns the results in an array.
     *
     * @param key       Specifies the field for which to return the distinct values
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return A {@code List} of the distinct values
     * @throws BaseException
     */
    public List distinct( String key, ReadPreference readPrefs ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find the distinct values for a specified field across a collection and returns the results in an array.
     *
     * @param key   Specifies the field for which to return the distinct values
     * @param query specifies the selection query to determine the subset of documents from which to retrieve the distinct values
     * @return A {@code List} of the distinct values
     * @throws BaseException
     */
    public List distinct( String key , BSONObject query ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find the distinct values for a specified field across a collection and returns the results in an array.
     *
     * @param key       Specifies the field for which to return the distinct values
     * @param query     specifies the selection query to determine the subset of documents from which to retrieve the distinct values
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return A {@code List} of the distinct values
     * @throws BaseException
     */
    public List distinct( String key , BSONObject query, ReadPreference readPrefs ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection.  Runs the command in REPLACE output mode (saves to named
     * collection).
     *
     * @param map            a JavaScript function that associates or "maps" a value with a key and emits the key and value pair.
     * @param reduce         a JavaScript function that "reduces" to a single object all the values associated with a particular key.
     * @param outputTarget   specifies the location of the result of the map-reduce operation (optional) - leave null if want to use temp
     *                       collection
     * @param query          specifies the selection criteria using query operators for determining the documents input to the map
     *                       function.
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws BaseException
     */
    public MapReduceOutput mapReduce( String map , String reduce , String outputTarget , BSONObject query ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection and saves to a named collection.
     * Specify an outputType to control job execution<ul>
     * <li>INLINE - Return results inline</li>
     * <li>REPLACE - Replace the output collection with the job output</li>
     * <li>MERGE - Merge the job output with the existing contents of outputTarget</li>
     * <li>REDUCE - Reduce the job output with the existing contents of outputTarget</li>
     * </ul>
     *
     * @param map          a JavaScript function that associates or "maps" a value with a key and emits the key and value pair.
     * @param reduce       a JavaScript function that "reduces" to a single object all the values associated with a particular key.
     * @param outputTarget specifies the location of the result of the map-reduce operation (optional) - leave null if want to use temp
     *                     collection
     * @param outputType   specifies the type of job output
     * @param query        specifies the selection criteria using query operators for determining the documents input to the map function.
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws BaseException
     */
    public MapReduceOutput mapReduce(String map, String reduce, String outputTarget, MapReduceCommand.OutputType outputType,
                                     BSONObject query) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection and saves to a named collection.
     * Specify an outputType to control job execution<ul>
     * <li>INLINE - Return results inline</li>
     * <li>REPLACE - Replace the output collection with the job output</li>
     * <li>MERGE - Merge the job output with the existing contents of outputTarget</li>
     * <li>REDUCE - Reduce the job output with the existing contents of outputTarget</li>
     * </ul>
     *
     * @param map            a JavaScript function that associates or "maps" a value with a key and emits the key and value pair.
     * @param reduce         a JavaScript function that "reduces" to a single object all the values associated with a particular key.
     * @param outputTarget   specifies the location of the result of the map-reduce operation (optional) - leave null if want to use temp
     *                       collection
     * @param outputType     specifies the type of job output
     * @param query          specifies the selection criteria using query operators for determining the documents input to the map
     *                       function.
     * @param readPrefs the read preference specifying where to run the query.  Only applied for Inline output type
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws BaseException
     */
    public MapReduceOutput mapReduce(String map, String reduce, String outputTarget, MapReduceCommand.OutputType outputType, BSONObject query,
                                     ReadPreference readPrefs) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection and saves to a named collection.
     *
     * @param command object representing the parameters to the operation
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws BaseException
     */
    public MapReduceOutput mapReduce( MapReduceCommand command ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection
     *
     * @param command document representing the parameters to this operation.
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws BaseException
     * @deprecated Use {@link com.sequoiadb.DBCollection#mapReduce(MapReduceCommand)} instead
     */
    @Deprecated
    public MapReduceOutput mapReduce( BSONObject command ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Method implements aggregation framework.
     *
     * @param firstOp       requisite first operation to be performed in the aggregation pipeline
     * @param additionalOps additional operations to be performed in the aggregation pipeline
     * @return the aggregation operation's result set
     * @deprecated Use {@link com.sequoiadb.DBCollection#aggregate(java.util.List)} instead
     *
     * @sequoiadb.server.release 2.2
     */
    @Deprecated
    @SuppressWarnings("unchecked")
    public AggregationOutput aggregate(final BSONObject firstOp, final BSONObject... additionalOps) {
        List<BSONObject> pipeline = new ArrayList<BSONObject>();
        pipeline.add(firstOp);
        Collections.addAll(pipeline, additionalOps);
        return aggregate(pipeline);
    }

    /**
     * Method implements aggregation framework.
     *
     * @param pipeline operations to be performed in the aggregation pipeline
     * @return the aggregation's result set
     *
     * @sequoiadb.server.release 2.2
     */
    public AggregationOutput aggregate(final List<BSONObject> pipeline) {

        final List<BSONObject> myList = convertBson(pipeline, false);

        LinkedList<BSONObject> list = execute(new CLCallback<LinkedList<BSONObject>>() {
            @Override
            public LinkedList<BSONObject> doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                LinkedList<BSONObject> ressult = new LinkedList<BSONObject>();
                com.sequoiadb.base.DBCursor cursor = cl.aggregate(myList);
                try {
                    while (cursor.hasNext()) {
                        ressult.add(cursor.getNext());
                    }
                } finally {
                    cursor.close();
                }
                return ressult;
            }
        });
        return new AggregationOutput(list);
    }

    /**
     * Method implements aggregation framework.
     *
     * @param pipeline       operations to be performed in the aggregation pipeline
     * @param readPreference the read preference specifying where to run the query
     * @return the aggregation's result set
     *
     * @sequoiadb.server.release 2.2
     */
    public AggregationOutput aggregate(final List<BSONObject> pipeline, ReadPreference readPreference) {
        return aggregate(pipeline);
    }

    /**
     * Method implements aggregation framework.
     *
     * @param pipeline operations to be performed in the aggregation pipeline
     * @param options  options to apply to the aggregation
     * @return the aggregation operation's result set
     *
     * @sequoiadb.server.release 2.2
     */
    public Cursor aggregate(final List<BSONObject> pipeline, AggregationOptions options) {
        final List<BSONObject> myList = convertBson(pipeline, false);
        DBQueryResult result =
                _cm.execute(_csName, _clName, new QueryInCLCallback<DBQueryResult>(){
                    public DBQueryResult doQuery(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                        com.sequoiadb.base.DBCursor cursor =
                                cl.aggregate(myList);
                        return new DBQueryResult(cursor, cl.getSequoiadb());
                    }
                });
        return new DBCursor(new QueryResultIterator(_cm, result));
    }

    /**
     * Method implements aggregation framework.
     *
     * @param pipeline operations to be performed in the aggregation pipeline
     * @param options options to apply to the aggregation
     * @param readPreference {@link ReadPreference} to be used for this operation
     * @return the aggregation operation's result set
     *
     * @sequoiadb.server.release 2.2
     */
    public abstract Cursor aggregate(final List<BSONObject> pipeline, final AggregationOptions options,
                                     final ReadPreference readPreference);

    /**
     * Return the explain plan for the aggregation pipeline.
     *
     * @param pipeline the aggregation pipeline to explain
     * @param options  the options to apply to the aggregation
     * @return the command result.  The explain output may change from release to release, so best to simply log this.
     *
     * @sequoiadb.server.release 2.6
     */
    public CommandResult explainAggregate(List<BSONObject> pipeline, AggregationOptions options) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Return a list of cursors over the collection that can be used to scan it in parallel.
     * <p>
     *     Note: As of SequoiaDB 2.6, this method will work against a sdb, but not a sdb.
     * </p>
     *
     * @param options the parallel scan options
     * @return a list of cursors, whose size may be less than the number requested
     * @since 2.12
     *
     * @sequoiadb.server.release 2.6
     */
    public abstract List<Cursor> parallelScan(final ParallelScanOptions options);

    /**
     * Creates a builder for an ordered bulk write operation, consisting of an ordered collection of write requests,
     * which can be any combination of inserts, updates, replaces, or removes. Write requests included in the bulk operations will be
     * executed in order, and will halt on the first failure.
     * <p>
     * Note: While this bulk write operation will execute on SequoiaDB 2.4 servers and below, the writes will be performed one at a time,
     * as that is the only way to preserve the semantics of the value returned from execution or the exception thrown.
     * <p>
     * Note: While a bulk write operation with a mix of inserts, updates, replaces, and removes is supported,
     * the implementation will batch up consecutive requests of the same type and send them to the server one at a time.  For example,
     * if a bulk write operation consists of 10 inserts followed by 5 updates, followed by 10 more inserts,
     * it will result in three round trips to the server.
     *
     * @return the builder
     *
     * @since 2.12
     */
    public BulkWriteOperation initializeOrderedBulkOperation() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Creates a builder for an unordered bulk operation, consisting of an unordered collection of write requests,
     * which can be any combination of inserts, updates, replaces, or removes. Write requests included in the bulk operation will be
     * executed in an undefined  order, and all requests will be executed even if some fail.
     * <p>
     * Note: While this bulk write operation will execute on SequoiaDB 2.4 servers and below, the writes will be performed one at a time,
     * as that is the only way to preserve the semantics of the value returned from execution or the exception thrown.
     *
     * @return the builder
     *
     * @since 2.12
     */
    public BulkWriteOperation initializeUnorderedBulkOperation() {
        throw new UnsupportedOperationException("not supported!");
    }


    /**
     * Return a list of the indexes for this collection.  Each object in the list is the "info document" from SequoiaDB
     *
     * @return list of index documents
     * @throws BaseException
     */
    public List<BSONObject> getIndexInfo() {
        return execute(new CLCallback<List<BSONObject>>() {
            @Override
            public List<BSONObject> doInCL(com.sequoiadb.base.DBCollection cl) throws com.sequoiadb.exception.BaseException {
                List<BSONObject> list = new ArrayList<BSONObject>();
                com.sequoiadb.base.DBCursor cursor = cl.getIndexes();
                try {
                    while (cursor.hasNext()) {
                        list.add(cursor.getNext());
                    }
                } finally {
                    cursor.close();
                }
                return list;
            }
        });
    }

    /**
     * Drops an index from this collection
     * @param keys keys of the index
     * @throws BaseException
     */
    public void dropIndex( BSONObject keys ){
        dropIndexes( genIndexName( keys ) );
    }

    /**
     * Drops an index from this collection
     * @param name name of index to drop
     * @throws BaseException
     */
    public void dropIndex( String name ){
        dropIndexes( name );
    }

    /**
     * The collStats command returns a variety of storage statistics for a given collection
     *
     * @return a CommandResult containing the statistics about this collection
     */
    public CommandResult getStats() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Checks whether this collection is capped
     *
     * @return true if this is a capped collection
     * @throws BaseException
     */
    public boolean isCapped() {
        return false;
    }


    /**
     * @deprecated This method should not be a part of API.
     *             If you override one of the {@code DBCollection} methods please rely on superclass
     *             implementation in checking argument correctness and validity.
     */
    @Deprecated
    protected BSONObject _checkObject(BSONObject o, boolean canBeNull, boolean query) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find a collection that is prefixed with this collection's name. A typical use of this might be
     * <pre>{@code
     *    DBCollection users = sdb.getCollection( "wiki" ).getCollection( "users" );
     * }</pre>
     * Which is equivalent to
     * <pre>{@code
     *   DBCollection users = sdb.getCollection( "wiki.users" );
     * }</pre>
     *
     * @param n the name of the collection to find
     * @return the matching collection
     */
    public DBCollection getCollection( String n ){
        String csName = null;
        String clName = null;
        if (n == null || n.isEmpty()) {
            throw new IllegalArgumentException("invalid collection name!");
        }
        if (n.indexOf(".") > 0) {
            String[] tmp = n.split("\\.");
            csName = _csName;
            clName = tmp[1];
        } else {
            csName = _csName;
            clName = n;
        }
        boolean isExist = false;
        final String myCLName = clName;
        try {
            isExist = _cm.execute(csName, new CSCallback<Boolean>() {
                @Override
                public Boolean doInCS(CollectionSpace cs) throws com.sequoiadb.exception.BaseException {
                    return cs.isCollectionExist(myCLName);
                }
            });
        } catch (com.sequoiadb.exception.BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                isExist = false;
            } else {
                throw e;
            }
        }
        if (isExist) {
            return new DBCollectionImpl((DBApiLayer)_db, myCLName);
        } else {
            return null;
        }
    }

    /**
     * Returns the name of this collection.
     * @return  the name of this collection
     */
    public String getName(){
        return _name;
    }

    /**
     * Returns the full name of this collection, with the database name as a prefix.
     * @return  the name of this collection
     */
    public String getFullName(){
        return _fullName;
    }

    /**
     * Returns the database this collection is a member of.
     * @return this collection's database
     */
    public DB getDB(){
        return _db;
    }

    /**
     * Returns if this collection's database is read-only
     *
     * @param strict if an exception should be thrown if the database is read-only
     * @return if this collection's database is read-only
     * @throws RuntimeException if the database is read-only and {@code strict} is set
     * @deprecated See {@link DB#setReadOnly(Boolean)}
     */
    @Deprecated
    protected boolean checkReadOnly( boolean strict ){
        throw new UnsupportedOperationException("not supported!");
    }

    @Override
    public int hashCode(){
        return _fullName.hashCode();
    }

    @Override
    public boolean equals( Object o ){
        return o == this;
    }

    @Override
    public String toString(){
        return _name;
    }

    /**
     * Sets a default class for objects in this collection; null resets the class to nothing.
     * @param c the class
     * @throws IllegalArgumentException if {@code c} is not a BSONObject
     */
    public void setObjectClass( Class c ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the default class for objects in the collection
     *
     * @return the class
     */
    public Class getObjectClass(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the internal class for the given path in the document hierarchy
     *
     * @param path the path to map the given Class to
     * @param c    the Class to map the given path to
     */
    public void setInternalClass( String path , Class c ){
        throw new UnsupportedOperationException("not supported!");

    }

    /**
     * Gets the internal class for the given path in the document hierarchy
     *
     * @param path the path to map the given Class to
     * @return the class for a given path in the hierarchy
     */
    protected Class getInternalClass( String path ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Set the write concern for this collection. Will be used for
     * writes to this collection. Overrides any setting of write
     * concern at the DB level. See the documentation for
     * {@link WriteConcern} for more information.
     *
     * @param concern write concern to use
     */
    public void setWriteConcern( WriteConcern concern ){
    }

    /**
     * Get the {@link WriteConcern} for this collection.
     *
     * @return the default write concern for this collection
     */
    public WriteConcern getWriteConcern(){
        return null;
    }

    /**
     * Sets the read preference for this collection. Will be used as default
     * for reads from this collection; overrides DB & Connection level settings.
     * See the * documentation for {@link ReadPreference} for more information.
     *
     * @param preference Read Preference to use
     */
    public void setReadPreference( ReadPreference preference ){
    }

    /**
     * Gets the {@link ReadPreference}.
     *
     * @return the default read preference for this collection
     */
    public ReadPreference getReadPreference(){
        return null;
    }
    /**
     * Makes this query ok to run on a slave node
     *
     * @deprecated Replaced with {@link ReadPreference#secondaryPreferred()}
     */
    @Deprecated
    public void slaveOk(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Adds the given flag to the query options.
     *
     * @param option value to be added
     */
    public void addOption( int option ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Sets the query options, overwriting previous value.
     *
     * @param options bit vector of query options
     */
    public void setOptions( int options ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Resets the default query options
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
     * Set a customer decoder factory for this collection.  Set to null to use the default from SequoiadbOptions.
     *
     * @param fact the factory to set.
     */
    public synchronized void setDBDecoderFactory(DBDecoderFactory fact) {
    }

    /**
     * Get the decoder factory for this collection.  A null return value means that the default from SequoiadbOptions is being used.
     *
     * @return the factory
     */
    public synchronized DBDecoderFactory getDBDecoderFactory() {
        return null;
    }

    /**
     * Set a customer encoder factory for this collection.  Set to null to use the default from SequoiadbOptions.
     *
     * @param fact the factory to set.
     */
    public synchronized void setDBEncoderFactory(DBEncoderFactory fact) {
    }

    /**
     * Get the encoder factory for this collection.  A null return value means that the default from SequoiadbOptions is being used.
     *
     * @return the factory
     */
    public synchronized DBEncoderFactory getDBEncoderFactory() {
        return null;
    }


}

