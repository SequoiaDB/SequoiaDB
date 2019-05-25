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
package org.springframework.data.mongodb.core;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

import org.springframework.beans.factory.DisposableBean;
import org.springframework.beans.factory.FactoryBean;
import org.springframework.beans.factory.InitializingBean;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoOptions;
import org.springframework.data.mongodb.assist.ServerAddress;

/**
 * Convenient factory for configuring MongoDB.
 */
public class MongoFactoryBean implements FactoryBean<Mongo>, InitializingBean, DisposableBean,
		PersistenceExceptionTranslator {

	private static final int DEFAULT_COORD_PORT = 11810;
	private String host;
	private Integer port;
	private String userName;
	private String password;
	private MongoOptions mongoOptions;
	private List<ServerAddress> coordAddress;
	private Mongo mongo;

	private PersistenceExceptionTranslator exceptionTranslator = new MongoExceptionTranslator();

	public void setMongoOptions(MongoOptions mongoOptions) {
		this.mongoOptions = mongoOptions;
	}

	public void setCoordAddress(List<ServerAddress> coordAddress) {
		this.coordAddress = filterNonNullElementsAsList(coordAddress);
	}

	/**
	 * @param elements the elements to filter <T>
	 * @return a new unmodifiable {@link List#} from the given elements without nulls
	 */
	private <T> List<T> filterNonNullElementsAsList(List<T> elements) {

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

	public void setUserName(String userName) {
		this.userName = userName;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public void setExceptionTranslator(PersistenceExceptionTranslator exceptionTranslator) {
		this.exceptionTranslator = exceptionTranslator;
	}

	public Mongo getObject() throws Exception {
		return mongo;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.FactoryBean#getObjectType()
	 */
	public Class<? extends Mongo> getObjectType() {
		return Mongo.class;
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

		Mongo mongo;
		ServerAddress defaultOptions = new ServerAddress();

		if (mongoOptions == null) {
			mongoOptions = new MongoOptions.Builder().build();
		}

		if (!isNullOrEmpty(coordAddress)) {
			mongo = new Mongo(coordAddress, userName, password, mongoOptions);
		} else {
			String mongoHost = StringUtils.hasText(host) ? host : defaultOptions.getHost();
			if (port != null) {
				mongo = new Mongo(new ServerAddress(mongoHost, port), userName, password, mongoOptions);
			} else {
				mongo = new Mongo(mongoHost, DEFAULT_COORD_PORT, userName, password, mongoOptions);
			}
		}

		this.mongo = mongo;
	}

	private boolean isNullOrEmpty(Collection<?> elements) {
		return elements == null || elements.isEmpty();
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.DisposableBean#destroy()
	 */
	public void destroy() throws Exception {
		this.mongo.close();
	}
}
