package org.springframework.data.sequoiadb.assist;

import org.bson.util.annotations.Immutable;

import javax.net.SocketFactory;

/**
 * Various settings to control the behavior of a <code>SdbClient</code>.
 * <p/>
 * Note: This class is a replacement for {@code SequoiadbOptions}, to be used with {@code SdbClient}.  The main difference
 * in behavior is that the default write concern is {@code WriteConcern.ACKNOWLEDGED}.
 *
 * @see SdbClient
 * @since 2.10.0
 */
@Immutable
public class SdbClientOptions {
    /**
     * A builder for SdbClientOptions so that SdbClientOptions can be immutable, and to support easier
     * construction through chaining.
     *
     * @since 2.10.0
     */
    public static class Builder {

        private String description;
        private int minConnectionsPerHost;
        private int connectionsPerHost = 100;
        private int threadsAllowedToBlockForConnectionMultiplier = 5;
        private int maxWaitTime = 1000 * 60 * 2;
        private int maxConnectionIdleTime;
        private int maxConnectionLifeTime;
        private int connectTimeout = 1000 * 10;
        private int socketTimeout = 0;
        private boolean socketKeepAlive = false;
        private boolean autoConnectRetry = false;
        private long maxAutoConnectRetryTime = 0;
        private ReadPreference readPreference = null;
        private DBDecoderFactory dbDecoderFactory = null;
        private DBEncoderFactory dbEncoderFactory = null;
        private WriteConcern writeConcern = null;
        private SocketFactory socketFactory = SocketFactory.getDefault();
        private boolean cursorFinalizerEnabled = true;
        private boolean alwaysUseMBeans = false;
        private int heartbeatFrequency = Integer.parseInt(System.getProperty("com.sequoiadb.updaterIntervalMS", "5000"));
        private int heartbeatConnectRetryFrequency = Integer.parseInt(System.getProperty("com.sequoiadb.updaterIntervalNoMasterMS", "10"));
        private int heartbeatConnectTimeout = Integer.parseInt(System.getProperty("com.sequoiadb.updaterConnectTimeoutMS", "20000"));
        private int heartbeatSocketTimeout = Integer.parseInt(System.getProperty("com.sequoiadb.updaterSocketTimeoutMS", "20000"));
        private int heartbeatThreadCount;
        private int acceptableLatencyDifference = Integer.parseInt(System.getProperty("com.sequoiadb.slaveAcceptableLatencyMS", "15"));
        private String requiredReplicaSetName;

        /**
         * Sets the heartbeat frequency.
         *
         * @param heartbeatFrequency the heartbeat frequency, in milliseconds
         * @return {@code this}
         * @throws IllegalArgumentException if heartbeatFrequency < 1
         * @see com.sequoiadb.SdbClientOptions#getHeartbeatFrequency()
         * @since 2.12.0
         */
        public Builder heartbeatFrequency(final int heartbeatFrequency) {
            if (heartbeatFrequency < 1) {
                throw new IllegalArgumentException("heartbeatFrequency must be greater than 0");
            }
            this.heartbeatFrequency = heartbeatFrequency;
            return this;
        }

        /**
         * Sets the heartbeat connect retry frequency.
         *
         * @param heartbeatConnectRetryFrequency the heartbeat connect retry frequency, in milliseconds
         * @return {@code this}
         * @throws IllegalArgumentException if heartbeatConnectRetryFrequency < 1
         * @see com.sequoiadb.SdbClientOptions#getHeartbeatConnectRetryFrequency()
         * @since 2.12.0
         */
        public Builder heartbeatConnectRetryFrequency(final int heartbeatConnectRetryFrequency) {
            if (heartbeatConnectRetryFrequency < 1) {
                throw new IllegalArgumentException("heartbeatConnectRetryFrequency must be greater than 0");
            }
            this.heartbeatConnectRetryFrequency = heartbeatConnectRetryFrequency;
            return this;
        }

        /**
         * Sets the heartbeat connect timeout.
         *
         * @param heartbeatConnectTimeout the heartbeat connect timeout, in milliseconds
         * @return {@code this}
         * @throws IllegalArgumentException if heartbeatConnectTimeout < 0
         * @see com.sequoiadb.SdbClientOptions#getHeartbeatConnectTimeout()
         * @since 2.12.0
         */
        public Builder heartbeatConnectTimeout(final int heartbeatConnectTimeout) {
            if (heartbeatConnectTimeout < 0) {
                throw new IllegalArgumentException("heartbeatConnectTimeout must be greater than or equal to 0");
            }
            this.heartbeatConnectTimeout = heartbeatConnectTimeout;
            return this;
        }

