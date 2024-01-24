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

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Options of data source
 *
 * @since 2.2
 */
public class DatasourceOptions implements Cloneable {
    private static final List<String> MODE = Arrays.asList("M", "m", "S", "s", "A", "a");
    private static final String DEFAULT_PREFERRED_INSTANCE_MODE = DatasourceConstants.PREFERRED_INSTANCE_MODE_RANDOM;
    private static final int DEFAULT_SESSION_TIMEOUT = -1;
    private int _deltaIncCount = 10;
    private int _maxIdleCount = 10;
    private int _minIdleCount = 0;
    private int _maxCount = 500;
    private int _keepAliveTimeout = 0 * 60 * 1000; // 0 min
    private int _checkInterval = 1 * 60 * 1000; // 1 min
    private int _syncCoordInterval = 0; // 0 min
    private int syncLocationInterval = 60 * 1000; // 60s
    private boolean _validateConnection = false;
    private ConnectStrategy _connectStrategy = ConnectStrategy.SERIAL;
    private List<Object> _preferredInstance = null;
    private String _preferredInstanceMode = DEFAULT_PREFERRED_INSTANCE_MODE; // "random" or "ordered"
    private int _sessionTimeout = DEFAULT_SESSION_TIMEOUT;
    private int _networkBlockTimeout = 6000; //network block timeout, default 6s
    private int _cacheLimit = 131072; // 128k

    /**
     * Clone the current options.
     *
     * @since 2.2
     */
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

