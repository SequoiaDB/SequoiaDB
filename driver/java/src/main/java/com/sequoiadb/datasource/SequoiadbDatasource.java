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

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.log.Log;
import com.sequoiadb.log.LogFactory;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * The implements for SequoiaDB data source
 * @since v1.12.6 & v2.2
 */
public class SequoiadbDatasource {
    private IAddressMgr addrMgr;
    // for created connections
    private final LinkedBlockingQueue<Sequoiadb> _destroyConnQueue = new LinkedBlockingQueue<Sequoiadb>();
    private IConnectionPool _idleConnPool = null;
    private IConnectionPool _usedConnPool = null;
    private IConnectStrategy _strategy = null;
    private ConnectionItemMgr _connItemMgr = null;
    private final Object _createConnSignal = new Object();
    private final Object idleConnSignal = new Object();
    // for creating connections
    private String _username = null;
    private String _password = null;
    private ConfigOptions _normalNwOpt = null;
    private ConfigOptions _internalNwOpt = null;
    private ConfigOptions _userNwOpt = null;
    private DatasourceOptions _dsOpt = null;
    private long _currentSequenceNumber = 0;
    // for  thread
    private ExecutorService _threadExec = null;
    private ScheduledExecutorService _timerExec = null;
    // for pool status
    private volatile boolean _isDatasourceOn = false;
    private volatile boolean _hasClosed = false;
    // for thread safe
    private final ReentrantReadWriteLock _rwLock = new ReentrantReadWriteLock();
    private final Object _objForReleaseConn = new Object();
    // for session
    private volatile BSONObject _sessionAttr = null;
    // for others
    private final Random _rand = new Random(47);
    private int minTimeOut = 0;
    private static final double MULTIPLE = 1.5;
    private volatile int _preDeleteInterval = 0;
    private static final int _deleteInterval = 180000; // 3min
    // for error report
    private static final ThreadLocal<BaseException> lastException = new ThreadLocal<>();
    private static final int FAST_CONNECTION_TIME = 200; // 200ms
    private static Log log = LogFactory.getLog(SequoiadbDatasource.class);

    private boolean needUpdateLocation = false;

    // finalizer guardian
    @SuppressWarnings("unused")
    private final Object finalizerGuardian = new Object() {
        @Override
        protected void finalize() throws Throwable {
            try {
                close();
            } catch (Exception e) {
                // do nothing
            }
        }
    };

    class CreateConnectionTask implements Runnable {
        public void run() {
            try {
                while (!Thread.interrupted()) {
                    synchronized (_createConnSignal) {
                        _createConnSignal.wait();
                    }
                    Lock rlock = _rwLock.readLock();
                    rlock.lock();
                    try {
                        if (Thread.interrupted()) {
                            return;
                        } else {
                            _createConnections();
                        }
                    } finally {
                        rlock.unlock();
                    }
                }
            } catch (InterruptedException e) {
                // do nothing
            }
        }
    }

    class DestroyConnectionTask implements Runnable {
        public void run() {
            try {
                while (!Thread.interrupted()) {
                    Sequoiadb sdb = _destroyConnQueue.take();
                    try {
                        sdb.close();
                    } catch (BaseException e) {
                        continue;
                    }
                }
            } catch (InterruptedException e) {
                try {
                    Sequoiadb[] arr = _destroyConnQueue.toArray(new Sequoiadb[0]);
                    for (Sequoiadb db : arr) {
                        try {
                            db.close();
                        } catch (Exception ex) {
                        }
                    }
                } catch (Exception exp) {
                }
            }
        }
    }

    class CheckConnectionTask implements Runnable {
        @Override
        public void run() {
            Lock wlock = _rwLock.writeLock();
            wlock.lock();
            int timeoutCount = 0;
            int unnecessaryCount = 0;
            try {
                if (Thread.interrupted()) {
                    return;
                }
                if (_hasClosed) {
                    return;
                }
                if (!_isDatasourceOn) {
                    return;
                }
                // check keep alive timeout
                if (_dsOpt.getKeepAliveTimeout() > 0) {
                    long lastTime = 0;
                    long currentTime = System.currentTimeMillis();
                    ConnItem connItem = null;
                    while ((connItem = _strategy.peekConnItemForDeleting()) != null) {
                        Sequoiadb sdb = _idleConnPool.peek(connItem);
                        lastTime = sdb.getLastUseTime();
                        if (currentTime - lastTime + _preDeleteInterval >= _dsOpt.getKeepAliveTimeout()) {
                            connItem = _strategy.pollConnItemForDeleting();
                            sdb = _idleConnPool.poll(connItem);
                            try {
                                _destroyConnQueue.add(sdb);
                            } finally {
                                _connItemMgr.releaseItem(connItem);
                                timeoutCount++;
                            }
                        } else {
                            break;
                        }
                    }
                    if (timeoutCount != 0) {
                        log.debug(String.format("Clean timeout idle connections, timeout idle connections: %d, " +
                                "keepAliveTimeout: %d", timeoutCount, _dsOpt.getKeepAliveTimeout()));
                    }
                }
                // try to reduce the amount of idle connections
                if (_idleConnPool.count() > _dsOpt.getMaxIdleCount()) {
                    int destroyCount = _idleConnPool.count() - _dsOpt.getMaxIdleCount();
                    unnecessaryCount = _reduceIdleConnections(destroyCount);
                    log.debug(String.format("Clean unnecessary idle connections, minIdleCount: %d, maxIdleCount: %d, " +
                            "unnecessary connections: %d", _dsOpt.getMinIdleCount(), _dsOpt.getMaxIdleCount(), unnecessaryCount));
                }
                // when the number of idle connections in the pool is less than the average of minIdleCount
                // and maxIdleCount, we are going to create some connections
                int avgCount = (_dsOpt.getMinIdleCount() + _dsOpt.getMaxIdleCount()) / 2;
                if (_idleConnPool.count() < avgCount) {
                    synchronized (_createConnSignal) {
                        _createConnSignal.notify();
                    }
                    log.debug(String.format("Short of idle connections, notify background task to create connections, " +
                            "used connections: %d", _usedConnPool.count()));
                }
                if (timeoutCount > 0 || unnecessaryCount > 0) {
                    log.info(String.format("Finish check connection task, clean timeout idle connections: %d, clean " +
                            "unnecessary idle connections: %d, %s", timeoutCount, unnecessaryCount, getConnPoolSnapshot()));
                }
            } finally {
                wlock.unlock();
            }
        }
    }

