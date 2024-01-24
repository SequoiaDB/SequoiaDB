/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.flink.sink.writer;

import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.client.SDBCollectionProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.state.SDBBulk;

import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.sink.Sink;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.runtime.metrics.scope.ScopeFormat;
import org.apache.flink.table.data.RowData;
import org.apache.flink.util.concurrent.ExecutorThreadFactory;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.Executors;

/**
 * SinkWriter for appending data (insert-only) to SequoiaDB.
 * It consists of a Producer (running in main thread, see {@link SDBSinkWriter#write(Object, Context)})
 * and a Consumer (running in async thread, see {@link SDBSinkWriter#flush()})
 *
 * <p>
 * The producer will continuously obtain RowData from upstream operator
 * and adds it to the thread-safe BlockingQue.
 * The consumer will periodically consume and flush data in mini-batch.
 * </p>
 */
public class SDBSinkWriter<IN>  implements SinkWriter<IN, SDBBulk, SDBBulk> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBSinkWriter.class);

    /**
     * NON_TRANSACTIONAL flush mode
     *  If user doesn't enable overwrite configuration, data will be written
     *  out without transaction.
     *  When jobs failed, there may have dirty data in SequoiaDB.
     */
    private static final int NON_TRANSACTIONAL = 0x1;

    /**
     * IDEMPOTENT flush mode
     *  If user enables overwrite configuration, and ensure that SequoiaDB has
     *  same unique index same as Flink's one, multiple writes of the same data
     *  can guarantee eventually consistency.
     *
     *  User don't need to care about how to clean up dirty data, when re-submit
     *  job, data is SequoiaDB will eventually be consistent.
     */
    private static final int IDEMPOTENT = 0x2;

    private final Sink.InitContext context;

    private transient final SDBCollectionProvider provider;

    private transient final SDBDataConverter dataConverter;
    private final SDBSinkOptions sinkOptions;

    private int flushMode;

    private ConcurrentLinkedQueue<BSONObject> currentBulk = new ConcurrentLinkedQueue<>();

    // async scheduler
    private transient ScheduledExecutorService scheduler;
    private transient ScheduledFuture future;
    private transient volatile Exception flushException;

    private transient volatile boolean initialized = false;
    private transient volatile boolean closed = false;

    /**
     * Constructor, is going to prepare every thing we need.
     * Such as,
     *  1. SequoiaDB Connection, {@link SDBCollectionProvider}
     *  2. flush method deduction
     *  3. Async flusher, using {@link ScheduledExecutorService} to schedule
     *     an async task periodically.
     *
     * @param sdbSinkOptions
     * @param dataConverter
     * @param context
     * @param states
     */
    public SDBSinkWriter(SDBSinkOptions sdbSinkOptions,
                SDBDataConverter dataConverter, Sink.InitContext context, List<SDBBulk> states) {
        this.sinkOptions = sdbSinkOptions;
        this.dataConverter = dataConverter;
        this.context = context;

        // select one coord node for this writer to use, when write transaction with unique index or nontransaction
        List<String> hosts = sdbSinkOptions.getHosts();
        String host = sdbSinkOptions.getHosts().get(context.getSubtaskId() % hosts.size());
        List<String> currentHost = new ArrayList<>();
        currentHost.add(host);

        this.provider = (SDBCollectionProvider) SDBClientProvider.builder()
                .withHosts(currentHost)
                .withCollectionSpace(sinkOptions.getCollectionSpace())
                .withCollection(sinkOptions.getCollection())
                .withUsername(sinkOptions.getUsername())
                .withPassword(sinkOptions.getPassword())
                .withOptions(sinkOptions)
                .build();

        if (!sinkOptions.isOverwrite()) {
            // if the parameter 'overwrite' hasn't been set to true,
            // then use NOT_TRANSACTIONAL as the flush mode
            flushMode = NON_TRANSACTIONAL;
        } else if (sinkOptions.isIdempotent()) {
            // otherwise, will use IDEMPOTENT_WRITE
            flushMode = IDEMPOTENT;
        }

        //log info
        LOG.info("Sink Writer {} started", context.getSubtaskId());
        LOG.info("Sink Writer options {}", sdbSinkOptions.toString());
//        LOG.info("Sink write method: {}", method.toString());

        long flushTtl = sinkOptions.getMaxBulkFillTime();
        if (flushTtl > 0) {
            this.scheduler =
                    Executors.newScheduledThreadPool(
                            1, new ExecutorThreadFactory("async-flusher"));
            // schedule an async flusher periodically
            this.future =
                scheduler.scheduleWithFixedDelay(
                        () -> {
                            if (initialized && !closed) {
                                try {
                                    // the only task is to regularly consume and write
                                    // out data in mini-batch periodically
                                    flush();
                                } catch (Exception ex) {
                                    // need to expose exception to Producer,
                                    // otherwise Producer cannot sense and throws an exception to
                                    // terminate the job.
                                    flushException = ex;
                                    String jobId = "";
                                    Map<String, String> allVars = context.metricGroup()
                                            .getAllVariables();
                                    if (allVars != null) {
                                        jobId = allVars.get(ScopeFormat.SCOPE_JOB_ID);
                                    }
                                    LOG.warn("async flusher occurs exception, job id={}, msg={}",
                                            jobId,
                                            ex.getMessage());
                                }
                            }
                        },
                        0,
                        flushTtl,
                        TimeUnit.MILLISECONDS);
        }

        this.initialized = true;
    }

    /**
     * Producer
     *
     * Receive data from upstream operator, and add to current SinkWriter
     *
     * Note:
     *  here will not write a RowData as soon as we receive it, instead writing
     *  data out in mini-batch.
     *  So here just add data to BlockingQueue, async flusher (Consumer) will consumer
     *  data and flush
     *  see also {@link SDBSinkWriter#flush()}.
     *
     *  This method is thread-safe, synchronization between {@link SDBSinkWriter#write(Object, Context)}
     *  and {@link SDBSinkWriter#flush()} is go through by Java native BlockingQueue.
     *
     * @param element The input record
     * @param context The additional information about the input record
     * @throws IOException
     * @throws InterruptedException
     */
    @Override
    public void write(IN element, Context context) throws IOException, InterruptedException {
        checkFlushException();

        // convert RowData to BsonObject
        BSONObject record = dataConverter
                .toExternal((RowData) element, sinkOptions.getIgnoreNullField());

        // producer add data to BlockingQueue
        currentBulk.add(record);
    }

    /**
     * Consumer
     *
     * It will consume data from {@link SDBSinkWriter#currentBulk} and package data
     * into SDBBulk to write out.
     *
     * Notes:
     *  This method is thread-safe, synchronization between {@link SDBSinkWriter#write(Object, Context)}
     *  and {@link SDBSinkWriter#flush()} is go through by Java native BlockingQueue.
     *
     * @throws SDBException when cannot flush data to SequoiaDB
     */
    private void flush() {
        if (initialized && !closed) {
            List<SDBBulk> pendingBulks = rollAvailableBulksOut();
            try {
                pendingBulks.forEach(sdbBulk -> {
                    // idempotent write use FLG_INSERT_REPLACEONDUP
                    int flags = flushMode == IDEMPOTENT ?
                            InsertOption.FLG_INSERT_REPLACEONDUP :
                            0;
                    if (sdbBulk.size() > 0) {
                        provider.getCollection()
                                .bulkInsert(sdbBulk.getBsonObjects(), flags);
                    }
                });
            } catch (Exception ex) {
                throw new SDBException("Failed to flush data to SequoiaDB", ex);
            }
        }
    }

    /**
     * snapshot BlockingQueue {@link SDBSinkWriter#currentBulk} and packaging
     * all available BSONObjects into SDBBulk.
     *
     * @return pendingBulks which are ready to flush
     */
    private List<SDBBulk> rollAvailableBulksOut() {
        List<SDBBulk> pendingBulks = new ArrayList<>();

        int size = currentBulk.size();
        int maxBulkSize = sinkOptions.getBulkSize();

        SDBBulk bulk = new SDBBulk(maxBulkSize);

        for (int i = 0; i < size; i++) {
            if (bulk.size() >= maxBulkSize) {
                pendingBulks.add(bulk);
                bulk = new SDBBulk(maxBulkSize);
            }
            bulk.add(currentBulk.poll());
        }
        pendingBulks.add(bulk);

        return pendingBulks;
    }

    private void checkFlushException() throws IOException {
        if (flushException != null) {
            throw new IOException("Failed to flush records to SequoiaDB.", flushException);
        }
    }

    @Override
    public void close() throws Exception {
        // shutdown scheduler
        if (future != null) {
            future.cancel(false);
        }

        if (scheduler != null) {
            scheduler.shutdown();

            // await scheduler termination
            while (!scheduler.awaitTermination(1, TimeUnit.SECONDS))
                ;

            /*
             * In batch mode, close method is triggered by Flink.
             * After async flusher is shutdown, we have to double-check if there are any
             * cached RowData, and flush all cached RowData to SequoiaDB.
             */
            if (!currentBulk.isEmpty()) {
                flush();
            }
        }

        // close SequoiaDB connection
        if (provider != null) {
            provider.close();
        }

        closed = true;
    }

    /*
        prepareCommit function is called by sink operator when creating a checkpoint.
        prepareCommit comes with a boolean value flush, this will be triggered
        if there is no more data streaming in, for example end of a stream or bounded
        data. The reason for this boolean value is that, currently Flink is not able
        to `commit` at the end of process, the last batch of data will never committed
        a bandit solution here is when reach end of input, flush is set to true, do a
        commit at prepareCommit, since committer will be closed and committer.commit will
        never called.
        Flink issue #: FLINK-23883 and FLINK-2491 current plan for resolve this issue: 1.15
        @param flush            set to true when it is to the end of bounded data
        returns List<SDBBulk>   List of SDBBulks that is about to be committed
    */
    @Override
    public List<SDBBulk> prepareCommit(boolean flush) throws IOException, InterruptedException {
        // Not support yet.
        // TODO: road map
        return Lists.newArrayList();
    }

    /**
     * Not support currently, but in the road map.
     *
     * @param checkpointId
     * @return
     * @throws IOException
     */
    @Override
    public List<SDBBulk> snapshotState(long checkpointId) throws IOException {
        return Lists.newArrayList();
    }

}
