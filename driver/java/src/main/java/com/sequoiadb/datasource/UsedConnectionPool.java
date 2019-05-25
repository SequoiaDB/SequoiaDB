package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;

import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

class UsedConnectionPool implements IConnectionPool {

    private HashMap<Sequoiadb, ConnItem> _conns = new HashMap<Sequoiadb, ConnItem>();
    private ReentrantLock _lockForConns = new ReentrantLock();

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

    /**
     * @return a connection or null for no connection in that ConnItem
     * @throws
     * @fn Sequoiadb poll(ConnItem item)
     * @brief Poll a connection out from the pool according to the offered ConnItem.
     */
    @Override
    public Sequoiadb poll(ConnItem item) {
        return null;
    }

    @Override
    public ConnItem poll(Sequoiadb sdb) {
        _lockForConns.lock();
        try {
            return _conns.remove(sdb);
        } finally {
            _lockForConns.unlock();
        }
    }

    /**
     * @return void.
     * @throws
     * @fn void insert(ConnItem pos, Sequoiadb sdb)
     * @brief Insert a connection into the pool.
     */
    @Override
    public void insert(ConnItem item, Sequoiadb sdb) {
        _lockForConns.lock();
        try {
            _conns.put(sdb, item);
        } finally {
            _lockForConns.unlock();
        }
    }

    /**
     * @return the iterator
     * @fn Iterator<ConnItem> getConnItemIterator()
     * @brief Return a iterator for the item of the items of the idle connections.
     */
    @Override
    public Iterator<Pair> getIterator() {
        return new UsedPairIterator(_conns.entrySet().iterator());
    }

    /**
     * @return the count of idle connections
     * @fn int count()
     * @brief Return the count of idle connections in idle container.
     */
    @Override
    public int count() {
        _lockForConns.lock();
        try {
            return _conns.size();
        } finally {
            _lockForConns.unlock();
        }
    }

    @Override
    public boolean contains(Sequoiadb sdb) {
        _lockForConns.lock();
        try {
            return _conns.containsKey(sdb);
        } finally {
            _lockForConns.unlock();
        }
    }

    @Override
    public List<ConnItem> clear() {
        List<ConnItem> list = new ArrayList<ConnItem>();
        _lockForConns.lock();
        try {
            for (ConnItem item : _conns.values()) {
                list.add(item);
            }
            _conns.clear();
            return list;
        } finally {
            _lockForConns.unlock();
        }
    }

}
