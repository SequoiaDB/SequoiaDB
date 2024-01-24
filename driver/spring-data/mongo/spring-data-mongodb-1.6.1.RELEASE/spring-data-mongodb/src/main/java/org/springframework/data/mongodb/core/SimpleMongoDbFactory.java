/*
 * Copyright 2011-2013 the original author or authors.
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

import java.net.UnknownHostException;

import org.springframework.beans.factory.DisposableBean;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.DB;
import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoException;
import org.springframework.data.mongodb.assist.MongoURI;
import org.springframework.data.mongodb.assist.WriteConcern;

/**
 * Factory to create {@link DB} instances from a {@link Mongo} instance.
 * 
 * @author Mark Pollack
 * @author Oliver Gierke
 * @author Thomas Darimont
 */
public class SimpleMongoDbFactory implements DisposableBean, MongoDbFactory {

	private final Mongo mongo;
	private final String databaseName;
	private final boolean mongoInstanceCreated;
	private final UserCredentials credentials;
	private final PersistenceExceptionTranslator exceptionTranslator;
	private final String authenticationDatabaseName;

	private WriteConcern writeConcern;

	/**
	 * Create an instance of {@link SimpleMongoDbFactory} given the {@link Mongo} instance and database name.
	 * 
	 * @param mongo Mongo instance, must not be {@literal null}.
	 * @param databaseName database name, not be {@literal null} or empty.
	 */
	public SimpleMongoDbFactory(Mongo mongo, String databaseName) {
		this(mongo, databaseName, null);
	}

	/**
	 * Create an instance of SimpleMongoDbFactory given the Mongo instance, database name, and username/password
	 * 
	 * @param mongo Mongo instance, must not be {@literal null}.
	 * @param databaseName Database name, must not be {@literal null} or empty.
	 * @param credentials username and password.
	 */
	public SimpleMongoDbFactory(Mongo mongo, String databaseName, UserCredentials credentials) {
		this(mongo, databaseName, credentials, false, null);
	}

	/**
	 * Create an instance of SimpleMongoDbFactory given the Mongo instance, database name, and username/password
	 * 
	 * @param mongo Mongo instance, must not be {@literal null}.
	 * @param databaseName Database name, must not be {@literal null} or empty.
	 * @param credentials username and password.
	 * @param authenticationDatabaseName the database name to use for authentication
	 */
	public SimpleMongoDbFactory(Mongo mongo, String databaseName, UserCredentials credentials,
			String authenticationDatabaseName) {
		this(mongo, databaseName, credentials, false, authenticationDatabaseName);
	}

	/**
	 * Creates a new {@link SimpleMongoDbFactory} instance from the given {@link MongoURI}.
	 * 
	 * @param uri must not be {@literal null}.
	 * @throws MongoException
	 * @throws UnknownHostException
	 * @see MongoURI
	 */
	@SuppressWarnings("deprecation")
	public SimpleMongoDbFactory(MongoURI uri) throws MongoException, UnknownHostException {
		throw new UnsupportedOperationException("not supported for using uri");
	}

	private SimpleMongoDbFactory(Mongo mongo, String databaseName, UserCredentials credentials,
			boolean mongoInstanceCreated, String authenticationDatabaseName) {

		Assert.notNull(mongo, "Mongo must not be null");
		Assert.hasText(databaseName, "Database name must not be empty");
		Assert.isTrue(databaseName.matches("[\\w-]+"),
				"Database name must only contain letters, numbers, underscores and dashes!");

		this.mongo = mongo;
		this.databaseName = databaseName;
		this.mongoInstanceCreated = mongoInstanceCreated;
		this.credentials = credentials == null ? UserCredentials.NO_CREDENTIALS : credentials;
		this.exceptionTranslator = new MongoExceptionTranslator();
		this.authenticationDatabaseName = StringUtils.hasText(authenticationDatabaseName) ? authenticationDatabaseName
				: databaseName;

		Assert.isTrue(this.authenticationDatabaseName.matches("[\\w-]+"),
				"Authentication database name must only contain letters, numbers, underscores and dashes!");
	}

	/**
	 * Configures the {@link WriteConcern} to be used on the {@link DB} instance being created.
	 * 
	 * @param writeConcern the writeConcern to set
	 */
	public void setWriteConcern(WriteConcern writeConcern) {
		this.writeConcern = writeConcern;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.MongoDbFactory#getDb()
	 */
	public DB getDb() throws DataAccessException {
		return getDb(databaseName);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.MongoDbFactory#getDb(java.lang.String)
	 */
	public DB getDb(String dbName) throws DataAccessException {

		Assert.hasText(dbName, "Database name must not be empty.");

		DB db = MongoDbUtils.getDB(mongo, dbName, credentials, authenticationDatabaseName);

		if (writeConcern != null) {
			db.setWriteConcern(writeConcern);
		}

		return db;
	}

	/**
	 * Clean up the Mongo instance if it was created by the factory itself.
	 * 
	 * @see DisposableBean#destroy()
	 */
	public void destroy() throws Exception {
		if (mongoInstanceCreated) {
			mongo.close();
		}
	}

	private static String parseChars(char[] chars) {
		return chars == null ? null : String.valueOf(chars);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.MongoDbFactory#getExceptionTranslator()
	 */
	@Override
	public PersistenceExceptionTranslator getExceptionTranslator() {
		return this.exceptionTranslator;
	}
}
