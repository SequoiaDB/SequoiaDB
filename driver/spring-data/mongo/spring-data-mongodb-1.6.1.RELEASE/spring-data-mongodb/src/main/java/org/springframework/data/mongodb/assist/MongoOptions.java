package org.springframework.data.mongodb.assist;

import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.net.ConfigOptions;
import org.bson.util.annotations.Immutable;

import javax.xml.crypto.Data;
import java.util.ArrayList;
import java.util.List;

/**
 * Various settings to control the behavior of a <code>MongoClient</code>.
 * <p/>
 * Note: This class is a replacement for {@code MongoOptions}, to be used with {@code MongoClient}.
 *
 * @see MongoClient
 * @since 2.10.0
 */
@Immutable
public class MongoOptions {
    private static final String DEFAULT_PREFERRD_INSTANCE_MODE = "random";
    private static final int DEFAULT_SESSION_TIMEOUT = -1;
    /**
     * A builder for MongoOptions so that MongoOptions can be immutable, and to support easier
     * construction through chaining.
     *
     */
    public static class Builder {
        private int connectTimeout = 10000;
        private int socketTimeout = 0;
        private boolean socketKeepAlive = false;
        private boolean useNagle = false;
        private boolean useSSL = false;

        private int deltaIncCount = 10;
        private int maxIdleCount = 10;
        private int maxCount = 500;
        private int keepAliveTimeout = 0 * 60 * 1000; // 0 min
        private int checkInterval = 1 * 60 * 1000; // 1 min
        private int syncCoordInterval = 0; // 0 min
        private boolean validateConnection = false;
        private ConnectStrategy connectStrategy = ConnectStrategy.BALANCE;

        private List<String> preferedInstance = null;
        private String preferedInstanceMode = DEFAULT_PREFERRD_INSTANCE_MODE; // "random" or "ordered"
        private int sessionTimeout = DEFAULT_SESSION_TIMEOUT;

