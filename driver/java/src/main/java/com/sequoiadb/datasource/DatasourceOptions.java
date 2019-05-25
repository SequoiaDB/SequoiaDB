/*
 * Copyright 2017 SequoiaDB Inc.
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
 * @since 2.2
 */
public class DatasourceOptions implements Cloneable {
    private static final List<String> MODE = Arrays.asList("M", "m", "S", "s", "A", "a");
    private static final String DEFAULT_PREFERRD_INSTANCE_MODE = DatasourceConstants.PREFERED_INSTANCE_MODE_RANDON;
    private static final int DEFAULT_SESSION_TIMEOUT = -1;
    private int _deltaIncCount = 10;
    private int _maxIdleCount = 10;
    private int _maxCount = 500;
    private int _keepAliveTimeout = 0 * 60 * 1000; // 0 min
    private int _checkInterval = 1 * 60 * 1000; // 1 min
    private int _syncCoordInterval = 0; // 0 min
    private boolean _validateConnection = false;
    private ConnectStrategy _connectStrategy = ConnectStrategy.BALANCE;
    private List<Object> _preferedInstance = null;
    private String _preferedInstanceMode = DEFAULT_PREFERRD_INSTANCE_MODE; // "random" or "ordered"
    private int _sessionTimeout = DEFAULT_SESSION_TIMEOUT;

    /**
     * Clone the current options.
     * @since 2.2
     */
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

    /**
     * Set the number of new connections to create once running out the
     * connection pool.
     * @param deltaIncCount Default to be 10.
     */
    public void setDeltaIncCount(int deltaIncCount) {
        _deltaIncCount = deltaIncCount;
    }

    /**
     * Set the max number of the idle connection left in connection
     * pool after periodically cleaning.
     * @param maxIdleCount Default to be 10.
     * @since 2.2
     */
    public void setMaxIdleCount(int maxIdleCount) {
        _maxIdleCount = maxIdleCount;
    }

    /**
     * Set the capacity of the connection pool.
     * When maxCount is set to 0, the connection pool will be disabled.
     * @param maxCount Default to be 500.
     * @since 2.2
     */
    public void setMaxCount(int maxCount) {
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
     * @param keepAliveTimeout Default to be 0ms, means not care about how long does a connection
     *                         have not be used(send and receive).
     * @since 2.2
     */
    public void setKeepAliveTimeout(int keepAliveTimeout) {
        _keepAliveTimeout = keepAliveTimeout;
    }

    /**
     * Set the checking interval in milliseconds. Every interval,
     * the pool cleans all the idle connection which keep alive time is up,
     * and keeps the number of idle connection not more than "maxIdleCount".
     * When "keepAliveTimeout" is not be 0, "checkInterval" should be less than it.
     * It's better to set "keepAliveTimeout" greater than "checkInterval" triple over.
     * @param checkInterval Default to be 1 * 60 * 1000ms.
     * @since 2.2
     */
    public void setCheckInterval(int checkInterval) {
        _checkInterval = checkInterval;
    }

    /**
     * Set the interval for updating coord's addresses from catalog in milliseconds.
     * The updated coord addresses will cover the addresses in the pool.
     * When "syncCoordInterval" is 0, the pool will stop updating coord's addresses from
     * catalog.
     * @param syncCoordInterval Default to be 1 * 60 * 1000ms.
     * @since 2.2
     */
    public void setSyncCoordInterval(int syncCoordInterval) {
        _syncCoordInterval = syncCoordInterval;
    }

    /**
     * When a idle connection is got out of pool, we need
     * to validate whether it can be used or not.
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
     * @param strategy Should one of the follow:
     *                 ConnectStrategy.SERIAL,
     *                 ConnectStrategy.RANDOM,
     *                 ConnectStrategy.LOCAL,
     *                 ConnectStrategy.BALANCE
     * @since 2.2
     */
    public void setConnectStrategy(ConnectStrategy strategy) {
        _connectStrategy = strategy;
    }

    /**
     * Set Preferred instance for read request in the session..
     * When user does not set any preferred instance,
     * use the setting in the coord's setting file.
     * Note: When specifying preferred instance, Datasource will set the session attribute only
     * when it creating a connection. That means when user get a connection out from the Datasource,
     * if user reset the session attribute of the connection, Datasource will keep the latest changes of the setting.
     *
     * @param preferedInstance Could be single value in "M", "m", "S", "s", "A", "a", "1"-"255", or multiple values of them.
     *          <ul>
     *              <li>"M", "m": read and write instance( master instance ). If multiple numeric instances are given with "M", matched master instance will be chosen in higher priority. If multiple numeric instances are given with "M" or "m", master instance will be chosen if no numeric instance is matched.</li>
     *              <li>"S", "s": read only instance( slave instance ). If multiple numeric instances are given with "S", matched slave instances will be chosen in higher priority. If multiple numeric instances are given with "S" or "s", slave instance will be chosen if no numeric instance is matched.</li>
     *              <li>"A", "a": any instance.</li>
     *              <li>"1"-"255": the instance with specified instance ID.</li>
     *              <li>If multiple alphabet instances are given, only first one will be used.</li>
     *              <li>If matched instance is not found, will choose instance by random.</li>
     *          </ul>
     */
    public void setPreferedInstance(final List<String> preferedInstance) {
        if (preferedInstance == null || preferedInstance.size() == 0) {
            return;
        }
        List<String> list = new ArrayList<String>();

        for(String s : preferedInstance) {
            if (isValidMode(s)) {
                if (!list.contains(s)) {
                    list.add(s);
                }
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, "invalid preferred instance: " + s);
            }
        }
        if (list.size() == 0) {
            return;
        }
        _preferedInstance = new ArrayList<Object>();
        for(String s : list) {
            try {
                _preferedInstance.add(Integer.valueOf(s));
            } catch(NumberFormatException e) {
                _preferedInstance.add(s);
            }
        }
    }

