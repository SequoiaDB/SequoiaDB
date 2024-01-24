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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.sequoiadb.CannotGetSequoiadbConnectionException;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.transaction.support.TransactionSynchronizationManager;
import org.springframework.util.Assert;

import org.springframework.data.sequoiadb.assist.DB;

/**
 * Helper class featuring helper methods for internal Sequoiadb classes. Mainly intended for internal use within the
 * framework.
 *
 * @since 1.0
 */
public abstract class SequoiadbUtils {

	private static final Logger LOGGER = LoggerFactory.getLogger(SequoiadbUtils.class);

	/**
	 * Private constructor to prevent instantiation.
	 */
	private SequoiadbUtils() {

	}

	/**
	 * Obtains a {@link DB} connection for the given {@link Sdb} instance and database name
	 * 
	 * @param sdb the {@link Sdb} instance, must not be {@literal null}.
	 * @param databaseName the database name, must not be {@literal null} or empty.
	 * @return the {@link DB} connection
	 */
	public static DB getDB(Sdb sdb, String databaseName) {
		return doGetDB(sdb, databaseName, UserCredentials.NO_CREDENTIALS, true, databaseName);
	}

	/**
	 * Obtains a {@link DB} connection for the given {@link Sdb} instance and database name
	 * 
	 * @param sdb the {@link Sdb} instance, must not be {@literal null}.
	 * @param databaseName the database name, must not be {@literal null} or empty.
	 * @param credentials the credentials to use, must not be {@literal null}.
	 * @return the {@link DB} connection
	 */
	public static DB getDB(Sdb sdb, String databaseName, UserCredentials credentials) {
		return getDB(sdb, databaseName, credentials, databaseName);
	}

	public static DB getDB(Sdb sdb, String databaseName, UserCredentials credentials,
                           String authenticationDatabaseName) {

		Assert.notNull(sdb, "No Sdb instance specified!");
		Assert.hasText(databaseName, "Database name must be given!");
		Assert.notNull(credentials, "Credentials must not be null, use UserCredentials.NO_CREDENTIALS!");
		Assert.hasText(authenticationDatabaseName, "Authentication database name must not be null or empty!");

		return doGetDB(sdb, databaseName, credentials, true, authenticationDatabaseName);
	}

	private static DB doGetDB(Sdb sdb, String databaseName, UserCredentials credentials, boolean allowCreate,
                              String authenticationDatabaseName) {

		DbHolder dbHolder = (DbHolder) TransactionSynchronizationManager.getResource(sdb);

		// Do we have a populated holder and TX sync active?
		if (dbHolder != null && !dbHolder.isEmpty() && TransactionSynchronizationManager.isSynchronizationActive()) {

			DB db = dbHolder.getDB(databaseName);

			// DB found but not yet synchronized
			if (db != null && !dbHolder.isSynchronizedWithTransaction()) {

				LOGGER.debug("Registering Spring transaction synchronization for existing SequoiaDB {}.", databaseName);

				TransactionSynchronizationManager.registerSynchronization(new SequoiadbSynchronization(dbHolder, sdb));
				dbHolder.setSynchronizedWithTransaction(true);
			}

			if (db != null) {
				return db;
			}
		}

		// Lookup fresh database instance
		LOGGER.debug("Getting Sdb Database name=[{}]", databaseName);

		DB db = sdb.getDB(databaseName);
		boolean credentialsGiven = credentials.hasUsername() && credentials.hasPassword();

		DB authDb = databaseName.equals(authenticationDatabaseName) ? db : sdb.getDB(authenticationDatabaseName);

		synchronized (authDb) {

			if (credentialsGiven && !authDb.isAuthenticated()) {

				String username = credentials.getUsername();
				String password = credentials.hasPassword() ? credentials.getPassword() : null;

				if (!authDb.authenticate(username, password == null ? null : password.toCharArray())) {
					throw new CannotGetSequoiadbConnectionException("Failed to authenticate to database [" + databaseName + "], "
							+ credentials.toString(), databaseName, credentials);
				}
			}
		}

		// TX sync active, bind new database to thread
		if (TransactionSynchronizationManager.isSynchronizationActive()) {

			LOGGER.debug("Registering Spring transaction synchronization for SequoiaDB instance {}.", databaseName);

			DbHolder holderToUse = dbHolder;

			if (holderToUse == null) {
				holderToUse = new DbHolder(databaseName, db);
			} else {
				holderToUse.addDB(databaseName, db);
			}

			// synchronize holder only if not yet synchronized
			if (!holderToUse.isSynchronizedWithTransaction()) {
				TransactionSynchronizationManager.registerSynchronization(new SequoiadbSynchronization(holderToUse, sdb));
				holderToUse.setSynchronizedWithTransaction(true);
			}

			if (holderToUse != dbHolder) {
				TransactionSynchronizationManager.bindResource(sdb, holderToUse);
			}
		}

		// Check whether we are allowed to return the DB.
		if (!allowCreate && !isDBTransactional(db, sdb)) {
			throw new IllegalStateException("No Sdb DB bound to thread, "
					+ "and configuration does not allow creation of non-transactional one here");
		}

		return db;
	}

	/**
	 * Return whether the given DB instance is transactional, that is, bound to the current thread by Spring's transaction
	 * facilities.
	 * 
	 * @param db the DB to check
	 * @param sdb the Sdb instance that the DB was created with (may be <code>null</code>)
	 * @return whether the DB is transactional
	 */
	public static boolean isDBTransactional(DB db, Sdb sdb) {

		if (sdb == null) {
			return false;
		}
		DbHolder dbHolder = (DbHolder) TransactionSynchronizationManager.getResource(sdb);
		return dbHolder != null && dbHolder.containsDB(db);
	}

	/**
	 * Perform actual closing of the Sdb DB object, catching and logging any cleanup exceptions thrown.
	 * 
	 * @param db the DB to close (may be <code>null</code>)
	 */
	public static void closeDB(DB db) {

		if (db != null) {
			LOGGER.debug("Closing Sdb DB object");
			try {
				db.requestDone();
			} catch (Throwable ex) {
				LOGGER.debug("Unexpected exception on closing Sdb DB object", ex);
			}
		}
	}
}
