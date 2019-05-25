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
package org.springframework.data.mongodb.core;

import javax.net.ssl.SSLSocketFactory;

import com.sequoiadb.datasource.ConnectStrategy;
import org.springframework.beans.factory.FactoryBean;
import org.springframework.beans.factory.InitializingBean;

import org.springframework.data.mongodb.assist.MongoOptions;

import java.util.ArrayList;
import java.util.List;

/**
 * A factory bean for construction of a {@link MongoOptions} instance.
 *
 * @author Graeme Rocher
 * @author Mark Pollack
 * @author Mike Saavedra
 * @author Thomas Darimont
 */
@SuppressWarnings("deprecation")
public class MongoOptionsFactoryBean implements FactoryBean<MongoOptions>, InitializingBean {

	private static final String DEFAULT_PREFERRD_INSTANCE_MODE = "ordered";
	private static final int DEFAULT_SESSION_TIMEOUT = -1;
	private static final MongoOptions DEFAULT_MONGO_OPTIONS = new MongoOptions.Builder().build();
	private MongoOptions options;

	private int connectTimeout = DEFAULT_MONGO_OPTIONS.getConnectTimeout();
	private int socketTimeout = DEFAULT_MONGO_OPTIONS.getSocketTimeout();
	private boolean socketKeepAlive = DEFAULT_MONGO_OPTIONS.isSocketKeepAlive();
	private boolean useNagle = DEFAULT_MONGO_OPTIONS.isUseNagle();
	private boolean useSSL = DEFAULT_MONGO_OPTIONS.isUseSSL();

	private int deltaIncCount = DEFAULT_MONGO_OPTIONS.deltaIncCount();
	private int maxIdleCount = DEFAULT_MONGO_OPTIONS.maxIdleCount();
	private int maxCount = DEFAULT_MONGO_OPTIONS.maxCount();
	private int keepAliveTimeout = DEFAULT_MONGO_OPTIONS.keepAliveTimeout();
	private int checkInterval = DEFAULT_MONGO_OPTIONS.checkInterval();
	private int syncCoordInterval = DEFAULT_MONGO_OPTIONS.syncCoordInterval();
	private boolean validateConnection = DEFAULT_MONGO_OPTIONS.isValidateConnection();
	private ConnectStrategy connectStrategy = ConnectStrategy.SERIAL;

	private List<String> preferedInstance = DEFAULT_MONGO_OPTIONS.getPreferedInstance();
	private String preferedInstanceMode = DEFAULT_MONGO_OPTIONS.getPreferedInstanceMode();
	private int sessionTimeout = DEFAULT_MONGO_OPTIONS.getSessionTimeout();



	/**
	 * Option for Connection. The connection timeout in milliseconds. A timeout of zero is interpreted as an infinite timeout.
	 * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
	 * <p/>
	 * Default is 10,000.
	 *
	 * @return the socket connect timeout
	 */
	public void setConnectTimeout(final int connectTimeout) {
		if (connectTimeout < 0) {
			throw new IllegalArgumentException("Minimum value is 0");
		}
		this.connectTimeout = connectTimeout;
	}

	/**
	 * Option for Connection. The socket timeout in milliseconds.
	 * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
	 * <p/>
	 * Default is 0 and means no timeout.
	 * @param socketTimeout the socket timeout in milliseconds.
	 * @return
	 */
	public void setSocketTimeout(final int socketTimeout) {
		if (socketTimeout < 0) {
			throw new IllegalArgumentException("Minimum value is 0");
		}
		this.socketTimeout = socketTimeout;
	}

	/**
	 * Option for Connection. Enable/disable SO_KEEPALIVE.
	 *
	 * @param on     whether or not to have socket keep alive turned on, default to be false.
	 */
	public void setSocketKeepAlive(final boolean on) {
		this.socketKeepAlive = on;
	}

	/**
	 * Option for Connection. Enable/disable Nagle's algorithm(disable/enable TCP_NODELAY)
	 *
	 * @param on <code>true</code> to disable TCP_NODELAY,
	 * <code>false</code> to enable, default to be false and going to use enable TCP_NODELAY.
	 */
	public void setUseNagle(final boolean on) {
		this.useNagle = on;
	}