    /**
     * Set the mode to choose query instance when multiple preferred instances are found in the session.
     *
     * @param mode can be one of the follow, default to be "random".
     *                    <ul>
     *                        <li>"random": choose the instance from matched instances by random.</li>
     *                        <li>"ordered": choose the instance from matched instances by the order of "PreferedInstance".</li>
     *                    </ul>
     */
    public void setPreferedInstanceMode(String mode) {
        if (mode == null || mode.isEmpty()) {
            _preferedInstanceMode = DEFAULT_PREFERRD_INSTANCE_MODE;
        } else {
            _preferedInstanceMode = mode;
        }
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
     * Get the number of connections to create once running out the
     * connection pool.
     * @return the number of connections created each time
     */
    public int getDeltaIncCount() {
        return _deltaIncCount;
    }

    /**
     * Get the max number of idle connection.
     * @return The max number of idle connection after checking.
     */
    public int getMaxIdleCount() {
        return _maxIdleCount;
    }

    /**
     * Get the capacity of the pool.
     * @return The capacity of the pool.
     */
    public int getMaxCount() {
        return _maxCount;
    }

    /**
     * Get the setup time for abandoning a connection
     * which has not been used for long time.
     * @return the keep alive timeout time
     * @since 2.2
     */
    public int getKeepAliveTimeout() {
        return _keepAliveTimeout;
    }

    /**
     * Get the interval for checking the idle connections periodically.
     * @return the interval
     * @since 2.2
     */
    public int getCheckInterval() {
        return _checkInterval;
    }

    /**
     * Get the interval for updating coord's addresses from catalog periodically.
     * @return the interval
     * @since 2.2
     */
    public int getSyncCoordInterval() {
        return _syncCoordInterval;
    }

    /**
     * Get whether to validate a connection which is got from the pool or not.
     * @return true or false
     * @since 2.2
     */
    public boolean getValidateConnection() {
        return _validateConnection;
    }

    /**
     * Get the current strategy of creating connections.
     * @return the strategy
     * @since 2.2
     */
    public ConnectStrategy getConnectStrategy() {
        return _connectStrategy;
    }

    /**
     * Get the preferred instance.
     * @return The preferred instance or null for no any setting.
     */
    public List<String> getPreferedInstance() {
        if (_preferedInstance == null) {
            return null;
        }
        List<String> list = new ArrayList<String>();
        for(Object o : _preferedInstance) {
            if (o instanceof String) {
                list.add((String)o);
            } else if(o instanceof Integer) {
                list.add(o + "");
            }
        }
        return list;
    }

    List<Object> getPreferedInstanceObjects() {
        return _preferedInstance;
    }

    /**
     * Get the preferred instance node.
     *
     * @return The preferred instance node.
     */
    public String getPreferedInstanceMode() {
        return _preferedInstanceMode;
    }

    /**
     * The Session timeout value.
     * @return Session timeout value.
     */
    public int getSessionTimeout() {
        return _sessionTimeout;
    }

    /**
     * Set the initial number of connection.
     * When the connection pool is enabled, the first time to get connection,
     * the pool increases "deltaIncCount" number of connections. Used
     * setDeltaIncCount() instead.
     * @param initConnectionNum default to be 10
     * @see #setDeltaIncCount(int)
     * @deprecated Does not work since 2.2.
     *
     */
    public void setInitConnectionNum(int initConnectionNum) {
    }

    /**
     * Set the max number of the idle connection left in connection
     * pool after periodically cleaning.
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
     * @param abandonTime default to be 10 * 60 * 1000ms
     * @see #setKeepAliveTimeout(int)
     * @deprecated Use setKeepAliveTimeout() instead.
     */
    public void setAbandonTime(int abandonTime) {
        setKeepAliveTimeout(abandonTime);
    }

    /**
     * Get the setup number of initial connection.
     * @deprecated Always return 0.
     */
    public int getInitConnectionNum() {
        return 0;
    }

    /**
     * Get the max number of connection.
     * @see #getMaxCount()
     * @deprecated Use getMaxCount() instead.
     */
    public int getMaxConnectionNum() {
        return getMaxCount();
    }

    /**
     * Get the max number of the idle connection.
     * @see #getMaxIdleCount()
     * @deprecated Use getMaxIdleCount() instead.
     */
    public int getMaxIdeNum() {
        return getMaxIdleCount();
    }

    /**
     * Get the setup time for abandoning a connection
     * which is not used for long time.
     * @see #getKeepAliveTimeout()
     * @deprecated Use getKeepAliveTimeout() instead.
     */
    public int getAbandonTime() {
        return getKeepAliveTimeout();
    }

    /**
     * Get the cycle for checking.
     * @see #getCheckInterval()
     * @deprecated Use getCheckInterval() instead.
     */
    public int getRecheckCyclePeriod() {
        return getCheckInterval();
    }

    /**
     * Get the period for getting back useful addresses.
     * @deprecated Always return 0.
     */
    public int getRecaptureConnPeriod() {
        return 0;
    }

    /**
     * Get the wait time.
     * @deprecated Always return 0.
     */
    public int getTimeout() {
        return 0;
    }

    BSONObject getSessionAttr() {
        BSONObject obj = new BasicBSONObject();
        if (_preferedInstance != null && _preferedInstance.size() > 0) {
            BSONObject list = new BasicBSONList();
            int i = 0;
            for(Object o : _preferedInstance) {
                list.put("" + i++, o);
            }
            obj.put(DatasourceConstants.FIELD_NAME_PREFERED_INSTANCE, list);
            obj.put(DatasourceConstants.FIELD_NAME_PREFERED_INSTANCE_MODE, _preferedInstanceMode);
            obj.put(DatasourceConstants.FIELD_NAME_SESSION_TIMEOUT, _sessionTimeout);
        }
        return obj;
    }

    private boolean isCharMode(String s) {
        for(int i = 0; i < MODE.size(); i++) {
            if (MODE.get(i).equals(s)) {
                return true;
            }
        }
        return false;
    }

    private boolean isValidMode(String s) {
        if (isCharMode(s)) {
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

}