    /**
     * Set the number of new connections to create once running out the
     * connection pool.
     *
     * @param deltaIncCount DeltaIncCount should be more than 0, default to be 10.
     * @throws BaseException If error happens.
     */
    public void setDeltaIncCount(int deltaIncCount) {
        if (deltaIncCount <= 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "deltaIncCount should be more than 0");
        }
        _deltaIncCount = deltaIncCount;
    }

    /**
     * Set the maximum number of idle connections. When the number of idle connections in the
     *         pool is more than 'maxIdleCount', the pool will destroy some connections.
     * @param maxIdleCount MaxIdleCount can't be less than 0, default to be 10.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void setMaxIdleCount(int maxIdleCount) {
        if (maxIdleCount < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "maxIdleCount can't be less than 0");
        }
        _maxIdleCount = maxIdleCount;
    }

    /**
     *  Set the minimum number of idle connections. When the number of idle connections in the
     *         pool is less than the average of 'minIdleCount' and 'maxIdleCount', the pool will
     *         create some connections.
     * @param minIdleCount MinIdleCount can't be less than 0, default to be 0.
     * @throws BaseException If error happens.
     * @since v2.8.10
     */
    public void setMinIdleCount(int minIdleCount) {
        if (minIdleCount < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "minIdleCount can't be less than 0");
        }
        _minIdleCount = minIdleCount;
    }

    /**
     * Set the capacity of the connection pool.
     * When maxCount is set to 0, the connection pool will be disabled.
     *
     * @param maxCount MaxCount should be can't be less than 0, default to be 500.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void setMaxCount(int maxCount) {
        if (maxCount < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "maxCount can't be less than 0");
        }
        _maxCount = maxCount;
    }

    /**
     * Set the time in milliseconds for abandoning a connection which keep alive time is up.
     * If a connection has not be used(send and receive) for a long time(longer
     * than "keepAliveTimeout"), the pool will not let it come back.
     * The pool will also clean this kind of idle connections in the pool periodically.
     * When "keepAliveTimeout" is not set to 0, it's better to set it
     * greater than "checkInterval" triple over. Besides, unless you know what you need,
     * never enable this option.
     *
     * @param keepAliveTimeout KeepAliveTimeout can't be less than 0, default to be 0ms, means not care
     *                         about how long does a connection have not be used(send and receive).
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void setKeepAliveTimeout(int keepAliveTimeout) {
        if (keepAliveTimeout < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "keepAliveTimeout can't be less than 0");
        }
        _keepAliveTimeout = keepAliveTimeout;
    }

    /**
     * Set the checking interval in milliseconds. Every interval,
     * the pool cleans all the idle connection which keep alive time is up,
     * and keeps the number of idle connection not more than "maxIdleCount".
     * When "keepAliveTimeout" is not be 0, "checkInterval" should be less than it.
     * It's better to set "keepAliveTimeout" greater than "checkInterval" triple over.
     *
     * @param checkInterval CheckInterval should be more than 0, default to be 1 * 60 * 1000ms.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void setCheckInterval(int checkInterval) {
        if (checkInterval <= 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "checkInterval should be more than 0");
        }
        _checkInterval = checkInterval;
    }

    /**
     * Set the interval for updating coord's addresses from catalog in milliseconds.
     * The updated coord addresses will cover the addresses in the pool.
     * When "syncCoordInterval" is 0, the pool will stop updating coord's addresses from
     * catalog. when "syncCoordInterval" is less than 60,000 milliseconds,
     * use 60,000 milliseconds instead.
     *
     * @param syncCoordInterval SyncCoordInterval can't be less than 0, default to be 0ms.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void setSyncCoordInterval(int syncCoordInterval) {
        if (syncCoordInterval < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "syncCoordInterval can't be less than 0");
        }

        if (syncCoordInterval > 0 && syncCoordInterval < 60000) {
            _syncCoordInterval = 60000;
        } else {
            _syncCoordInterval = syncCoordInterval;
        }
    }

    /**
     * Set the interval for updating coord's location information in milliseconds. It takes effect only after
     * location is set for the connection pool.
     *
     * @param syncLocationInterval The interval for updating coord's location information, can't be less than 0,
     *                             default to be 60000ms
     * @throws BaseException If error happens.
     */
    public void setSyncLocationInterval(int syncLocationInterval) {
        if (syncLocationInterval < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "syncLocationInterval can't be less than 0");
        }
        this.syncLocationInterval = syncLocationInterval;
    }

    /**
     * When a idle connection is got out of pool, we need
     * to validate whether it can be used or not.
     *
     * @param validateConnection Default to be false.
     * @since 2.2
     */
    public void setValidateConnection(boolean validateConnection) {
        _validateConnection = validateConnection;
    }

    /**
     * Set connection strategy.
     * When choosing ConnectStrategy.LOCAL, if there have no local coord address,
     * use other address instead.
     *
     * @param strategy Should one of the follow:
     *                 ConnectStrategy.SERIAL,
     *                 ConnectStrategy.RANDOM,
     *                 ConnectStrategy.LOCAL,
     *                 ConnectStrategy.BALANCE
     * @since 2.2
     */
    public void setConnectStrategy(ConnectStrategy strategy) {
        if (strategy == ConnectStrategy.BALANCE) {
            _connectStrategy = ConnectStrategy.SERIAL;
        } else {
            _connectStrategy = strategy;
        }
    }

    /**
     * Set preferred instance for read request in the session.
     * When user does not set any preferred instance, use the setting in the coord's setting file.
     * Note: When specifying preferred instance, Datasource will set the session attribute only
     * when it creating a connection. That means when user get a connection out from the Datasource,
     * if user reset the session attribute of the connection, Datasource will keep the latest changes
     * of the setting.
     *
     * @param preferredInstance Could be single value in "M", "m", "S", "s", "A", "a", "1"-"255", or
     *                          multiple values of them.
     *                         <ul>
     *                         <li>"M", "m": read and write instance( master instance ). If multiple numeric
     *                          instances are given with "M", matched master instance will be chosen in higher
     *                          priority. If multiple numeric instances are given with "M" or "m", master
     *                          instance will be chosen if no numeric instance is matched.</li>
     *                         <li>"S", "s": read only instance( slave instance ). If multiple numeric instances
     *                          are given with "S", matched slave instances will be chosen in higher priority.
     *                          If multiple numeric instances are given with "S" or "s", slave instance will
     *                          be chosen if no numeric instance is matched.</li>
     *                         <li>"A", "a": any instance.</li>
     *                         <li>"1"-"255": the instance with specified instance ID.</li>
     *                         <li>If multiple alphabet instances are given, only first one will be used.</li>
     *                         <li>If matched instance is not found, will choose instance by random.</li>
     *                         </ul>
     * @throws BaseException If error happens.
     */
    public void setPreferredInstance(final List<String> preferredInstance) {
        if (preferredInstance == null || preferredInstance.size() == 0) {
            return;
        }
        List<String> list = new ArrayList<String>();

        for (String s : preferredInstance) {
            if (isValidMode(s)) {
                if (!list.contains(s)) {
                    list.add(s);
                }
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid preferred instance: " + s);
            }
        }
        if (list.size() == 0) {
            return;
        }
        _preferredInstance = new ArrayList<Object>();
        for (String s : list) {
            try {
                _preferredInstance.add(Integer.valueOf(s));
            } catch (NumberFormatException e) {
                _preferredInstance.add(s);
            }
        }
    }

    /**
     * Set the mode to choose query instance when multiple preferred instances are found in the session.
     *
     * @param mode can be one of the follow, default to be "random".
     *             <ul>
     *             <li>"random": choose the instance from matched instances by random.</li>
     *             <li>"ordered": choose the instance from matched instances by the order of
     *             "PreferedInstance".</li>
     *             </ul>
     * @throws BaseException If error happens.
     */
    public void setPreferredInstanceMode(String mode) {
        if (mode == null || mode.isEmpty()) {
            _preferredInstanceMode = DEFAULT_PREFERRED_INSTANCE_MODE;
            return;
        }

        if (!DatasourceConstants.PREFERRED_INSTANCE_MODE_ORDERED.equals(mode) &&
                !DatasourceConstants.PREFERRED_INSTANCE_MODE_RANDOM.equals(mode)) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    String.format("The preferred instance mode should be '%s' or '%s', but it is %s",
                            DatasourceConstants.PREFERRED_INSTANCE_MODE_ORDERED,
                            DatasourceConstants.PREFERRED_INSTANCE_MODE_RANDOM,
                            mode));
        }
        _preferredInstanceMode = mode;
    }

    /**
     * Set the timeout (in ms) for operations in the session. -1 means no timeout for operations. Default te be -1.
     *
     * @param timeout The timeout (in ms) for operations in the session.
     */
    public void setSessionTimeout(int timeout) {
        if (timeout < 0) {
            _sessionTimeout = DEFAULT_SESSION_TIMEOUT;
        } else {
            _sessionTimeout = timeout;
        }
    }

    /**
     * Set the cache size limit of the session. 0 means not set the limit for the session cache size.
     *         Default to be 131072 bytes(128 KB). When the cache size of the session reaches the limit, the
     *         session will be destroyed after the connection release to pool.
     * @param limitBytes The cache size limit of the session in bytes.
     */
    public void setCacheLimit(int limitBytes) {
        if (limitBytes < 0) {
            _cacheLimit = 0;
        } else {
            _cacheLimit = limitBytes;
        }
    }

    /**
     * Get the number of connections to create once running out the
     * connection pool.
     *
     * @return the number of connections created each time
     */
    public int getDeltaIncCount() {
        return _deltaIncCount;
    }

    /**
     * Get the maximum number of idle connections.
     * @return The maximum number of idle connections.
     */
    public int getMaxIdleCount() {
        return _maxIdleCount;
    }

    /**
     * Get the minimum number of idle connections.
     * @return The minimum number of idle connections.
     */
    public int getMinIdleCount() {
        return _minIdleCount;
    }

    /**
     * Get the capacity of the pool.
     *
     * @return The capacity of the pool.
     */
    public int getMaxCount() {
        return _maxCount;
    }

    /**
     * Get the setup time for abandoning a connection
     * which has not been used for long time.
     *
     * @return the keep alive timeout time
     * @since 2.2
     */
    public int getKeepAliveTimeout() {
        return _keepAliveTimeout;
    }

    /**
     * Get the interval for checking the idle connections periodically.
     *
     * @return the interval
     * @since 2.2
     */
    public int getCheckInterval() {
        return _checkInterval;
    }

    /**
     * Get the interval for updating coord's addresses from catalog periodically.
     *
     * @return the interval
     * @since 2.2
     */
    public int getSyncCoordInterval() {
        return _syncCoordInterval;
    }

    /**
     * Get the interval for updating coord's location information in milliseconds.
     * @return The interval
     */
    public int getSyncLocationInterval() {
        return syncLocationInterval;
    }

    /**
     * Get whether to validate a connection which is got from the pool or not.
     *
     * @return true or false
     * @since 2.2
     */
    public boolean getValidateConnection() {
        return _validateConnection;
    }

    /**
     * Get the current strategy of creating connections.
     *
     * @return the strategy
     * @since 2.2
     */
    public ConnectStrategy getConnectStrategy() {
        return _connectStrategy;
    }

    /**
     * Get the preferred instance.
     *
     * @return The preferred instance or null for no any setting.
     */
    public List<String> getPreferredInstance() {
        if (_preferredInstance == null) {
            return null;
        }
        List<String> list = new ArrayList<String>();
        for (Object o : _preferredInstance) {
            if (o instanceof String) {
                list.add((String) o);
            } else if (o instanceof Integer) {
                list.add(o + "");
            }
        }
        return list;
    }


    List<Object> getPreferredInstanceObjects() {
        return _preferredInstance;
    }

    /**
     * Get the preferred instance node.
     *
     * @return The preferred instance node.
     */
    public String getPreferredInstanceMode() {
        return _preferredInstanceMode;
    }


    /**
     * The Session timeout value.
     *
     * @return Session timeout value.
     */
    public int getSessionTimeout() {
        return _sessionTimeout;
    }

    /**
     * Get the cache size limit of the session.
     * @return The cache size limit of the session.
     */
    public int getCacheLimit() {
        return _cacheLimit;
    }

    /**
     * Set the network block timeout, which is used to set the send and receive timeout of the connections
     * inside the connection pool. After a connection get out from the connection pool, its send and receive
     * timeout will be restored to the original state.
     *
     * @param networkBlockTimeout NetworkBlockTimeout can't be less than 0, default to be 6000ms,
     *                            0ms and means no timeout.
     * @throws BaseException If error happens.
     */
    public void setNetworkBlockTimeout(int networkBlockTimeout) {
        if (networkBlockTimeout < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "networkBlockTimeout can't be less than 0");
        }
        this._networkBlockTimeout = networkBlockTimeout;
    }

    /**
     * Get the network block timeout
     *
     * @return The network block timeout
     */
    public int getNetworkBlockTimeout() {
        return _networkBlockTimeout;
    }

    /// the follow APIs are deprecated

    /**
     * Set the initial number of connection.
     * When the connection pool is enabled, the first time to get connection,
     * the pool increases "deltaIncCount" number of connections. Used
     * setDeltaIncCount() instead.
     *
     * @param initConnectionNum default to be 10
     * @see #setDeltaIncCount(int)
     * @deprecated Does not work since 2.2.
     */
    public void setInitConnectionNum(int initConnectionNum) {
    }

    /**
     * Set the max number of the idle connection left in connection
     * pool after periodically cleaning.
     *
     * @param maxIdeNum default to be 10
     * @see #setMaxIdleCount(int)
     * @deprecated Used setMaxIdleCount() instead.
     */
    public void setMaxIdeNum(int maxIdeNum) {
        setMaxIdleCount(maxIdeNum);
    }

    /**
     * Set the max number of connection for use. When maxConnectionNum is 0,
     * the connection pool doesn't really work. In this situation, when request comes,
     * it builds a connection and return it directly. When a connection goes back to pool,
     * it disconnects the connection directly and will not put the connection back to pool.
     *
     * @param maxConnectionNum default to be 500
     * @see #setMaxCount(int)
     * @deprecated Use setMaxCount() instead.
     */
    public void setMaxConnectionNum(int maxConnectionNum) {
        setMaxCount(maxConnectionNum);
    }

    /**
     * Set the wait time in milliseconds. If the number of connection reaches
     * maxConnectionNum, the pool can't offer connection immediately, the
     * requests will be blocked to wait for a moment. When timeout, and there is
     * still no available connection, connection pool throws exception
     *
     * @param timeout Default to be 5 * 1000ms.
     * @see SequoiadbDatasource#getConnection(long)
     * @since 2.2
     * @deprecated Use SequoiadbDatasource.getConnection(long timeout) instead.
     */
    public void setTimeout(int timeout) {
    }

    /**
     * Set the recheck cycle in milliseconds. In each cycle
     * connection pool cleans all the discardable connection,
     * and keep the number of valid connection not more than maxIdeNum.
     * It's better to set abandonTime greater than recheckCyclePeriod twice over.
     *
     * @param recheckCyclePeriod recheckCyclePeriod should be less than abandonTime. Default to be 1 * 60 * 1000ms
     * @see #setCheckInterval(int)
     * @deprecated Use setCheckInterval() instead.
     */
    public void setRecheckCyclePeriod(int recheckCyclePeriod) {
        setCheckInterval(recheckCyclePeriod);
    }

    /**
     * Set the time in milliseconds for getting back the useful address.
     * When offer several addresses for connection pool to use, if
     * some of them are not available(invalid address, network error, coord shutdown,
     * catalog replica group is not available), we will put these addresses
     * into a queue, and check them periodically. If some of them is valid again,
     * get them back for use.
     * The pool will test the invalid address automatically every 30 seconds.
     *
     * @param recaptureConnPeriod default to be 30 * 1000ms
     * @deprecated
     */
    public void setRecaptureConnPeriod(int recaptureConnPeriod) {
    }

    /**
     * Set the time in milliseconds for abandoning discardable connection.
     * If a connection has not be used for a long time(longer than abandonTime),
     * connection pool would not let it come back to pool. And it will clean this
     * kind of connections in the pool periodically.
     * It's better to set abandonTime greater than recheckCyclePeriod twice over.
     *
     * @param abandonTime default to be 10 * 60 * 1000ms
     * @see #setKeepAliveTimeout(int)
     * @deprecated Use setKeepAliveTimeout() instead.
     */
    public void setAbandonTime(int abandonTime) {
        setKeepAliveTimeout(abandonTime);
    }

    /**
     * Get the setup number of initial connection.
     *
     * @deprecated Always return 0.
     */
    public int getInitConnectionNum() {
        return 0;
    }

    /**
     * Get the max number of connection.
     *
     * @see #getMaxCount()
     * @deprecated Use getMaxCount() instead.
     */
    public int getMaxConnectionNum() {
        return getMaxCount();
    }

    /**
     * Get the max number of the idle connection.
     *
     * @see #getMaxIdleCount()
     * @deprecated Use getMaxIdleCount() instead.
     */
    public int getMaxIdeNum() {
        return getMaxIdleCount();
    }

    /**
     * Get the setup time for abandoning a connection
     * which is not used for long time.
     *
     * @see #getKeepAliveTimeout()
     * @deprecated Use getKeepAliveTimeout() instead.
     */
    public int getAbandonTime() {
        return getKeepAliveTimeout();
    }

    /**
     * Get the cycle for checking.
     *
     * @see #getCheckInterval()
     * @deprecated Use getCheckInterval() instead.
     */
    public int getRecheckCyclePeriod() {
        return getCheckInterval();
    }

    /**
     * Get the period for getting back useful addresses.
     *
     * @deprecated Always return 0.
     */
    public int getRecaptureConnPeriod() {
        return 0;
    }

    /**
     * Get the wait time.
     *
     * @deprecated Always return 0.
     */
    public int getTimeout() {
        return 0;
    }

    /**
     * Set preferred instance for read request in the session.
     * When user does not set any preferred instance, use the setting in the coord's setting file.
     * Note: When specifying preferred instance, Datasource will set the session attribute only
     * when it creating a connection. That means when user get a connection out from the Datasource,
     * if user reset the session attribute of the connection, Datasource will keep the latest changes
     * of the setting.
     *
     * @param preferredInstance Could be single value in "M", "m", "S", "s", "A", "a", "1"-"255", or
     *                          multiple values of them.
     *                         <ul>
     *                         <li>"M", "m": read and write instance( master instance ). If multiple numeric
     *                          instances are given with "M", matched master instance will be chosen in higher
     *                          priority. If multiple numeric instances are given with "M" or "m", master
     *                          instance will be chosen if no numeric instance is matched.</li>
     *                         <li>"S", "s": read only instance( slave instance ). If multiple numeric instances
     *                          are given with "S", matched slave instances will be chosen in higher priority.
     *                          If multiple numeric instances are given with "S" or "s", slave instance will
     *                          be chosen if no numeric instance is matched.</li>
     *                         <li>"A", "a": any instance.</li>
     *                         <li>"1"-"255": the instance with specified instance ID.</li>
     *                         <li>If multiple alphabet instances are given, only first one will be used.</li>
     *                         <li>If matched instance is not found, will choose instance by random.</li>
     *                         </ul>
     * @deprecated Use {@link DatasourceOptions#setPreferredInstance(List)} instead.
     */
    @Deprecated
    public void setPreferedInstance(final List<String> preferredInstance) {
        setPreferredInstance( preferredInstance );
    }

    /**
     * Set the mode to choose query instance when multiple preferred instances are found in the session.
     *
     * @param mode can be one of the follow, default to be "random".
     *             <ul>
     *             <li>"random": choose the instance from matched instances by random.</li>
     *             <li>"ordered": choose the instance from matched instances by the order of
     *             "PreferedInstance".</li>
     *             </ul>
     * @deprecated Use {@link DatasourceOptions#setPreferredInstanceMode(String)} instead.
     */
    @Deprecated
    public void setPreferedInstanceMode(String mode) {
        setPreferredInstanceMode( mode );
    }

    /**
     * Get the preferred instance.
     *
     * @return The preferred instance or null for no any setting.
     * @deprecated Use {@link DatasourceOptions#getPreferredInstance()} instead.
     */
    @Deprecated
    public List<String> getPreferedInstance() {
        return getPreferredInstance();
    }

    /**
     * Get the preferred instance node.
     *
     * @return The preferred instance node.
     * @deprecated Use {@link DatasourceOptions#getPreferredInstanceMode()} instead.
     */
    @Deprecated
    public String getPreferedInstanceMode() {
        return getPreferredInstanceMode();
    }

    BSONObject getSessionAttr() {
        BSONObject obj = new BasicBSONObject();
        if (_preferredInstance != null && _preferredInstance.size() > 0) {
            // preferred instance
            BSONObject list = new BasicBSONList();
            int i = 0;
            for (Object o : _preferredInstance) {
                list.put("" + i++, o);
            }
            // XXX_LEGACY is used for compatibility with older versions, they can only
            // be replaced when everyone is on 3.6 and above.
            obj.put(DatasourceConstants.FIELD_NAME_PREFERRED_INSTANCE_LEGACY, list);
            obj.put(DatasourceConstants.FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY, _preferredInstanceMode);
            // timeout
            obj.put(DatasourceConstants.FIELD_NAME_SESSION_TIMEOUT, _sessionTimeout);
        }
        return obj;
    }

    private boolean isValidMode(String s) {
        if (MODE.contains(s)) {
            return true;
        } else {
            int n = 0;
            try {
                n = Integer.parseInt(s);
            } catch (NumberFormatException e) {
                return false;
            }
            if (n >= 1 && n <= 255) {
                return true;
            } else {
                return false;
            }
        }
    }

    @Override
    public String toString() {
        return "DatasourceOptions: { " +
                "maxCount: " + _maxCount +
                ", maxIdleCount: " + _maxIdleCount +
                ", minIdleCount: " + _minIdleCount +
                ", deltaIncCount: " + _deltaIncCount +
                ", keepAliveTimeout: " + _keepAliveTimeout +
                ", sessionTimeout: " + _sessionTimeout +
                ", networkBlockTimeout: " + _networkBlockTimeout +
                ", cacheLimit: " + _cacheLimit +
                ", checkInterval: " + _checkInterval +
                ", syncCoordInterval: " + _syncCoordInterval +
                ", syncLocationInterval: " + syncLocationInterval +
                ", validateConnection: " + _validateConnection +
                ", connectStrategy: " + _connectStrategy +
                ", preferredInstance: " + _preferredInstance +
                ", preferredInstanceMode: " + _preferredInstanceMode
                + " }";
    }
}