        /**
         * Option for Connection. The connection timeout in milliseconds. A timeout of zero is interpreted as an infinite timeout.
         * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
         * <p/>
         * Default is 10,000.
         *
         * @return the socket connect timeout
         */
        public Builder connectTimeout(final int connectTimeout) {
            if (connectTimeout < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.connectTimeout = connectTimeout;
            return this;
        }

        /**
         * Option for Connection. The socket timeout in milliseconds.
         * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
         * <p/>
         * Default is 0 and means no timeout.
         * @param socketTimeout the socket timeout in milliseconds.
         * @return
         */
        public Builder socketTimeout(final int socketTimeout) {
            if (socketTimeout < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.socketTimeout = socketTimeout;
            return this;
        }

        /**
         * Option for Connection. Enable/disable SO_KEEPALIVE.
         *
         * @param on     whether or not to have socket keep alive turned on, default to be false.
         */
        public Builder socketKeepAlive(final boolean on) {
            this.socketKeepAlive = on;
            return this;
        }

        /**
         * Option for Connection. Enable/disable Nagle's algorithm(disable/enable TCP_NODELAY)
         *
         * @param on <code>true</code> to disable TCP_NODELAY,
         * <code>false</code> to enable, default to be false and going to use enable TCP_NODELAY.
         */
        public Builder useNagle(final boolean on) {
            this.useNagle = on;
            return this;
        }

        /**
         * Option for Connection. Set whether use the SSL or not.
         *
         * @param on <code>true</code> for using,
         * <code>false</code> for not.
         */
        public Builder useSSL(final boolean on) {
            this.useSSL = on;
            return this;
        }

        /**
         * Option for Datasource.
         * Set the number of new connections to create once running out the idle connections
         * @param deltaIncCount Default to be 10.
         * @return
         */
        public Builder deltaIncCount(final int deltaIncCount) {
            this.deltaIncCount = deltaIncCount;
            return this;
        }

        /**
         * Option for Datasource.
         * Set the max number of the idle connection left in connection
         * pool after periodically cleaning.
         * @param maxIdleCount Default to be 10.
         * @since 2.2
         */
        public Builder maxIdleCount(final int maxIdleCount) {
            this.maxIdleCount = maxIdleCount;
            return this;
        }

        /**
         * Option for Datasource.
         * Set the capacity of the connection pool.
         * When maxCount is set to 0, the connection pool will be disabled.
         * @param maxCount Default to be 500.
         * @since 2.2
         */
        public Builder maxCount(final int maxCount) {
            this.maxCount = maxCount;
            return this;
        }

        /**
         * Option for Datasource.
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
        public Builder keepAliveTimeout(final int keepAliveTimeout) {
            this.keepAliveTimeout = keepAliveTimeout;
            return this;
        }

        /**
         * Option for Datasource.
         * Set the checking interval in milliseconds. Every interval,
         * the pool cleans all the idle connection which keep alive time is up,
         * and keeps the number of idle connection not more than "maxIdleCount".
         * When "keepAliveTimeout" is not be 0, "checkInterval" should be less than it.
         * It's better to set "keepAliveTimeout" greater than "checkInterval" triple over.
         * @param checkInterval Default to be 1 * 60 * 1000ms.
         * @since 2.2
         */
        public Builder checkInterval(final int checkInterval) {
            this.checkInterval = checkInterval;
            return this;
        }

        /**
         * Option for Datasource.
         * Set the interval for updating coord's addresses from catalog in milliseconds.
         * The updated coord addresses will cover the addresses in the pool.
         * When "syncCoordInterval" is 0, the pool will stop updating coord's addresses from
         * catalog.
         * @param syncCoordInterval Default to be 1 * 60 * 1000ms.
         * @since 2.2
         */
        public Builder syncCoordInterval(final int syncCoordInterval) {
            this.syncCoordInterval = syncCoordInterval;
            return this;
        }

        /**
         * Option for Datasource.
         * When a idle connection is got out of pool, we need
         * to validate whether it can be used or not.
         * @param validateConnection Default to be false.
         * @since 2.2
         */
        public Builder validateConnection(final boolean validateConnection ) {
            this.validateConnection = validateConnection;
            return this;
        }

        /**
         * Option for Datasource.
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
        public Builder connectStrategy(final ConnectStrategy connectStrategy) {
            this.connectStrategy = connectStrategy;
            return this;
        }

        public Builder preferedInstance(final List<String> instance) {
            if (instance == null || instance.size() == 0) {
                return this;
            }
            preferedInstance = new ArrayList<String>();
            for(String o : instance) {
                preferedInstance.add(o);
            }
            return this;
        }

        public Builder preferedInstanceMode(final String mode) {
            if (mode == null || mode.isEmpty()) {
                preferedInstanceMode = DEFAULT_PREFERRD_INSTANCE_MODE;
            } else {
                preferedInstanceMode = mode;
            }
            return this;
        }

        public Builder sessionTimeout(final int timeout) {
            if (timeout < 0) {
                sessionTimeout = DEFAULT_SESSION_TIMEOUT;
            } else {
                sessionTimeout = timeout;
            }
            return this;
        }

        /**
         * Build an instance of MongoClientOptions.
         *
         * @return the options from this builder
         */
        public MongoOptions build() {
            return new MongoOptions(this);
        }
    }

    /**
     * Create a new Builder instance.  This is a convenience method, equivalent to {@code new MongoClientOptions.Builder()}.
     *
     * @return a new instance of a Builder
     */
    public static Builder builder() {
        return new Builder();
    }

    /**
     * Option for Connection. The connection timeout in milliseconds.  A value of 0 means no timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     * <p/>
     * Default is 10,000.
     *
     * @return the socket connect timeout
     */
    public int getConnectTimeout() {
        return connectTimeout;
    }

    /**
     * Option for Connection.
     * Get the socket timeout in milliseconds.
     * It is used for I/O socket read and write operations {@link java.net.Socket#setSoTimeout(int)}
     * <p/>
     * Default is 0 and means no timeout.
     *
     * @return the socket timeout
     */
    public int getSocketTimeout() {
        return socketTimeout;
    }

    /**
     * Option for Connection.
     * This flag controls the socket keep alive feature that keeps a connection alive through firewalls {@link java.net.Socket#setKeepAlive(boolean)}
     * <p/>
     * * Default is false.
     *
     * @return whether keep-alive is enabled on each socket
     */
    public boolean isSocketKeepAlive() {
        return socketKeepAlive;
    }

    /**
     * Option for Connection.
     * Get whether enable/disable Nagle's algorithm(disable/enable TCP_NODELAY)
     *
     * @return  <code>true</code> to disable TCP_NODELAY,
     * <code>false</code> to enable, default to be false and going to use enable TCP_NODELAY.
     */
    public boolean isUseNagle() {
        return useNagle;
    }

    /**
     * Option for Connection.
     * Get whether use the SSL or not. Default to be false.
     *
     * @return  <code>true</code> for using, <code>false</code> for not.
     */
    public boolean isUseSSL() {
        return useSSL;
    }

    /**
     * Option for Datasource.
     * Get the number of new connections to create once running out the idle connections. Default to be 10.
     * @return Delta increase count
     */
    public int deltaIncCount() {
        return deltaIncCount;
    }

    /**
     * Option for Datasource.
     * Get the max number of the idle connection left in connection
     * pool after periodically cleaning. Default to be 10.
     * @return maximum count of the idle connection
     * @since 2.2
     */
    public int maxIdleCount() {
        return maxIdleCount;
    }

    /**
     * Option for Datasource.
     * Get the capacity of the connection pool.
     * When maxCount is set to 0, the connection pool will be disabled. Default to be 500.
     * @return the capacity of the connection pool.
     * @since 2.2
     */
    public int maxCount() {
        return maxCount;
    }

    /**
     * Option for Datasource.
     * Get the time in milliseconds for abandoning a connection which keep alive time is up.
     * If a connection has not be used(send and receive) for a long time(longer
     * than "keepAliveTimeout"), the pool will not let it come back.
     * The pool will also clean this kind of idle connections in the pool periodically.
     * When "keepAliveTimeout" is not set to 0, it's better to set it
     * greater than "checkInterval" triple over. Besides, unless you know what you need,
     * never enable this option. Default to be 0ms, means not care about how long does a connection
     * have not be used(send and receive).
     * @return keepAliveTimeout
     * @since 2.2
     */
    public int keepAliveTimeout() {
        return keepAliveTimeout;
    }

    /**
     * Option for Datasource.
     * Get the checking interval in milliseconds. Every interval,
     * the pool cleans all the idle connection which keep alive time is up,
     * and keeps the number of idle connection not more than "maxIdleCount".
     * When "keepAliveTimeout" is not be 0, "checkInterval" should be less than it.
     * It's better to set "keepAliveTimeout" greater than "checkInterval" triple over.
     * Default to be 1 * 60 * 1000ms.
     * @return checkInterval
     * @since 2.2
     */
    public int checkInterval() {
        return checkInterval;
    }

    /**
     * Option for Datasource.
     * Set the interval for updating coord's addresses from catalog in milliseconds.
     * The updated coord addresses will cover the addresses in the pool.
     * When "syncCoordInterval" is 0, the pool will stop updating coord's addresses from
     * catalog. Default to be 1 * 60 * 1000ms.
     * @return syncCoordInterval
     * @since 2.2
     */
    public int syncCoordInterval() {
        return syncCoordInterval;
    }

    /**
     * Option for Datasource.
     * When a idle connection is got out of pool, we need
     * to validate whether it can be used or not. Default to be false.
     * @return  validateConnection
     * @since 2.2
     */
    public boolean isValidateConnection() {
        return validateConnection;
    }

    /**
     * Option for Datasource.
     * Get connection strategy.
     * When choosing ConnectStrategy.LOCAL, if there have no local coord address,
     * use other address instead.
     * @return  strategy of getting address to create connections :
     * @since 2.2
     */
    public ConnectStrategy connectStrategy() {
        return connectStrategy;
    }

    /**
     * Get the preferred instance.
     * @return The preferred instance or null for no any setting.
     */
    public List<String> getPreferedInstance() {
        return preferedInstance;
    }

    /**
     * Get the preferred instance node.
     *
     * @return The preferred instance node.
     */
    public String getPreferedInstanceMode() {
        return preferedInstanceMode;
    }

    /**
     * The Session timeout value.
     * @return Session timeout value.
     */
    public int getSessionTimeout() {
        return sessionTimeout;
    }

    @Override
    public boolean equals(final Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        final MongoOptions that = (MongoOptions) o;

        if (connectTimeout != that.connectTimeout) {
            return false;
        }
        if (socketTimeout != that.socketTimeout) {
            return false;
        }
        if (socketKeepAlive != that.socketKeepAlive) {
            return false;
        }
        if (connectTimeout != that.connectTimeout) {
            return false;
        }
        if (useNagle != that.useNagle) {
            return false;
        }
        if (useSSL != that.useSSL) {
            return false;
        }
        if (deltaIncCount != that.deltaIncCount) {
            return false;
        }
        if (maxIdleCount != that.maxIdleCount) {
            return false;
        }
        if (maxCount != that.maxCount) {
            return false;
        }
        if (keepAliveTimeout != that.keepAliveTimeout) {
            return false;
        }
        if (checkInterval != that.checkInterval) {
            return false;
        }
        if (syncCoordInterval != that.syncCoordInterval) {
            return false;
        }
        if (validateConnection != that.validateConnection) {
            return false;
        }
        if (connectStrategy != that.connectStrategy) {
            return false;
        }
        if (preferedInstance != that.preferedInstance) {
            return false;
        }
        if (!preferedInstanceMode.equals(that.preferedInstanceMode)) {
            return false;
        }
        if (sessionTimeout != that.sessionTimeout) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int result = 1;
        result = 31 * result + connectTimeout;
        result = 31 * result + socketTimeout;
        result = 31 * result + (socketKeepAlive ? 1 : 0);
        result = 31 * result + (useNagle ? 1 : 0);
        result = 31 * result + (useSSL ? 1 : 0);
        result = 31 * result + deltaIncCount;
        result = 31 * result + maxIdleCount;
        result = 31 * result + maxCount;
        result = 31 * result + keepAliveTimeout;
        result = 31 * result + checkInterval;
        result = 31 * result + syncCoordInterval;
        result = 31 * result + (validateConnection ? 1 : 0);
        result = 31 * result + connectStrategy.hashCode();
        result = 31 * result + preferedInstanceMode.hashCode();
        result = 31 * result + sessionTimeout;
        if (preferedInstance != null) {
            for (Object o : preferedInstance) {
                result = 31 * result + o.hashCode();
            }
        }
        return result;
    }

    @Override
    public String toString() {
        return "MongoClientOptions{"
                + "connectTimeout='" + connectTimeout
                + ", socketTimeout=" + socketTimeout
                + ", socketKeepAlive=" + socketKeepAlive
                + ", useNagle=" + useNagle
                + ", useSSL=" + useSSL
                + ", deltaIncCount=" + deltaIncCount
                + ", maxIdleCount=" + maxIdleCount
                + ", maxCount=" + maxCount
                + ", keepAliveTimeout=" + keepAliveTimeout
                + ", checkInterval=" + checkInterval
                + ", syncCoordInterval=" + syncCoordInterval
                + ", validateConnection=" + validateConnection
                + ", connectStrategy=" + connectStrategy
                + ", preferedInstance=" + preferedInstance
                + ", preferedInstanceMode=" + preferedInstanceMode
                + ", sessionTimeout=" + sessionTimeout
                + '}';
    }

    ConfigOptions getNetworkOptions() {
        ConfigOptions networkOptions = new ConfigOptions();
        networkOptions.setMaxAutoConnectRetryTime(0);
        networkOptions.setConnectTimeout(connectTimeout);
        networkOptions.setSocketTimeout(socketTimeout);
        networkOptions.setSocketKeepAlive(socketKeepAlive);
        networkOptions.setUseNagle(useNagle);
        networkOptions.setUseSSL(useSSL);
        return networkOptions;
    }

    DatasourceOptions getDatasourceOptions() {
        DatasourceOptions datasourceOptions = new DatasourceOptions();
        datasourceOptions.setDeltaIncCount(deltaIncCount);
        datasourceOptions.setMaxIdleCount(maxIdleCount);
        datasourceOptions.setMaxCount(maxCount);
        datasourceOptions.setKeepAliveTimeout(keepAliveTimeout);
        datasourceOptions.setCheckInterval(checkInterval);
        datasourceOptions.setSyncCoordInterval(syncCoordInterval);
        datasourceOptions.setValidateConnection(validateConnection);
        datasourceOptions.setConnectStrategy(connectStrategy);
        datasourceOptions.setPreferedInstance(preferedInstance);
        datasourceOptions.setPreferedInstanceMode(preferedInstanceMode);
        datasourceOptions.setSessionTimeout(sessionTimeout);
        return datasourceOptions;
    }

    private MongoOptions(final Builder builder) {
        connectTimeout = builder.connectTimeout;
        socketTimeout = builder.socketTimeout;
        socketKeepAlive = builder.socketKeepAlive;
        useNagle = builder.useNagle;
        useSSL = builder.useSSL;

        deltaIncCount = builder.deltaIncCount;
        maxIdleCount = builder.maxIdleCount;
        maxCount = builder.maxCount;
        keepAliveTimeout = builder.keepAliveTimeout;
        checkInterval = builder.checkInterval;
        syncCoordInterval = builder.syncCoordInterval;
        validateConnection = builder.validateConnection;
        connectStrategy = builder.connectStrategy;
        preferedInstance = builder.preferedInstance;
        preferedInstanceMode = builder.preferedInstanceMode;
        sessionTimeout = builder.sessionTimeout;
    }

    private final int connectTimeout;
    private final int socketTimeout;
    private final boolean socketKeepAlive;
    private final boolean useNagle;
    private final boolean useSSL;
    private final int deltaIncCount;
    private final int maxIdleCount;
    private final int maxCount;
    private final int keepAliveTimeout;
    private final int checkInterval;
    private final int syncCoordInterval;
    private final boolean validateConnection;
    private final ConnectStrategy connectStrategy;
    private final List<String> preferedInstance;
    private final String preferedInstanceMode;
    private final int sessionTimeout;
}

