package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;

import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

class IdleConnectionPool implements IConnectionPool {

    private HashMap<ConnItem, Sequoiadb> _conns = new HashMap<ConnItem, Sequoiadb>();
    private ReentrantLock _lockForConns = new ReentrantLock();


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

    /**
     * @return a connection or null for no connection in that ConnItem
     * @throws
     * @fn Sequoiadb poll(ConnItem pos)
     * @brief Poll a connection out from the pool according to the offered ConnItem.
     */
    @Override
    public Sequoiadb poll(ConnItem pos) {
        _lockForConns.lock();
        try {
            return _conns.remove(pos);
        } finally {
            _lockForConns.unlock();
        }
    }

    @Override
    public ConnItem poll(Sequoiadb sdb) {
        return null;
    }

    /**
     * @return void.
     * @throws
     * @fn void insert(ConnItem pos, Sequoiadb sdb)
     * @brief Insert a connection into the pool.
     */
    @Override
    public void insert(ConnItem pos, Sequoiadb sdb) {
        _lockForConns.lock();
        try {
            _conns.put(pos, sdb);
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
        return new IdlePairIterator(_conns.entrySet().iterator());
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
        return false;
    }

    @Override
    public List<ConnItem> clear() {
        List<ConnItem> list = new ArrayList<ConnItem>();
        _lockForConns.lock();
        try {
            for (ConnItem item : _conns.keySet()) {
                list.add(item);
            }
            _conns.clear();
            return list;
        } finally {
            _lockForConns.unlock();
        }
    }

}
