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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.mongodb.CannotGetMongoDbConnectionException;
import org.springframework.transaction.support.TransactionSynchronizationManager;
import org.springframework.util.Assert;

import org.springframework.data.mongodb.assist.DB;
import org.springframework.data.mongodb.assist.Mongo;

/**
 * Helper class featuring helper methods for internal MongoDb classes. Mainly intended for internal use within the
 * framework.
 * 
 * @author Thomas Risberg
 * @author Graeme Rocher
 * @author Oliver Gierke
 * @author Randy Watler
 * @author Thomas Darimont
 * @since 1.0
 */
public abstract class MongoDbUtils {

	private static final Logger LOGGER = LoggerFactory.getLogger(MongoDbUtils.class);

	/**
	 * Private constructor to prevent instantiation.
	 */
	private MongoDbUtils() {

	}

	/**
	 * Obtains a {@link DB} connection for the given {@link Mongo} instance and database name
	 * 
	 * @param mongo the {@link Mongo} instance, must not be {@literal null}.
	 * @param databaseName the database name, must not be {@literal null} or empty.
	 * @return the {@link DB} connection
	 */
	public static DB getDB(Mongo mongo, String databaseName) {
		return doGetDB(mongo, databaseName, UserCredentials.NO_CREDENTIALS, true, databaseName);
	}

	/**
	 * Obtains a {@link DB} connection for the given {@link Mongo} instance and database name
	 * 
	 * @param mongo the {@link Mongo} instance, must not be {@literal null}.
	 * @param databaseName the database name, must not be {@literal null} or empty.
	 * @param credentials the credentials to use, must not be {@literal null}.
	 * @return the {@link DB} connection
	 */
	public static DB getDB(Mongo mongo, String databaseName, UserCredentials credentials) {
		return getDB(mongo, databaseName, credentials, databaseName);
	}

	public static DB getDB(Mongo mongo, String databaseName, UserCredentials credentials,
			String authenticationDatabaseName) {

		Assert.notNull(mongo, "No Mongo instance specified!");
		Assert.hasText(databaseName, "Database name must be given!");
		Assert.notNull(credentials, "Credentials must not be null, use UserCredentials.NO_CREDENTIALS!");
		Assert.hasText(authenticationDatabaseName, "Authentication database name must not be null or empty!");

		return doGetDB(mongo, databaseName, credentials, true, authenticationDatabaseName);
	}

	private static DB doGetDB(Mongo mongo, String databaseName, UserCredentials credentials, boolean allowCreate,
			String authenticationDatabaseName) {

		DbHolder dbHolder = (DbHolder) TransactionSynchronizationManager.getResource(mongo);

		if (dbHolder != null && !dbHolder.isEmpty() && TransactionSynchronizationManager.isSynchronizationActive()) {

			DB db = dbHolder.getDB(databaseName);

			if (db != null && !dbHolder.isSynchronizedWithTransaction()) {

				LOGGER.debug("Registering Spring transaction synchronization for existing MongoDB {}.", databaseName);

				TransactionSynchronizationManager.registerSynchronization(new MongoSynchronization(dbHolder, mongo));
				dbHolder.setSynchronizedWithTransaction(true);
			}

			if (db != null) {
				return db;
			}
		}

		LOGGER.debug("Getting Mongo Database name=[{}]", databaseName);

		DB db = mongo.getDB(databaseName);
		boolean credentialsGiven = credentials.hasUsername() && credentials.hasPassword();

		DB authDb = databaseName.equals(authenticationDatabaseName) ? db : mongo.getDB(authenticationDatabaseName);

		synchronized (authDb) {

			if (credentialsGiven && !authDb.isAuthenticated()) {

				String username = credentials.getUsername();
				String password = credentials.hasPassword() ? credentials.getPassword() : null;

				if (!authDb.authenticate(username, password == null ? null : password.toCharArray())) {
					throw new CannotGetMongoDbConnectionException("Failed to authenticate to database [" + databaseName + "], "
							+ credentials.toString(), databaseName, credentials);
				}
			}
		}

		if (TransactionSynchronizationManager.isSynchronizationActive()) {

			LOGGER.debug("Registering Spring transaction synchronization for MongoDB instance {}.", databaseName);

			DbHolder holderToUse = dbHolder;

			if (holderToUse == null) {
				holderToUse = new DbHolder(databaseName, db);
			} else {
				holderToUse.addDB(databaseName, db);
			}

			if (!holderToUse.isSynchronizedWithTransaction()) {
				TransactionSynchronizationManager.registerSynchronization(new MongoSynchronization(holderToUse, mongo));
				holderToUse.setSynchronizedWithTransaction(true);
			}

			if (holderToUse != dbHolder) {
				TransactionSynchronizationManager.bindResource(mongo, holderToUse);
			}
		}

		if (!allowCreate && !isDBTransactional(db, mongo)) {
			throw new IllegalStateException("No Mongo DB bound to thread, "
					+ "and configuration does not allow creation of non-transactional one here");
		}

		return db;
	}

	/**
	 * Return whether the given DB instance is transactional, that is, bound to the current thread by Spring's transaction
	 * facilities.
	 * 
	 * @param db the DB to check
	 * @param mongo the Mongo instance that the DB was created with (may be <code>null</code>)
	 * @return whether the DB is transactional
	 */
	public static boolean isDBTransactional(DB db, Mongo mongo) {

		if (mongo == null) {
			return false;
		}
		DbHolder dbHolder = (DbHolder) TransactionSynchronizationManager.getResource(mongo);
		return dbHolder != null && dbHolder.containsDB(db);
	}

	/**
	 * Perform actual closing of the Mongo DB object, catching and logging any cleanup exceptions thrown.
	 * 
	 * @param db the DB to close (may be <code>null</code>)
	 */
	public static void closeDB(DB db) {

		if (db != null) {
			LOGGER.debug("Closing Mongo DB object");
			try {
				db.requestDone();
			} catch (Throwable ex) {
				LOGGER.debug("Unexpected exception on closing Mongo DB object", ex);
			}
		}
	}
}
