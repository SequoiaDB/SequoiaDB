package org.springframework.data.mongodb.assist;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.exception.BaseException;
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
 * MongoClient mongoClient = new MongoClient(new ServerAddress("localhost", 27017));
 * DB db = mongo.getDB("mydb");
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
 * BasicDBObject doc = new BasicDBObject("name", "MongoDB").append("type", "database")
 *                                                         .append("count", 1)
 *                                                         .append("info", new BasicDBObject("x", 203).append("y", 102));
 * coll.insert(doc); }
 * </pre>
 * To show that the document we inserted in the previous step is there, we can do a simple findOne() operation to get the first document in
 * the collection:
 * <pre>
 * {@code
 * DBObject myDoc = coll.findOne();
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
    protected List<DBObject> _hintFields = new ArrayList<DBObject>();

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

    protected List<BSONObject> convertBson(List<DBObject> list, boolean appendOID) {
        List<BSONObject> myList = new ArrayList<BSONObject>(list.size());
        for(DBObject obj : list) {
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
     * @param arr     {@code DBObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @dochub insert Insert
     */
    public WriteResult insert(DBObject[] arr , WriteConcern concern ){
        return insert( arr, concern, null);
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param arr     {@code DBObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder {@code DBEncoder} to be used
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @dochub insert Insert
     */
    public WriteResult insert(DBObject[] arr , WriteConcern concern, DBEncoder encoder) {
        return insert(Arrays.asList(arr), concern, encoder);
    }

    /**
     * Insert a document into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param o       {@code DBObject} to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @dochub insert Insert
     */
    public WriteResult insert(DBObject o , WriteConcern concern ){
        return insert( Arrays.asList(o) , concern );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added. Collection wide {@code WriteConcern} will be used.
     *
     * @param arr {@code DBObject}'s to be inserted
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/insert-documents/ Insert
     */
    public WriteResult insert(DBObject ... arr){
        return insert( arr , getWriteConcern() );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param arr     {@code DBObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/insert-documents/ Insert
     */
    public WriteResult insert(WriteConcern concern, DBObject ... arr){
        return insert( arr, concern );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param list list of {@code DBObject} to be inserted
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/insert-documents/ Insert
     */
    public WriteResult insert(List<DBObject> list ){
        return insert( list, getWriteConcern() );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param list    list of {@code DBObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/insert-documents/ Insert
     */
    public WriteResult insert(List<DBObject> list, WriteConcern concern ){
        return insert(list, concern, null );
    }

    /**
     * Insert documents into a collection. If the collection does not exists on the server, then it will be created. If the new document
     * does not contain an '_id' field, it will be added.
     *
     * @param list    a list of {@code DBObject}'s to be inserted
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder {@code DBEncoder} to use to serialise the documents
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/insert-documents/ Insert
     */
    public abstract WriteResult insert(List<DBObject> list, WriteConcern concern, DBEncoder encoder);

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(DBObject)}.
     *
     * @param q       the selection criteria for the update
     * @param o       the modifications to apply
     * @param upsert  when true, inserts a document if no document matches the update query criteria
     * @param multi   when true, updates all documents in the collection that match the update query criteria, otherwise only updates one
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/modify-documents/ Modify
     */
    public WriteResult update( DBObject q , DBObject o , boolean upsert , boolean multi , WriteConcern concern ){
        return update( q, o, upsert, multi, concern, null);
    }

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(DBObject)}.
     *
     * @param matcher       the selection criteria for the update
     * @param rule       the modifications to apply
     * @param upsert  when true, inserts a document if no document matches the update query criteria
     * @param multi   when true, updates all documents in the collection that match the update query criteria, otherwise only updates one
     * @param concern {@code WriteConcern} to be used during operation
     * @param encoder the DBEncoder to use
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/modify-documents/ Modify
     */
    public abstract WriteResult update(DBObject matcher , DBObject rule , boolean upsert , boolean multi , WriteConcern concern, DBEncoder encoder );

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.
     *
     * @param matcher       the selection criteria for the update
     * @param rule       the modifications to apply
     * @param hint       the index to apply
     * @param upsert  when true, inserts a document if no document matches the update query criteria
     * @return the result of the operation
     * @throws MongoException
     */
    public abstract WriteResult update(DBObject matcher, DBObject rule, DBObject hint, boolean upsert);

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(BSONObject)}.  Calls {@link DBCollection#update(com.mongodb.DBObject,
     * com.mongodb.DBObject, boolean, boolean, com.mongodb.WriteConcern)} with default WriteConcern.
     *
     * @param matcher      the selection criteria for the update
     * @param rule      the modifications to apply
     * @param upsert when true, inserts a document if no document matches the update query criteria
     * @param multi  when true, updates all documents in the collection that match the update query criteria, otherwise only updates one
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/modify-documents/ Modify
     */
    public WriteResult update(DBObject matcher, DBObject rule, boolean upsert , boolean multi ){
        return update( matcher , rule , upsert , multi , getWriteConcern() );
    }

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find(DBObject)}.  Calls {@link DBCollection#update(com.mongodb.DBObject,
     * com.mongodb.DBObject, boolean, boolean)} with upsert=false and multi=false
     *
     * @param matcher the selection criteria for the update
     * @param rule the modifications to apply
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/modify-documents/ Modify
     */
    public WriteResult update(DBObject matcher , DBObject rule){
        return update( matcher , rule , false , false );
    }

    /**
     * Modify an existing document or documents in collection. By default the method updates a single document. The query parameter employs
     * the same query selectors, as used in {@link DBCollection#find()}.  Calls {@link DBCollection#update(com.mongodb.DBObject,
     * com.mongodb.DBObject, boolean, boolean)} with upsert=false and multi=true
     *
     * @param matcher the selection criteria for the update
     * @param rule the modifications to apply
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/modify-documents/ Modify
     */
    public WriteResult updateMulti(DBObject matcher, DBObject rule ){
        return update( matcher , rule , false , true );
    }

    /**
     * Adds any necessary fields to a given object before saving it to the collection.
     * @param o object to which to add the fields
     */
    protected abstract void doapply( DBObject o );

    /**
     * Remove documents from a collection.
     *
     * @param o       the deletion criteria using query operators. Omit the query parameter or pass an empty document to delete all
     *                documents in the collection.
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/remove-documents/ Remove
     */
    public WriteResult remove( DBObject o , WriteConcern concern ){
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
     * @throws MongoException
     * @mongodb.driver.manual tutorial/remove-documents/ Remove
     */
    public abstract WriteResult remove( DBObject o , WriteConcern concern, DBEncoder encoder );

    /**
     * Remove documents from a collection. Calls {@link DBCollection#remove(com.mongodb.DBObject, com.mongodb.WriteConcern)} with the
     * default WriteConcern
     *
     * @param o the deletion criteria using query operators. Omit the query parameter or pass an empty document to delete all documents in
     *          the collection.
     * @return the result of the operation
     * @throws MongoException
     * @mongodb.driver.manual tutorial/remove-documents/ Remove
     */
    public WriteResult remove( DBObject o ){
        return remove( o , getWriteConcern() );
    }


    /**
     * Finds objects
     */
    abstract QueryResultIterator find(DBObject ref, DBObject fields, int numToSkip, int batchSize, int limit, int options,
                                      ReadPreference readPref, DBDecoder decoder);

    abstract QueryResultIterator find(DBObject ref, DBObject fields, int numToSkip, int batchSize, int limit, int options,
                                      ReadPreference readPref, DBDecoder decoder, DBEncoder encoder);

    abstract QueryResultIterator findInternal(final BSONObject matcher, final BSONObject selector,
                                              final BSONObject orderBy, final BSONObject hint,
                                              final long skipRows, final long returnRows, final int flags);
    /**
     * Calls {@link DBCollection#find(com.mongodb.DBObject, com.mongodb.DBObject, int, int)} and applies the query options
     *
     * @param query     query used to search
     * @param fields    the fields of matching objects to return
     * @param numToSkip number of objects to skip
     * @param batchSize the batch size. This option has a complex behavior, see {@link DBCursor#batchSize(int) }
     * @param options   see {@link com.mongodb.Bytes} QUERYOPTION_*
     * @return the cursor
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     * @deprecated use {@link com.mongodb.DBCursor#skip(int)}, {@link com.mongodb.DBCursor#batchSize(int)} and {@link
     * com.mongodb.DBCursor#setOptions(int)} on the {@code DBCursor} returned from {@link com.mongodb.DBCollection#find(DBObject,
     * DBObject)}
     */
    @Deprecated
    public DBCursor find( DBObject query , DBObject fields , int numToSkip , int batchSize , int options ){
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
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     * @deprecated use {@link com.mongodb.DBCursor#skip(int)} and {@link com.mongodb.DBCursor#batchSize(int)} on the {@code DBCursor}
     * returned from {@link com.mongodb.DBCollection#find(DBObject, DBObject)}
     */
    @Deprecated
    public DBCursor find( DBObject query , DBObject fields , int numToSkip , int batchSize ) {
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
     * @throws MongoException
     */
    public DBCursor find(BSONObject matcher, BSONObject selector, BSONObject orderBy, BSONObject hint,
                         long skipRows, long returnRows, int flags) {
        QueryResultIterator resultIterator =  findInternal(matcher, selector, orderBy, hint, skipRows, returnRows, flags);
        return new DBCursor(resultIterator);
    }

    // ------

    /**
     * Finds an object by its id.
     * This compares the passed in value to the _id field of the document
     *
     * @param obj any valid object
     * @return the object, if found, otherwise null
     * @throws MongoException
     */
    public DBObject findOne( Object obj ){
        return findOne(obj, null);
    }


    /**
     * Finds an object by its id.
     * This compares the passed in value to the _id field of the document
     *
     * @param obj any valid object
     * @param fields fields to return
     * @return the object, if found, otherwise null
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne( Object obj, DBObject fields ){
        Iterator<DBObject> iterator = find(new BasicDBObject("_id", obj), fields, 0, -1,
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/findAndModify/ Find and Modify
     */
    public DBObject findAndModify(DBObject query, DBObject fields, DBObject sort, boolean remove, DBObject update, boolean returnNew, boolean upsert){
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
     * @mongodb.driver.manual reference/command/findAndModify/ Find and Modify
     * @since 2.12.0
     */
    public DBObject findAndModify(final DBObject query, final DBObject fields, final DBObject sort,
                                  final boolean remove, final DBObject update,
                                  final boolean returnNew, final boolean upsert,
                                  final long maxTime, final TimeUnit maxTimeUnit) {

        return _cm.execute(_csName, _clName, new CLCallback<DBObject>() {
            @Override
            public DBObject doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
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
                return Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject) ret);
            }
        });
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.  Calls {@link DBCollection#findAndModify(com.mongodb.DBObject, com.mongodb.DBObject, com.mongodb.DBObject, boolean,
     * com.mongodb.DBObject, boolean, boolean)} with fields=null, remove=false, returnNew=false, upsert=false
     *
     * @param query  specifies the selection criteria for the modification
     * @param sort   determines which document the operation will modify if the query selects multiple documents
     * @param update the modifications to apply
     * @return the document as it was before the modifications.
     * @throws MongoException
     * @mongodb.driver.manual reference/command/findAndModify/ Find and Modify
     */
    public DBObject findAndModify( DBObject query , DBObject sort , DBObject update) {
        return findAndModify( query, null, sort, false, update, false, false);
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.  Calls {@link DBCollection#findAndModify(com.mongodb.DBObject, com.mongodb.DBObject, com.mongodb.DBObject, boolean,
     * com.mongodb.DBObject, boolean, boolean)} with fields=null, sort=null, remove=false, returnNew=false, upsert=false
     *
     * @param query  specifies the selection criteria for the modification
     * @param update the modifications to apply
     * @return the document as it was before the modifications.
     * @throws MongoException
     * @mongodb.driver.manual reference/command/findAndModify/ Find and Modify
     */
    public DBObject findAndModify( DBObject query , DBObject update ){
        return findAndModify( query, null, null, false, update, false, false );
    }

    /**
     * Atomically modify and return a single document. By default, the returned document does not include the modifications made on the
     * update.  Ccalls {@link DBCollection#findAndModify(com.mongodb.DBObject, com.mongodb.DBObject, com.mongodb.DBObject, boolean,
     * com.mongodb.DBObject, boolean, boolean)} with fields=null, sort=null, remove=true, returnNew=false, upsert=false
     *
     * @param query specifies the selection criteria for the modification
     * @return the document as it was before it was removed
     * @throws MongoException
     * @mongodb.driver.manual reference/command/findAndModify/ Find and Modify
     */
    public DBObject findAndRemove( DBObject query ) {
        return findAndModify( query, null, null, true, null, false, false );
    }

    // --- START INDEX CODE ---

    /**
     * Calls {@link DBCollection#createIndex(com.mongodb.DBObject, com.mongodb.DBObject)} with default index options
     *
     * @param keys a document that contains pairs with the name of the field or fields to index and order of the index
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     */
    public void createIndex( final DBObject keys ){
        createIndex( keys , null );
    }

    /**
     * Forces creation of an index on a set of fields, if one does not already exist.
     *
     * @param keys    a document that contains pairs with the name of the field or fields to index and order of the index
     * @param options a document that controls the creation of the index.
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     */
    public void createIndex( DBObject keys , DBObject options ){
        createIndex( keys, options, null);
    }

    /**
     * Forces creation of an index on a set of fields, if one does not already exist.
     *
     * @param keys    a document that contains pairs with the name of the field or fields to index and order of the index
     * @param options a document that controls the creation of the index.
     * @param encoder specifies the encoder that used during operation
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     * @deprecated use {@link #createIndex(DBObject, com.mongodb.DBObject)} the encoder is not used.
     */
    @Deprecated
    public abstract void createIndex(DBObject keys, DBObject options, DBEncoder encoder);

    /**
     * Creates an ascending index on a field with default options, if one does not already exist.
     *
     * @param name name of field to index on
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     * @deprecated use {@link DBCollection#createIndex(DBObject)} instead
     */
    @Deprecated
    public void ensureIndex( final String name ){
        ensureIndex( new BasicDBObject( name , 1 ) );
    }

    /**
     * Calls {@link DBCollection#ensureIndex(com.mongodb.DBObject, com.mongodb.DBObject)} with default options
     * @param keys an object with a key set of the fields desired for the index
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     *
     * @deprecated use {@link DBCollection#createIndex(DBObject)} instead
     */
    @Deprecated
    public void ensureIndex( final DBObject keys ){
        ensureIndex( keys , "" );
    }

    /**
     * Calls {@link DBCollection#ensureIndex(com.mongodb.DBObject, java.lang.String, boolean)} with unique=false
     *
     * @param keys fields to use for index
     * @param name an identifier for the index
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     * @deprecated use {@link DBCollection#createIndex(DBObject, DBObject)} instead
     */
    @Deprecated
    public void ensureIndex( DBObject keys , String name ){
        ensureIndex( keys , name , false );
    }

    /**
     * Ensures an index on this collection (that is, the index will be created if it does not exist).
     *
     * @param keys   fields to use for index
     * @param name   an identifier for the index. If null or empty, the default name will be used.
     * @param unique if the index should be unique
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     * @deprecated use {@link DBCollection#createIndex(DBObject, DBObject)} instead
     */
    @Deprecated
    public void ensureIndex( DBObject keys , String name , boolean unique ){

        final DBObject myKeys = keys;
        final boolean myUnique = unique;
        // get index name
        final String indexName = (name == null || name.isEmpty()) ? genIndexName(keys) : name;

        // create index
        execute(new CLCallback<Void>() {
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
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
     * @throws MongoException
     * @mongodb.driver.manual /administration/indexes-creation/ Index Creation Tutorials
     * @deprecated use {@link DBCollection#createIndex(DBObject, DBObject)} instead
     */
    @Deprecated
    public void ensureIndex( final DBObject keys , final DBObject optionsIN ){
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

//    DBObject defaultOptions( DBObject keys ){
//        DBObject o = new BasicDBObject();
//        o.put( "name" , genIndexName( keys ) );
//        o.put( "ns" , _fullName );
//        return o;
//    }

    /**
     * Convenience method to generate an index name from the set of fields it is over.
     * @param keys the names of the fields used in this index
     * @return a string representation of this index's fields
     *
     * @deprecated This method is NOT a part of public API and will be dropped in 3.x versions.
     */
    @Deprecated
    public static String genIndexName( DBObject keys ){
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

    // --- END INDEX CODE ---

    /**
     * Set hint fields for this collection (to optimize queries).
     * @param lst a list of {@code DBObject}s to be used as hints
     */
    public void setHintFields( List<DBObject> lst ){
        _hintFields = lst;
    }

    /**
     * Get hint fields for this collection (used to optimize queries).
     * @return a list of {@code DBObject} to be used as hints.
     */
    protected List<DBObject> getHintFields() {
        return _hintFields;
    }

    /**
     * Queries for an object in this collection.
     *
     * @param ref A document outlining the search query
     * @return an iterator over the results
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBCursor find( DBObject ref ){
        return find( ref, null, 0, -1 );
    }

    /**
     * Queries for an object in this collection.
     * <p>
     * An empty DBObject will match every document in the collection.
     * Regardless of fields specified, the _id fields are always returned.
     * </p>
     * <p>
     * An example that returns the "x" and "_id" fields for every document
     * in the collection that has an "x" field:
     * </p>
     * <pre>
     * {@code
     * BasicDBObject keys = new BasicDBObject();
     * keys.put("x", 1);
     *
     * DBCursor cursor = collection.find(new BasicDBObject(), keys);}
     * </pre>
     *
     * @param ref object for which to search
     * @param keys fields to return
     * @return a cursor to iterate over results
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBCursor find( DBObject ref , DBObject keys ){
        return find( ref, keys, 0, -1 );
    }


    /**
     * Queries for all objects in this collection.
     *
     * @return a cursor which will iterate over every object
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBCursor find(){
        return find( null, null, 0, -1 );
    }

    /**
     * Returns a single object from this collection.
     *
     * @return the object found, or {@code null} if the collection is empty
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne(){
        return findOne( new BasicDBObject() );
    }

    /**
     * Returns a single object from this collection matching the query.
     * @param o the query object
     * @return the object found, or {@code null} if no such object exists
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne( DBObject o ){
        return findOne( o, null, null, null);
    }

    /**
     * Returns a single object from this collection matching the query.
     * @param o the query object
     * @param fields fields to return
     * @return the object found, or {@code null} if no such object exists
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne( DBObject o, DBObject fields ) {
        return findOne( o, fields, null, null);
    }

    /**
     * Returns a single object from this collection matching the query.
     * @param o the query object
     * @param fields fields to return
     * @param orderBy fields to order by
     * @return the object found, or {@code null} if no such object exists
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne( DBObject o, DBObject fields, DBObject orderBy){
        return findOne(o, fields, orderBy, null);
    }

    /**
     * Get a single document from collection.
     *
     * @param o        the selection criteria using query operators.
     * @param fields   specifies which fields MongoDB will return from the documents in the result set.
     * @param readPref {@link ReadPreference} to be used for this operation
     * @return A document that satisfies the query specified as the argument to this method, or {@code null} if no such object exists
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne( DBObject o, DBObject fields, ReadPreference readPref ){
        return findOne(o, fields, null, null);
    }

    /**
     * Get a single document from collection.
     *
     * @param o        the selection criteria using query operators.
     * @param fields   specifies which projection MongoDB will return from the documents in the result set.
     * @param orderBy  A document whose fields specify the attributes on which to sort the result set.
     * @param readPref {@code ReadPreference} to be used for this operation
     * @return A document that satisfies the query specified as the argument to this method, or {@code null} if no such object exists
     * @throws MongoException
     * @mongodb.driver.manual tutorial/query-documents/ Query
     */
    public DBObject findOne(DBObject o, DBObject fields, DBObject orderBy, ReadPreference readPref) {
        return findOne(o, fields, orderBy, null, 0, MILLISECONDS);
    }

    /**
     * Get a single document from collection.
     *
     * @param o           the selection criteria using query operators.
     * @param fields      specifies which projection MongoDB will return from the documents in the result set.
     * @param orderBy     A document whose fields specify the attributes on which to sort the result set.
     * @param readPref    {@code ReadPreference} to be used for this operation
     * @param maxTime     the maximum time that the server will allow this operation to execute before killing it
     * @param maxTimeUnit the unit that maxTime is specified in
     * @return A document that satisfies the query specified as the argument to this method.
     * @mongodb.driver.manual tutorial/query-documents/ Query
     * @since 2.12.0
     */
    DBObject findOne(DBObject o, DBObject fields, DBObject orderBy, ReadPreference readPref,
                     long maxTime, TimeUnit maxTimeUnit) {
        final DBObject myMatcher = o;
        final DBObject myFields = fields;
        final DBObject myOrderBy = orderBy;
        final DBObject myHint = (_hintFields.size() > 0) ? _hintFields.get(0) : null;

        DBObject obj =
                _cm.execute(_csName, _clName, new CLCallback<DBObject>(){
                    public DBObject doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                         BSONObject retObj = cl.queryOne(myMatcher, myFields, myOrderBy, myHint, 0);
                         return Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject)retObj);
                    }
                });
        return obj;
    }

//
//    // Only create a new decoder if there is a decoder factory explicitly set on the collection.  Otherwise return null
//    // so that DBPort will use a cached decoder from the default factory.
//    DBDecoder getDecoder() {
//        return getDBDecoderFactory() != null ? getDBDecoderFactory().create() : null;
//    }
//
//    // Only create a new encoder if there is an encoder factory explicitly set on the collection.  Otherwise return null
//    // to allow DB to create its own or use a cached one.
//    private DBEncoder getDBEncoder() {
//        return getDBEncoderFactory() != null ? getDBEncoderFactory().create() : null;
//    }
//

    /**
     * calls {@link DBCollection#apply(com.mongodb.DBObject, boolean)} with ensureID=true
     * @param o {@code DBObject} to which to add fields
     * @return the modified parameter object
     */
    public Object apply( DBObject o ){
        return apply( o , true );
    }

    /**
     * calls {@link DBCollection#doapply(com.mongodb.DBObject)}, optionally adding an automatic _id field
     * @param jo object to add fields to
     * @param ensureID whether to add an {@code _id} field
     * @return the modified object {@code o}
     */
    public Object apply( DBObject jo , boolean ensureID ){
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
     * with the fields from the document.</li> </ul>. Calls {@link DBCollection#save(com.mongodb.DBObject, com.mongodb.WriteConcern)} with
     * default WriteConcern
     *
     * @param jo {@link DBObject} to save to the collection.
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/modify-documents/#modify-a-document-with-save-method Save
     */
    public WriteResult save( DBObject jo ){
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
     * @param jo      {@link DBObject} to save to the collection.
     * @param concern {@code WriteConcern} to be used during operation
     * @return the result of the operation
     * @throws MongoException if the operation fails
     * @mongodb.driver.manual tutorial/modify-documents/#modify-a-document-with-save-method Save
     */
    public WriteResult save( DBObject jo, WriteConcern concern ){

        Object id = jo.get( "_id" );

        if (id == null || (id instanceof ObjectId && ((ObjectId) id).isNew())) {
            if (id != null) {
                ((ObjectId) id).notNew();
            }
            return insert(jo);
        }

        DBObject q = new BasicDBObject();
        q.put("_id", id);
        DBObject m = new BasicDBObject();
        m.put("$replace", jo);
        return update(q, m, true, false, concern);
    }

    // ---- DB COMMANDS ----
    /**
     * Drops all indices from this collection
     * @throws MongoException
     */
    public void dropIndexes(){
        execute(new CLCallback<Void>(){
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                List<String> indexNames = new ArrayList<String>();
                com.sequoiadb.base.DBCursor cursor = cl.getIndexes();
                while(cursor.hasNext()) {
                    BSONObject obj = cursor.getNext();
                    BSONObject indexDef = (BSONObject)obj.get("IndexDef");
                    String name = (String)indexDef.get("name");
                    indexNames.add(name);
                }
                cursor.close();
                // drop index
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
     * @throws MongoException
     */
    public void dropIndexes( String name ){
        final String myName = name;
        execute(new CLCallback<Void>() {
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                cl.dropIndex(myName);
                return null;
            }
        });
    }

    /**
     * Drops (deletes) this collection. Use with care.
     * @throws MongoException
     */
    public void drop(){
        _cm.execute(_csName, new CSCallback<Void>(){
            @Override
            public Void doInCS(CollectionSpace cs) throws BaseException {
                cs.dropCollection(_clName);
                return null;
            }
        });
    }

    /**
     * Get the number of documents in the collection.
     *
     * @return the number of documents
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long count(){
        return getCount(new BasicDBObject(), null);
    }

    /**
     * Get the count of documents in collection that would match a criteria.
     *
     * @param query specifies the selection criteria
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long count(DBObject query){
        return getCount(query, null);
    }

    /**
     * Get the count of documents in collection that would match a criteria.
     *
     * @param query     specifies the selection criteria
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long count(DBObject query, ReadPreference readPrefs ){
        return getCount(query, null, readPrefs);
    }


    /**
     * Get the count of documents in a collection.  Calls {@link DBCollection#getCount(com.mongodb.DBObject, com.mongodb.DBObject)} with an
     * empty query and null fields.
     *
     * @return the number of documents in the collection
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(){
        return getCount(new BasicDBObject(), null);
    }

    /**
     * Get the count of documents in a collection. Calls {@link DBCollection#getCount(com.mongodb.DBObject, com.mongodb.DBObject,
     * com.mongodb.ReadPreference)} with empty query and null fields.
     *
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(ReadPreference readPrefs){
        return getCount(new BasicDBObject(), null, readPrefs);
    }

    /**
     * Get the count of documents in collection that would match a criteria. Calls {@link DBCollection#getCount(com.mongodb.DBObject,
     * com.mongodb.DBObject)} with null fields.
     *
     * @param query specifies the selection criteria
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(DBObject query){
        return getCount(query, null);
    }


    /**
     * Get the count of documents in collection that would match a criteria. Calls {@link DBCollection#getCount(com.mongodb.DBObject,
     * com.mongodb.DBObject, long, long)} with limit=0 and skip=0
     *
     * @param query  specifies the selection criteria
     * @param fields this is ignored
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(DBObject query, DBObject fields){
        return getCount( query , fields , 0 , 0 );
    }

    /**
     * Get the count of documents in collection that would match a criteria.  Calls {@link DBCollection#getCount(com.mongodb.DBObject,
     * com.mongodb.DBObject, long, long, com.mongodb.ReadPreference)} with limit=0 and skip=0
     *
     * @param query     specifies the selection criteria
     * @param fields    this is ignored
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(DBObject query, DBObject fields, ReadPreference readPrefs){
        return getCount( query , fields , 0 , 0, readPrefs );
    }

    /**
     * Get the count of documents in collection that would match a criteria.  Calls {@link DBCollection#getCount(com.mongodb.DBObject,
     * com.mongodb.DBObject, long, long, com.mongodb.ReadPreference)} with the DBCollection's ReadPreference
     *
     * @param query          specifies the selection criteria
     * @param fields     this is ignored
     * @param limit          limit the count to this value
     * @param skip           number of documents to skip
     * @return the number of documents that matches selection criteria
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(DBObject query, DBObject fields, long limit, long skip){
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     */
    public long getCount(DBObject query, DBObject fields, long limit, long skip, ReadPreference readPrefs ){
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/count/ Count
     * @since 2.12
     */
    long getCount(final DBObject query, final DBObject fields, final long limit, final long skip,
                  final ReadPreference readPrefs, final long maxTime, final TimeUnit maxTimeUnit) {
        final DBObject myHint = (_hintFields.size() > 0) ? _hintFields.get(0) : null;
        return execute(new CLCallback<Long>() {
            @Override
            public Long doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                return cl.getCount(query, myHint);
            }
        });
    }

    /**
     * Calls {@link DBCollection#rename(java.lang.String, boolean)} with dropTarget=false
     * @param newName new collection name (not a full namespace)
     * @return the new collection
     * @throws MongoException
     */
    public DBCollection rename( String newName ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Renames of this collection to newName
     * @param newName new collection name (not a full namespace)
     * @param dropTarget if a collection with the new name exists, whether or not to drop it
     * @return the new collection
     * @throws MongoException
     */
    public DBCollection rename( String newName, boolean dropTarget ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL. Calls {@link DBCollection#group(com.mongodb.DBObject,
     * com.mongodb.DBObject, com.mongodb.DBObject, java.lang.String, java.lang.String)} with finalize=null
     *
     * @param key     specifies one or more document fields to group
     * @param cond    specifies the selection criteria to determine which documents in the collection to process
     * @param initial initializes the aggregation result document
     * @param reduce  specifies an $reduce Javascript function, that operates on the documents during the grouping operation
     * @return a document with the grouped records as well as the command meta-data
     * @throws MongoException
     * @mongodb.driver.manual reference/command/group/ Group Command
     */
    public DBObject group( DBObject key , DBObject cond , DBObject initial , String reduce ){
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/group/ Group Command
     */
    public DBObject group( DBObject key , DBObject cond , DBObject initial , String reduce , String finalize ){
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/group/ Group Command
     */
    public DBObject group( DBObject key , DBObject cond , DBObject initial , String reduce , String finalize, ReadPreference readPrefs ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param cmd the group command containing the details of how to perform the operation.
     * @return a document with the grouped records as well as the command meta-data
     * @throws MongoException
     * @mongodb.driver.manual reference/command/group/ Group Command
     */
    public DBObject group( GroupCommand cmd ) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param cmd       the group command containing the details of how to perform the operation.
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return a document with the grouped records as well as the command meta-data
     * @throws MongoException
     * @mongodb.driver.manual reference/command/group/ Group Command
     */
    public DBObject group( GroupCommand cmd, ReadPreference readPrefs ) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Group documents in a collection by the specified key and performs simple aggregation functions such as computing counts and sums.
     * This is analogous to a {@code SELECT ... GROUP BY} statement in SQL.
     *
     * @param args object representing the arguments to the group function
     * @return a document with the grouped records as well as the command meta-data
     * @throws MongoException
     * @mongodb.driver.manual reference/command/group/ Group Command
     * @deprecated use {@link DBCollection#group(com.mongodb.GroupCommand)} instead.  This method will be removed in 3.0
     */
    @Deprecated
    public DBObject group( DBObject args ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find the distinct values for a specified field across a collection and returns the results in an array.
     *
     * @param key Specifies the field for which to return the distinct values
     * @return A {@code List} of the distinct values
     * @throws MongoException
     * @mongodb.driver.manual reference/command/distinct Distinct Command
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/distinct Distinct Command
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
     * @throws MongoException
     * @mongodb.driver.manual reference/command/distinct Distinct Command
     */
    public List distinct( String key , DBObject query ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find the distinct values for a specified field across a collection and returns the results in an array.
     *
     * @param key       Specifies the field for which to return the distinct values
     * @param query     specifies the selection query to determine the subset of documents from which to retrieve the distinct values
     * @param readPrefs {@link ReadPreference} to be used for this operation
     * @return A {@code List} of the distinct values
     * @throws MongoException
     * @mongodb.driver.manual reference/command/distinct Distinct Command
     */
    public List distinct( String key , DBObject query, ReadPreference readPrefs ){
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
     * @throws MongoException
     * @mongodb.driver.manual core/map-reduce/ Map-Reduce
     */
    public MapReduceOutput mapReduce( String map , String reduce , String outputTarget , DBObject query ){
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
     * @throws MongoException
     * @mongodb.driver.manual core/map-reduce/ Map-Reduce
     */
    public MapReduceOutput mapReduce(String map, String reduce, String outputTarget, MapReduceCommand.OutputType outputType,
                                     DBObject query) {
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
     * @throws MongoException
     * @mongodb.driver.manual core/map-reduce/ Map-Reduce
     */
    public MapReduceOutput mapReduce(String map, String reduce, String outputTarget, MapReduceCommand.OutputType outputType, DBObject query,
                                     ReadPreference readPrefs) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection and saves to a named collection.
     *
     * @param command object representing the parameters to the operation
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws MongoException
     * @mongodb.driver.manual core/map-reduce/ Map-Reduce
     */
    public MapReduceOutput mapReduce( MapReduceCommand command ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Allows you to run map-reduce aggregation operations over a collection
     *
     * @param command document representing the parameters to this operation.
     * @return A MapReduceOutput which contains the results of this map reduce operation
     * @throws MongoException
     * @mongodb.driver.manual core/map-reduce/ Map-Reduce
     * @deprecated Use {@link com.mongodb.DBCollection#mapReduce(MapReduceCommand)} instead
     */
    @Deprecated
    public MapReduceOutput mapReduce( DBObject command ){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Method implements aggregation framework.
     *
     * @param firstOp       requisite first operation to be performed in the aggregation pipeline
     * @param additionalOps additional operations to be performed in the aggregation pipeline
     * @return the aggregation operation's result set
     * @deprecated Use {@link com.mongodb.DBCollection#aggregate(java.util.List)} instead
     * @mongodb.driver.manual core/aggregation-pipeline/ Aggregation
     *
     * @mongodb.server.release 2.2
     */
    @Deprecated
    @SuppressWarnings("unchecked")
    public AggregationOutput aggregate(final DBObject firstOp, final DBObject... additionalOps) {
        List<DBObject> pipeline = new ArrayList<DBObject>();
        pipeline.add(firstOp);
        Collections.addAll(pipeline, additionalOps);
        return aggregate(pipeline);
    }

    /**
     * Method implements aggregation framework.
     *
     * @param pipeline operations to be performed in the aggregation pipeline
     * @return the aggregation's result set
     * @mongodb.driver.manual core/aggregation-pipeline/ Aggregation
     *
     * @mongodb.server.release 2.2
     */
    public AggregationOutput aggregate(final List<DBObject> pipeline) {

        final List<BSONObject> myList = convertBson(pipeline, false);

        LinkedList<DBObject> list = execute(new CLCallback<LinkedList<DBObject>>() {
            @Override
            public LinkedList<DBObject> doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                LinkedList<DBObject> ressult = new LinkedList<DBObject>();
                com.sequoiadb.base.DBCursor cursor = cl.aggregate(myList);
                try {
                    while (cursor.hasNext()) {
                        ressult.add(Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject)cursor.getNext()));
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
     * @mongodb.driver.manual core/aggregation-pipeline/ Aggregation
     *
     * @mongodb.server.release 2.2
     */
    public AggregationOutput aggregate(final List<DBObject> pipeline, ReadPreference readPreference) {
        return aggregate(pipeline);
    }

    /**
     * Method implements aggregation framework.
     *
     * @param pipeline operations to be performed in the aggregation pipeline
     * @param options  options to apply to the aggregation
     * @return the aggregation operation's result set
     * @mongodb.driver.manual core/aggregation-pipeline/ Aggregation
     *
     * @mongodb.server.release 2.2
     */
    public Cursor aggregate(final List<DBObject> pipeline, AggregationOptions options) {
        final List<BSONObject> myList = convertBson(pipeline, false);
        DBQueryResult result =
                _cm.execute(_csName, _clName, new QueryInCLCallback<DBQueryResult>(){
                    public DBQueryResult doQuery(com.sequoiadb.base.DBCollection cl) throws BaseException {
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
     * @mongodb.driver.manual core/aggregation-pipeline/ Aggregation
     *
     * @mongodb.server.release 2.2
     */
    public abstract Cursor aggregate(final List<DBObject> pipeline, final AggregationOptions options,
                                     final ReadPreference readPreference);

    /**
     * Return the explain plan for the aggregation pipeline.
     *
     * @param pipeline the aggregation pipeline to explain
     * @param options  the options to apply to the aggregation
     * @return the command result.  The explain output may change from release to release, so best to simply log this.
     * @mongodb.driver.manual core/aggregation-pipeline/ Aggregation
     * @mongodb.driver.manual reference/operator/meta/explain/ Explain query
     *
     * @mongodb.server.release 2.6
     */
    public CommandResult explainAggregate(List<DBObject> pipeline, AggregationOptions options) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Return a list of cursors over the collection that can be used to scan it in parallel.
     * <p>
     *     Note: As of MongoDB 2.6, this method will work against a mongod, but not a mongos.
     * </p>
     *
     * @param options the parallel scan options
     * @return a list of cursors, whose size may be less than the number requested
     * @since 2.12
     *
     * @mongodb.server.release 2.6
     */
    public abstract List<Cursor> parallelScan(final ParallelScanOptions options);

    /**
     * Creates a builder for an ordered bulk write operation, consisting of an ordered collection of write requests,
     * which can be any combination of inserts, updates, replaces, or removes. Write requests included in the bulk operations will be
     * executed in order, and will halt on the first failure.
     * <p>
     * Note: While this bulk write operation will execute on MongoDB 2.4 servers and below, the writes will be performed one at a time,
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
     * Note: While this bulk write operation will execute on MongoDB 2.4 servers and below, the writes will be performed one at a time,
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
     * Return a list of the indexes for this collection.  Each object in the list is the "info document" from MongoDB
     *
     * @return list of index documents
     * @throws MongoException
     */
    public List<DBObject> getIndexInfo() {
        return execute(new CLCallback<List<DBObject>>() {
            @Override
            public List<DBObject> doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                List<DBObject> list = new ArrayList<DBObject>();
                com.sequoiadb.base.DBCursor cursor = cl.getIndexes();
                try {
                    while (cursor.hasNext()) {
                        list.add(Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject)cursor.getNext()));
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
     * @throws MongoException
     */
    public void dropIndex( DBObject keys ){
        dropIndexes( genIndexName( keys ) );
    }

    /**
     * Drops an index from this collection
     * @param name name of index to drop
     * @throws MongoException
     */
    public void dropIndex( String name ){
        dropIndexes( name );
    }

    /**
     * The collStats command returns a variety of storage statistics for a given collection
     *
     * @return a CommandResult containing the statistics about this collection
     * @mongodb.driver.manual /reference/command/collStats/ collStats command
     */
    public CommandResult getStats() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Checks whether this collection is capped
     *
     * @return true if this is a capped collection
     * @throws MongoException
     * @mongodb.driver.manual /core/capped-collections/#check-if-a-collection-is-capped Capped Collections
     */
    public boolean isCapped() {
        return false;
    }

    // ------

    /**
     * @deprecated This method should not be a part of API.
     *             If you override one of the {@code DBCollection} methods please rely on superclass
     *             implementation in checking argument correctness and validity.
     */
    @Deprecated
    protected DBObject _checkObject(DBObject o, boolean canBeNull, boolean query) {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Find a collection that is prefixed with this collection's name. A typical use of this might be
     * <pre>{@code
     *    DBCollection users = mongo.getCollection( "wiki" ).getCollection( "users" );
     * }</pre>
     * Which is equivalent to
     * <pre>{@code
     *   DBCollection users = mongo.getCollection( "wiki.users" );
     * }</pre>
     *
     * @param n the name of the collection to find
     * @return the matching collection
     */
    public DBCollection getCollection( String n ){
        String csName = null;
        String clName = null;
        if (n == null || n.isEmpty()) {
            // TODO:
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
                public Boolean doInCS(CollectionSpace cs) throws BaseException {
                    return cs.isCollectionExist(myCLName);
                }
            });
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                isExist = false;
            } else {
                throw e;
            }
        }
        if (isExist) {
            // TODO: not set the csName yet.
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
     * @throws IllegalArgumentException if {@code c} is not a DBObject
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
     * Set a customer decoder factory for this collection.  Set to null to use the default from MongoOptions.
     *
     * @param fact the factory to set.
     */
    public synchronized void setDBDecoderFactory(DBDecoderFactory fact) {
    }

    /**
     * Get the decoder factory for this collection.  A null return value means that the default from MongoOptions is being used.
     *
     * @return the factory
     */
    public synchronized DBDecoderFactory getDBDecoderFactory() {
        return null;
    }

    /**
     * Set a customer encoder factory for this collection.  Set to null to use the default from MongoOptions.
     *
     * @param fact the factory to set.
     */
    public synchronized void setDBEncoderFactory(DBEncoderFactory fact) {
    }

    /**
     * Get the encoder factory for this collection.  A null return value means that the default from MongoOptions is being used.
     *
     * @return the factory
     */
    public synchronized DBEncoderFactory getDBEncoderFactory() {
        return null;
    }


}

