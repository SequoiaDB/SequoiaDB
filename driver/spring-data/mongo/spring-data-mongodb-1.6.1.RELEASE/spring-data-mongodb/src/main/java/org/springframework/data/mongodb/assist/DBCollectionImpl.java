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

    // TODO: remove it
//    <T> T execute(CLCallback<T> callback) {
//        return _cm.execute(_csName, _clName, callback);
//    }

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
        // get index name
        final String indexName = genIndexName(keys);
        // get index options
        DBObject myOptions = options != null ? options : new BasicDBObject();
        final boolean isUnique =
                myOptions.containsField(IDX_OPTS_UNIQUE) ? (Boolean)(myOptions.get(IDX_OPTS_UNIQUE)) :
                        (myOptions.containsField(IDX_OPTS_ISUNIQUE) ? (Boolean)(myOptions.get(IDX_OPTS_ISUNIQUE)) : false);
        final boolean enfored =
                myOptions.containsField(IDX_OPTS_ENFORCED) ? (Boolean)(myOptions.get(IDX_OPTS_ENFORCED)) : false;
        final int sortBufferSize =
                myOptions.containsField(IDX_OPTS_SORTBUFFERSIZE) ? (Integer) (myOptions.get(IDX_OPTS_SORTBUFFERSIZE)) :
                        IDX_DEFAULT_SORTBUFFERSIZE;
        // create index
        execute(new CLCallback<Void>() {
            @Override
            public Void doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException {
                cl.createIndex(indexName, keys, isUnique, enfored, sortBufferSize);
                return null;
            }
        });
    }





