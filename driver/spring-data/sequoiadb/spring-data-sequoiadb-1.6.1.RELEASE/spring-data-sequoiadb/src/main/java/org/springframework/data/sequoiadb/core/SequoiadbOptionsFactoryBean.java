/*
 * Copyright 2010-2014 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.core;

import javax.net.ssl.SSLSocketFactory;

import org.springframework.beans.factory.FactoryBean;
import org.springframework.beans.factory.InitializingBean;

import org.springframework.data.sequoiadb.assist.SequoiadbOptions;

/**
 * A factory bean for construction of a {@link SequoiadbOptions} instance.
 *
 */
@SuppressWarnings("deprecation")
public class SequoiadbOptionsFactoryBean implements FactoryBean<SequoiadbOptions>, InitializingBean {

	private static final SequoiadbOptions DEFAULT_SEQUOIADB_OPTIONS = new SequoiadbOptions();

	private int connectionsPerHost = DEFAULT_SEQUOIADB_OPTIONS.connectionsPerHost;
	private int threadsAllowedToBlockForConnectionMultiplier = DEFAULT_SEQUOIADB_OPTIONS.threadsAllowedToBlockForConnectionMultiplier;
	private int maxWaitTime = DEFAULT_SEQUOIADB_OPTIONS.maxWaitTime;
	private int connectTimeout = DEFAULT_SEQUOIADB_OPTIONS.connectTimeout;
	private int socketTimeout = DEFAULT_SEQUOIADB_OPTIONS.socketTimeout;
	private boolean socketKeepAlive = DEFAULT_SEQUOIADB_OPTIONS.socketKeepAlive;
	private boolean autoConnectRetry = DEFAULT_SEQUOIADB_OPTIONS.autoConnectRetry;
	private long maxAutoConnectRetryTime = DEFAULT_SEQUOIADB_OPTIONS.maxAutoConnectRetryTime;
	private int writeNumber = DEFAULT_SEQUOIADB_OPTIONS.w;
	private int writeTimeout = DEFAULT_SEQUOIADB_OPTIONS.wtimeout;
	private boolean writeFsync = DEFAULT_SEQUOIADB_OPTIONS.fsync;
	private boolean slaveOk = DEFAULT_SEQUOIADB_OPTIONS.slaveOk;
	private boolean ssl;
	private SSLSocketFactory sslSocketFactory;

	private SequoiadbOptions options;

	/**
	 * Configures the maximum number of connections allowed per host until we will block.
	 * 
	 * @param connectionsPerHost
	 */
	public void setConnectionsPerHost(int connectionsPerHost) {
		this.connectionsPerHost = connectionsPerHost;
	}

	/**
	 * A multiplier for connectionsPerHost for # of threads that can block a connection. If connectionsPerHost is 10, and
	 * threadsAllowedToBlockForConnectionMultiplier is 5, then 50 threads can block. If more threads try to block an
	 * exception will be thrown.
	 * 
	 * @param threadsAllowedToBlockForConnectionMultiplier
	 */
	public void setThreadsAllowedToBlockForConnectionMultiplier(int threadsAllowedToBlockForConnectionMultiplier) {
		this.threadsAllowedToBlockForConnectionMultiplier = threadsAllowedToBlockForConnectionMultiplier;
	}

	/**
	 * Max wait time of a blocking thread for a connection.
	 * 
	 * @param maxWaitTime
	 */
	public void setMaxWaitTime(int maxWaitTime) {
		this.maxWaitTime = maxWaitTime;
	}

	/**
	 * Configures the connect timeout in milliseconds. Defaults to 0 (infinite time).
	 * 
	 * @param connectTimeout
	 */
	public void setConnectTimeout(int connectTimeout) {
		this.connectTimeout = connectTimeout;
	}

	/**
	 * Configures the socket timeout. Defaults to 0 (infinite time).
	 * 
	 * @param socketTimeout
	 */
	public void setSocketTimeout(int socketTimeout) {
		this.socketTimeout = socketTimeout;
	}

	/**
	 * Configures whether or not to have socket keep alive turned on (SO_KEEPALIVE). Defaults to {@literal false}.
	 * 
	 * @param socketKeepAlive
	 */
	public void setSocketKeepAlive(boolean socketKeepAlive) {
		this.socketKeepAlive = socketKeepAlive;
	}

