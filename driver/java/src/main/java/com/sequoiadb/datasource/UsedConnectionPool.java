/*
 * Copyright 2018 SequoiaDB Inc.
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

package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;

import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

class UsedConnectionPool implements IConnectionPool {

    private HashMap<Sequoiadb, ConnItem> _conns = new HashMap<Sequoiadb, ConnItem>();

    class UsedPairIterator implements Iterator<Pair> {
        Iterator<Map.Entry<Sequoiadb, ConnItem>> _entries;

        public UsedPairIterator(Iterator<Map.Entry<Sequoiadb, ConnItem>> entries) {
            _entries = entries;
        }

        @Override
        public boolean hasNext() {
            return _entries.hasNext();
        }

        @Override
        public Pair next() {
            Map.Entry<Sequoiadb, ConnItem> entry = _entries.next();
            return new Pair(entry.getValue(), entry.getKey());
        }

        @Override
        public void remove() {
            return;
        }
    }

    @Override
    public synchronized Sequoiadb peek(ConnItem connItem) {
        return null;
    }

    /**
     * @return a connection or null for no connection in that ConnItem
     * @throws
     * @fn Sequoiadb poll(ConnItem item)
     * @brief Poll a connection out from the pool according to the offered ConnItem.
     */
    @Override
    public synchronized Sequoiadb poll(ConnItem item) {
        return null;
    }

    @Override
    public synchronized ConnItem poll(Sequoiadb sdb) {
        return _conns.remove(sdb);
    }

    /**
     * @return void.
     * @throws
     * @fn void insert(ConnItem pos, Sequoiadb sdb)
     * @brief Insert a connection into the pool.
     */
    @Override
    public synchronized void insert(ConnItem item, Sequoiadb sdb) {
        _conns.put(sdb, item);
    }

    /**
     * @return the iterator
     * @fn Iterator<ConnItem> getConnItemIterator()
     * @brief Return a iterator for the item of the items of the idle connections.
     */
    @Override
    public synchronized Iterator<Pair> getIterator() {
        return new UsedPairIterator(_conns.entrySet().iterator());
    }

    /**
     * @return the count of idle connections
     * @fn int count()
     * @brief Return the count of idle connections in idle container.
     */
    @Override
    public synchronized int count() {
        return _conns.size();
    }

    @Override
    public synchronized boolean contains(Sequoiadb sdb) {
        return _conns.containsKey(sdb);
    }

    @Override
    public synchronized List<ConnItem> clear() {
        List<ConnItem> list = new ArrayList<ConnItem>();
        for (ConnItem item : _conns.values()) {
            list.add(item);
        }
        _conns.clear();
        return list;
    }

}
