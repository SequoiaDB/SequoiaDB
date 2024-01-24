package com.sequoiadb.flink.sink.writer;

import com.sequoiadb.base.result.DeleteResult;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.metadata.ExtraRowKind;
import com.sequoiadb.flink.config.SDBConfigOptions;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.state.EventState;
import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.api.java.tuple.Tuple3;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.data.TimestampData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.RowKind;
import org.apache.flink.util.concurrent.ExecutorThreadFactory;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.time.Duration;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.*;
import java.util.stream.Stream;

import static com.sequoiadb.flink.sink.writer.SDBPartitionedSinkWriter.ReadableMetadata.EXTRA_ROW_KIND;

/**
 * SDBPartitionedSinkWriter supports consuming changelogs in multi partitions, such as
 * Kafka multi partitions in the topic.
 */
public class SDBPartitionedSinkWriter
        implements SinkWriter<RowData, Void, Map<BSONObject, EventState>> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBPartitionedSinkWriter.class);

    private static final String MODIFIER_SET = "$set";

    // trigger state cleaner on every minute
    private static final int DEFAULT_STATE_CLEANUP_DURATION = 1;

    private static final int MAX_TIMESTAMP_PRECISION = 9;

    private final String[] upsertKeys;
    private final int[] metadataPositions;

    private final String eventTsFieldName;
    private final int eventTsPos;

    /**
     * Notes:
     *   All state is exclusively owned by per Writer, not shared among Writers.
     *   So concurrency control is not required.
     */

    /** stateMap is to keep the latest primary key change time of each record **/
    private Map<BSONObject, EventState> stateMap = new ConcurrentHashMap<>();

    /**
     * blockedMap is to keep those changelogs which can not apply to SequoiaDB.
     * For example,
     *  1. waiting for late update_pk_aft(insert) changelog
     *  2. waiting for late update_pk_bef(delete) changelog
     *
     *  So queue those changelogs until late changelog arrives.
     */
    private final Map<BSONObject,
            Deque<Tuple3<RowKind, BSONObject, TimestampData>>> blockedMap = new HashMap<>();

    private final SDBDataConverter converter;
    private final SDBSinkOptions sinkOptions;

    private transient final SDBClientProvider provider;

    // using async thread pool for state cleanup
    private transient ScheduledExecutorService stateCleanupDaemon;
    private transient ScheduledFuture cleanupFuture;

    public SDBPartitionedSinkWriter(
            SDBDataConverter converter, SDBSinkOptions sinkOptions, List<Map<BSONObject, EventState>> states) {
        this.upsertKeys = sinkOptions.getUpsertKey();
        this.eventTsFieldName = sinkOptions.getEventTsFieldName();
        if (eventTsFieldName == null || "".equals(eventTsFieldName)) {
            throw new SDBException(
                    String.format("%s must be specified, can not be null or empty.",
                            SDBConfigOptions.SINK_RETRACT_EVENT_TS_FIELD_NAME
                                    .toString()));
        }

        RowType rowType = converter.getRowType();
        this.metadataPositions = getMetadataPositions(rowType);
        this.eventTsPos = getEventTsPos(rowType);
        if (eventTsPos < 0) {
            throw new SDBException(
                    String.format("event timestamp field [%s] are not in %s.",
                            eventTsFieldName,
                            converter.getRowType().asSummaryString()));
        }

        this.provider = SDBClientProvider.builder()
                .withHosts(sinkOptions.getHosts())
                .withCollectionSpace(sinkOptions.getCollectionSpace())
                .withCollection(sinkOptions.getCollection())
                .withUsername(sinkOptions.getUsername())
                .withPassword(sinkOptions.getPassword())
                .withOptions(sinkOptions)
                .build();
        this.converter = converter;
        this.sinkOptions = sinkOptions;

        if (!states.isEmpty()) { // if last state is not empty, recover from it.
            this.stateMap = new ConcurrentHashMap<>(states.get(0));

            // considering that the job may fail and retry.
            // when job recovers from last checkpoint's state, it is necessary
            // to reset the processing time of all states.
            LocalDateTime now = LocalDateTime.now();
            this.stateMap.forEach((pk, state) -> {
                state.setProcessingTime(TimestampData.fromLocalDateTime(now));
            });
        }

        // stateTtl is less or equal than zero means state will be never clean up.
        if (sinkOptions.getStateTtl() > 0) {
            final int stateTtl = sinkOptions.getStateTtl();
            this.stateCleanupDaemon =
                    Executors.newScheduledThreadPool(
                            1, new ExecutorThreadFactory("sequoiadb-retract-state-cleaner"));
            this.cleanupFuture =
                    stateCleanupDaemon.scheduleAtFixedRate(
                            () -> {
                                /**
                                 * There is no need to consider concurrency control.
                                 * 1. ConcurrentHashMap can avoid write-write conflicts
                                 * 2. For read-write conflict, reader will only see
                                      an expired state, just like the state not being cleaned up in time.
                                 * So It can be ignored.
                                 */
                                Iterator<Map.Entry<BSONObject, EventState>> iterator = stateMap
                                        .entrySet()
                                        .iterator();

                                LocalDateTime currSysTime = LocalDateTime.now();
                                int tot = 0;
                                while (iterator.hasNext()) {
                                    Map.Entry<BSONObject, EventState> entry = iterator.next();

                                    LocalDateTime processingTime = entry.getValue()
                                            .getProcessingTime()
                                            .toLocalDateTime();
                                    Duration duration = Duration.between(processingTime, currSysTime);
                                    if (duration.toMinutes() > stateTtl) {
                                        iterator.remove();
                                    }

                                    ++tot;
                                }

                                LOG.info("state cleanup finished, total: {}", tot);
                            },
                            // initial delay is 0
                            0,
                            // clean up period, default is on every minute
                            DEFAULT_STATE_CLEANUP_DURATION,
                            TimeUnit.MINUTES);
        }
    }

    @Override
    public void write(RowData changelog, Context context) throws IOException, InterruptedException {
        ExtraRowKind rowKind = ExtraRowKind.from(
                Objects.requireNonNull(readMetadata(changelog, EXTRA_ROW_KIND)));
        switch (rowKind) {
            case INSERT:
                handleInsert(changelog);
                break;
            case UPDATE_AFT:
                handleUpdate(changelog);
                break;
            case DELETE:
                handleDelete(changelog);
                break;
            case UPDATE_PK_BEF:
            case UPDATE_PK_AFT:
                handlePkUpdate(rowKind, changelog);
                break;
        }
    }

    /**
     * check if changelog is expired
     *
     * @param matcher
     * @param currentEventTs
     * @return
     */
    private boolean checkIfExpired(
            BSONObject matcher, TimestampData currentEventTs) {
        EventState latestState = stateMap.get(matcher);

        if (latestState == null) {
            return false;
        }

        return currentEventTs
                .compareTo(latestState.getEventTime()) <= 0;
    }

    // ===================================================
    //                Changelog Handler
    // ===================================================

    /**
     * handle INSERT changelog,
     *
     * 1. check if expired
     * 2. append to queue if it is already blocked
     * 3. try to write changelog to SequoiaDB, append to blocking queue if failed
     *
     * @param changelog
     */
    private void handleInsert(RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        // discard if changelog is expired
        if (checkIfExpired(matcher, currEventTs)) {
            return;
        }

        // if corresponding queue is already exists, then append current changelog
        // directly to the blocking queue.
        if (blockedMap.containsKey(matcher)) {
            blockedMap.get(matcher)
                      .offer(new Tuple3<>(RowKind.INSERT, record, currEventTs));
            return;
        }

        // try to insert changelog to SequoiaDB, if -38(duplicate key exists occurs)
        // occurs, means that missing a delay UPDATE_PK_BEF(delete).
        // So, we need to append current changelog to blocking queue.
        try {
            provider.getCollection()
                    .insertRecord(record);
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                Deque<Tuple3<RowKind, BSONObject, TimestampData>> blockedChangelogs = blockedMap.get(matcher);
                // init blocking queue
                if (blockedChangelogs == null) {
                    blockedChangelogs = new ArrayDeque<>();
                    blockedMap.put(matcher, blockedChangelogs);
                }
                blockedChangelogs.offer(new Tuple3<>(RowKind.INSERT, record, currEventTs));
            } else {
                throw ex;
            }
        }
    }

    /**
     * handle UPDATE changelog,
     *
     * 1. check if expired
     * 2. append to queue if it is already blocked
     * 3. try to write changelog to SequoiaDB, append to blocking queue if failed
     *
     * @param changelog
     */
    private void handleUpdate(RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        // discard if changelog is expired
        if (checkIfExpired(matcher, currEventTs)) {
            return;
        }

        // if corresponding queue is already exists, then append current changelog
        // directly to the blocking queue.
        if (blockedMap.containsKey(matcher)) {
            blockedMap.get(matcher)
                      .offer(new Tuple3<>(RowKind.UPDATE_AFTER, record, currEventTs));
            return;
        }

        // try to update record in SequoiaDB, if it doesn't match a record
        // (ModifiedNum == 0), means that missing a delay UPDATE_PK_AFT(insert).
        // So, we need to append current changelog to blocking queue.
        UpdateResult result = provider
                .getCollection()
                .updateRecords(matcher, createModifier(MODIFIER_SET, record));
        if (0 == result.getModifiedNum()) {
            Deque<Tuple3<RowKind, BSONObject, TimestampData>> blockedChangelogs = blockedMap.get(matcher);
            // init blocking queue
            if (blockedChangelogs == null) {
                blockedChangelogs = new ArrayDeque<>();
                blockedMap.put(matcher, blockedChangelogs);
            }
            blockedChangelogs.offer(new Tuple3<>(RowKind.UPDATE_AFTER, record, currEventTs));
        }
    }

    /**
     * handle DELETE changelog,
     *
     * 1. check if expired
     * 2. append to queue if it is already blocked
     * 3. try to write changelog to SequoiaDB, append to blocking queue if failed
     *
     * @param changelog
     */
    private void handleDelete(RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        if (checkIfExpired(matcher, currEventTs)) {
            return;
        }

        if (blockedMap.containsKey(matcher)) {
            blockedMap.get(matcher)
                      .offer(new Tuple3<>(RowKind.DELETE, record, currEventTs));
        }

        // try to delete record in SequoiaDB, if it doesn't match a record
        // (DeletedNum == 0), means that missing a delay UPDATE_PK_AFT(insert).
        // So, we need to append current changelog to blocking queue.
        DeleteResult result = provider
                .getCollection()
                .deleteRecords(matcher);
        if (0 == result.getDeletedNum()) {
            Deque<Tuple3<RowKind, BSONObject, TimestampData>> blockedChangelogs = blockedMap.get(matcher);
            // init blocking queue
            if (blockedChangelogs == null) {
                blockedChangelogs = new ArrayDeque<>();
                blockedMap.put(matcher, blockedChangelogs);
            }
            blockedChangelogs.offer(new Tuple3<>(RowKind.DELETE, record, currEventTs));
        }
    }

    // ===================================================
    //        UPDATE_PK_BEF, UPDATE_PK_AFT handler
    // ===================================================

    /**
     * handle UPDATE_PK_BEF, UPDATE_PK_AFT changelog
     *
     * @param rowKind
     * @param changelog
     */
    private void handlePkUpdate(ExtraRowKind rowKind, RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        if (stateMap.containsKey(matcher)) {
            TimestampData latestUpdatePkTs = stateMap.get(matcher)
                    .getEventTime();
            /**
             * discard expired record, time equation can not be ruled out for now.
             * because when processing UPDATE_PK_BEF, UPDATE_PK_AFT, the writer
             * will update stateMap firstly, then write to SequoiaDB (not an atomic op).
             * So the writer still need to write the record once again when recovering
             * from checkpoint.
             */
            if (currEventTs.compareTo(latestUpdatePkTs) < 0) {
                return;
            }
        }

        // update event state, including the latest update pk time, and processing time.
        // using write ahead policy.
        EventState state = new EventState(currEventTs,
                TimestampData.fromLocalDateTime(LocalDateTime.now()));
        stateMap.put(matcher, state);

        if (rowKind.equals(ExtraRowKind.UPDATE_PK_AFT)) {
            provider.getCollection()
                    .upsertRecords(matcher, createModifier(MODIFIER_SET, record));
        } else if (rowKind.equals(ExtraRowKind.UPDATE_PK_BEF)) {
            provider.getCollection()
                    .deleteRecords(matcher);
        }

        // flush blocked queue
        flushBlockedQueue(matcher);
    }

    /**
     * flush the blocking queue corresponding to the given matcher
     *
     * @param matcher
     */
    private void flushBlockedQueue(BSONObject matcher) {
        Deque<Tuple3<RowKind, BSONObject, TimestampData>> blockedQueue = blockedMap.get(matcher);
        if (blockedQueue == null || blockedQueue.isEmpty()) {
            return;
        }

        /**
         * The changelogs in blocking queue is sequential and full-column,
         * replay the last changelog directly is the same as replaying all changelogs
         * in sequence.
         * So here just need to replay the last changelog in blocking queue.
         */
        Tuple3<RowKind, BSONObject, TimestampData> tuple = blockedQueue.getLast();

        // discard if the latest changelog in blocking queue is expired
        EventState state = stateMap.get(matcher);
        if (state != null) {
            TimestampData latestUpdatePkTs = state.getEventTime();
            if (tuple.f2.compareTo(latestUpdatePkTs) < 0) {
                // all changelogs are expired,
                // remove the whole blocking queue from the map.
                blockedMap.remove(matcher);
                return;
            }
        }

        // write latest changelog in blocking queue
        BSONObject record = tuple.f1;
        switch (tuple.f0) {
            case INSERT:
            case UPDATE_AFTER:
                provider.getCollection()
                        .upsertRecords(matcher, createModifier(MODIFIER_SET, record));
                break;

            case UPDATE_BEFORE:
            case DELETE:
                provider.getCollection()
                        .deleteRecords(matcher);
                break;
        }

        // remove blocking queue from blockedMap
        blockedMap.remove(matcher);
    }

    private BSONObject createMatcher(BSONObject record) {
        BSONObject matcher = new BasicBSONObject();

        for (String upsertKey : upsertKeys) {
            matcher.put(upsertKey, record.get(upsertKey));
        }
        return matcher;
    }

    private BSONObject createModifier(String mType, BSONObject updater) {
        BSONObject modifier = new BasicBSONObject();
        modifier.put(mType, updater);
        return modifier;
    }

    @Override
    public List<Void> prepareCommit(boolean flush) throws IOException, InterruptedException {
        return Lists.newArrayList();
    }

    /**
     * snapshot current state, add to checkpoint
     *
     * @param checkpointId
     * @return
     * @throws IOException
     */
    @Override
    public List<Map<BSONObject, EventState>> snapshotState(long checkpointId)
            throws IOException {
        List<Map<BSONObject, EventState>> stateList = new ArrayList<>();
        stateList.add(stateMap);
        return stateList;
    }

    @Override
    public void close() throws Exception {
        if (provider != null) {
            provider.close();
        }

        if (stateCleanupDaemon != null) {
            stateCleanupDaemon.shutdown();
        }
    }

    enum ReadableMetadata {
        EXTRA_ROW_KIND(
                "$extra-row-kind",
                DataTypes.INT().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        if (pos < 0 || consumedRow.isNullAt(pos)) {
                            return null;
                        }
                        return consumedRow.getInt(pos);
                    }
                }),
        ;

        final String key;
        final DataType dataType;
        final MetadataConverter converter;

        ReadableMetadata(String key,
                         DataType dataType,
                         MetadataConverter converter) {
            this.key = key;
            this.dataType = dataType;
            this.converter = converter;
        }
    }

    interface MetadataConverter {
        Object convert(RowData consumedRow, int pos);
    }

    /**
     * calculate metadata positions in schema
     *
     * @param rowType
     * @return
     */
    private int[] getMetadataPositions(RowType rowType) {
        return Stream.of(ReadableMetadata.values())
                .mapToInt(
                        m -> {
                            final int pos = rowType.getFieldNames().indexOf(m.key);
                            if (pos < 0) {
                                return -1;
                            }
                            return pos;
                        })
                .toArray();
    }

    /**
     * read metadata from row data by metadata definition
     *
     * @param rowData
     * @param metadata
     * @return
     * @param <T>
     */
    private <T> T readMetadata(RowData rowData, ReadableMetadata metadata) {
        final int pos = metadataPositions[metadata.ordinal()];
        if (pos < 0) {
            return null;
        }
        return (T) metadata.converter.convert(rowData, pos);
    }

    private int getEventTsPos(RowType rowType) {
        int pos = rowType
                .getFieldNames().indexOf(eventTsFieldName);
        if (pos < 0) {
            return -1;
        }
        return pos;
    }

    private TimestampData readEventTs(RowData changelog) {
        return changelog.getTimestamp(
                eventTsPos,
                MAX_TIMESTAMP_PRECISION); // always get it with maximum precision
    }

}
