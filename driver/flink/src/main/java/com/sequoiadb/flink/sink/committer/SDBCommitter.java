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

package com.sequoiadb.flink.sink.committer;

import com.sequoiadb.flink.common.client.SDBClient;
import com.sequoiadb.flink.common.client.SDBSinkClient;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.sink.Executor.WriteThreads;
import com.sequoiadb.flink.sink.state.SDBBulk;

import org.apache.flink.api.connector.sink.Committer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class SDBCommitter implements Committer<SDBBulk> {

    private final static Logger LOG = LoggerFactory.getLogger(SDBCommitter.class);

    private SDBSinkOptions sdbSinkOptions;

    private final int THREAD_NUMBER;
    private final ExecutorService executorService;

    public SDBCommitter(SDBSinkOptions sdbSinkOptions){
        this.sdbSinkOptions = sdbSinkOptions;
        this.THREAD_NUMBER = sdbSinkOptions.getHosts().size();
        executorService = Executors.newFixedThreadPool(THREAD_NUMBER);
        LOG.info("Committer created");
    }

    @Override
    public void close() throws Exception {
        executorService.shutdown();
        LOG.debug("Committer closed");
    }

    /* commit function that writes out every data in the state
     * @param committables           uncommitted data
     */
    @Override
    public List<SDBBulk> commit(List<SDBBulk> committables) throws IOException, InterruptedException {
        LOG.debug("committer commit"); 
        List<SDBBulk> failedBulk = new ArrayList<>();  // failedBulk is not used here
        int numOfBulks = committables.size();
        int numThread = 1 * THREAD_NUMBER;
        int numOfLatch = numOfBulks < THREAD_NUMBER ? numOfBulks : THREAD_NUMBER;
        int dividedBulkListSize = numOfBulks < numThread ? numOfBulks : numOfBulks / numThread;

        CountDownLatch latch = new CountDownLatch(numOfLatch);
        List<Future<?>> threadStatus = new ArrayList<>();
        for (int i = 0; i < numThread; i++) {
            // create list
            int start = 0 + dividedBulkListSize * i;
            int end = dividedBulkListSize + dividedBulkListSize * i;
            end = end > numOfBulks ? numOfBulks : end;

            List<SDBBulk> sublist =  committables.subList(start, end);

            // create host
            List<String> host = new ArrayList<String>();
            host.add(sdbSinkOptions.getHosts().get(i % THREAD_NUMBER));
            SDBClient client = SDBSinkClient.createClientWithHost(sdbSinkOptions, host);

            // create thread
            WriteThreads thread = new WriteThreads(client, sublist, latch);
            /*
             * Here added a future. the reason is if using .execute, it will throw a unexcepted exception by default
             * flink will simply log it and ignore, we want it to be catched and handled by flink so instead of throw
             * unexcepted exception, we use future to create a SDB exception this will catched and handled by flink and
             * trigger retry
             */
            try {
                Future<?>submitted =  executorService.submit(thread);
                threadStatus.add(submitted);
            } catch (Exception e) {
                throw new SDBException("Thread exceptions", e);
            }

        }
        latch.await();
        // catch exception here
        try {
            for (Future<?> submitted: threadStatus){
                submitted.get();
            }
        }catch (Exception e) {
            throw new SDBException("Thread exceptions", e);
        }
        return failedBulk;
    }
 }