    class RetrieveAddressTask implements Runnable {
        public void run() {
            Lock rlock = _rwLock.readLock();
            rlock.lock();
            try {
                if (Thread.interrupted()) {
                    return;
                }
                if (_hasClosed) {
                    return;
                }
                List<ServerAddress> serAddrLst = addrMgr.getAbnormalAddress();
                for (ServerAddress serAddr : serAddrLst) {
                    Sequoiadb db = null;
                    try {
                        db = new Sequoiadb(serAddr.getAddress(), _username, _password, _internalNwOpt);
                        addrMgr.enableAddress(serAddr.getAddress());
                        log.debug(String.format("Change abnormal address %s to normal address", serAddr.getAddress()));
                    } catch (Exception e) {
                        // ignore the exception
                    } finally {
                        if ( db != null ) {
                            try {
                                db.close();
                            } catch (Exception e) {
                                // do nothing
                            }
                        }
                    }
                }
            } finally {
                rlock.unlock();
            }
        }
    }

    class SyncAddressInfoTask implements Runnable {

        private final UpdateType type;
        private int version;  // The version of SYSCoord group

        SyncAddressInfoTask(UpdateType type) {
            this.type = type;
            this.version = 0;
        }

        @Override
        public void run() {
            Lock wlock = _rwLock.writeLock();
            wlock.lock();
            try {
                if (Thread.interrupted()) {
                    return;
                }
                if (_hasClosed) {
                    return;
                }
                if (_dsOpt.getSyncCoordInterval() == 0 && _dsOpt.getSyncLocationInterval() == 0) {
                    return;
                }

                Sequoiadb db = createTempConn();
                if (db == null) {
                    // if we can't connect to database, let's return
                    return;
                }

                BSONObject addrInfoObj;
                int version;
                try {
                    addrInfoObj = queryAddressInfo(db);
                    version = parseVersionInfo(addrInfoObj);
                    if (version == -1) {
                        log.debug("Failed to get version of SYSCoord group");
                    }
                } catch (Exception e) {
                    // if we failed, let's return
                    log.debug("Synchronize coord address fail", e);
                    return;
                } finally {
                    try {
                        db.close();
                    } catch (Exception e) {
                        // ignore
                    }
                }

                List<String> decList = null;
                switch (type) {
                    case ADDRESS:
                        if (version == -1 || version > this.version) {
                            decList = addrMgr.updateAddressInfo(addrInfoObj, type);
                        }
                        break;
                    case LOCATION:
                        if (needUpdateLocation) {
                            // some new address maybe add from addCoord(), so the sync location task need update
                            // location for they
                            decList = addrMgr.updateAddressInfo(addrInfoObj, type);
                            needUpdateLocation = false;
                        } else if (version == -1 || version > this.version) {
                            decList = addrMgr.updateAddressInfo(addrInfoObj, type);
                        }
                        break;
                    default:
                        throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid update type");
                }

                if (decList != null) {
                    for (String addr : decList) {
                        _removeConnItemInStrategy(addr);
                    }
                }

                if (version > this.version) {
                    this.version = version;
                }
            } finally {
                wlock.unlock();
            }
        }
    }

    /**
     * Get a builder to create SequoiadbDatasource instance.
     *
     * @return A builder of SequoiadbDatasource
     */
    public static Builder builder() {
        return new Builder();
    }

    private SequoiadbDatasource( Builder builder ) {
        _init(builder.addressList, builder.location, builder.userConfig.getUserName(), builder.userConfig.getPassword(),
                builder.configOptions, builder.datasourceOptions);
    }

    /**
     * When offer several addresses for connection pool to use, if
     * some of them are not available(invalid address, network error, coord shutdown,
     * catalog replica group is not available), we will put these addresses
     * into a queue, and check them periodically. If some of them is valid again,
     * get them back for use. When connection pool get a unavailable address to connect,
     * the default timeout is 100ms, and default retry time is 0. Parameter nwOpt can
     * can change both of the default value.
     * @param addressList the addresses of coord nodes, can't be null or empty,
     *                 e.g."ubuntu1:11810","ubuntu2:11810",...
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param nwOpt    the options for connection
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     * @see ConfigOptions
     * @see DatasourceOptions
     */
    public SequoiadbDatasource(List<String> addressList, String username, String password,
                               ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        if (null == addressList || 0 == addressList.size())
            throw new BaseException(SDBError.SDB_INVALIDARG, "coord addresses can't be empty or null");

        // init connection pool
        _init(addressList, "", username, password, nwOpt, dsOpt);
    }

    /**
     * @deprecated Use com.sequoiadb.base.ConfigOptions instead of com.sequoiadb.net.ConfigOptions.
     * @see #SequoiadbDatasource(List, String, String, com.sequoiadb.base.ConfigOptions, DatasourceOptions)
     */
    @Deprecated
    public SequoiadbDatasource(List<String> addressList, String username, String password,
                               com.sequoiadb.net.ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        this(addressList, username, password, (ConfigOptions) nwOpt, dsOpt);
    }

    /**
     * @param address      the address of coord, can't be empty or null, e.g."ubuntu1:11810"
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     */
    public SequoiadbDatasource(String address, String username, String password,
                               DatasourceOptions dsOpt) throws BaseException {
        if (address == null || address.isEmpty())
            throw new BaseException(SDBError.SDB_INVALIDARG, "coord address can't be empty or null");
        ArrayList<String> addrLst = new ArrayList<String>();
        addrLst.add(address);
        _init(addrLst, "", username, password, null, dsOpt);
    }

    /**
     * Get the current idle connection amount.
     */
    public int getIdleConnNum() {
        if (_idleConnPool == null)
            return 0;
        else
            return _idleConnPool.count();
    }

    /**
     * Get the current used connection amount.
     */
    public int getUsedConnNum() {
        if (_usedConnPool == null)
            return 0;
        else
            return _usedConnPool.count();
    }

    /**
     * Get the current normal address amount.
     */
    public int getNormalAddrNum() {
        return addrMgr.getNormalAddressSize();
    }

    /**
     * Get the current abnormal address amount.
     */
    public int getAbnormalAddrNum() {
        return addrMgr.getAbnormalAddressSize();
    }

