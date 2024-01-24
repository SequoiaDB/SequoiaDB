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
package org.springframework.data.sequoiadb.core;

import java.net.UnknownHostException;

import com.sequoiadb.exception.BaseException;
import org.springframework.beans.factory.DisposableBean;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;



/**
 * Factory to create {@link DB} instances from a {@link Sdb} instance.
 * 



 */
public class SimpleSequoiadbFactory implements DisposableBean, SequoiadbFactory {

	private final Sdb sdb;
	private final String databaseName;
	private final boolean sequoiadbInstanceCreated;
	private final UserCredentials credentials;
	private final PersistenceExceptionTranslator exceptionTranslator;
	private final String authenticationDatabaseName;

	private WriteConcern writeConcern;

	/**
	 * Create an instance of {@link SimpleSequoiadbFactory} given the {@link Sdb} instance and database name.
	 * 
	 * @param sdb Sdb instance, must not be {@literal null}.
	 * @param databaseName database name, not be {@literal null} or empty.
	 */
	public SimpleSequoiadbFactory(Sdb sdb, String databaseName) {
		this(sdb, databaseName, null);
	}

	/**
	 * Create an instance of SimpleSequoiadbFactory given the Sdb instance, database name, and username/password
	 * 
	 * @param sdb Sdb instance, must not be {@literal null}.
	 * @param databaseName Database name, must not be {@literal null} or empty.
	 * @param credentials username and password.
	 */
	public SimpleSequoiadbFactory(Sdb sdb, String databaseName, UserCredentials credentials) {
		this(sdb, databaseName, credentials, false, null);
	}

	/**
	 * Create an instance of SimpleSequoiadbFactory given the Sdb instance, database name, and username/password
	 * 
	 * @param sdb Sdb instance, must not be {@literal null}.
	 * @param databaseName Database name, must not be {@literal null} or empty.
	 * @param credentials username and password.
	 * @param authenticationDatabaseName the database name to use for authentication
	 */
	public SimpleSequoiadbFactory(Sdb sdb, String databaseName, UserCredentials credentials,
								  String authenticationDatabaseName) {
		this(sdb, databaseName, credentials, false, authenticationDatabaseName);
	}

	/**
	 * Creates a new {@link SimpleSequoiadbFactory} instance from the given {@link SequoiadbURI}.
	 * 
	 * @param uri must not be {@literal null}.
	 * @throws BaseException
	 * @throws UnknownHostException
	 * @see SequoiadbURI
	 */
	@SuppressWarnings("deprecation")
	public SimpleSequoiadbFactory(SequoiadbURI uri) throws BaseException, UnknownHostException {
		this(new Sdb(uri), uri.getDatabase(), new UserCredentials(uri.getUsername(), parseChars(uri.getPassword())),
				true, uri.getDatabase());
	}

	private SimpleSequoiadbFactory(Sdb sdb, String databaseName, UserCredentials credentials,
								   boolean sequoiadbInstanceCreated, String authenticationDatabaseName) {

		Assert.notNull(sdb, "Sdb must not be null");
		Assert.hasText(databaseName, "Database name must not be empty");
		Assert.isTrue(databaseName.matches("[\\w-]+"),
				"Database name must only contain letters, numbers, underscores and dashes!");

		this.sdb = sdb;
		this.databaseName = databaseName;
		this.sequoiadbInstanceCreated = sequoiadbInstanceCreated;
		this.credentials = credentials == null ? UserCredentials.NO_CREDENTIALS : credentials;
		this.exceptionTranslator = new SequoiadbExceptionTranslator();
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
	 * @see org.springframework.data.sequoiadb.SequoiadbFactory#getDb()
	 */
	public DB getDb() throws DataAccessException {
		return getDb(databaseName);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.SequoiadbFactory#getDb(java.lang.String)
	 */
	public DB getDb(String dbName) throws DataAccessException {

		Assert.hasText(dbName, "Database name must not be empty.");

		DB db = SequoiadbUtils.getDB(sdb, dbName, credentials, authenticationDatabaseName);

		if (writeConcern != null) {
			db.setWriteConcern(writeConcern);
		}

		return db;
	}

	/**
	 * Clean up the Sdb instance if it was created by the factory itself.
	 * 
	 * @see DisposableBean#destroy()
	 */
	public void destroy() throws Exception {
		if (sequoiadbInstanceCreated) {
			sdb.close();
		}
	}

	private static String parseChars(char[] chars) {
		return chars == null ? null : String.valueOf(chars);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.SequoiadbFactory#getExceptionTranslator()
	 */
	@Override
	public PersistenceExceptionTranslator getExceptionTranslator() {
		return this.exceptionTranslator;
	}
}
