package com.sequoiadb.datasource;

import java.util.Iterator;
import java.util.List;
import java.util.TreeSet;
import java.util.concurrent.locks.ReentrantLock;


class ConnItem implements Comparable<ConnItem> {
    private String _addr;
    private long _sequenceNumber;

    public ConnItem(String addr, long sequenceNumber) {
        _addr = addr;
        _sequenceNumber = sequenceNumber;
    }

    public String getAddr() {
        return _addr;
    }

    public void setAddr(String addr) {
        _addr = addr;
    }

    public long getSequenceNumber() {
        return _sequenceNumber;
    }

    public void setSequenceNumber(long sequenceNumber) {
        _sequenceNumber = sequenceNumber;
    }

    @Override
    public int compareTo(ConnItem other) {
        if (_sequenceNumber != other._sequenceNumber)
            return (int) (_sequenceNumber - other._sequenceNumber);
        else
            return _addr.compareTo(other._addr);
    }
}

class ConnectionItemMgr {
    private int _capacity;
    private static long _sequenceNumber = -1;
    private TreeSet<ConnItem> _idleItem = null;
    private TreeSet<ConnItem> _usedItem = null;
    private ReentrantLock _lock = new ReentrantLock();

    public ConnectionItemMgr(int capacity, List<ConnItem> usedItems) {
        _capacity = capacity;
        _idleItem = new TreeSet<ConnItem>();
        _usedItem = new TreeSet<ConnItem>();
        int initIdleItemCount = 0;
        if (usedItems != null) {
            Iterator<ConnItem> itr = usedItems.iterator();
            while (itr.hasNext()) {
                _usedItem.add(itr.next());
            }
            initIdleItemCount = (capacity > _usedItem.size()) ? (capacity - _usedItem.size()) : 0;
        } else {
            initIdleItemCount = capacity;
        }
        for (int i = 0; i < initIdleItemCount; i++) {
            _idleItem.add(new ConnItem("", ++_sequenceNumber));
        }
    }

    public long getCurrentSequenceNumber() {
        _lock.lock();
        try {
            return _sequenceNumber;
        } finally {
            _lock.unlock();
        }
    }

    public int getCapacity() {
        _lock.lock();
        try {
            return _capacity;
        } finally {
            _lock.unlock();
        }
    }

    public int getIdleItemNum() {
        _lock.lock();
        try {
            return _idleItem.size();
        } finally {
            _lock.unlock();
        }
    }

    public int getUsedItemNum() {
        _lock.lock();
        try {
            return _usedItem.size();
        } finally {
            _lock.unlock();
        }
    }

    public void resetCapacity(int capacity) {
        _lock.lock();
        try {
            if (_capacity < capacity) {
                for (int i = _capacity; i < capacity; i++) {
                    _idleItem.add(new ConnItem("", ++_sequenceNumber));
                }
            } else {
                int deltaNum = _capacity - capacity;
                while (deltaNum-- != 0) {
                    ConnItem connItem = _idleItem.pollFirst();
                    if (connItem == null) {
                        break;
                    }
                }
            }
            _capacity = capacity;
        } finally {
            _lock.unlock();
        }
    }

    public ConnItem getItem() {
        ConnItem connItem = null;
        _lock.lock();
        try {
            connItem = _idleItem.pollFirst();
            if (connItem != null) {
                connItem.setSequenceNumber(++_sequenceNumber);
                _usedItem.add(connItem);
            }
        } finally {
            _lock.unlock();
        }
        return connItem;
    }

    public void releaseItem(ConnItem item) {
        _lock.lock();
        try {
            _usedItem.remove(item);
            if (_usedItem.size() + _idleItem.size() < _capacity) {
                item.setAddr("");
                _idleItem.add(item);
            }
        } finally {
            _lock.unlock();
        }
    }
}
