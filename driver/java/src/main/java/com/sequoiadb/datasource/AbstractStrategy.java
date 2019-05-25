package com.sequoiadb.datasource;

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


abstract class AbstractStrategy implements IConnectStrategy {

    protected LinkedList<ConnItem> _idleConnItemList = new LinkedList<ConnItem>();
    protected ArrayList<String> _addrs = new ArrayList<String>();
    protected Lock _lockForConnItemList = new ReentrantLock();
    protected Lock _lockForAddr = new ReentrantLock();


    @Override
    public void init(Set<String> addresses, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {

        Iterator<String> addrItr = addresses.iterator();
        while (addrItr.hasNext()) {
            String addr = addrItr.next();
            if (!_addrs.contains(addr)) {
                _addrs.add(addr);
            }
        }
        if (_idleConnPairs != null) {
            Iterator<Pair> connPairItr = _idleConnPairs.iterator();
            while (connPairItr.hasNext()) {
                Pair pair = connPairItr.next();
                String addr = pair.first().getAddr();
                _idleConnItemList.add(pair.first());
                if (!_addrs.contains(addr)) {
                    _addrs.add(addr);
                }
            }
        }
    }

    @Override
    public abstract String getAddress();


    @Override
    public ConnItem pollConnItem(Operation opt) {
        _lockForConnItemList.lock();
        try {
            return _idleConnItemList.poll();
        } finally {
            _lockForConnItemList.unlock();
        }
    }

    @Override
    public void addAddress(String addr) {
        _lockForAddr.lock();
        try {
            if (!_addrs.contains(addr)) {
                _addrs.add(addr);
            }
        } finally {
            _lockForAddr.unlock();
        }
    }

    @Override
    public List<ConnItem> removeAddress(String addr) {
        List<ConnItem> list = new ArrayList<ConnItem>();
        _lockForAddr.lock();
        try {
            if (_addrs.contains(addr)) {
                _addrs.remove(addr);
            }
        } finally {
            _lockForAddr.unlock();
        }
        _lockForConnItemList.lock();
        try {
            Iterator<ConnItem> itr = _idleConnItemList.iterator();
            while (itr.hasNext()) {
                ConnItem item = itr.next();
                if (item.getAddr().equals(addr)) {
                    list.add(item);
                    itr.remove();
                }
            }
        } finally {
            _lockForConnItemList.unlock();
        }
        return list;
    }

    @Override
    public void update(ItemStatus itemStatus, ConnItem connItem, int incDecItemCount) {
        _lockForConnItemList.lock();
        try {
            if (itemStatus == ItemStatus.IDLE && incDecItemCount > 0) {
                _idleConnItemList.add(connItem);
            }
        } finally {
            _lockForConnItemList.unlock();
        }
        return;
    }

}