	/**
	 * Option for Connection. Set whether use the SSL or not.
	 *
	 * @param on <code>true</code> for using,
	 * <code>false</code> for not.
	 */
	public void setUseSSL(final boolean on) {
		this.useSSL = on;
	}

	/**
	 * Option for Datasource.
	 * Set the number of new connections to create once running out the idle connections
	 * @param deltaIncCount Default to be 10.
	 * @return
	 */
	public void setDeltaIncCount(final int deltaIncCount) {
		this.deltaIncCount = deltaIncCount;
	}

	/**
	 * Option for Datasource.
	 * Set the max number of the idle connection left in connection
	 * pool after periodically cleaning.
	 * @param maxIdleCount Default to be 10.
	 * @since 2.2
	 */
	public void setMaxIdleCount(final int maxIdleCount) {
		this.maxIdleCount = maxIdleCount;
	}

	/**
	 * Option for Datasource.
	 * Set the capacity of the connection pool.
	 * When maxCount is set to 0, the connection pool will be disabled.
	 * @param maxCount Default to be 500.
	 * @since 2.2
	 */
	public void setMaxCount(final int maxCount) {
		this.maxCount = maxCount;
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
	public void setKeepAliveTimeout(final int keepAliveTimeout) {
		this.keepAliveTimeout = keepAliveTimeout;
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
	public void setCheckInterval(final int checkInterval) {
		this.checkInterval = checkInterval;
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
	public void setSyncCoordInterval(final int syncCoordInterval) {
		this.syncCoordInterval = syncCoordInterval;
	}

	/**
	 * Option for Datasource.
	 * When a idle connection is got out of pool, we need
	 * to validate whether it can be used or not.
	 * @param validateConnection Default to be false.
	 * @since 2.2
	 */
	public void setValidateConnection(final boolean validateConnection ) {
		this.validateConnection = validateConnection;
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
	public void setConnectStrategy(final ConnectStrategy connectStrategy) {
		this.connectStrategy = connectStrategy;
	}

	public void setPreferedInstance(final List<String> instance) {
		if (instance == null || instance.size() == 0) {
			return ;
		}
		preferedInstance = new ArrayList<String>();
		for(String s : instance) {
			preferedInstance.add(s);
		}
	}

	public void setPreferedInstanceMode(final String mode) {
		if (mode == null || mode.isEmpty()) {
			preferedInstanceMode = DEFAULT_PREFERRD_INSTANCE_MODE;
		} else {
			preferedInstanceMode = mode;
		}
	}

	public void setSessionTimeout(final int timeout) {
		if (timeout < 0) {
			sessionTimeout = DEFAULT_SESSION_TIMEOUT;
		} else {
			sessionTimeout = timeout;
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.InitializingBean#afterPropertiesSet()
	 */
	public void afterPropertiesSet() {
		MongoOptions options = new MongoOptions.Builder()
				.connectTimeout(connectTimeout)
				.socketTimeout(socketTimeout)
				.socketKeepAlive(socketKeepAlive)
				.useNagle(useNagle)
				.useSSL(useSSL)
				.deltaIncCount(deltaIncCount)
				.maxIdleCount(maxIdleCount)
				.maxCount(maxCount)
				.keepAliveTimeout(keepAliveTimeout)
				.checkInterval(checkInterval)
				.syncCoordInterval(syncCoordInterval)
				.validateConnection(validateConnection)
				.connectStrategy(connectStrategy)
				.preferedInstance(preferedInstance)
				.preferedInstanceMode(preferedInstanceMode)
				.sessionTimeout(sessionTimeout).build();
		this.options = options;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#getObject()
	 */
	public MongoOptions getObject() {
		return this.options;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#getObjectType()
	 */
	public Class<?> getObjectType() {
		return MongoOptions.class;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#isSingleton()
	 */
	public boolean isSingleton() {
		return true;
	}
}
