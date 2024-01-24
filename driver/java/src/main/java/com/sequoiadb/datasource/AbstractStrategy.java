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

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

abstract class AbstractStrategy implements IConnectStrategy {

    protected ArrayDeque<ConnItem> _activeConnItemDeque = new ArrayDeque<ConnItem>();
    protected ArrayDeque<ConnItem> _newlyCreatedConnItemDeque = new ArrayDeque<ConnItem>();
    protected Lock _opLock = new ReentrantLock();

    // we need to keep all the API thread safe
    @Override
    public void init(List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {
        // Notice that, we won't depend on the address in used queue, for
        // some addresses may have been removed, but, they may be still in used pool.

        // get idle connections information
        if (_idleConnPairs != null) {
            Iterator<Pair> idleConnPairItr = _idleConnPairs.iterator();
            while (idleConnPairItr.hasNext()) {
                Pair pair = idleConnPairItr.next();
                _addConnItem(pair.first());
            }
        }
    }

    @Override
    public ConnItem pollConnItemForGetting() {
        _opLock.lock();
        try {
            // get the youngest one from active deque or get the oldest on from newly created deque
            // for we don't want the newly created connection to be destroy by background cleaning task
            ConnItem connItem = _activeConnItemDeque.pollFirst();
            if (connItem == null) {
                connItem = _newlyCreatedConnItemDeque.pollFirst();
            }
            return connItem;
        } finally {
            _opLock.unlock();
        }
    }

    @Override
    public ConnItem pollConnItemForDeleting() {
        _opLock.lock();
        try {
            // get the oldest one
            ConnItem connItem = _newlyCreatedConnItemDeque.pollFirst();
            if (connItem == null) {
                connItem = _activeConnItemDeque.pollLast();
            }
            return connItem;
        } finally {
            _opLock.unlock();
        }
    }

    @Override
    public ConnItem peekConnItemForDeleting() {
        _opLock.lock();
        try {
            // get the oldest one
            ConnItem connItem = _newlyCreatedConnItemDeque.peekFirst();
            if (connItem == null) {
                connItem = _activeConnItemDeque.peekLast();
            }
            return connItem;
        } finally {
            _opLock.unlock();
        }
    }

    @Override
    public void removeConnItemAfterCleaning(ConnItem connItem) {
        // do nothing
    }

    @Override
    public void updateUsedConnItemCount(ConnItem connItem, int change) {
        // do nothing
    }

    @Override
    public abstract ServerAddress selectAddress(List<ServerAddress> addressList);

    @Override
    public List<ConnItem> removeConnItemByAddress(String address) {
        List<ConnItem> retLst = new ArrayList<>();

        _opLock.lock();
        try {
            Iterator<ConnItem> iterator = _activeConnItemDeque.iterator();
            while (iterator.hasNext()) {
                ConnItem connItem = iterator.next();
                if (address.equals(connItem.getAddr())) {
                    retLst.add(connItem);
                    iterator.remove();
                }
            }
            iterator = _newlyCreatedConnItemDeque.iterator();
            while (iterator.hasNext()) {
                ConnItem connItem = iterator.next();
                if (address.equals(connItem.getAddr())) {
                    retLst.add(connItem);
                    iterator.remove();
                }
            }
        } finally {
            _opLock.unlock();
        }
        return retLst;
    }

    @Override
    public void addConnItemAfterCreating(ConnItem connItem) {
        _addConnItem(connItem);
    }

    @Override
    public void addConnItemAfterReleasing(ConnItem connItem) {
        _releaseConnItem(connItem);
    }

    private void _addConnItem(ConnItem connItem) {
        _opLock.lock();
        try {
            // all the newly created connections are put to the last of the deque
            _newlyCreatedConnItemDeque.addLast(connItem);
        } finally {
            _opLock.unlock();
        }
    }

    private void _releaseConnItem(ConnItem connItem) {
        _opLock.lock();
        try {
            // all the release connections are put to the header of the deque
            _activeConnItemDeque.addFirst(connItem);
        } finally {
            _opLock.unlock();
        }
    }
}