//
//    @SuppressWarnings("unchecked")
//    private BulkWriteResult updateWithCommandProtocol(final List<ModifyRequest> updates,
//                                                      final WriteConcern writeConcern,
//                                                      final DBEncoder encoder, final DBPort port) {
//        BaseWriteCommandMessage message = new UpdateCommandMessage(getNamespace(), writeConcern, updates,
//                DefaultDBEncoder.FACTORY.create(), encoder,
//                getMessageSettings(port));
//        return writeWithCommandProtocol(port, UPDATE, message, writeConcern);
//    }
//
//    private BulkWriteResult writeWithCommandProtocol(final DBPort port, final WriteRequest.Type type, BaseWriteCommandMessage message,
//                                                     final WriteConcern writeConcern) {
//        int batchNum = 0;
//        int currentRangeStartIndex = 0;
//        BulkWriteBatchCombiner bulkWriteBatchCombiner = new BulkWriteBatchCombiner(port.getAddress(), writeConcern);
//        do {
//            batchNum++;
//            BaseWriteCommandMessage nextMessage = sendWriteCommandMessage(message, batchNum, port);
//            int itemCount = nextMessage != null ? message.getItemCount() - nextMessage.getItemCount() : message.getItemCount();
//            IndexMap indexMap = IndexMap.create(currentRangeStartIndex, itemCount);
//            CommandResult commandResult = receiveWriteCommandMessage(port);
//            if (willTrace() && nextMessage != null || batchNum > 1) {
//                getLogger().fine(format("Received response for batch %d", batchNum));
//            }
//
//            if (hasError(commandResult)) {
//                bulkWriteBatchCombiner.addErrorResult(getBulkWriteException(type, commandResult), indexMap);
//            } else {
//                bulkWriteBatchCombiner.addResult(getBulkWriteResult(type, commandResult), indexMap);
//            }
//            currentRangeStartIndex += itemCount;
//            message = nextMessage;
//        } while (message != null && !bulkWriteBatchCombiner.shouldStopSendingMoreBatches());
//
//        return bulkWriteBatchCombiner.getResult();
//    }
//
//    private boolean useWriteCommands(final WriteConcern concern, final DBPort port) {
//        return concern.callGetLastError() &&
//                db.getConnector().getServerDescription(port.getAddress()).getVersion().compareTo(new ServerVersion(2, 6)) >= 0;
//    }
//
//    private MessageSettings getMessageSettings(final DBPort port) {
//        ServerDescription serverDescription = db.getConnector().getServerDescription(port.getAddress());
//        return MessageSettings.builder()
//                .maxDocumentSize(serverDescription.getMaxDocumentSize())
//                .maxMessageSize(serverDescription.getMaxMessageSize())
//                .maxWriteBatchSize(serverDescription.getMaxWriteBatchSize())
//                .build();
//    }
//
//    private int getMaxWriteBatchSize(final DBPort port) {
//        return db.getConnector().getServerDescription(port.getAddress()).getMaxWriteBatchSize();
//    }
//
//    private MongoNamespace getNamespace() {
//        return new MongoNamespace(getDB().getName(), getName());
//    }
//
//    private BaseWriteCommandMessage sendWriteCommandMessage(final BaseWriteCommandMessage message, final int batchNum,
//                                                            final DBPort port) {
//        final PoolOutputBuffer buffer = new PoolOutputBuffer();
//        try {
//            BaseWriteCommandMessage nextMessage = message.encode(buffer);
//            if (nextMessage != null || batchNum > 1) {
//                getLogger().fine(format("Sending batch %d", batchNum));
//            }
//            db.getConnector().doOperation(getDB(), port, new DBPort.Operation<Void>() {
//                @Override
//                public Void execute() throws IOException {
//                    buffer.pipe(port.getOutputStream());
//                    return null;
//                }
//            });
//            return nextMessage;
//        } finally {
//            buffer.reset();
//        }
//    }
//
//    private CommandResult receiveWriteCommandMessage(final DBPort port) {
//        return db.getConnector().doOperation(getDB(), port, new DBPort.Operation<CommandResult>() {
//            @Override
//            public CommandResult execute() throws IOException {
//                Response response = new Response(port.getAddress(), null, port.getInputStream(),
//                        DefaultDBDecoder.FACTORY.create());
//                CommandResult writeCommandResult = new CommandResult(port.getAddress());
//                writeCommandResult.putAll(response.get(0));
//                writeCommandResult.throwOnError();
//                return writeCommandResult;
//            }
//        });
//    }
//
//
//    private WriteResult insertWithWriteProtocol(final List<DBObject> list, final WriteConcern concern, final DBEncoder encoder,
//                                                final DBPort port, final boolean shouldApply) {
//        if ( shouldApply ){
//            applyRulesForInsert(list);
//        }
//
//        WriteResult last = null;
//
//        int cur = 0;
//        int maxsize = db._mongo.getMaxBsonObjectSize();
//        while ( cur < list.size() ) {
//
//            OutMessage om = OutMessage.insert( this , encoder, concern );
//
//            for ( ; cur < list.size(); cur++ ){
//                DBObject o = list.get(cur);
//                om.putObject( o );
//
//                // limit for batch insert is 4 x maxbson on server, use 2 x to be safe
//                if ( om.size() > 2 * maxsize ){
//                    cur++;
//                    break;
//                }
//            }
//
//            last = db.getConnector().say(_db, om, concern, port);
//        }
//
//        return last;
//    }
//
//    private Iterable<Run> getRunGenerator(final boolean ordered, final List<WriteRequest> writeRequests,
//                                          final WriteConcern writeConcern, final DBEncoder encoder, final DBPort port) {
//        if (ordered) {
//            return new OrderedRunGenerator(writeRequests, writeConcern, encoder, port);
//        } else {
//            return new UnorderedRunGenerator(writeRequests, writeConcern, encoder, port);
//        }
//    }
//
//    private static final Logger TRACE_LOGGER = Logger.getLogger( "com.mongodb.TRACE" );
//    private static final Level TRACE_LEVEL = Boolean.getBoolean( "DB.TRACE" ) ? Level.INFO : Level.FINEST;
//
//    private boolean willTrace(){
//        return TRACE_LOGGER.isLoggable(TRACE_LEVEL);
//    }
//
//    private void trace( String s ){
//        TRACE_LOGGER.log( TRACE_LEVEL , s );
//    }
//
//    private Logger getLogger() {
//        return TRACE_LOGGER;
//    }
//
//    private class OrderedRunGenerator implements Iterable<Run> {
//        private final List<WriteRequest> writeRequests;
//        private final WriteConcern writeConcern;
//        private final DBEncoder encoder;
//        private final int maxBatchWriteSize;
//
//        public OrderedRunGenerator(final List<WriteRequest> writeRequests, final WriteConcern writeConcern, final DBEncoder encoder,
//                                   final DBPort port) {
//            this.writeRequests = writeRequests;
//            this.writeConcern = writeConcern.continueOnError(false);
//            this.encoder = encoder;
//            this.maxBatchWriteSize = getMaxWriteBatchSize(port);
//        }
//
//        @Override
//        public Iterator<Run> iterator() {
//            return new Iterator<Run>() {
//                private int curIndex;
//
//                @Override
//                public boolean hasNext() {
//                    return curIndex < writeRequests.size();
//                }
//
//                @Override
//                public Run next() {
//                    Run run = new Run(writeRequests.get(curIndex).getType(), writeConcern, encoder);
//                    int startIndexOfNextRun = getStartIndexOfNextRun();
//                    for (int i = curIndex; i < startIndexOfNextRun; i++) {
//                        run.add(writeRequests.get(i), i);
//                    }
//                    curIndex = startIndexOfNextRun;
//                    return run;
//                }
//
//                private int getStartIndexOfNextRun() {
//                    WriteRequest.Type type = writeRequests.get(curIndex).getType();
//                    for (int i = curIndex; i < writeRequests.size(); i++) {
//                        if (i == curIndex + maxBatchWriteSize || writeRequests.get(i).getType() != type) {
//                            return i;
//                        }
//                    }
//                    return writeRequests.size();
//                }
//
//                @Override
//                public void remove() {
//                    throw new UnsupportedOperationException("Not implemented");
//                }
//            };
//        }
//    }
//
//
//    private class UnorderedRunGenerator implements Iterable<Run> {
//        private final List<WriteRequest> writeRequests;
//        private final WriteConcern writeConcern;
//        private final DBEncoder encoder;
//        private final int maxBatchWriteSize;
//
//        public UnorderedRunGenerator(final List<WriteRequest> writeRequests, final WriteConcern writeConcern,
//                                     final DBEncoder encoder, final DBPort port) {
//            this.writeRequests = writeRequests;
//            this.writeConcern = writeConcern.continueOnError(true);
//            this.encoder = encoder;
//            this.maxBatchWriteSize = getMaxWriteBatchSize(port);
//        }
//
//        @Override
//        public Iterator<Run> iterator() {
//            return new Iterator<Run>() {
//                private final Map<WriteRequest.Type, Run> runs =
//                        new TreeMap<WriteRequest.Type, Run>(new Comparator<WriteRequest.Type>() {
//                            @Override
//                            public int compare(final WriteRequest.Type first, final WriteRequest.Type second) {
//                                return first.compareTo(second);
//                            }
//                        });
//                private int curIndex;
//
//                @Override
//                public boolean hasNext() {
//                    return curIndex < writeRequests.size() || !runs.isEmpty();
//                }
//
//                @Override
//                public Run next() {
//                    while (curIndex < writeRequests.size()) {
//                        WriteRequest writeRequest = writeRequests.get(curIndex);
//                        Run run = runs.get(writeRequest.getType());
//                        if (run == null) {
//                            run = new Run(writeRequest.getType(), writeConcern, encoder);
//                            runs.put(run.type, run);
//                        }
//                        run.add(writeRequest, curIndex);
//                        curIndex++;
//                        if (run.size() == maxBatchWriteSize) {
//                            return runs.remove(run.type);
//                        }
//                    }
//
//                    return runs.remove(runs.keySet().iterator().next());
//                }
//
//                @Override
//                public void remove() {
//                    throw new UnsupportedOperationException("Not implemented");
//                }
//            };
//        }
//    }
//
//    private class Run {
//        private final List<WriteRequest> writeRequests = new ArrayList<WriteRequest>();
//        private final WriteRequest.Type type;
//        private final WriteConcern writeConcern;
//        private final DBEncoder encoder;
//        private IndexMap indexMap;
//
//        Run(final WriteRequest.Type type, final WriteConcern writeConcern, final DBEncoder encoder) {
//            this.type = type;
//            this.indexMap = IndexMap.create();
//            this.writeConcern = writeConcern;
//            this.encoder = encoder;
//        }
//
//        @SuppressWarnings("unchecked")
//        void add(final WriteRequest writeRequest, final int originalIndex) {
//            indexMap = indexMap.add(writeRequests.size(), originalIndex);
//            writeRequests.add(writeRequest);
//        }
//
//        public int size() {
//            return writeRequests.size();
//        }
//
//        @SuppressWarnings("unchecked")
//        BulkWriteResult execute(final DBPort port) {
//            if (type == UPDATE) {
//                return executeUpdates(getWriteRequestsAsModifyRequests(), port);
//            } else if (type == REPLACE) {
//                return executeReplaces(getWriteRequestsAsModifyRequests(), port);
//            } else if (type == INSERT) {
//                return executeInserts(getWriteRequestsAsInsertRequests(), port);
//            } else if (type == REMOVE) {
//                return executeRemoves(getWriteRequestsAsRemoveRequests(), port);
//            } else {
//                throw new MongoInternalException(format("Unsupported write of type %s", type));
//            }
//        }
//
//        private List getWriteRequestsAsRaw() {
//            return writeRequests;
//        }
//
//        @SuppressWarnings("unchecked")
//        private List<RemoveRequest> getWriteRequestsAsRemoveRequests() {
//            return (List<RemoveRequest>) getWriteRequestsAsRaw();
//        }
//
//        @SuppressWarnings("unchecked")
//        private List<InsertRequest> getWriteRequestsAsInsertRequests() {
//            return (List<InsertRequest>) getWriteRequestsAsRaw();
//        }
//
//        @SuppressWarnings("unchecked")
//        private List<ModifyRequest> getWriteRequestsAsModifyRequests() {
//            return (List<ModifyRequest>) getWriteRequestsAsRaw();
//        }
//
//        BulkWriteResult executeUpdates(final List<ModifyRequest> updateRequests, final DBPort port) {
//            for (ModifyRequest request : updateRequests) {
//                for (String key : request.getUpdateDocument().keySet()) {
//                    if (!key.startsWith("$")) {
//                        throw new IllegalArgumentException("Update document keys must start with $: " + key);
//                    }
//                }
//            }
//
//            return new RunExecutor(port) {
//                @Override
//                BulkWriteResult executeWriteCommandProtocol() {
//                    return updateWithCommandProtocol(updateRequests, writeConcern, encoder, port);
//                }
//
//                @Override
//                WriteResult executeWriteProtocol(final int i) {
//                    ModifyRequest update = updateRequests.get(i);
//                    WriteResult writeResult = update(update.getQuery(), update.getUpdateDocument(), update.isUpsert(),
//                            update.isMulti(), writeConcern, encoder);
//                    return addMissingUpserted(update, writeResult);
//                }
//
//                @Override
//                WriteRequest.Type getType() {
//                    return UPDATE;
//                }
//            }.execute();
//        }
//
//        BulkWriteResult executeReplaces(final List<ModifyRequest> replaceRequests, final DBPort port) {
//            for (ModifyRequest request : replaceRequests) {
//                _checkObject(request.getUpdateDocument(), false, false);
//            }
//
//            return new RunExecutor(port) {
//                @Override
//                BulkWriteResult executeWriteCommandProtocol() {
//                    return updateWithCommandProtocol(replaceRequests, writeConcern, encoder, port);
//                }
//
//                @Override
//                WriteResult executeWriteProtocol(final int i) {
//                    ModifyRequest update = replaceRequests.get(i);
//                    WriteResult writeResult = update(update.getQuery(), update.getUpdateDocument(), update.isUpsert(),
//                            update.isMulti(), writeConcern, encoder);
//                    return addMissingUpserted(update, writeResult);
//                }
//
//                @Override
//                WriteRequest.Type getType() {
//                    return REPLACE;
//                }
//            }.execute();
//        }
//
//        BulkWriteResult executeRemoves(final List<RemoveRequest> removeRequests, final DBPort port) {
//            return new RunExecutor(port) {
//                @Override
//                BulkWriteResult executeWriteCommandProtocol() {
//                    return removeWithCommandProtocol(removeRequests, writeConcern, encoder, port);
//                }
//
//                @Override
//                WriteResult executeWriteProtocol(final int i) {
//                    RemoveRequest removeRequest = removeRequests.get(i);
//                    return remove(removeRequest.getQuery(), removeRequest.isMulti(), writeConcern, encoder);
//                }
//
//                @Override
//                WriteRequest.Type getType() {
//                    return REMOVE;
//                }
//            }.execute();
//        }
//
//        BulkWriteResult executeInserts(final List<InsertRequest> insertRequests, final DBPort port) {
//            return new RunExecutor(port) {
//                @Override
//                BulkWriteResult executeWriteCommandProtocol() {
//                    List<DBObject> documents = new ArrayList<DBObject>(insertRequests.size());
//                    for (InsertRequest cur : insertRequests) {
//                        documents.add(cur.getDocument());
//                    }
//                    return insertWithCommandProtocol(documents, writeConcern, encoder, port, true);
//                }
//
//                @Override
//                WriteResult executeWriteProtocol(final int i) {
//                    return insert(asList(insertRequests.get(i).getDocument()), writeConcern, encoder);
//                }
//
//                @Override
//                WriteRequest.Type getType() {
//                    return INSERT;
//                }
//
//            }.execute();
//        }
//
//
//        private abstract class RunExecutor {
//            private final DBPort port;
//
//            RunExecutor(final DBPort port) {
//                this.port = port;
//            }
//
//            abstract BulkWriteResult executeWriteCommandProtocol();
//
//            abstract WriteResult executeWriteProtocol(final int i);
//
//            abstract WriteRequest.Type getType();
//
//            BulkWriteResult getResult(final WriteResult writeResult) {
//                int count = getCount(writeResult);
//                List<BulkWriteUpsert> upsertedItems = getUpsertedItems(writeResult);
//                Integer modifiedCount = (getType() == UPDATE || getType() == REPLACE) ? null : 0;
//                return new AcknowledgedBulkWriteResult(getType(), count - upsertedItems.size(), modifiedCount, upsertedItems);
//            }
//
//            BulkWriteResult execute() {
//                if (useWriteCommands(writeConcern, port)) {
//                    return executeWriteCommandProtocol();
//                } else {
//                    return executeWriteProtocol();
//                }
//            }
//
//            private BulkWriteResult executeWriteProtocol() {
//                BulkWriteBatchCombiner bulkWriteBatchCombiner = new BulkWriteBatchCombiner(port.getAddress(), writeConcern);
//                for (int i = 0; i < writeRequests.size(); i++) {
//                    IndexMap indexMap = IndexMap.create(i, 1);
//                    try {
//                        WriteResult writeResult = executeWriteProtocol(i);
//                        if (writeConcern.callGetLastError()) {
//                            bulkWriteBatchCombiner.addResult(getResult(writeResult), indexMap);
//                        }
//                    } catch (WriteConcernException writeException) {
//                        if (isWriteConcernError(writeException.getCommandResult()))  {
//                            bulkWriteBatchCombiner.addResult(getResult(new WriteResult(writeException.getCommandResult(), writeConcern)),
//                                    indexMap);
//                            bulkWriteBatchCombiner.addWriteConcernErrorResult(getWriteConcernError(writeException.getCommandResult()));
//                        } else {
//                            bulkWriteBatchCombiner.addWriteErrorResult(getBulkWriteError(writeException), indexMap);
//                        }
//                        if (bulkWriteBatchCombiner.shouldStopSendingMoreBatches()) {
//                            break;
//                        }
//                    }
//                }
//                return bulkWriteBatchCombiner.getResult();
//            }
//
//
//            private int getCount(final WriteResult writeResult) {
//                return getType() == INSERT ? 1 : writeResult.getN();
//            }
//
//            List<BulkWriteUpsert> getUpsertedItems(final WriteResult writeResult) {
//                return writeResult.getUpsertedId() == null
//                        ? Collections.<BulkWriteUpsert>emptyList()
//                        : asList(new BulkWriteUpsert(0, writeResult.getUpsertedId()));
//            }
//
//            private BulkWriteError getBulkWriteError(final WriteConcernException writeException) {
//                return new BulkWriteError(writeException.getCode(),
//                        writeException.getCommandResult().getString("err"),
//                        getErrorResponseDetails(writeException.getCommandResult()),
//                        0);
//            }
//
//            // Accommodating GLE representation of write concern errors
//            private boolean isWriteConcernError(final CommandResult commandResult) {
//                return commandResult.get("wtimeout") != null;
//            }
//
//            private WriteConcernError getWriteConcernError(final CommandResult commandResult) {
//                return new WriteConcernError(commandResult.getCode(), getWriteConcernErrorMessage(commandResult),
//                        getErrorResponseDetails(commandResult));
//            }
//
//            private String getWriteConcernErrorMessage(final CommandResult commandResult) {
//                return commandResult.getString("err");
//            }
//
//            private DBObject getErrorResponseDetails(final DBObject response) {
//                DBObject details = new BasicDBObject();
//                for (String key : response.keySet()) {
//                    if (!asList("ok", "err", "code").contains(key)) {
//                        details.put(key, response.get(key));
//                    }
//                }
//                return details;
//            }
//
//            WriteResult addMissingUpserted(final ModifyRequest update, final WriteResult writeResult) {
//                // On pre 2.6 servers upserts with custom _id's would be not be reported so we  check if _id
//                // was in the update query or the find query then massage the writeResult.
//                if (update.isUpsert() && writeConcern.callGetLastError() && !writeResult.isUpdateOfExisting()
//                        && writeResult.getUpsertedId() == null) {
//                    DBObject updateDocument = update.getUpdateDocument();
//                    DBObject query = update.getQuery();
//                    if (updateDocument.containsField("_id")) {
//                        CommandResult commandResult = writeResult.getLastError();
//                        commandResult.put("upserted", updateDocument.get("_id"));
//                        return new WriteResult(commandResult, writeResult.getLastConcern());
//                    } else  if (query.containsField("_id")) {
//                        CommandResult commandResult = writeResult.getLastError();
//                        commandResult.put("upserted", query.get("_id"));
//                        return new WriteResult(commandResult, writeResult.getLastConcern());
//                    }
//                }
//                return writeResult;
//            }
//        }
//    }




}
