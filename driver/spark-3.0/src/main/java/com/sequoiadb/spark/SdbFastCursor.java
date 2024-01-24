/*
 * Copyright 2017 SequoiaDB Inc.
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

package com.sequoiadb.spark;

import com.sequoiadb.base.DBCursor;
import org.bson.BSON;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

class SdbFastCursor implements SdbCursor {
    private static final Logger logger = LoggerFactory.getLogger(SdbFastCursor.class);
    private final DBCursor cursor;
    private final int bufSize;
    private final BlockingQueue<byte[]> rawObjs;
    private final BlockingQueue<BSONObject> bsonObjs;
    private final AtomicInteger decodingThreadNum = new AtomicInteger();
    private final AtomicBoolean queryEOC = new AtomicBoolean();
    private final AtomicBoolean decodeEOC = new AtomicBoolean();
    private final AtomicBoolean isClosed = new AtomicBoolean();
    private final AtomicBoolean hasThreadException = new AtomicBoolean();
    private Throwable threadException;
    private final Thread queryThread;
    private final List<Thread> decodeThreads;

    public SdbFastCursor(DBCursor cursor, int bufSize, int decoderNum) {
        if (cursor == null) {
            throw new SdbException("Cursor is null.");
        }

        if (bufSize <= 0) {
            throw new SdbException("Invalid bufSize: " + bufSize);
        }

        if (decoderNum <= 0) {
            throw new SdbException("Invalid decoderNum: " + decoderNum);
        }

        this.cursor = cursor;
        this.bufSize = bufSize;
        this.rawObjs = new LinkedBlockingQueue<>(bufSize);
        this.bsonObjs = new LinkedBlockingQueue<>(bufSize);

        queryThread = new Thread(new Query());
        queryThread.setDaemon(true);
        queryThread.start();

        decodeThreads = new ArrayList<>(decoderNum);
        for (int i = 0; i < decoderNum; i++) {
            Thread thread = new Thread(new Decode());
            thread.setDaemon(true);
            thread.start();
            decodingThreadNum.incrementAndGet();
            decodeThreads.add(thread);
        }
    }

    private class Query implements Runnable {
        @Override
        public void run() {
            try {
                while (!isClosed.get() && !hasThreadException.get()) {
                    if (cursor.hasNextRaw()) {
                        byte[] obj = cursor.getNextRaw();
                        while (!rawObjs.offer(obj, 1, TimeUnit.MILLISECONDS)) {
                            if (isClosed.get()) {
                                break;
                            }
                        }
                    } else {
                        queryEOC.set(true);
                        break;
                    }
                }
            } catch (Exception e) {
                logger.error("Exception happened in query thread", e);
                hasThreadException.set(true);
                threadException = e;
            }
        }
    }

    private class Decode implements Runnable {
        @Override
        public void run() {
            try {
                while (!isClosed.get() && !hasThreadException.get()) {
                    byte[] raw = rawObjs.poll(1, TimeUnit.MILLISECONDS);
                    if (raw == null) {
                        if (queryEOC.get() && rawObjs.size() == 0) {
                            int num = decodingThreadNum.decrementAndGet();
                            if (num == 0) {
                                decodeEOC.set(true);
                            }
                            break;
                        } else {
                            continue;
                        }
                    }

                    BSONObject obj = BSON.decode(raw);
                    while (!bsonObjs.offer(obj, 1, TimeUnit.MILLISECONDS)) {
                        if (isClosed.get()) {
                            break;
                        }
                    }
                }
            } catch (Exception e) {
                logger.error("Exception happened in decode thread", e);
                hasThreadException.set(true);
                threadException = e;
            }
        }
    }

    @Override
    public boolean hasNext() {
        if (isClosed.get()) {
            return false;
        }

        if (hasThreadException.get()) {
            throw new SdbException(threadException);
        }

        while (bsonObjs.size() == 0) {
            if (isClosed.get()) {
                return false;
            }

            if (hasThreadException.get()) {
                throw new SdbException(threadException);
            }

            if (decodeEOC.get() && bsonObjs.size() == 0) {
                return false;
            }
        }

        return true;
    }

    @Override
    public BSONObject next() {
        if (hasNext()) {
            try {
                while (!isClosed.get()) {
                    BSONObject obj = bsonObjs.poll(1, TimeUnit.MILLISECONDS);
                    if (obj == null) {
                        continue;
                    }

                    return obj;
                }

                return null;
            } catch (InterruptedException e) {
                throw new SdbException(e);
            }
        } else {
            return null;
        }
    }

    @Override
    public void close() {
        if (!isClosed.get()) {
            isClosed.set(true);
            try {
                queryThread.join();
                for (Thread t : decodeThreads) {
                    t.join();
                }
            } catch (InterruptedException e) {
                throw new SdbException(e);
            }
            cursor.close();
        }
    }
}
