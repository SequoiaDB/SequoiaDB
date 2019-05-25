package org.springframework.data.mongodb.assist;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;

import java.util.*;

import static java.lang.String.format;

/**
 * Created by tanzhaobo on 2017/9/7.
 */
public class DBCollectionImpl extends DBCollection {

    private static final String IDX_OPTS_UNIQUE = "unique";
    private static final String IDX_OPTS_ISUNIQUE = "isUnique";
    private static final String IDX_OPTS_ENFORCED = "enforced";
    private static final String IDX_OPTS_SORTBUFFERSIZE = "sortBufferSize";
    private static final int IDX_DEFAULT_SORTBUFFERSIZE = 64;

    DBCollectionImpl(final DBApiLayer db, final String name) {
        super(db.getMongo().getConnectionManager(), db, name);
    }


    @Override
    QueryResultIterator find(DBObject ref, DBObject fields, int numToSkip, int batchSize, int limit, int options,
                             ReadPreference readPref, DBDecoder decoder) {
        return find(ref, fields, numToSkip, batchSize, limit, options, readPref, decoder, null);
    }

    /**
     *
     */
    @Override
    QueryResultIterator find(DBObject ref, DBObject fields, int numToSkip, int batchSize, int limit, int options,
                             ReadPreference readPref, DBDecoder decoder, DBEncoder encoder) {
        final DBObject hint = (_hintFields.size() > 0) ? _hintFields.get(0) : null;
        return findInternal(ref, fields, null, hint, numToSkip, limit,0);
    }

    @Override
    QueryResultIterator findInternal(final BSONObject matcher, final BSONObject selector,
                             final BSONObject orderBy, final BSONObject hint,
                             final long skipRows, final long returnRows, final int flags) {
        DBQueryResult result =
            _cm.execute(_csName, _clName, new QueryInCLCallback<DBQueryResult>(){
                public DBQueryResult doQuery(com.sequoiadb.base.DBCollection cl) throws BaseException {
                    com.sequoiadb.base.DBCursor cursor =
                            cl.query(matcher, selector, orderBy, hint, skipRows, returnRows, flags);
                    return new DBQueryResult(cursor, cl.getSequoiadb());
                }
            });
        return new QueryResultIterator(_cm, result);
    }

    public Cursor aggregate(final List<DBObject> pipeline, final AggregationOptions options,
                            final ReadPreference readPreference) {
        return aggregate(pipeline, (AggregationOptions)null);
    }

    @Override
    @SuppressWarnings("unchecked")
    public List<Cursor> parallelScan(final ParallelScanOptions options) {
        throw new UnsupportedOperationException("not support");
    }

    public WriteResult insert(List<DBObject> list, WriteConcern concern, DBEncoder encoder ){
        return insert(list, true, concern, encoder);
    }

    protected WriteResult insert(List<DBObject> list, boolean shouldApply , WriteConcern concern, DBEncoder encoder ){
        final List<DBObject> myList = list;
        return execute(new CLCallback<WriteResult>() {
            @Override
            public WriteResult doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                WriteResult result = Helper.getDefaultWriteResult(cl.getSequoiadb());
                cl.bulkInsert(convertBson(myList, true), 0);
                return result;
            }
        });
    }

    public WriteResult remove( DBObject query, WriteConcern concern, DBEncoder encoder ) {
        return remove(query, true, concern, encoder);
    }

    public WriteResult remove(final DBObject query, boolean multi, WriteConcern concern, DBEncoder encoder ){
        final DBObject myQuery = query;
        return execute(new CLCallback<WriteResult>() {
            @Override
            public WriteResult doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                WriteResult result = Helper.getDefaultWriteResult(cl.getSequoiadb());
                cl.delete(myQuery);
                return result;
            }
        });
    }

    @Override
    public WriteResult update(DBObject matcher, DBObject rule, boolean upsert , boolean multi , WriteConcern concern,
                              DBEncoder encoder ) {
        return update(matcher, rule, null, upsert);
    }

    @Override
    public WriteResult update(DBObject matcher, DBObject rule, DBObject hint, boolean upsert) {
        if (rule == null) {
            throw new IllegalArgumentException("update can not be null");
        }

        final BSONObject myQuery = matcher;
        final BSONObject myRule = rule;
        final BSONObject myHint = hint;
        final boolean myUpsert = upsert;
        return execute(new CLCallback<WriteResult>() {
            @Override
            public WriteResult doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                WriteResult result = Helper.getDefaultWriteResult(cl.getSequoiadb());
                if (myUpsert) {
                    cl.upsert(myQuery, myRule, myHint);
                } else {
                    cl.update(myQuery, myRule, myHint);
                }
                return result;
            }
        });
    }

    @Override
    public void drop(){
        _cm.execute(_csName, new CSCallback<Void>() {
            @Override
            public Void doInCS(CollectionSpace cs) throws BaseException {
                cs.dropCollection(_clName);
                return null;
            }
        });
    }

    public void doapply( DBObject o ){
    }

    public void createIndex(final DBObject keys, final DBObject options, DBEncoder encoder) {
        final String indexName = genIndexName(keys);
        DBObject myOptions = options != null ? options : new BasicDBObject();
        final boolean isUnique =
                myOptions.containsField(IDX_OPTS_UNIQUE) ? (Boolean)(myOptions.get(IDX_OPTS_UNIQUE)) :
                        (myOptions.containsField(IDX_OPTS_ISUNIQUE) ? (Boolean)(myOptions.get(IDX_OPTS_ISUNIQUE)) : false);
        final boolean enfored =
                myOptions.containsField(IDX_OPTS_ENFORCED) ? (Boolean)(myOptions.get(IDX_OPTS_ENFORCED)) : false;
        final int sortBufferSize =
                myOptions.containsField(IDX_OPTS_SORTBUFFERSIZE) ? (Integer) (myOptions.get(IDX_OPTS_SORTBUFFERSIZE)) :
                        IDX_DEFAULT_SORTBUFFERSIZE;
        execute(new CLCallback<Void>() {
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                cl.createIndex(indexName, keys, isUnique, enfored, sortBufferSize);
                return null;
            }
        });
    }









}
