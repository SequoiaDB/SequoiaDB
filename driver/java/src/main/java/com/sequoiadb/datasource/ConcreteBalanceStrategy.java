package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.util.*;
import java.util.concurrent.locks.ReentrantLock;


class CountInfo implements Comparable<CountInfo> {
    private String _addr;
    private int _count;
    private boolean _available;

    public CountInfo(String addr, int count, boolean availdable) {
        _addr = addr;
        _count = count;
        _available = availdable;
    }

    public void setAddr(String addr) {
        _addr = addr;
    }

    public String getAddr() {
        return _addr;
    }

    public void setCount(int count) {
        _count = count;
    }

    public int getCount() {
        return _count;
    }

    public boolean getAvailable() {
        return _available;
    }

    public void setAvailable(boolean available) {
        _available = available;
    }

    private void _changeCount(int count) {
        _count += count;
    }

    public void increaseCount(int count) {
        _changeCount(count);
    }

    public void decreaseCount(int count) {
        _changeCount(count);
    }

    @Override
    public int compareTo(CountInfo other) {
        if (this._available == true && other._available == false) {
            return -1;
        } else if (this._available == false && other._available == true) {
            return 1;
        } else {
            if (this._count != other._count) {
                return this._count - other._count;
            } else {
                return this._addr.compareTo(other._addr);
            }
        }
    }
}

class ConcreteBalanceStrategy implements IConnectStrategy {

    private HashMap<String, LinkedList<ConnItem>> _idleConnItemMap = new HashMap<String, LinkedList<ConnItem>>();
    private HashMap<String, CountInfo> _countInfoMap = new HashMap<String, CountInfo>();
    private TreeSet<CountInfo> _countInfoSet = new TreeSet<CountInfo>();
    private ReentrantLock _lock = new ReentrantLock();
    private static CountInfo _dumpCountInfo = new CountInfo("", 0, false);

    @Override
    public void init(Set<String> addresses, List<Pair> _idleConnPairs,
                     List<Pair> _usedConnPairs) {
        Iterator<String> addrItr = addresses.iterator();
        while (addrItr.hasNext()) {
            String addr = addrItr.next();
            if (!_idleConnItemMap.containsKey(addr)) {
                _idleConnItemMap.put(addr, new LinkedList<ConnItem>());
                CountInfo obj = new CountInfo(addr, 0, false);
                _countInfoMap.put(addr, obj);
                _countInfoSet.add(obj);
            }
        }

        Iterator<Pair> connPairItr = null;
        if (_idleConnPairs != null) {
            connPairItr = _idleConnPairs.iterator();
            while (connPairItr.hasNext()) {
                Pair pair = connPairItr.next();
                ConnItem item = pair.first();
                String addr = item.getAddr();
                if (!_idleConnItemMap.containsKey(addr)) {
                    LinkedList<ConnItem> list = new LinkedList<ConnItem>();
                    _idleConnItemMap.put(addr, list);
                    list.add(item);
                    CountInfo info = new CountInfo(addr, 0, true);
                    _countInfoMap.put(addr, info);
                    _countInfoSet.add(info);
                } else {
                    LinkedList<ConnItem> list = _idleConnItemMap.get(addr);
                    list.add(item);
                    CountInfo info = _countInfoMap.get(addr);
                    if (false == info.getAvailable()) {
                        _countInfoSet.remove(info);
                        info.setAvailable(true);
                        _countInfoSet.add(info);
                    }
                }
            }
        }

        if (_usedConnPairs != null) {
            connPairItr = _usedConnPairs.iterator();
            while (connPairItr.hasNext()) {
                Pair pair = connPairItr.next();
                ConnItem item = pair.first();
                String addr = item.getAddr();
                if (_idleConnItemMap.containsKey(addr)) {
                    CountInfo info = _countInfoMap.get(addr);
                    _countInfoSet.remove(info);
                    info.increaseCount(1);
                    _countInfoSet.add(info);
                } else {
                    continue;
                }
            }
        }

    }

    @Override
    public ConnItem pollConnItem(Operation opr) {
        ConnItem item = null;
        _lock.lock();
        try {
            while (true) {
                CountInfo info = null;
                String addr = null;
                if (opr == Operation.GET) {
                    try {
                        info = _countInfoSet.first();
                    } catch (NoSuchElementException e) {
                        info = null;
                    }
                } else if (opr == Operation.DELETE) {
                    info = _countInfoSet.lower(_dumpCountInfo);
                } else {
                    throw new BaseException(SDBError.SDB_SYS, "Invalid operation: " + opr);
                }
                if (info == null || info.getAvailable() == false) {
                    return null;
                }
                addr = info.getAddr();
                LinkedList<ConnItem> list = _idleConnItemMap.get(addr);
                if (list != null) {
                    item = list.poll();
                } else {
                    throw new BaseException(SDBError.SDB_SYS, "Invalid state in strategy");
                }

                if (item == null) {
                    info = _countInfoMap.get(addr);
                    _countInfoSet.remove(info);
                    info.setAvailable(false);
                    _countInfoSet.add(info);
                    continue;
                } else {
                    break;
                }
            }
        } finally {
            _lock.unlock();
        }
        return item;
    }

