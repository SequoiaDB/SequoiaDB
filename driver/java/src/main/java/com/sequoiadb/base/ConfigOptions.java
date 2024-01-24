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

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

/**
 * Database Connection Configuration Option
 */
public class ConfigOptions implements Cloneable {
    private long maxAutoConnectRetryTime = 15000;
    private int connectTimeout = 10000;
    private int socketTimeout = 0;
    private boolean socketKeepAlive = true;
    private boolean useNagle = false;
    private boolean useSSL = false;

    /**
     * Set the max auto connect retry time in milliseconds. Default to be 15,000ms.
     * when "connectTimeout" is set to 10,000ms(default value), the max number of retries is
     * ceiling("maxAutoConnectRetryTime" / "connectTimeout"), which is 2.
     *
     * @param maxRetryTimeMillis the max auto connect retry time in milliseconds
     */
    public void setMaxAutoConnectRetryTime(long maxRetryTimeMillis) {
        if (maxRetryTimeMillis < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "should not less than 0");
        }
        this.maxAutoConnectRetryTime = maxRetryTimeMillis;
    }

    /**
     * Set the connection timeout in milliseconds. A value of 0 means no timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     *
     * @param connectTimeoutMillis The connection timeout in milliseconds. Default is 10,000ms.
     */
    public void setConnectTimeout(int connectTimeoutMillis) {
        if (connectTimeoutMillis < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "should not less than 0");
        }
        this.connectTimeout = connectTimeoutMillis;
    }

    /**
     * Set the socket timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     *
     * @param socketTimeoutMillis The socket timeout in milliseconds. Default is 0ms and means no timeout.
     */
    public void setSocketTimeout(int socketTimeoutMillis) {
        if (socketTimeoutMillis < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "should not less than 0");
        }
        this.socketTimeout = socketTimeoutMillis;
    }

    /**
     * This flag controls the socket keep alive feature that keeps a connection alive through firewalls {@link java.net.Socket#setKeepAlive(boolean)}
     *
     * @param on whether keep-alive is enabled on each socket. Default is true.
     */
    public void setSocketKeepAlive(boolean on) {
        this.socketKeepAlive = on;
    }

    /**
     * Set whether enable/disable Nagle's algorithm(disable/enable TCP_NODELAY)
     *
     * @param on <code>false</code> to enable TCP_NODELAY, default to be false and going to use enable TCP_NODELAY.
     */
    public void setUseNagle(boolean on) {
        this.useNagle = on;
    }

    /**
     * Set whether use the SSL or not
     *
     * @param on Default to be false.
     */
    public void setUseSSL(boolean on) {
        this.useSSL = on;
    }

    /**
     * Get the max auto connect retry time in milliseconds.
     *
     * @return the max auto connect retry time in milliseconds.
     */
    public long getMaxAutoConnectRetryTime() {
        return maxAutoConnectRetryTime;
    }

    /**
     * The connection timeout in milliseconds. A timeout of zero is interpreted as an infinite timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     * <p/>
     * Default is 10,000ms.
     *
     * @return the socket connect timeout
     */
    public int getConnectTimeout() {
        return connectTimeout;
    }

    /**
     * Get the socket timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     * <p/>
     * Default is 0ms and means no timeout.
     *
     * @return the socket timeout
     */
    public int getSocketTimeout() {
        return socketTimeout;
    }

    /**
     * Get whether the socket keeps alive or not
     *
     * @return the status of setting
     */
    public boolean getSocketKeepAlive() {
        return socketKeepAlive;
    }

    /**
     * Get whether use the Nagle Algorithm or not
     *
     * @return the status of setting
     */
    public boolean getUseNagle() {
        return useNagle;
    }

    /**
     * Get whether use the SSL or not
     *
     * @return the status of setting
     */
    public boolean getUseSSL() {
        return useSSL;
    }

    /**
     * Clone the current options.
     *
     * @since 3.4.3
     */
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

    @Override
    public String toString() {
        return "ConfigOptions: { " +
                "maxAutoConnectRetryTime: " + maxAutoConnectRetryTime +
                ", connectTimeout: " + connectTimeout +
                ", socketTimeout: " + socketTimeout +
                ", socketKeepAlive: " + socketKeepAlive +
                ", useNagle: " + useNagle +
                ", useSSL: " + useSSL
                + " }";
    }
}