	/**
	 * This specifies the number of servers to wait for on the write operation, and exception raising behavior. The 'w'
	 * option to the getlasterror command. Defaults to 0.
	 * <ul>
	 * <li>-1 = don't even report network errors</li>
	 * <li>0 = default, don't call getLastError by default</li>
	 * <li>1 = basic, call getLastError, but don't wait for slaves</li>
	 * <li>2 += wait for slaves</li>
	 * </ul>
	 * 
	 * @param writeNumber the number of servers to wait for on the write operation, and exception raising behavior.
	 */
	public void setWriteNumber(int writeNumber) {
		this.writeNumber = writeNumber;
	}

	/**
	 * Configures the timeout for write operations in milliseconds. This defaults to {@literal 0} (indefinite).
	 * 
	 * @param writeTimeout
	 */
	public void setWriteTimeout(int writeTimeout) {
		this.writeTimeout = writeTimeout;
	}

	/**
	 * Configures whether or not to fsync. The 'fsync' option to the getlasterror command. Defaults to {@literal false}.
	 * 
	 * @param writeFsync to fsync on <code>write (true)<code>, otherwise {@literal false}.
	 */
	public void setWriteFsync(boolean writeFsync) {
		this.writeFsync = writeFsync;
	}

	/**
	 * Configures whether or not the system retries automatically on a failed connect. This defaults to {@literal false}.
	 */
	public void setAutoConnectRetry(boolean autoConnectRetry) {
		this.autoConnectRetry = autoConnectRetry;
	}

	/**
	 * Configures the maximum amount of time in millisecons to spend retrying to open connection to the same server. This
	 * defaults to {@literal 0}, which means to use the default {@literal 15s} if {@link #autoConnectRetry} is on.
	 * 
	 * @param maxAutoConnectRetryTime the maxAutoConnectRetryTime to set
	 */
	public void setMaxAutoConnectRetryTime(long maxAutoConnectRetryTime) {
		this.maxAutoConnectRetryTime = maxAutoConnectRetryTime;
	}

	/**
	 * Specifies if the driver is allowed to read from secondaries or slaves. Defaults to {@literal false}.
	 * 
	 * @param slaveOk true if the driver should read from secondaries or slaves.
	 */
	public void setSlaveOk(boolean slaveOk) {
		this.slaveOk = slaveOk;
	}

	/**
	 * Specifies if the driver should use an SSL connection to Sdb. This defaults to {@literal false}. By default
	 * {@link SSLSocketFactory#getDefault()} will be used. See {@link #setSslSocketFactory(SSLSocketFactory)} if you want
	 * to configure a custom factory.
	 * 
	 * @param ssl true if the driver should use an SSL connection.
	 * @see #setSslSocketFactory(SSLSocketFactory)
	 */
	public void setSsl(boolean ssl) {
		this.ssl = ssl;
	}

	/**
	 * Specifies the {@link SSLSocketFactory} to use for creating SSL connections to Sdb. Defaults to
	 * {@link SSLSocketFactory#getDefault()}. Implicitly activates {@link #setSsl(boolean)} if a non-{@literal null} value
	 * is given.
	 * 
	 * @param sslSocketFactory the sslSocketFactory to use.
	 * @see #setSsl(boolean)
	 */
	public void setSslSocketFactory(SSLSocketFactory sslSocketFactory) {

		setSsl(sslSocketFactory != null);
		this.sslSocketFactory = sslSocketFactory;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.InitializingBean#afterPropertiesSet()
	 */
	public void afterPropertiesSet() {

		SequoiadbOptions options = new SequoiadbOptions();

		options.connectionsPerHost = connectionsPerHost;
		options.threadsAllowedToBlockForConnectionMultiplier = threadsAllowedToBlockForConnectionMultiplier;
		options.maxWaitTime = maxWaitTime;
		options.connectTimeout = connectTimeout;
		options.socketTimeout = socketTimeout;
		options.socketKeepAlive = socketKeepAlive;
		options.autoConnectRetry = autoConnectRetry;
		options.maxAutoConnectRetryTime = maxAutoConnectRetryTime;
		options.slaveOk = slaveOk;
		options.w = writeNumber;
		options.wtimeout = writeTimeout;
		options.fsync = writeFsync;

		if (ssl) {
			options.setSocketFactory(sslSocketFactory != null ? sslSocketFactory : SSLSocketFactory.getDefault());
		}

		this.options = options;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#getObject()
	 */
	public SequoiadbOptions getObject() {
		return this.options;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#getObjectType()
	 */
	public Class<?> getObjectType() {
		return SequoiadbOptions.class;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#isSingleton()
	 */
	public boolean isSingleton() {
		return true;
	}
}
