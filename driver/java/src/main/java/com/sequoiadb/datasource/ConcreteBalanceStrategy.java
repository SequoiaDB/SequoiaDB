/**
 * Copyright (C) 2018 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.util.*;


class CountInfo implements Comparable<CountInfo> {
    private String _address;
    // the count of used connection of current address
    private int _usedCount;
    // when there has no idle connections of current address, set _hasLeftIdleConn to be false
    private boolean _hasLeftIdleConn;

    public CountInfo(String addr, int usedCount, boolean hasLeft) {
        _address = addr;
        _usedCount = usedCount;
        _hasLeftIdleConn = hasLeft;
    }

    public void setAddress(String addr) {
        _address = addr;
    }

    public String getAddress() {
        return _address;
    }

    public boolean getHasLeftIdleConn() {
        return _hasLeftIdleConn;
    }

    public void setHasLeftIdleConn(boolean val) {
        _hasLeftIdleConn = val;
    }

    public void changeCount(int change) {
        _usedCount += change;
    }

    @Override
    public int compareTo(CountInfo other) {
        if (this._hasLeftIdleConn == true && other._hasLeftIdleConn == false) {
            return -1;
        } else if (this._hasLeftIdleConn == false && other._hasLeftIdleConn == true) {
            return 1;
        } else {
            if (this._usedCount != other._usedCount) {
                return this._usedCount - other._usedCount;
            } else {
                return this._address.compareTo(other._address);
            }
        }
    }

    @Override
    public String toString() {
        return String.format("{ %s, %d, %b}", _address, _usedCount, _hasLeftIdleConn);
    }
}

class ConcreteBalanceStrategy implements IConnectStrategy {
    // TODO: use 2 deque to hold the ConnItem
    private HashMap<String, ArrayDeque<ConnItem>> _idleConnItemMap = new HashMap<String, ArrayDeque<ConnItem>>();
    private HashMap<String, CountInfo> _countInfoMap = new HashMap<String, CountInfo>();
    private TreeSet<CountInfo> _countInfoSet = new TreeSet<CountInfo>();
    private static CountInfo _dumpCountInfo = new CountInfo("", 0, false);

    public synchronized void init(List<String> addressList, List<Pair> _idleConnPairs,
                                  List<Pair> _usedConnPairs) {
        // initialize info from giving addresses
        Iterator<String> itr1 = addressList.iterator();
        while (itr1.hasNext()) {
            String addr = itr1.next();
            if (!_idleConnItemMap.containsKey(addr)) {
                _idleConnItemMap.put(addr, new ArrayDeque<ConnItem>());
                CountInfo obj = new CountInfo(addr, 0, false);
                _countInfoMap.put(addr, obj);
                _countInfoSet.add(obj);
            }
        }

        // Initialize info from idle connections.
        // If a connection in idle pool has no information in _idleConnItemMap,
        // let's register this connection to _idleConnItemMap, _countInfoMap and
        // _countInfoSet.
        Iterator<Pair> itr2 = null;
        if (_idleConnPairs != null) {
            itr2 = _idleConnPairs.iterator();
            while (itr2.hasNext()) {
                Pair pair = itr2.next();
                ConnItem item = pair.first();
                String addr = item.getAddr();
                if (!_idleConnItemMap.containsKey(addr)) {
                    ArrayDeque<ConnItem> deque = new ArrayDeque<ConnItem>();
                    deque.add(item);
                    _idleConnItemMap.put(addr, deque);
                    // we set this count info to be usable, for now we initialize from
                    // idle connections, but, we don't know how many connections had been
                    // used, so we it to be 0
                    CountInfo info = new CountInfo(addr, 0, true);
                    _countInfoMap.put(addr, info);
                    _countInfoSet.add(info);
                } else {
                    ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
                    deque.add(item);
                    CountInfo info = _countInfoMap.get(addr);
                    if (info.getHasLeftIdleConn() == false) {
                        _countInfoSet.remove(info);
                        info.setHasLeftIdleConn(true);
                        _countInfoSet.add(info);
                    }
                }
            }
        }

        // Initialize info from used connections.
        // Notice that, we won't keep the info of connections whose address had been remove
        // from the pool. So, when _idleConnItemMap does't contain the address of a connection,
        // we will ignore that kind of connections.
        if (_usedConnPairs != null) {
            itr2 = _usedConnPairs.iterator();
            while (itr2.hasNext()) {
                Pair pair = itr2.next();
                ConnItem item = pair.first();
                String addr = item.getAddr();
                if (_idleConnItemMap.containsKey(addr)) {
                    // should remove the original one then modify and insert again
                    CountInfo info = _countInfoMap.get(addr);
                    _countInfoSet.remove(info);
                    info.changeCount(1);
                    _countInfoSet.add(info);
                } else {
                    continue;
                }
            }
        }

    }

    @Override
    public void init(List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {
        // do nothing
    }

    @Override
    public ServerAddress selectAddress(List<ServerAddress> addressList) {
        // do nothing
        return null;
    }

    @Override
    public List<ConnItem> removeConnItemByAddress(String address) {
        // do nothing
        return null;
    }

    @Override
    public synchronized ConnItem pollConnItemForGetting() {
        return _pollConnItem(Operation.GET_CONN);
    }

    @Override
    public synchronized ConnItem pollConnItemForDeleting() {
        return _pollConnItem(Operation.DEL_CONN);
    }

    @Override
    public synchronized ConnItem peekConnItemForDeleting() {
        ConnItem connItem = null;
        while (true) {
            CountInfo countInfo = null;
            String addr = null;
            // we are going to remove connection, so let's
            // get the valid item which count is the largest
            countInfo = _countInfoSet.lower(_dumpCountInfo);
            // if we have no countInfo or all the countInfo are unavailable, let's return
            if (countInfo == null || countInfo.getHasLeftIdleConn() == false) {
                return null;
            }
            addr = countInfo.getAddress();
            /// Now, let's get the ConnItem which associated with "addr".
            ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
            if (deque != null) {
                connItem = deque.peekLast();// .pollLast();
            } else {
                // should never happen
                throw new BaseException(SDBError.SDB_SYS, "Invalid state in strategy");
            }
            /// Check the connItem can be use or not.
            if (connItem == null) {
                // When address "addr" has no idle connection, we get another one.
                // But, before this, let's mark the countInfo of address "addr" to be unavailable.
                // And update this countInfo
                countInfo = _countInfoMap.get(addr);
                _countInfoSet.remove(countInfo);
                countInfo.setHasLeftIdleConn(false);
                _countInfoSet.add(countInfo);
                continue;
            } else {
                // when we get it, let's stop
                break;
            }
        }
        // return
        return connItem;
    }

    private ConnItem _pollConnItem(Operation operation) {
        ConnItem connItem = null;
        while (true) {
            CountInfo countInfo = null;
            String addr = null;
            if (operation == Operation.GET_CONN) {
                // get countInfo of connection which count is the least
                try {
                    countInfo = _countInfoSet.first();
                } catch (NoSuchElementException e) {
                    countInfo = null;
                }
            } else {
                // we are going to remove connection, so let's
                // get the valid item which count is the largest
                countInfo = _countInfoSet.lower(_dumpCountInfo);
            }
            // if we have no countInfo or all the countInfo are unavailable, let's return
            if (countInfo == null || countInfo.getHasLeftIdleConn() == false) {
                return null;
            }
            addr = countInfo.getAddress();
            /// Now, let's get the ConnItem which associated with "addr".
            ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
            if (deque != null) {
                if (operation == Operation.GET_CONN) {
                    connItem = deque.pollFirst();
                } else {
                    connItem = deque.pollLast();
                }
            } else {
                // should never happen
                throw new BaseException(SDBError.SDB_SYS, "Invalid state in strategy");
            }
            /// Check the connItem can be use or not.
            if (connItem == null) {
                // When address "addr" has no idle connection, we get another one.
                // But, before this, let's mark the countInfo of address "addr" to be unavailable.
                // And update this countInfo
                countInfo = _countInfoMap.get(addr);
                _countInfoSet.remove(countInfo);
                countInfo.setHasLeftIdleConn(false);
                _countInfoSet.add(countInfo);
                continue;
            } else {
                // when we get it, let's stop
                break;
            }
        }
        // return
        return connItem;
    }

    @Override
    public synchronized void addConnItemAfterCreating(ConnItem connItem) {
        /// in this case, we are adding connections to idle pool
        String addr = connItem.getAddr();
        if (!_idleConnItemMap.containsKey(addr)) {
            // maybe the information of this address was remove by "removeAddress()"
            // so let's rebuild those information
            _restoreIdleConnItemInfo(addr);
        }
        CountInfo countInfo = _countInfoMap.get(addr);
        if (countInfo == null) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        // update the countInfo
        if (countInfo.getHasLeftIdleConn() == false) {
            _countInfoSet.remove(countInfo);
            countInfo.setHasLeftIdleConn(true);
            _countInfoSet.add(countInfo);
        }

        // add the connItem at the last of deque
        ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
        if (deque == null) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        deque.addLast(connItem);
    }

    @Override
    public synchronized void addConnItemAfterReleasing(ConnItem connItem) {
        /// in this case, we are adding connections to idle pool
        String addr = connItem.getAddr();
        if (!_idleConnItemMap.containsKey(addr)) {
            // maybe the information of this address was remove by "removeAddress()"
            // so let's rebuild those information
            _restoreIdleConnItemInfo(addr);
        }
        CountInfo countInfo = _countInfoMap.get(addr);
        if (countInfo == null) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        // update the countInfo
        if (countInfo.getHasLeftIdleConn() == false) {
            _countInfoSet.remove(countInfo);
            countInfo.setHasLeftIdleConn(true);
            _countInfoSet.add(countInfo);
        }

        // add the connItem at the head of deque
        ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
        if (deque == null) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        deque.addFirst(connItem);
    }

    @Override
    public synchronized void removeConnItemAfterCleaning(ConnItem connItem) {
        /// in this case, we are removing connections from idle pool
        /// when we come here, the CLEAN TASK is working.
        String addr = connItem.getAddr();
        if (!_idleConnItemMap.containsKey(addr)) {
            // maybe the information of this address was remove by "removeAddress()"
            // so let's rebuild those information
            _restoreIdleConnItemInfo(addr);
        }
        ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
        if (deque == null) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        if (deque.size() == 0) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        if (deque.remove(connItem) == false) {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS,
                    "the pool has no information about address: " + addr);
        }
        // when current list has no connItem any more, let's set current address unusable.
        if (deque.size() == 0) {
            CountInfo countInfo = _countInfoMap.get(addr);
            _countInfoSet.remove(countInfo);
            countInfo.setHasLeftIdleConn(false);
            _countInfoSet.add(countInfo);
        }
    }

    @Override
    public synchronized void updateUsedConnItemCount(ConnItem connItem, int change) {
        String addr = connItem.getAddr();
        // when _countInfoMap does not contain this address, this address may be remove by user.
        // see "removeAddress" for more detail.
        if (_countInfoMap.containsKey(addr)) {
            CountInfo countInfo = _countInfoMap.get(addr);
            // the info may be removed when strategy removed address
            if (countInfo == null) {
                // should never happen
                throw new BaseException(SDBError.SDB_SYS,
                        "the pool has no information about address: " + addr);
            }
            _countInfoSet.remove(countInfo);
            countInfo.changeCount(change);
            _countInfoSet.add(countInfo);
        }
    }

    public synchronized String getAddress() {
        // when an address have no connection which had been built,
        // this address will be marked to "false"
        // we are going to get an address for getting or creating a connection,
        // so, we try to get those address which count is 0. if we can't get this
        // kind of address, we try to get an address which count is the least.
        CountInfo info = _countInfoSet.higher(_dumpCountInfo);
        if (info == null) {
            try {
                info = _countInfoSet.first();
            } catch (NoSuchElementException e) {
                // in this case, _countInfoSet is empty
                return null;
            }
        }
        return info.getAddress();
    }

    public synchronized void addAddress(String addr) {

        ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
        if (deque == null) {
            // when we have no info about address "addr", let't prepare
            _idleConnItemMap.put(addr, new ArrayDeque<ConnItem>());
            CountInfo info = new CountInfo(addr, 0, false);
            _countInfoMap.put(addr, info);
            _countInfoSet.add(info);
        }
    }

    public synchronized List<ConnItem> removeAddress(String addr) {
        List<ConnItem> list = new ArrayList<ConnItem>();
        if (_idleConnItemMap.containsKey(addr)) {
            CountInfo obj = _countInfoMap.remove(addr);
            if (obj != null) {
                _countInfoSet.remove(obj);
            }
            ArrayDeque<ConnItem> deque = _idleConnItemMap.remove(addr);
            if (deque != null) {
                for (ConnItem item : deque) {
                    list.add(item);
                }
            }
        }
        return list;
    }

    private void _restoreIdleConnItemInfo(String addr) {
        addAddress(addr);
    }
}
