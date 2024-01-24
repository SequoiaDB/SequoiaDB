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

class IdleConnectionPool implements IConnectionPool {

    private HashMap<ConnItem, Sequoiadb> _conns = new HashMap<ConnItem, Sequoiadb>();

    class IdlePairIterator implements Iterator<Pair> {
        Iterator<Map.Entry<ConnItem, Sequoiadb>> _entries;

        public IdlePairIterator(Iterator<Map.Entry<ConnItem, Sequoiadb>> entries) {
            _entries = entries;
        }

        @Override
        public boolean hasNext() {
            return _entries.hasNext();
        }

        @Override
        public Pair next() {
            Map.Entry<ConnItem, Sequoiadb> entry = _entries.next();
            return new Pair(entry.getKey(), entry.getValue());
        }

        @Override
        public void remove() {
            return;
        }
    }

    public synchronized Sequoiadb peek(ConnItem connItem) {
        return _conns.get(connItem);
    }

    /**
     * @return a connection or null for no connection in that ConnItem
     * @brief Poll a connection out from the pool according to the offered ConnItem.
     */
    @Override
    public synchronized Sequoiadb poll(ConnItem connItem) {
        return _conns.remove(connItem);
    }

    @Override
    public synchronized ConnItem poll(Sequoiadb sdb) {
        return null;
    }

    /**
     * @return void.
     * @throws
     * @fn void insert(ConnItem pos, Sequoiadb sdb)
     * @brief Insert a connection into the pool.
     */
    @Override
    public synchronized void insert(ConnItem pos, Sequoiadb sdb) {
        _conns.put(pos, sdb);
    }

    /**
     * @return the iterator
     * @fn Iterator<ConnItem> getConnItemIterator()
     * @brief Return a iterator for the item of the items of the idle connections.
     */
    @Override
    public synchronized Iterator<Pair> getIterator() {
        return new IdlePairIterator(_conns.entrySet().iterator());
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
        return false;
    }

    @Override
    public synchronized List<ConnItem> clear() {
        List<ConnItem> list = new ArrayList<ConnItem>();
        for (ConnItem item : _conns.keySet()) {
            list.add(item);
        }
        _conns.clear();
        return list;
    }

}
