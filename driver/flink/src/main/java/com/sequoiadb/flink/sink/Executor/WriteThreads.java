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

package com.sequoiadb.flink.sink.Executor;

import java.util.List;
import java.util.concurrent.CountDownLatch;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.flink.common.client.SDBClient;
import com.sequoiadb.flink.sink.state.SDBBulk;

import org.slf4j.LoggerFactory;

import org.slf4j.Logger;


/* 
    Thread class extracted from Committer, because when ending, 
    the last batch of data is not handled by committer, 
    it will be handled at prepareCommit in SDBSinkWriter
*/

public class WriteThreads implements Runnable {
    private SDBClient client;
    private List<SDBBulk> bulks;
    private CountDownLatch latch;
    private static final  Logger LOG = LoggerFactory.getLogger(WriteThreads.class);
    public WriteThreads(SDBClient client, List<SDBBulk> bulks, CountDownLatch latch){
        this.client = client;
        this.bulks = bulks;
        this.latch = latch;
    }

    @Override
    public void run() {
        try {
            // for each bulk, create a transaction to write
            for (SDBBulk bulk : bulks) {
                if (bulk.size() > 0) {
                    client.getClient().beginTransaction();
                    client.getCL().insert(bulk.getBsonObjects(), DBCollection.FLG_INSERT_REPLACEONDUP);
                    client.getClient().commit();    
                }
            }
        } catch (Exception e) {
            LOG.error("Writer Thread {}" + e.toString(), Thread.currentThread());
            throw e;
        } finally {
            LOG.debug("Writer Thread {} FINSHED", Thread.currentThread());
            client.close();
            latch.countDown();
        }
    } 
}