        /**
         * Sets the heartbeat connect socket timeout.
         *
         * @param heartbeatSocketTimeout the heartbeat socket timeout, in milliseconds
         * @return {@code this}
         * @throws IllegalArgumentException if heartbeatSocketTimeout < 0
         * @see com.sequoiadb.SdbClientOptions#getHeartbeatSocketTimeout()
         * @since 2.12.0
         */
        public Builder heartbeatSocketTimeout(final int heartbeatSocketTimeout) {
            if (heartbeatSocketTimeout < 0) {
                throw new IllegalArgumentException("heartbeatSocketTimeout must be greater than or equal to 0");
            }
            this.heartbeatSocketTimeout = heartbeatSocketTimeout;
            return this;
        }

        /**
         * Sets the heartbeat thread count.
         *
         * @param heartbeatThreadCount the heartbeat thread count
         * @return {@code this}
         * @throws IllegalArgumentException if heartbeatThreadCount < 1
         * @see SdbClientOptions#getHeartbeatThreadCount()
         * @since 2.12.0
         */
        public Builder heartbeatThreadCount(final int heartbeatThreadCount) {
            if (heartbeatThreadCount < 1) {
                throw new IllegalArgumentException("heartbeatThreadCount must be greater than 0");
            }
            this.heartbeatThreadCount = heartbeatThreadCount;
            return this;
        }

        /**
         * Sets the acceptable latency difference.
         *
         * @param acceptableLatencyDifference the acceptable latency different, in milliseconds
         * @return {@code this}
         * @throws IllegalArgumentException if acceptableLatencyDifference < 0
         * @see com.sequoiadb.SdbClientOptions#getAcceptableLatencyDifference()
         * @since 2.12.0
         */
        public Builder acceptableLatencyDifference(final int acceptableLatencyDifference) {
            if (acceptableLatencyDifference < 0) {
                throw new IllegalArgumentException("acceptableLatencyDifference must be greater than or equal to 0");
            }
            this.acceptableLatencyDifference = acceptableLatencyDifference;
            return this;
        }

        /**
         * Sets the description.
         *
         * @param description the description of this SdbClient
         * @return {@code this}
         * @see com.sequoiadb.SdbClientOptions#getDescription()
         */
        public Builder description(final String description) {
            this.description = description;
            return this;
        }