    @Override
    public String getAddress() {
        String addr = null;
        CountInfo info = null;
        _lock.lock();
        try {
            info = _countInfoSet.higher(_dumpCountInfo);
            if (info == null) {
                try {
                    info = _countInfoSet.first();
                } catch (NoSuchElementException e) {
                    return null;
                }
            }
            addr = info.getAddr();
        } finally {
            _lock.unlock();
        }
        return addr;
    }

    /*
     * only when the amount of connections in used pool or idle pool change,
     * we need to update
     * */
    @Override
    public void update(ItemStatus itemStatus, ConnItem connItem, int incDecItemCount) {
        String addr = connItem.getAddr();
        CountInfo countInformation = null;
        LinkedList<ConnItem> idleConnItemList = null;
        _lock.lock();
        try {
            if (itemStatus == ItemStatus.IDLE) {
                if (!_idleConnItemMap.containsKey(addr)) {
                    _restoreIdleConnItemInfo(addr);
                }
                if (incDecItemCount > 0) {
                    countInformation = _countInfoMap.get(addr);
                    if (countInformation == null) {
                        throw new BaseException(SDBError.SDB_SYS, "Point1: the pool has no information about address: " + addr);
                    }
                    if (countInformation.getAvailable() == false) {
                        _countInfoSet.remove(countInformation);
                        countInformation.setAvailable(true);
                        _countInfoSet.add(countInformation);
                    }

                    idleConnItemList = _idleConnItemMap.get(addr);
                    if (idleConnItemList == null) {
                        throw new BaseException(SDBError.SDB_SYS, "Point2: the pool has no information about address: " + addr);
                    }
                    idleConnItemList.add(connItem);
                } else if (incDecItemCount < 0) {
                    idleConnItemList = _idleConnItemMap.get(addr);
                    if (idleConnItemList == null) {
                        throw new BaseException(SDBError.SDB_SYS, "Point3: the pool has no information about address: " + addr);
                    }
                    if (idleConnItemList.size() == 0) {
                        throw new BaseException(SDBError.SDB_SYS, "Point4: the pool has no information about address: " + addr);
                    }
                    if (idleConnItemList.remove(connItem) == false) {
                        throw new BaseException(SDBError.SDB_SYS, "Point5: the pool has no information about address: " + addr);
                    }
                    if (idleConnItemList.size() == 0) {
                        countInformation = _countInfoMap.get(addr);
                        _countInfoSet.remove(countInformation);
                        countInformation.setAvailable(false);
                        _countInfoSet.add(countInformation);
                    }
                } else {
                    throw new BaseException(SDBError.SDB_SYS, "Point1: invalid change in idle pool");
                }
            } else if (itemStatus == ItemStatus.USED) {
                if (_countInfoMap.containsKey(addr)) {
                    countInformation = _countInfoMap.get(addr);
                    if (countInformation == null) {
                        throw new BaseException(SDBError.SDB_SYS, "Point6: the pool has no information about address: " + addr);
                    }
                    _countInfoSet.remove(countInformation);
                    if (incDecItemCount > 0) {
                        countInformation.increaseCount(incDecItemCount);
                    } else if (incDecItemCount < 0) {
                        countInformation.decreaseCount(incDecItemCount);
                    } else {
                        throw new BaseException(SDBError.SDB_SYS, "Point2: invalid change in idle pool");
                    }
                    _countInfoSet.add(countInformation);
                }
            } else {
                throw new BaseException(SDBError.SDB_SYS, "Invalid item status: " + itemStatus);
            }
        } finally {
            _lock.unlock();
        }
    }

    @Override
    public void addAddress(String addr) {
        _lock.lock();
        try {
            List<ConnItem> list = _idleConnItemMap.get(addr);
            if (list == null) {
                _idleConnItemMap.put(addr, new LinkedList<ConnItem>());
                CountInfo info = new CountInfo(addr, 0, false);
                _countInfoMap.put(addr, info);
                _countInfoSet.add(info);
            }
        } finally {
            _lock.unlock();
        }
    }

    @Override
    public List<ConnItem> removeAddress(String addr) {
        List<ConnItem> list = null;
        _lock.lock();
        try {
            list = _idleConnItemMap.remove(addr);
            if (list == null) {
                list = new ArrayList<ConnItem>();
            }
            CountInfo obj = _countInfoMap.remove(addr);
            if (obj != null) {
                _countInfoSet.remove(obj);
            }
        } finally {
            _lock.unlock();
        }
        return list;
    }

    private void _restoreIdleConnItemInfo(String addr) {
        addAddress(addr);
    }
}