    /**
     * Get the amount of local coord node address.
     * This method works only when the pool is enabled and the connect
     * strategy is ConnectStrategy.LOCAL, otherwise return 0.
     * @return the amount of local coord node address
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public int getLocalAddrNum() {
        return addrMgr.getLocalAddressSize();
    }

    /**
     * Add coord address.
     * @param address The address of coord node. Format: "hostname:port"
     * @throws BaseException If error happens.
     */
    public void addCoord(String address) throws BaseException {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "Connection pool has closed");
            }
            if (address == null || address.equals( "" ) ) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "Address can't be empty or null");
            }
            String addr = Helper.parseAddress(address);
            addrMgr.addAddress(addr);
            needUpdateLocation = true;
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Remove coord address.
     * @param address The address of coord node. Format: "hostname:port"
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void removeCoord(String address) throws BaseException {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "Connection pool has closed");
            }
            if (address == null || address.equals( "" )) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "Address can't be empty or null");
            }
            String addr = Helper.parseAddress(address);
            addrMgr.removeAddress(addr);
            log.info(String.format("Remove address: %s", address));
            if (_isDatasourceOn) {
                // remove from strategy
                _removeConnItemInStrategy(addr);
            }
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Get a copy of the connection pool options.
     * @return a copy of the connection pool options
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public DatasourceOptions getDatasourceOptions() throws BaseException {
        Lock rlock = _rwLock.readLock();
        rlock.lock();
        try {
            return (DatasourceOptions) _dsOpt.clone();
        } catch (CloneNotSupportedException e) {
            throw new BaseException(SDBError.SDB_SYS, "failed to clone connnection pool options");
        } finally {
            rlock.unlock();
        }
    }

    /**
     * Update connection pool options.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void updateDatasourceOptions(DatasourceOptions dsOpt) throws BaseException {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "connection pool has closed");
            }
            // check options
            _checkDatasourceOptions(dsOpt);
            // save previous values
            int previousMaxCount = _dsOpt.getMaxCount();
            int previousCheckInterval = _dsOpt.getCheckInterval();
            int previousSyncCoordInterval = _dsOpt.getSyncCoordInterval();
            int previousSyncLocationInterval = _dsOpt.getSyncLocationInterval();
            ConnectStrategy previousStrategy = _dsOpt.getConnectStrategy();

            // reset options
            try {
                _dsOpt = (DatasourceOptions) dsOpt.clone();
            } catch (CloneNotSupportedException e) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "failed to clone connection pool options");
            }
            // when data source is disable, return directly
            if (!_isDatasourceOn) {
                return;
            }
            log.info(String.format("Sequoiadb datasource has been update datasource config, old %s, " +
                    "new %s", _dsOpt.toString(), dsOpt.toString()));
            // update network block timeout
            _normalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());
            _internalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());

            // when _maxCount is set to 0, disable data source and return
            if (_dsOpt.getMaxCount() == 0) {
                disableDatasource();
                return;
            }
            _preDeleteInterval = (int) (_dsOpt.getCheckInterval() * MULTIPLE);
            // check need to adjust the capacity of connection pool or not.
            // when the data source is disable, we can't change the "_currentSequenceNumber"
            // to the value we want, that's a problem, so we will change "_currentSequenceNumber"
            // in "_enableDatasource()"
            if (previousMaxCount != _dsOpt.getMaxCount()) {
                // when "_enableDatasource()" is not called, "_connItemMgr" will be null
                if (_connItemMgr != null) {
                    _connItemMgr.resetCapacity(_dsOpt.getMaxCount());
                    if (_dsOpt.getMaxCount() < previousMaxCount) {
                        // make sure we have not get connection item more then
                        // _dsOpt.getMaxCount(), if so, let't decrease some in
                        // idle pool. But, we won't decrease any in used pool.
                        // When a connection is get out from used pool, we will
                        // check whether the item pool is full or not, if so, we
                        // won't let the connection and the item for it go to idle
                        // pool, we will destroy both out them.
                        int deltaNum = getIdleConnNum() + getUsedConnNum() - _dsOpt.getMaxCount();
                        int destroyNum = (deltaNum > getIdleConnNum()) ? getIdleConnNum() : deltaNum;
                        if (destroyNum > 0)
                            _reduceIdleConnections(destroyNum);
                        // update the version, so, all the outdated caching connections in used pool
                        // can not go back to idle pool any more.
                        _currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
                    }
                } else {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "the item manager is null");
                }
            }
            // check need to restart timer and threads or not
            if (previousStrategy != _dsOpt.getConnectStrategy()) {
                _cancelTimer();
                _cancelThreads();
                _changeStrategy();
                _startTimer();
                _startThreads();
                _currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
            } else if (previousCheckInterval != _dsOpt.getCheckInterval() ||
                    previousSyncCoordInterval != _dsOpt.getSyncCoordInterval() ||
                    previousSyncLocationInterval != _dsOpt.getSyncLocationInterval()) {
                _cancelTimer();
                _startTimer();
            }
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Enable data source.
     * When maxCount is 0, set it to be the default value(500).
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void enableDatasource() {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "connection pool has closed");
            }
            if (_isDatasourceOn) {
                return;
            }
            if (_dsOpt.getMaxCount() == 0) {
                _dsOpt.setMaxCount(500);
            }
            _enableDatasource(_dsOpt.getConnectStrategy());
            log.info("Sequoiadb datasource has been enable");
        } finally {
            wlock.unlock();
        }
        return;
    }

    /**
     * Disable the data source.
     * After disable data source, the pool will not manage
     * the connections again. When a getting request comes,
     * the pool build and return a connection; When a connection
     * is put back, the pool disconnect it directly.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void disableDatasource() {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "connection pool has closed");
            }
            if (!_isDatasourceOn) {
                return;
            }
            // stop timer
            _cancelTimer();
            // stop threads
            _cancelThreads();
            // close the connections in idle pool
            log.debug(String.format("Close all idle connections, number: %d", _idleConnPool.count()));
            _closePoolConnections(_idleConnPool);
            _isDatasourceOn = false;
        } finally {
            wlock.unlock();
        }
        log.info("Sequoiadb datasource has been disable");
        return;
    }

    /**
     * Get a connection from current connection pool. The waiting time default to 5 seconds
     * @return Sequoiadb The connection for using
     * @throws BaseException When waiting for timeout, throw {@link SDBError#SDB_DRIVER_DS_RUNOUT}
     *                       if current connection pool is full, otherwise throw {@link SDBError#SDB_TIMEOUT}.
     * @throws InterruptedException Actually, nothing happen. Throw this for compatibility reason.
     */
    public Sequoiadb getConnection() throws BaseException, InterruptedException {
        return getConnection(5000);
    }

    /**
     * Get a connection from current connection pool.
     * @param timeout The time for waiting for connection in millisecond. 0 means infinite timeout.
     * @return Sequoiadb The connection for using
     * @throws BaseException When waiting for timeout, throw {@link SDBError#SDB_DRIVER_DS_RUNOUT}
     *                       if current connection pool is full, otherwise throw {@link SDBError#SDB_TIMEOUT}.
     * @throws InterruptedException Actually, nothing happen. Throw this for compatibility reason.
     * @since 2.2
     */
    public Sequoiadb getConnection(long timeout) throws BaseException, InterruptedException {
        if (timeout < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "timeout should >= 0");
        }

        Lock rlock = _rwLock.readLock();
        rlock.lock();
        try {
            Sequoiadb sdb;
            ConnItem connItem;
            Timer timer = new Timer(timeout);

            while (true) {
                if (_hasClosed) {
                    throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "connection pool has closed");
                }

                // disabled status
                if (!_isDatasourceOn) {
                    sdb = createConnByAddr(timer);
                    // Use external network configuration when connection leaving the pool
                    updateConnConf(sdb, _userNwOpt);
                    return sdb;
                }

                // enable status
                if ((connItem = _strategy.pollConnItemForGetting()) != null) {
                    // 1. get connection from idle pool
                    sdb = _idleConnPool.poll(connItem);
                    // sanity check
                    if (sdb == null) {
                        _connItemMgr.releaseItem(connItem);
                        // should never come here
                        throw new BaseException(SDBError.SDB_SYS, "no matching connection");
                    }
                } else if ((connItem = _connItemMgr.getItem()) != null) {
                    // 2. idle pool is empty, create a connection directly
                    try {
                        sdb = createConnByAddr(timer);
                    } catch (BaseException e) {
                        _connItemMgr.releaseItem(connItem);
                        throw e;
                    }
                    // set session attributes for new connection
                    if (_sessionAttr != null) {
                        try {
                            sdb.setSessionAttr(_sessionAttr);
                        } catch (Exception e) {
                            _connItemMgr.releaseItem(connItem);
                            _destroyConnQueue.add(sdb);
                            throw new BaseException(SDBError.SDB_SYS,
                                    String.format("failed to set the session attribute[%s]",
                                            _sessionAttr.toString()), e);
                        }
                    }
                    connItem.setAddr(sdb.getServerAddress().toString());
                    // wait up background thread to create connections
                    synchronized (_createConnSignal) {
                        _createConnSignal.notify();
                    }
                } else {
                    // release the read lock before wait up
                    rlock.unlock();
                    try {
                        // if timer is disabled, wait 5s each time
                        if (!timer.getStatus()) {
                            synchronized (idleConnSignal) {
                                idleConnSignal.wait(5000);
                            }
                        } else {
                            // the connection pool is full
                            if (timer.isTimeout()) {
                                throw new BaseException(SDBError.SDB_DRIVER_DS_RUNOUT, "Get connection timeout: " + timer.getOriginTime());
                            }
                            long startTime = System.currentTimeMillis();
                            synchronized (idleConnSignal) {
                                idleConnSignal.wait(timer.getTime());
                            }
                            timer.consumeTime(startTime);
                        }
                        continue;
                    } finally {
                        // let's get the read lock before going on
                        rlock.lock();
                    }
                }
                // check whether the connection is usable
                if (sdb.isClosed() || (_dsOpt.getValidateConnection() && !sdb.isValid())) {
                    // let the item go back to _connItemMgr and destroy
                    // the connection, then try again
                    _connItemMgr.releaseItem(connItem);
                    _destroyConnQueue.add(sdb);
                } else {
                    // stop looping
                    break;
                }
            } // while(true)

            // connItem and sdb never be null
            _usedConnPool.insert(connItem, sdb);
            _strategy.updateUsedConnItemCount(connItem, 1);
            // Use external network configuration when connection leaving the pool
            updateConnConf(sdb, _userNwOpt);
            return sdb;
        } finally {
            rlock.unlock();
        }
    }

    /**
     * Put the connection back to the connection pool.
     * When the data source is enable, we can't double release
     * one connection, and we can't offer a connection which is
     * not belong to the pool.
     * @param sdb the connection to come back, can't be null
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void releaseConnection(Sequoiadb sdb) throws BaseException {
        Lock rlock = _rwLock.readLock();
        rlock.lock();
        try {
            if (sdb == null) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "connection can't be null");
            }
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_CLIENT_CONNPOOL_CLOSE, "connection pool has closed");
            }
            // in case the data source is disable
            if (!_isDatasourceOn) {
                // when we disable data source, we should try to remove
                // the connections left in used connection pool
                synchronized (_objForReleaseConn) {
                    if (_usedConnPool != null && _usedConnPool.contains(sdb)) {
                        ConnItem item = _usedConnPool.poll(sdb);
                        if (item == null) {
                            // multi-thread may let item to be null, and it should never happen
                            throw new BaseException(SDBError.SDB_SYS,
                                    "the pool does't have item for the coming back connection");
                        }
                        _connItemMgr.releaseItem(item);
                        // Use internal network configuration when connection back to the pool
                        updateConnConf(sdb, _normalNwOpt);
                    }
                }
                try {
                    sdb.disconnect();
                } catch (Exception e) {
                    // do nothing
                }
                return;
            }
            // in case the data source is enable
            ConnItem item = null;
            synchronized (_objForReleaseConn) {
                // if the busy pool contains this connection
                if (_usedConnPool.contains(sdb)) {
                    // remove it from used queue
                    item = _usedConnPool.poll(sdb);
                    if (item == null) {
                        throw new BaseException(SDBError.SDB_SYS,
                                "the pool does not have item for the coming back connection");
                    }
                } else {
                    // throw exception to let user know current connection does't contained in the pool
                    throw new BaseException(SDBError.SDB_INVALIDARG,
                            "the connection pool doesn't contain the offered connection");
                }
                // Use internal network configuration when connection back to the pool
                updateConnConf(sdb, _normalNwOpt);
            }
            // we have decreased connection in used pool, let's update the strategy
            _strategy.updateUsedConnItemCount(item, -1);
            // check whether the connection can put back to idle pool or not
            if (_connIsValid(item, sdb)) {
                // let the connection come back to connection pool
                _idleConnPool.insert(item, sdb);
                // tell the strategy one connection is add to idle pool now
                _strategy.addConnItemAfterReleasing(item);
            } else {
                // let the item come back to item pool, and destroy the connection
                _connItemMgr.releaseItem(item);
                _destroyConnQueue.add(sdb);
            }
            synchronized (idleConnSignal) {
                idleConnSignal.notifyAll();
            }
        } finally {
            rlock.unlock();
        }
    }

    /**
     * Put the connection back to the connection pool.
     * When the data source is enable, we can't double release
     * one connection, and we can't offer a connection which is
     * not belong to the pool.
     * @param sdb the connection to come back, can't be null
     * @throws BaseException If error happens.
     * @see #releaseConnection(Sequoiadb)
     * @deprecated use releaseConnection() instead
     */
    public void close(Sequoiadb sdb) throws BaseException {
        releaseConnection(sdb);
    }

    /**
     * Clean all resources of current connection pool.
     */
    public void close() {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                return;
            }
            if (_isDatasourceOn) {
                _cancelTimer();
                _cancelThreads();
            }
            // close connections
            if (_idleConnPool != null) {
                log.debug(String.format("Close all idle connections, number: %d", _idleConnPool.count()));
                _closePoolConnections(_idleConnPool);
            }
            if (_usedConnPool != null) {
                log.debug(String.format("Close all used connections, number: %d", _usedConnPool.count()));
                _closePoolConnections(_usedConnPool);
            }
            _isDatasourceOn = false;
            _hasClosed = true;
            log.info("Sequoiadb datasource has been closed");
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Get the location name of current connection pool.
     * @return The location name
     */
    public String getLocation() {
        return addrMgr.getLocation();
    }

    private void _init(List<String> addrList, String location, String username, String password,
                       ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        addrMgr = new AddressMgr(addrList, location);

        _username = (username == null) ? "" : username;
        _password = (password == null) ? "" : password;

        if (dsOpt == null) {
            _dsOpt = new DatasourceOptions();
        } else {
            try {
                _dsOpt = (DatasourceOptions) dsOpt.clone();
            } catch (CloneNotSupportedException e) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "failed to clone connection pool options");
            }
        }
        // check options
        _checkDatasourceOptions(_dsOpt);

        ConfigOptions temp = nwOpt;
        if (temp == null){
            temp = new ConfigOptions();
            temp.setConnectTimeout(100);
            temp.setMaxAutoConnectRetryTime(0);
        }
        try {
            // used inside of the connection pool
            _normalNwOpt = (ConfigOptions) temp.clone();
            _internalNwOpt = (ConfigOptions) temp.clone();
            // used outside of the connection pool
            _userNwOpt = (ConfigOptions) temp.clone();
        } catch (CloneNotSupportedException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "failed to clone connection pool options");
        }
        // set the network block timeout inside the connection pool
        // to avoid socket stuck due to network errors.
        _normalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());
        _internalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());
        // for fast connection, no need to set retry
        if (_normalNwOpt.getConnectTimeout() == 0) {
            minTimeOut = FAST_CONNECTION_TIME;
        } else {
            minTimeOut = Math.min(FAST_CONNECTION_TIME, _normalNwOpt.getConnectTimeout());
        }
        _internalNwOpt.setConnectTimeout(minTimeOut);
        _internalNwOpt.setMaxAutoConnectRetryTime(0);

        if (!addrMgr.getLocation().isEmpty()) {
            updateLocationInfo();
        }

        // if connection is shutdown, return directly
        if (_dsOpt.getMaxCount() == 0) {
            _isDatasourceOn = false;
        } else {
            _enableDatasource(_dsOpt.getConnectStrategy());
        }

        log.info(String.format("Sequoiadb datasource initialized successfully, status is %s, %s, %s",
                _isDatasourceOn? "enable" : "disable", _dsOpt.toString(), _userNwOpt.toString()));
    }

    private void _startTimer() {
        _timerExec = Executors.newScheduledThreadPool(1,
                new ThreadFactory() {
                    public Thread newThread(Runnable r) {
                        Thread t = Executors.defaultThreadFactory().newThread(r);
                        t.setDaemon(true);
                        return t;
                    }
                }
        );
        if (_dsOpt.getSyncCoordInterval() > 0) {
            _timerExec.scheduleAtFixedRate(new SyncAddressInfoTask(UpdateType.ADDRESS), 0, _dsOpt.getSyncCoordInterval(),
                    TimeUnit.MILLISECONDS);
            log.debug("Synchronize address task has been started");
        }
        if (_dsOpt.getSyncLocationInterval() > 0 && !addrMgr.getLocation().isEmpty()) {
            _timerExec.scheduleAtFixedRate(new SyncAddressInfoTask(UpdateType.LOCATION), 0, _dsOpt.getSyncLocationInterval(),
                    TimeUnit.MILLISECONDS);
            log.debug("Synchronize location task has been started");
        }

        _timerExec.scheduleAtFixedRate(new CheckConnectionTask(), _dsOpt.getCheckInterval(),
                _dsOpt.getCheckInterval(), TimeUnit.MILLISECONDS);
        log.debug("Check connection task has been started");
        _timerExec.scheduleAtFixedRate(new RetrieveAddressTask(), 60, 60, TimeUnit.SECONDS);
        log.debug("Retrieve address task has been started");
    }

    private void _cancelTimer() {
        _timerExec.shutdownNow();
        if (_dsOpt.getSyncCoordInterval() > 0) {
            log.debug("Synchronize address task has been closed");
        }
        if (_dsOpt.getSyncLocationInterval() > 0 && !addrMgr.getLocation().isEmpty()) {
            log.debug("Synchronize location task has been closed");
        }
        log.debug("Check connection task has been closed");
        log.debug("Retrieve address task has been closed");
    }

    private void _startThreads() {
        _threadExec = Executors.newCachedThreadPool(
                new ThreadFactory() {
                    public Thread newThread(Runnable r) {
                        Thread t = Executors.defaultThreadFactory().newThread(r);
                        t.setDaemon(true);
                        return t;
                    }
                }
        );
        _threadExec.execute(new CreateConnectionTask());
        log.debug("Start create connection task");
        _threadExec.execute(new DestroyConnectionTask());
        // stop adding task
        _threadExec.shutdown();
    }

    private void _cancelThreads() {
        _threadExec.shutdownNow();
        log.debug("Create connection task has been closed");
    }

    private void _changeStrategy() {
        List<Pair> idleConnPairs = new ArrayList<Pair>();
        List<Pair> usedConnPairs = new ArrayList<Pair>();
        Iterator<Pair> itr = null;
        itr = _idleConnPool.getIterator();
        while (itr.hasNext()) {
            idleConnPairs.add(itr.next());
        }
        itr = _usedConnPool.getIterator();
        while (itr.hasNext()) {
            usedConnPairs.add(itr.next());
        }
        _strategy = _createStrategy(_dsOpt.getConnectStrategy());
        // here we don't need to offer abnormal address, for "RetrieveAddressTask"
        // will here us to add those addresses to strategy when those addresses
        // can be use again
        _strategy.init(idleConnPairs, usedConnPairs);
    }

    private void _closePoolConnections(IConnectionPool pool) {
        if (pool == null) {
            return;
        }
        // disconnect all the connections
        Iterator<Pair> iter = pool.getIterator();
        while (iter.hasNext()) {
            Pair pair = iter.next();
            Sequoiadb sdb = pair.second();
            try {
                sdb.disconnect();
            } catch (Exception e) {
                // do nothing
            }
        }
        // clear them from the pool
        List<ConnItem> list = pool.clear();
        for (ConnItem item : list)
            _connItemMgr.releaseItem(item);
        // we are not clear the info in strategy,
        // for the strategy instance is abandoned,
        // and we will create a new one next time
    }

    private void _checkDatasourceOptions(DatasourceOptions newOpt) throws BaseException {
        if (newOpt == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "the offering datasource options can't be null");
        }
        int deltaIncCount = newOpt.getDeltaIncCount();
        int maxIdleCount = newOpt.getMaxIdleCount();
        int minIdleCount = newOpt.getMinIdleCount();
        int maxCount = newOpt.getMaxCount();
        int keepAliveTimeout = newOpt.getKeepAliveTimeout();
        int checkInterval = newOpt.getCheckInterval();

        if (minIdleCount > maxIdleCount)
            throw new BaseException(SDBError.SDB_INVALIDARG, "minIdleCount can't be more than maxIdleCount");

        if (keepAliveTimeout != 0 && checkInterval >= keepAliveTimeout)
            throw new BaseException(SDBError.SDB_INVALIDARG, "when keepAliveTimeout is not 0, checkInterval should be less than keepAliveTimeout");

        if (maxCount != 0) {
            if (deltaIncCount > maxCount)
                throw new BaseException(SDBError.SDB_INVALIDARG, "deltaIncCount can't be more than maxCount");
            if (maxIdleCount > maxCount)
                throw new BaseException(SDBError.SDB_INVALIDARG, "maxIdleCount can't be more than maxCount");
        }

        _sessionAttr = newOpt.getSessionAttr();
    }

    private Sequoiadb createConnByAddr(Timer timer) throws BaseException {
        try {
            // quickly skip unavailable addresses
            Sequoiadb db = fastCreateConn(timer);
            if (db != null) {
                return db;
            }

            // create connection with normal timeout
            db = createConnByNormalAddr(timer);
            if (db == null) {
                db = createConnByAbnormalAddr(timer);
            }

            if (db == null) {
                String detail = _getDataSourceSnapshot();
                BaseException exp = _getLastException();
                String errMsg = "No available address for connection, " + detail;
                if (exp != null) {
                    throw new BaseException(SDBError.SDB_NETWORK, errMsg, exp);
                } else {
                    throw new BaseException(SDBError.SDB_NETWORK, errMsg);
                }
            }
            return db;
        } catch (BaseException e) {
            throw e;
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_SYS, e);
        }
    }

    private Sequoiadb fastCreateConn(Timer timer) throws BaseException {
        Sequoiadb db = null;
        ServerAddress serAddr;
        List<ServerAddress> serAddrLst = addrMgr.getAddress();

        while (!serAddrLst.isEmpty()) {
            if (timer.isTimeout()) {
                throw new BaseException(SDBError.SDB_TIMEOUT, "Get connection timeout: " + timer.getOriginTime());
            }

            // check disable
            if (_isDatasourceOn) {
                serAddr = _strategy.selectAddress(serAddrLst);
            } else {
                serAddr = serAddrLst.get(_rand.nextInt(serAddrLst.size()));
            }

            if (serAddr == null) {
                break;
            }

            long startTime = System.currentTimeMillis();
            try {
                // use _internalNwOpt to quickly skip unavailable addresses
                db = new Sequoiadb(serAddr.getAddress(), _username, _password, _internalNwOpt);
                clearLastException();
                break;
            } catch (BaseException e) {
                _setLastException(e);
                if (e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT.getErrorCode()) {
                    throw e;
                }
                serAddrLst.remove(serAddr);
            } finally {
                timer.consumeTime(startTime);
            }
        }

        return db;
    }

    private Sequoiadb createConnByNormalAddr(Timer timer) throws BaseException {
        Sequoiadb sdb = null;
        ServerAddress serAddr = null;
        ConfigOptions netOpt;

        try {
            netOpt = (ConfigOptions)_normalNwOpt.clone();
        } catch (CloneNotSupportedException e) {
            throw new BaseException(SDBError.SDB_SYS, e);
        }

        while (true) {
            // Location maybe change, so need get addresses from addrMgr every time
            List<ServerAddress> serAddrLst = addrMgr.getAddress();

            if (timer.isTimeout()) {
                throw new BaseException(SDBError.SDB_TIMEOUT, "Get connection timeout: " + timer.getOriginTime());
            }
            long startTime = System.currentTimeMillis();
            // in order to control the time, the connection timeout time needs to be reset every time
            resetConnTime(timer, netOpt);

            // never forget to handle the situation of the datasource is disable
            if (_isDatasourceOn) {
                serAddr = _strategy.selectAddress(serAddrLst);
            } else {
                int size = serAddrLst.size();
                if (size > 0) {
                    serAddr = serAddrLst.get(_rand.nextInt(size));
                }
            }
            if (serAddr == null) {
                break;
            }

            try {
                sdb = new Sequoiadb(serAddr.getAddress(), _username, _password, netOpt);
                clearLastException();
                // when success, let's return the connection
                break;
            } catch (BaseException e) {
                _setLastException(e);
                if (e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT.getErrorCode()) {
                    throw e;
                }
                _handleErrorAddr(serAddr.getAddress());
                serAddrLst.remove(serAddr);
                serAddr = null;
            } finally {
                timer.consumeTime(startTime);
            }
        }
        return sdb;
    }

    private Sequoiadb createConnByAbnormalAddr(Timer timer) throws BaseException {
        Sequoiadb retConn = null;
        int retry = 3;

        while (retry-- > 0) {
            for (ServerAddress serAddr : addrMgr.getAbnormalAddress()) {
                if (timer.isTimeout()) {
                    throw new BaseException(SDBError.SDB_TIMEOUT, "Get connection timeout: " + timer.getOriginTime());
                }
                long startTime = System.currentTimeMillis();
                String addr = serAddr.getAddress();
                try {
                    // it takes very little time to create a connection with an abnormal address
                    retConn = new Sequoiadb(addr, _username, _password, _internalNwOpt);
                    clearLastException();
                } catch (BaseException e) {
                    _setLastException(e);
                    continue;
                } catch (Exception e) {
                    _setLastException(new BaseException(SDBError.SDB_SYS, e));
                    continue;
                } finally {
                    timer.consumeTime(startTime);
                }
                addrMgr.enableAddress(addr);
                log.debug(String.format("Create connections success with abnormal address: %s, " +
                        "change it to normal address", addr));
                break;
            }
            if (retConn != null) {
                break;
            }
        }
        return retConn;
    }

    private String _getDataSourceSnapshot() {
        String threadInfo = String.format("[thread id: %d]", Thread.currentThread().getId());
        return threadInfo + ", " + getConnItemSnapshot() + ", " + getConnPoolSnapshot() + ", " + addrMgr.getAddressSnapshot();
    }

    private void _setLastException(BaseException e) {
        lastException.set(e);
    }

    private BaseException _getLastException() {
        BaseException e = lastException.get();
        clearLastException();
        return e;
    }

    private void clearLastException() {
        lastException.remove();
    }

    private void _handleErrorAddr(String address) {
        addrMgr.disableAddress(address);
        log.debug(String.format("Create connections fail with normal address: %s, change it to abnormal address", address));
        if (_isDatasourceOn) {
            _removeConnItemInStrategy(address);
        }
    }

    private void _removeConnItemInStrategy(String address) {
        List<ConnItem> list = _strategy.removeConnItemByAddress(address);
        Iterator<ConnItem> itr = list.iterator();
        while (itr.hasNext()) {
            ConnItem item = itr.next();
            Sequoiadb sdb = _idleConnPool.poll(item);
            _destroyConnQueue.add(sdb);
            item.setAddr("");
            _connItemMgr.releaseItem(item);
        }
    }

    private void _createConnections() {
        int createNum = _dsOpt.getDeltaIncCount() ;
        int count = 0;
        int avgCount = (_dsOpt.getMinIdleCount() + _dsOpt.getMaxIdleCount()) / 2;

        while (createNum > 0 && _idleConnPool.count() < avgCount) {
            // never let "sdb" defined out of current scope
            Sequoiadb sdb = null;
            ServerAddress serAddr = null;
            // get item for new connection
            ConnItem connItem = _connItemMgr.getItem();
            if (connItem == null) {
                // let's stop for no item for new connection
                break;
            }
            // create new connection
            while (true) {
                serAddr = null;
                serAddr = _strategy.selectAddress(addrMgr.getAddress());
                if (serAddr == null) {
                    // when have no address, we don't want to report any error message,
                    // let it done by main branch.
                    break;
                }
                // create connection
                try {
                    sdb = new Sequoiadb(serAddr.getAddress(), _username, _password, _normalNwOpt);
                    break;
                } catch (BaseException e) {
                    if (e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() &&
                            e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() &&
                            e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT.getErrorCode()) {
                        // let's stop for another error
                        break;
                    }
                    // remove this address from normal address list
                    _handleErrorAddr(serAddr.getAddress());
                } catch (Exception e) {
                    // let's stop for another error
                    break;
                }
            }
            // if we failed to create connection,
            // let's release the item and then stop
            if (sdb == null) {
                _connItemMgr.releaseItem(connItem);
                break;
            } else if (_sessionAttr != null) {
                try {
                    sdb.setSessionAttr(_sessionAttr);
                } catch (Exception e) {
                    _connItemMgr.releaseItem(connItem);
                    connItem = null;
                    _destroyConnQueue.add(sdb);
                    break;
                }
            }
            // when we create a connection, let's put it to idle pool
            connItem.setAddr(serAddr.getAddress());
            // add to idle pool
            _idleConnPool.insert(connItem, sdb);
            // update info to strategy
            _strategy.addConnItemAfterCreating(connItem);
            // let's continue
            createNum--;
            count++;
        }
        log.debug(String.format("Finish create connection task, increase idle connections: %d", count));
    }

    private boolean _connIsValid(ConnItem item, Sequoiadb sdb) {
        // check the send/receive buffer size of the connection is out of the cache limit or not
        if (_dsOpt.getCacheLimit() > 0 && sdb.getCurrentCacheSize() > _dsOpt.getCacheLimit()) {
            return false;
        }

        if (!addrMgr.checkAddress(sdb.getNodeName())) {
            return false;
        }

        // release the resource contains in connection
        try {
            sdb.releaseResource();
        } catch (Exception e) {
            try {
                sdb.disconnect();
            } catch (Exception ex) {
                // to nothing
            }
            return false;
        }

        // check timeout or not
        if (0 != _dsOpt.getKeepAliveTimeout()) {
            long lastTime = sdb.getLastUseTime();
            long currentTime = System.currentTimeMillis();
            if (currentTime - lastTime + _preDeleteInterval >= _dsOpt.getKeepAliveTimeout())
                return false;
        }
        // check version
        // for "_currentSequenceNumber" is pointed to the last
        // item which has been used, so, we need to use "<=".
        if (item.getSequenceNumber() <= _currentSequenceNumber) {
            return false;
        }
        return true;
    }

    private IConnectStrategy _createStrategy(ConnectStrategy strategy) {
        IConnectStrategy obj = null;
        switch (strategy) {
            case BALANCE:
                obj = new ConcreteBalanceStrategy();
                break;
            case SERIAL:
                obj = new ConcreteSerialStrategy();
                break;
            case RANDOM:
                obj = new ConcreteRandomStrategy();
                break;
            case LOCAL:
                obj = new ConcreteLocalStrategy();
                break;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG, "invalid connection strategy: " + strategy);
        }
        return obj;
    }

    private void _enableDatasource(ConnectStrategy strategy) {
        _preDeleteInterval = (int) (_dsOpt.getCheckInterval() * MULTIPLE);
        // initialize idle connection pool
        _idleConnPool = new IdleConnectionPool();
        // initialize used connection pool
        if (_usedConnPool == null) {
            // when we disable data source, we won't clean
            // the connections in used pool.
            _usedConnPool = new UsedConnectionPool();
            // initialize connection item manager without anything
            _connItemMgr = new ConnectionItemMgr(_dsOpt.getMaxCount(), null);
        } else {
            // update the version, so, all the outdated caching connections in used pool
            // can not go back to idle pool any more.
            // initialize connection item manager with used items
            List<ConnItem> list = new ArrayList<ConnItem>();
            Iterator<Pair> itr = _usedConnPool.getIterator();
            while (itr.hasNext()) {
                list.add(itr.next().first());
            }
            _connItemMgr = new ConnectionItemMgr(_dsOpt.getMaxCount(), list);
            _currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
        }
        // initialize strategy
        _strategy = _createStrategy(strategy);
        _strategy.init(null, null);
        // start timer
        _startTimer();
        // start back group thread
        _startThreads();
        _isDatasourceOn = true;
    }

    private int _reduceIdleConnections(int count) {
        ConnItem connItem = null;
        int reduceCount = 0;
        long lastTime = 0;
        long currentTime = System.currentTimeMillis();
        while (count-- > 0 && (connItem = _strategy.peekConnItemForDeleting()) != null) {
            Sequoiadb sdb = _idleConnPool.peek(connItem);
            lastTime = sdb.getLastUseTime();
            if (currentTime - lastTime >= _deleteInterval) {
                connItem = _strategy.pollConnItemForDeleting();
                sdb = _idleConnPool.poll(connItem);
                try {
                    _destroyConnQueue.add(sdb);
                } finally {
                    _connItemMgr.releaseItem(connItem);
                    reduceCount++;
                }
            } else {
                break;
            }
        }
        return reduceCount;
    }

    private void updateConnConf(Sequoiadb sdb, ConfigOptions config){
        // the difference of _normalNwOpt, _internalNwOpt and _userNwOpt:
        // 1. maxAutoConnectRetryTime, used to create connection, without updating
        // 2. connectTimeout, used to create connection, without updating
        // 3. socketTimeout, used for I/O socket read operations, need updating
        sdb.getConnProxy().setSoTimeout(config.getSocketTimeout());
    }

    private void updateLocationInfo() {
        Sequoiadb db = createTempConn();
        if (db == null) {
            log.warn("Failed to update location information for addresses");
            return;
        }
        try {
            BSONObject addInfoObj = queryAddressInfo(db);
            addrMgr.updateAddressInfo(addInfoObj, UpdateType.LOCATION);
        } finally {
            try {
                db.close();
            } catch (Exception e) {
                // ignore
            }
        }
    }

    private Sequoiadb createTempConn() {
        Iterator<ServerAddress> itr = addrMgr.getAddress().iterator();
        Sequoiadb sdb = null;
        while (itr.hasNext()) {
            ServerAddress addr = itr.next();
            try {
                sdb = new Sequoiadb(addr.getAddress(), _username, _password, _normalNwOpt);
                break;
            } catch (BaseException e) {
                // ignore
            }
        }
        return sdb;
    }

    private BSONObject queryAddressInfo(Sequoiadb db) {
        BSONObject condition = new BasicBSONObject();
        condition.put("GroupName", "SYSCoord");
        BSONObject select = new BasicBSONObject();
        select.put("Group.HostName", "");
        select.put("Group.Service", "");
        select.put("Group.Location", "");
        select.put("Version", "");

        DBCursor cursor = db.getList(Sequoiadb.SDB_LIST_GROUPS, condition, select, null);
        try {
            return cursor.getNext();
        } finally {
            try {
                cursor.close();
            } catch (Exception e) {
                // ignore
            }
        }
    }

    // -1 mean failed to get version from BSONObj
    private int parseVersionInfo(BSONObject obj) {
        int version = -1;
        Object verObj = obj.get("Version");
        if (verObj == null ) {
            return version;
        }

        // if the 'Version' field not exist in old version, the verObj maybe an empty string
        if (verObj instanceof String) {
            try {
                version = Integer.parseInt((String) verObj);
            } catch (Exception e) {
                // ignore
            }
        } else if (verObj instanceof Number) {
            version = (Integer) verObj;
        }
        return version;
    }

    /**
     * The builder of SequoiadbDatasource.
     *
     * </p>
     * Usage example:
     * <pre>
     * List<String> addressList = new ArrayList();
     * addressList.add( "sdbserver1:11810" );
     * addressList.add( "sdbserver2:11810" );
     * addressList.add( "sdbserver3:11810" );
     *
     * SequoiadbDatasource ds = SequoiadbDatasource.builder()
     *         .serverAddress( addressList )
     *         .userConfig( new UserConfig( "admin", "admin" ) )
     *         .build();
     * </pre>
     */
    public static final class Builder {
        private List<String> addressList = null;
        private String location = "";
        private UserConfig userConfig = null;
        private ConfigOptions configOptions = null;
        private DatasourceOptions datasourceOptions = null;

        private Builder() {
        }

        /**
         * Set an address of SequoiaDB node, format: "Host:Port", eg: "sdbserver:11810"
         *
         * @param address The address of SequoiaDB node
         */
        public Builder serverAddress( String address ) {
            if ( address == null || address.isEmpty() ){
                throw new BaseException( SDBError.SDB_INVALIDARG, "The server address is null or empty" );
            }
            this.addressList = new ArrayList<>();
            this.addressList.add( address );
            return this;
        }

        /**
         * Set an address list of SequoiaDB node.
         *
         * @param addressList The address list of SequoiaDB node.
         */
        public Builder serverAddress( List<String> addressList ) {
            if ( addressList == null || addressList.isEmpty() ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The server address list is null or empty" );
            }
            this.addressList = addressList;
            return this;
        }

        /**
         * Set the location name of connection pool, the format is consistent with SequoiaDB node, eg: guangdong.guangzhou.
         * When the connection pool creates a new connection, it will use the address of the same location first, then use
         * the address of location affinity, and finally use the remaining address.
         *
         * @param location The location name, case sensitive. Default is "".
         */
        public Builder location(String location) {
            if (location == null) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The location name is null" );
            }
            this.location = location;
            return this;
        }

        /**
         * Set the user config.
         *
         * @param userConfig The user config.
         */
        public Builder userConfig( UserConfig userConfig ) {
            if ( userConfig == null ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The user config is null" );
            }
            this.userConfig = userConfig;
            return this;
        }

        /**
         * Set the options for connection.
         *
         * @param option The options for connection
         */
        public Builder configOptions( ConfigOptions option ) {
            if ( option == null ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The connection options is null" );
            }
            this.configOptions = option;
            return this;
        }

        /**
         * Set the options for connection pool.
         *
         * @param option The options for connection pool
         */
        public Builder datasourceOptions( DatasourceOptions option ) {
            if ( option == null ) {
                throw new BaseException( SDBError.SDB_INVALIDARG, "The connection pool options is null" );
            }
            this.datasourceOptions = option;
            return this;
        }

        /**
         * Create a SequoiadbDatasource instance.
         *
         * @return The SequoiadbDatasource instance
         */
        public SequoiadbDatasource build() {
            if ( userConfig == null ) {
                userConfig = new UserConfig();
            }
            if ( configOptions == null ) {
                configOptions = new ConfigOptions();
            }
            if ( datasourceOptions == null ) {
                datasourceOptions = new DatasourceOptions();
            }
            return new SequoiadbDatasource( this );
        }
    }

    private String getConnItemSnapshot() {
        ConnItemInfo itemInfo = _connItemMgr.getConnItemInfo();
        return String.format("total item: %d, idle item: %d, used item: %d", itemInfo.capacity,
                itemInfo.idleItemSize, itemInfo.usedItemSize);
    }

    private String getConnPoolSnapshot() {
        return String.format("use connections: %d, idle connections: %d",
                _usedConnPool != null ? _usedConnPool.count() : null,
                _idleConnPool != null ? _idleConnPool.count() : null);
    }

    private void resetConnTime(Timer timer, ConfigOptions netOpt) {
        if (!timer.getStatus()) {
            return;
        }

        long time = Math.max(timer.getTime(), minTimeOut);

        long retryTimeOut = Math.min(time, netOpt.getMaxAutoConnectRetryTime());
        long connTimeout = netOpt.getConnectTimeout();

        // connTimeout == 0 means socket.connect() infinite timeout.
        if (connTimeout == 0 || connTimeout > time) {
            connTimeout = time;
        }

        netOpt.setConnectTimeout((int) connTimeout);
        netOpt.setMaxAutoConnectRetryTime(retryTimeOut);
    }
}

