/*
 * Copyright 2010-2013 the original author or authors.
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

import org.springframework.beans.factory.DisposableBean;
import org.springframework.beans.factory.FactoryBean;
import org.springframework.beans.factory.InitializingBean;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.data.sequoiadb.CannotGetSequoiadbConnectionException;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.SequoiadbOptions;
import org.springframework.util.StringUtils;

import org.springframework.data.sequoiadb.assist.ServerAddress;
import org.springframework.data.sequoiadb.assist.WriteConcern;

/**
 * Convenient factory for configuring SequoiaDB.
 * 




 * @since 1.0
 */
public class SequoiadbFactoryBean implements FactoryBean<Sdb>, InitializingBean, DisposableBean,
		PersistenceExceptionTranslator {

	private Sdb sdb;

	private SequoiadbOptions sequoiadbOptions;
	private String host;
	private Integer port;
	private WriteConcern writeConcern;
	private List<ServerAddress> replicaSetSeeds;
	private List<ServerAddress> replicaPair;

	private PersistenceExceptionTranslator exceptionTranslator = new SequoiadbExceptionTranslator();

	public void setSequoiadbOptions(SequoiadbOptions sequoiadbOptions) {
		this.sequoiadbOptions = sequoiadbOptions;
	}

	public void setReplicaSetSeeds(ServerAddress[] replicaSetSeeds) {
		this.replicaSetSeeds = filterNonNullElementsAsList(replicaSetSeeds);
	}

	/**
	 * @deprecated use {@link #setReplicaSetSeeds(ServerAddress[])} instead
	 * 
	 * @param replicaPair
	 */
	@Deprecated
	public void setReplicaPair(ServerAddress[] replicaPair) {
		this.replicaPair = filterNonNullElementsAsList(replicaPair);
	}

	/**
	 * @param elements the elements to filter <T>
	 * @return a new unmodifiable {@link List#} from the given elements without nulls
	 */
	private <T> List<T> filterNonNullElementsAsList(T[] elements) {

		if (elements == null) {
			return Collections.emptyList();
		}

		List<T> candidateElements = new ArrayList<T>();

		for (T element : elements) {
			if (element != null) {
				candidateElements.add(element);
			}
		}

		return Collections.unmodifiableList(candidateElements);
	}

	public void setHost(String host) {
		this.host = host;
	}

	public void setPort(int port) {
		this.port = port;
	}

	/**
	 * Sets the {@link WriteConcern} to be configured for the {@link Sdb} instance to be created.
	 * 
	 * @param writeConcern
	 */
	public void setWriteConcern(WriteConcern writeConcern) {
		this.writeConcern = writeConcern;
	}

	public void setExceptionTranslator(PersistenceExceptionTranslator exceptionTranslator) {
		this.exceptionTranslator = exceptionTranslator;
	}

	public Sdb getObject() throws Exception {
		return sdb;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#getObjectType()
	 */
	public Class<? extends Sdb> getObjectType() {
		return Sdb.class;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#isSingleton()
	 */
	public boolean isSingleton() {
		return true;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.dao.support.PersistenceExceptionTranslator#translateExceptionIfPossible(java.lang.RuntimeException)
	 */
	public DataAccessException translateExceptionIfPossible(RuntimeException ex) {
		return exceptionTranslator.translateExceptionIfPossible(ex);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.InitializingBean#afterPropertiesSet()
	 */
	@SuppressWarnings("deprecation")
	public void afterPropertiesSet() throws Exception {

		Sdb sdb;
		ServerAddress defaultOptions = new ServerAddress();

		if (sequoiadbOptions == null) {
			sequoiadbOptions = new SequoiadbOptions();
		}

		if (!isNullOrEmpty(replicaPair)) {
			if (replicaPair.size() < 2) {
				throw new CannotGetSequoiadbConnectionException("A replica pair must have two server entries");
			}
			sdb = new Sdb(replicaPair.get(0), replicaPair.get(1), sequoiadbOptions);
		} else if (!isNullOrEmpty(replicaSetSeeds)) {
			sdb = new Sdb(replicaSetSeeds, sequoiadbOptions);
		} else {
			String sdbHost = StringUtils.hasText(host) ? host : defaultOptions.getHost();
			sdb = port != null ? new Sdb(new ServerAddress(sdbHost, port), sequoiadbOptions) : new Sdb(sdbHost,
					sequoiadbOptions);
		}

		if (writeConcern != null) {
			sdb.setWriteConcern(writeConcern);
		}

		this.sdb = sdb;
	}

	private boolean isNullOrEmpty(Collection<?> elements) {
		return elements == null || elements.isEmpty();
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.DisposableBean#destroy()
	 */
	public void destroy() throws Exception {
		this.sdb.close();
	}
}