        /**
         * Sets the minimum number of connections per host.
         *
         * @param minConnectionsPerHost minimum number of connections
         * @return {@code this}
         * @throws IllegalArgumentException if {@code minConnectionsPerHost < 0}
         * @see SdbClientOptions#getMinConnectionsPerHost()
         * @since 2.12
         */
        public Builder minConnectionsPerHost(final int minConnectionsPerHost) {
            if (minConnectionsPerHost < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.minConnectionsPerHost = minConnectionsPerHost;
            return this;
        }

        /**
         * Sets the maximum number of connections per host.
         *
         * @param connectionsPerHost maximum number of connections
         * @return {@code this}
         * @throws IllegalArgumentException if <code>connnectionsPerHost < 1</code>
         * @see com.sequoiadb.SdbClientOptions#getConnectionsPerHost()
         */
        public Builder connectionsPerHost(final int connectionsPerHost) {
            if (connectionsPerHost < 1) {
                throw new IllegalArgumentException("Minimum value is 1");
            }
            this.connectionsPerHost = connectionsPerHost;
            return this;
        }

        /**
         * Sets the multiplier for number of threads allowed to block waiting for a connection.
         *
         * @param threadsAllowedToBlockForConnectionMultiplier
         *         the multiplier
         * @return {@code this}
         * @throws IllegalArgumentException if <code>threadsAllowedToBlockForConnectionMultiplier < 1</code>
         * @see com.sequoiadb.SdbClientOptions#getThreadsAllowedToBlockForConnectionMultiplier()
         */
        public Builder threadsAllowedToBlockForConnectionMultiplier(final int threadsAllowedToBlockForConnectionMultiplier) {
            if (threadsAllowedToBlockForConnectionMultiplier < 1) {
                throw new IllegalArgumentException("Minimum value is 1");
            }
            this.threadsAllowedToBlockForConnectionMultiplier = threadsAllowedToBlockForConnectionMultiplier;
            return this;
        }

        /**
         * Sets the maximum time that a thread will block waiting for a connection.
         *
         * @param maxWaitTime the maximum wait time (in milliseconds)
         * @return {@code this}
         * @throws IllegalArgumentException if <code>maxWaitTime &lt; 0</code>
         * @see com.sequoiadb.SdbClientOptions#getMaxWaitTime()
         */
        public Builder maxWaitTime(final int maxWaitTime) {
            if (maxWaitTime < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.maxWaitTime = maxWaitTime;
            return this;
        }

        /**
         * Sets the maximum idle time for a pooled connection.
         *
         * @param maxConnectionIdleTime the maximum idle time
         * @return {@code this}
         * @throws IllegalArgumentException if {@code maxConnectionIdleTime < 0}
         * @see com.sequoiadb.SdbClientOptions#getMaxConnectionIdleTime()
         * @since 2.12
         */
        public Builder maxConnectionIdleTime(final int maxConnectionIdleTime) {
            if (maxConnectionIdleTime < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.maxConnectionIdleTime = maxConnectionIdleTime;
            return this;
        }

        /**
         * Sets the maximum life time for a pooled connection.
         *
         * @param maxConnectionLifeTime the maximum life time
         * @return {@code this}
         * @throws IllegalArgumentException if {@code maxConnectionIdleTime < 0}
         * @see SdbClientOptions#getMaxConnectionIdleTime()
         * @since 2.12
         */
        public Builder maxConnectionLifeTime(final int maxConnectionLifeTime) {
            if (maxConnectionLifeTime < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.maxConnectionLifeTime = maxConnectionLifeTime;
            return this;
        }

        /**
         * Sets the connection timeout.
         *
         * @param connectTimeout the connection timeout (in milliseconds)
         * @return {@code this}
         * @see com.sequoiadb.SdbClientOptions#getConnectTimeout()
         */
        public Builder connectTimeout(final int connectTimeout) {
            if (connectTimeout < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.connectTimeout = connectTimeout;
            return this;
        }

        /**
         * Sets the socket timeout.
         *
         * @param socketTimeout the socket timeout (in milliseconds)
         * @return {@code this}
         * @see com.sequoiadb.SdbClientOptions#getSocketTimeout()
         */
        public Builder socketTimeout(final int socketTimeout) {
            if (socketTimeout < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.socketTimeout = socketTimeout;
            return this;
        }

        /**
         * Sets whether socket keep alive is enabled.
         *
         * @param socketKeepAlive keep alive
         * @return {@code this}
         * @see com.sequoiadb.SdbClientOptions#isSocketKeepAlive()
         */
        public Builder socketKeepAlive(final boolean socketKeepAlive) {
            this.socketKeepAlive = socketKeepAlive;
            return this;
        }

        /**
         * Sets whether auto connect retry is enabled.
         *
         * @param autoConnectRetry auto connect retry
         * @return {@code this}
         * @see SdbClientOptions#isAutoConnectRetry()
         * @deprecated There is no replacement for this method.  Use the connectTimeout property to control connection timeout.
         */
        @Deprecated
        public Builder autoConnectRetry(final boolean autoConnectRetry) {
            this.autoConnectRetry = autoConnectRetry;
            return this;
        }

        /**
         * Sets the maximum auto connect retry time.
         *
         * @param maxAutoConnectRetryTime the maximum auto connect retry time
         * @return {@code this}
         * @see SdbClientOptions#getMaxAutoConnectRetryTime()
         * @deprecated There is no replacement for this method.  Use the connectTimeout property to control connection timeout.
         */
        @Deprecated
        public Builder maxAutoConnectRetryTime(final long maxAutoConnectRetryTime) {
            if (maxAutoConnectRetryTime < 0) {
                throw new IllegalArgumentException("Minimum value is 0");
            }
            this.maxAutoConnectRetryTime = maxAutoConnectRetryTime;
            return this;
        }

        /**
         * Sets the read preference.
         *
         * @param readPreference read preference
         * @return {@code this}
         * @see SdbClientOptions#getReadPreference()
         */
        public Builder readPreference(final ReadPreference readPreference) {
            if (readPreference == null) {
                throw new IllegalArgumentException("null is not a legal value");
            }
            this.readPreference = readPreference;
            return this;
        }

        /**
         * Sets the decoder factory.
         *
         * @param dbDecoderFactory the decoder factory
         * @return {@code this}
         * @see SdbClientOptions#getDbDecoderFactory()
         */
        public Builder dbDecoderFactory(final DBDecoderFactory dbDecoderFactory) {
            if (dbDecoderFactory == null) {
                throw new IllegalArgumentException("null is not a legal value");
            }
            this.dbDecoderFactory = dbDecoderFactory;
            return this;
        }

        /**
         * Sets the encoder factory.
         *
         * @param dbEncoderFactory the encoder factory
         * @return {@code this}
         * @see SdbClientOptions#getDbEncoderFactory()
         */
        public Builder dbEncoderFactory(final DBEncoderFactory dbEncoderFactory) {
            if (dbEncoderFactory == null) {
                throw new IllegalArgumentException("null is not a legal value");
            }
            this.dbEncoderFactory = dbEncoderFactory;
            return this;
        }

        /**
         * Sets the write concern.
         *
         * @param writeConcern the write concern
         * @return {@code this}
         * @see SdbClientOptions#getWriteConcern()
         */
        public Builder writeConcern(final WriteConcern writeConcern) {
            if (writeConcern == null) {
                throw new IllegalArgumentException("null is not a legal value");
            }
            this.writeConcern = writeConcern;
            return this;
        }

        /**
         * Sets the socket factory.
         *
         * @param socketFactory the socket factory
         * @return {@code this}
         * @see SdbClientOptions#getSocketFactory()
         */
        public Builder socketFactory(final SocketFactory socketFactory) {
            if (socketFactory == null) {
                throw new IllegalArgumentException("null is not a legal value");
            }
            this.socketFactory = socketFactory;
            return this;
        }

        /**
         * Sets whether cursor finalizers are enabled.
         *
         * @param cursorFinalizerEnabled whether cursor finalizers are enabled.
         * @return {@code this}
         * @see SdbClientOptions#isCursorFinalizerEnabled()
         */
        public Builder cursorFinalizerEnabled(final boolean cursorFinalizerEnabled) {
            this.cursorFinalizerEnabled = cursorFinalizerEnabled;
            return this;
        }

        /**
         * Sets whether JMX beans registered by the driver should always be MBeans, regardless of whether the VM is
         * Java 6 or greater. If false, the driver will use MXBeans if the VM is Java 6 or greater, and use MBeans if
         * the VM is Java 5.
         *
         * @param alwaysUseMBeans true if driver should always use MBeans, regardless of VM version
         * @return this
         * @see SdbClientOptions#isAlwaysUseMBeans()
         */
        public Builder alwaysUseMBeans(final boolean alwaysUseMBeans) {
            this.alwaysUseMBeans = alwaysUseMBeans;
            return this;
        }


        /**
         * Sets the required replica set name for the cluster.
         *
         * @param requiredReplicaSetName the required replica set name for the replica set.
         * @return this
         * @see SdbClientOptions#getRequiredReplicaSetName()
         * @since 2.12
         */
        public Builder requiredReplicaSetName(final String requiredReplicaSetName) {
            this.requiredReplicaSetName = requiredReplicaSetName;
            return this;
        }

        /**
         * Sets defaults to be what they are in {@code SequoiadbOptions}.
         *
         * @return {@code this}
         * @see SequoiadbOptions
         */
        public Builder legacyDefaults() {
            connectionsPerHost = 10;
            writeConcern = null;
            return this;
        }

        /**
         * Build an instance of SdbClientOptions.
         *
         * @return the options from this builder
         */
        public SdbClientOptions build() {
            return new SdbClientOptions(this);
        }
    }

    /**
     * Create a new Builder instance.  This is a convenience method, equivalent to {@code new SdbClientOptions.Builder()}.
     *
     * @return a new instance of a Builder
     */
    public static Builder builder() {
        return new Builder();
    }

    /**
     * Gets the description for this SdbClient, which is used in various places like logging and JMX.
     * <p/>
     * Default is null.
     *
     * @return the description
     */
    public String getDescription() {
        return description;
    }

    /**
     * The minimum number of connections per host for this SdbClient instance. Those connections will be kept in a pool when idle, and the
     * pool will ensure over time that it contains at least this minimum number.
     * <p/>
     * Default is 0.
     *
     * @return the minimum size of the connection pool per host
     * @since 2.12
     */
    public int getMinConnectionsPerHost() {
        return minConnectionsPerHost;
    }

    /**
     * The maximum number of connections allowed per host for this SdbClient instance.
     * Those connections will be kept in a pool when idle.
     * Once the pool is exhausted, any operation requiring a connection will block waiting for an available connection.
     * <p/>
     * Default is 100.
     *
     * @return the maximum size of the connection pool per host
     * @see SdbClientOptions#getThreadsAllowedToBlockForConnectionMultiplier()
     */
    public int getConnectionsPerHost() {
        return connectionsPerHost;
    }

    /**
     * this multiplier, multiplied with the connectionsPerHost setting, gives the maximum number of threads that
     * may be waiting for a connection to become available from the pool. All further threads will get an exception right
     * away. For example if connectionsPerHost is 10 and threadsAllowedToBlockForConnectionMultiplier is 5, then up to 50
     * threads can wait for a connection.
     * <p/>
     * Default is 5.
     *
     * @return the multiplier
     */
    public int getThreadsAllowedToBlockForConnectionMultiplier() {
        return threadsAllowedToBlockForConnectionMultiplier;
    }

    /**
     * The maximum wait time in milliseconds that a thread may wait for a connection to become available.
     * <p/>
     * Default is 120,000. A value of 0 means that it will not wait.  A negative value means to wait indefinitely.
     *
     * @return the maximum wait time.
     */
    public int getMaxWaitTime() {
        return maxWaitTime;
    }

    /**
     * The maximum idle time of a pooled connection.  A zero value indicates no limit to the idle time.  A pooled connection that has
     * exceeded its idle time will be closed and replaced when necessary by a new connection.
     *
     * @return the maximum idle time, in milliseconds
     * @since 2.12
     */
    public int getMaxConnectionIdleTime() {
        return maxConnectionIdleTime;
    }

    /**
     * The maximum life time of a pooled connection.  A zero value indicates no limit to the life time.  A pooled connection that has
     * exceeded its life time will be closed and replaced when necessary by a new connection.
     *
     * @return the maximum life time, in milliseconds
     * @since 2.12
     */
    public int getMaxConnectionLifeTime() {
        return maxConnectionLifeTime;
    }

    /**
     * The connection timeout in milliseconds.  A value of 0 means no timeout.
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
     * The socket timeout in milliseconds.
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
     * If true, the driver will keep trying to connect to the server in case that the socket cannot be established within the {code
     * connectTimeout} period. There is maximum amount of time to keep retrying, which is 15s by default. Note that use of this flag does
     * not prevent exceptions from being thrown from read/write operations,  and they must be handled by the application. The default
     * value is false.
     *
     * @return whether socket connect is retried
     * @deprecated There is no replacement for this method.  Use the connectTimeout property to control connection timeout.
     */
    @Deprecated
    public boolean isAutoConnectRetry() {
        return autoConnectRetry;
    }

    /**
     * The maximum amount of time in MS to spend trying to open connection to the same server. The default is 0,
     * which means to use the default 15s if autoConnectRetry is on.
     *
     * @return the maximum socket connect retry time.
     * @deprecated There is no replacement for this method.  Use the connectTimeout property to control connection timeout.
     */
    @Deprecated
    public long getMaxAutoConnectRetryTime() {
        return maxAutoConnectRetryTime;
    }

    /**
     * The read preference to use for queries, map-reduce, aggregation, and count.
     * <p/>
     * Default is {@code ReadPreference.primary()}.
     *
     * @return the read preference
     * @see com.sequoiadb.ReadPreference#primary()
     */
    public ReadPreference getReadPreference() {
        return readPreference;
    }

    /**
     * Override the decoder factory. Default is for the standard Sdb Java driver configuration.
     *
     * @return the decoder factory
     */
    public DBDecoderFactory getDbDecoderFactory() {
        return dbDecoderFactory;
    }

    /**
     * Override the encoder factory. Default is for the standard Sdb Java driver configuration.
     *
     * @return the encoder factory
     */
    public DBEncoderFactory getDbEncoderFactory() {
        return dbEncoderFactory;
    }

    /**
     * The write concern to use.
     * <p/>
     * Default is {@code WriteConcern.ACKNOWLEDGED}.
     *
     * @return the write concern
     * @see WriteConcern#ACKNOWLEDGED
     */
    public WriteConcern getWriteConcern() {
        return writeConcern;
    }

    /**
     * The socket factory for creating sockets to the sdb server.
     * <p/>
     * Default is SocketFactory.getDefault()
     *
     * @return the socket factory
     */
    public SocketFactory getSocketFactory() {
        return socketFactory;
    }

    /**
     * Gets whether there is a a finalize method created that cleans up instances of DBCursor that the client
     * does not close.  If you are careful to always call the close method of DBCursor, then this can safely be set to false.
     * <p/>
     * Default is true.
     *
     * @return whether finalizers are enabled on cursors
     * @see DBCursor
     * @see com.sequoiadb.DBCursor#close()
     */
    public boolean isCursorFinalizerEnabled() {
        return cursorFinalizerEnabled;
    }

    /**
     * Gets whether JMX beans registered by the driver should always be MBeans, regardless of whether the VM is
     * Java 6 or greater. If false, the driver will use MXBeans if the VM is Java 6 or greater, and use MBeans if
     * the VM is Java 5.
     * <p>
     * Default is false.
     * </p>
     */
    public boolean isAlwaysUseMBeans() {
        return alwaysUseMBeans;
    }

    /**
     * Gets the heartbeat frequency. This is the frequency that a background thread will attempt to connect to each SequoiaDB server that
     * the SdbClient is connected to.
     * <p>
     * The default value is 5,000 milliseconds.
     * </p>
     *
     * @return the heartbeat frequency, in milliseconds
     * @since 2.12.0
     */
    public int getHeartbeatFrequency() {
        return heartbeatFrequency;
    }

    /**
     * Gets the heartbeat connect retry frequency. This is the frequency that a background thread will attempt to connect to each SequoiaDB
     * server that the SdbClient is connected to, when that server is currently unreachable.
     * <p>
     * The default value is 10 milliseconds.
     * </p>
     *
     * @return the heartbeat connect retry frequency, in milliseconds
     * @since 2.12.0
     */
    public int getHeartbeatConnectRetryFrequency() {
        return heartbeatConnectRetryFrequency;
    }

    /**
     * Gets the heartbeat connect timeout. This is the socket connect timeout for sockets used by the background thread that is
     * monitoring each SequoiaDB server that the SdbClient is connected to.
     * <p>
     * The default value is 20,000 milliseconds.
     * </p>
     *
     * @return the heartbeat connect timeout, in milliseconds
     * @since 2.12.0
     */
    public int getHeartbeatConnectTimeout() {
        return heartbeatConnectTimeout;
    }

    /**
     * Gets the heartbeat socket timeout. This is the socket timeout for sockets used by the background thread that is monitoring each
     * SequoiaDB server that the SdbClient is connected to.
     * <p>
     * The default value is 20,000 milliseconds.
     * </p>

     * @return the heartbeat socket timeout, in milliseconds
     * @since 2.12.0
     */
    public int getHeartbeatSocketTimeout() {
        return heartbeatSocketTimeout;
    }

    /**
     * Gets the heartbeat thread count.  This is the number of threads that will be used to monitor the SequoiaDB servers that the
     * SdbClient is connected to.
     *
     * <p>
     * The default value is the number of servers in the seed list.
     * </p>
     * @return the heartbeat thread count
     * @since 2.12.0
     */
    public int getHeartbeatThreadCount() {
        return heartbeatThreadCount;
    }

    /**
     * Gets the acceptable latency difference.  When choosing among multiple SequoiaDB servers to send a request,
     * the SdbClient will only send that request to a server whose ping time is less than or equal to the server with the fastest ping
     * time plus the acceptable latency difference.
     * <p>
     * For example, let's say that the client is choosing a server to send a query when
     * the read preference is {@code ReadPreference.secondary()}, and that there are three secondaries, server1, server2, and server3,
     * whose ping times are 10, 15, and 16 milliseconds, respectively.  With an acceptable latency difference of 5 milliseconds,
     * the client will send the query to either server1 or server2 (randomly selecting between the two).
     * </p>
     * <p>
     * The default value is 15 milliseconds.
     * </p>
     *

     * @return the acceptable latency difference, in milliseconds
     * @since 2.12.0
     */
    public int getAcceptableLatencyDifference() {
        return acceptableLatencyDifference;
    }

    /**
     * Gets the required replica set name.  With this option set, the SdbClient instance will
     * <p> 1. Connect in replica set mode, and discover all members of the set based on the given servers
     * </p>
     * <p> 2. Make sure that the set name reported by all members matches the required set name.
     * </p>
     * <p> 3. Refuse to service any requests if any member of the seed list is not part of a replica set with the required name.
     * </p>
     *
     * @return the required replica set name
     * @since 2.12
     */
    public String getRequiredReplicaSetName() {
        return requiredReplicaSetName;
    }

    @Override
    public boolean equals(final Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        final SdbClientOptions that = (SdbClientOptions) o;

        if (acceptableLatencyDifference != that.acceptableLatencyDifference) {
            return false;
        }
        if (alwaysUseMBeans != that.alwaysUseMBeans) {
            return false;
        }
        if (autoConnectRetry != that.autoConnectRetry) {
            return false;
        }
        if (connectTimeout != that.connectTimeout) {
            return false;
        }
        if (connectionsPerHost != that.connectionsPerHost) {
            return false;
        }
        if (cursorFinalizerEnabled != that.cursorFinalizerEnabled) {
            return false;
        }
        if (heartbeatConnectRetryFrequency != that.heartbeatConnectRetryFrequency) {
            return false;
        }
        if (heartbeatConnectTimeout != that.heartbeatConnectTimeout) {
            return false;
        }
        if (heartbeatFrequency != that.heartbeatFrequency) {
            return false;
        }
        if (heartbeatSocketTimeout != that.heartbeatSocketTimeout) {
            return false;
        }
        if (heartbeatThreadCount != that.heartbeatThreadCount) {
            return false;
        }
        if (maxAutoConnectRetryTime != that.maxAutoConnectRetryTime) {
            return false;
        }
        if (maxConnectionIdleTime != that.maxConnectionIdleTime) {
            return false;
        }
        if (maxConnectionLifeTime != that.maxConnectionLifeTime) {
            return false;
        }
        if (maxWaitTime != that.maxWaitTime) {
            return false;
        }
        if (minConnectionsPerHost != that.minConnectionsPerHost) {
            return false;
        }
        if (socketKeepAlive != that.socketKeepAlive) {
            return false;
        }
        if (socketTimeout != that.socketTimeout) {
            return false;
        }
        if (threadsAllowedToBlockForConnectionMultiplier != that.threadsAllowedToBlockForConnectionMultiplier) {
            return false;
        }
        if (!dbDecoderFactory.equals(that.dbDecoderFactory)) {
            return false;
        }
        if (!dbEncoderFactory.equals(that.dbEncoderFactory)) {
            return false;
        }
        if (description != null ? !description.equals(that.description) : that.description != null) {
            return false;
        }
        if (!readPreference.equals(that.readPreference)) {
            return false;
        }
        if (!socketFactory.getClass().equals(that.socketFactory.getClass())) {
            return false;
        }
        if (!writeConcern.equals(that.writeConcern)) {
            return false;
        }
        if (requiredReplicaSetName != null ? !requiredReplicaSetName.equals(that.requiredReplicaSetName)
                : that.requiredReplicaSetName != null) {
            return false;
        }

        return true;
    }

    @Override
    public int hashCode() {
        int result = description != null ? description.hashCode() : 0;
        result = 31 * result + minConnectionsPerHost;
        result = 31 * result + connectionsPerHost;
        result = 31 * result + threadsAllowedToBlockForConnectionMultiplier;
        result = 31 * result + maxWaitTime;
        result = 31 * result + maxConnectionIdleTime;
        result = 31 * result + maxConnectionLifeTime;
        result = 31 * result + connectTimeout;
        result = 31 * result + socketTimeout;
        result = 31 * result + (socketKeepAlive ? 1 : 0);
        result = 31 * result + (autoConnectRetry ? 1 : 0);
        result = 31 * result + (int) (maxAutoConnectRetryTime ^ (maxAutoConnectRetryTime >>> 32));
        result = 31 * result + readPreference.hashCode();
        result = 31 * result + dbDecoderFactory.hashCode();
        result = 31 * result + dbEncoderFactory.hashCode();
        result = 31 * result + writeConcern.hashCode();
        result = 31 * result + (socketFactory != null ? socketFactory.getClass().hashCode() : 0);
        result = 31 * result + (cursorFinalizerEnabled ? 1 : 0);
        result = 31 * result + (alwaysUseMBeans ? 1 : 0);
        result = 31 * result + heartbeatFrequency;
        result = 31 * result + heartbeatConnectRetryFrequency;
        result = 31 * result + heartbeatConnectTimeout;
        result = 31 * result + heartbeatSocketTimeout;
        result = 31 * result + heartbeatThreadCount;
        result = 31 * result + acceptableLatencyDifference;
        result = 31 * result + (requiredReplicaSetName != null ? requiredReplicaSetName.hashCode() : 0);
        return result;
    }

    @Override
    public String toString() {
        return "SdbClientOptions{"
                + "description='" + description + '\''
                + ", connectionsPerHost=" + connectionsPerHost
                + ", threadsAllowedToBlockForConnectionMultiplier=" + threadsAllowedToBlockForConnectionMultiplier
                + ", maxWaitTime=" + maxWaitTime
                + ", connectTimeout=" + connectTimeout
                + ", socketTimeout=" + socketTimeout
                + ", socketKeepAlive=" + socketKeepAlive
                + ", autoConnectRetry=" + autoConnectRetry
                + ", maxAutoConnectRetryTime=" + maxAutoConnectRetryTime
                + ", readPreference=" + readPreference
                + ", dbDecoderFactory=" + dbDecoderFactory
                + ", dbEncoderFactory=" + dbEncoderFactory
                + ", writeConcern=" + writeConcern
                + ", socketFactory=" + socketFactory
                + ", cursorFinalizerEnabled=" + cursorFinalizerEnabled
                + ", alwaysUseMBeans=" + alwaysUseMBeans
                + ", heartbeatFrequency=" + heartbeatFrequency
                + ", heartbeatConnectRetryFrequency=" + heartbeatConnectRetryFrequency
                + ", heartbeatConnectTimeout=" + heartbeatConnectTimeout
                + ", heartbeatSocketTimeout=" + heartbeatSocketTimeout
                + ", heartbeatThreadCount=" + heartbeatThreadCount
                + ", acceptableLatencyDifference=" + acceptableLatencyDifference
                + ", requiredReplicaSetName=" + requiredReplicaSetName
                + '}';
    }

    private SdbClientOptions(final Builder builder) {
        description = builder.description;
        minConnectionsPerHost = builder.minConnectionsPerHost;
        connectionsPerHost = builder.connectionsPerHost;
        threadsAllowedToBlockForConnectionMultiplier = builder.threadsAllowedToBlockForConnectionMultiplier;
        maxWaitTime = builder.maxWaitTime;
        maxConnectionIdleTime = builder.maxConnectionIdleTime;
        maxConnectionLifeTime = builder.maxConnectionLifeTime;
        connectTimeout = builder.connectTimeout;
        socketTimeout = builder.socketTimeout;
        autoConnectRetry = builder.autoConnectRetry;
        socketKeepAlive = builder.socketKeepAlive;
        maxAutoConnectRetryTime = builder.maxAutoConnectRetryTime;
        readPreference = builder.readPreference;
        dbDecoderFactory = builder.dbDecoderFactory;
        dbEncoderFactory = builder.dbEncoderFactory;
        writeConcern = builder.writeConcern;
        socketFactory = builder.socketFactory;
        cursorFinalizerEnabled = builder.cursorFinalizerEnabled;
        alwaysUseMBeans = builder.alwaysUseMBeans;
        heartbeatFrequency = builder.heartbeatFrequency;
        heartbeatConnectRetryFrequency = builder.heartbeatConnectRetryFrequency;
        heartbeatConnectTimeout = builder.heartbeatConnectTimeout;
        heartbeatSocketTimeout = builder.heartbeatSocketTimeout;
        heartbeatThreadCount = builder.heartbeatThreadCount;
        acceptableLatencyDifference = builder.acceptableLatencyDifference;
        requiredReplicaSetName = builder.requiredReplicaSetName;
    }


    private final String description;
    private final int minConnectionsPerHost;
    private final int connectionsPerHost;
    private final int threadsAllowedToBlockForConnectionMultiplier;
    private final int maxWaitTime;
    private final int maxConnectionIdleTime;
    private final int maxConnectionLifeTime;
    private final int connectTimeout;
    private final int socketTimeout;
    private final boolean socketKeepAlive;
    private final boolean autoConnectRetry;
    private final long maxAutoConnectRetryTime;
    private final ReadPreference readPreference;
    private final DBDecoderFactory dbDecoderFactory;
    private final DBEncoderFactory dbEncoderFactory;
    private final WriteConcern writeConcern;
    private final SocketFactory socketFactory;
    private final boolean cursorFinalizerEnabled;
    private final boolean alwaysUseMBeans;
    private final int heartbeatFrequency;
    private final int heartbeatConnectRetryFrequency;
    private final int heartbeatConnectTimeout;
    private final int heartbeatSocketTimeout;
    private final int heartbeatThreadCount;
    private final int acceptableLatencyDifference;
    private final String requiredReplicaSetName;
}

