/*
 * Copyright 2014 the original author or authors.
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
package org.springframework.data.sequoiadb.test.util;

import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.util.CollectionUtils;
import org.springframework.util.StringUtils;

import org.springframework.data.sequoiadb.assist.DB;
import org.springframework.data.sequoiadb.assist.DBCollection;

/**
 * {@link CleanSequoiadb} is a junit {@link TestRule} implementation to be used as for wiping data from SequoiaDB instance.
 * SequoiaDB specific system databases like {@literal admin} and {@literal local} remain untouched. The rule will apply
 * <strong>after</strong> the base {@link Statement}. <br />
 * Use as {@link org.junit.ClassRule} to wipe data after finishing all tests within a class or as {@link org.junit.Rule}
 * to do so after each {@link org.junit.Test}.
 * 

 * @since 1.6
 */
public class CleanSequoiadb implements TestRule {

	private static final Logger LOGGER = LoggerFactory.getLogger(CleanSequoiadb.class);

	/**
	 * Defines contents of SequoiaDB.
	 */
	public enum Struct {
		DATABASE, COLLECTION, INDEX;
	}

	@SuppressWarnings("serial")//
	private Set<String> preserveDatabases = new HashSet<String>() {
		{
			add("admin");
			add("local");
		}
	};

	private Set<String> dbNames = new HashSet<String>();
	private Set<String> collectionNames = new HashSet<String>();
	private Set<Struct> types = new HashSet<CleanSequoiadb.Struct>();
	private SdbClient client;

	/**
	 * Create new instance using an internal {@link SdbClient}.
	 */
	public CleanSequoiadb() {
		this(null);
	}

	/**
	 * Create new instance using an internal {@link SdbClient} connecting to specified instance running at host:port.
	 * 
	 * @param host
	 * @param port
	 * @throws UnknownHostException
	 */
	public CleanSequoiadb(String host, int port) throws UnknownHostException {
		this(new SdbClient(host, port));
	}

	/**
	 * Create new instance using the given client.
	 * 
	 * @param client
	 */
	public CleanSequoiadb(SdbClient client) {
		this.client = client;
	}

	/**
	 * Removes everything by dropping every single {@link DB}.
	 * 
	 * @return
	 */
	public static CleanSequoiadb everything() {

		CleanSequoiadb cleanSequoiadb = new CleanSequoiadb();
		cleanSequoiadb.clean(Struct.DATABASE);
		return cleanSequoiadb;
	}

	/**
	 * Removes everything from the databases with given name by dropping the according {@link DB}.
	 * 
	 * @param dbNames
	 * @return
	 */
	public static CleanSequoiadb databases(String... dbNames) {

		CleanSequoiadb cleanSequoiadb = new CleanSequoiadb();
		cleanSequoiadb.clean(Struct.DATABASE);
		cleanSequoiadb.useDatabases(dbNames);
		return cleanSequoiadb;
	}

	/**
	 * Drops the {@link DBCollection} with given names from every single {@link DB} containing them.
	 * 
	 * @param collectionNames
	 * @return
	 */
	public static CleanSequoiadb collections(String... collectionNames) {
		return collections("", Arrays.asList(collectionNames));
	}

	/**
	 * Drops the {@link DBCollection} with given names from the named {@link DB}.
	 * 
	 * @param dbName
	 * @param collectionNames
	 * @return
	 */
	public static CleanSequoiadb collections(String dbName, Collection<String> collectionNames) {

		CleanSequoiadb cleanSequoiadb = new CleanSequoiadb();
		cleanSequoiadb.clean(Struct.COLLECTION);
		cleanSequoiadb.useCollections(dbName, collectionNames);
		return cleanSequoiadb;
	}

	/**
	 * Drops all index structures from every single {@link DBCollection}.
	 * 
	 * @return
	 */
	public static CleanSequoiadb indexes() {
		return indexes(Collections.<String> emptySet());
	}

	/**
	 * Drops all index structures from every single {@link DBCollection}.
	 * 
	 * @param collectionNames
	 * @return
	 */
	public static CleanSequoiadb indexes(Collection<String> collectionNames) {

		CleanSequoiadb cleanSequoiadb = new CleanSequoiadb();
		cleanSequoiadb.clean(Struct.INDEX);
		cleanSequoiadb.useCollections(collectionNames);
		return cleanSequoiadb;
	}

	/**
	 * Define {@link Struct} to be cleaned.
	 * 
	 * @param types
	 * @return
	 */
	public CleanSequoiadb clean(Struct... types) {

		this.types.addAll(Arrays.asList(types));
		return this;
	}

	/**
	 * Defines the {@link DB}s to be used. <br />
	 * Impact along with {@link CleanSequoiadb#clean(Struct...)}:
	 * <ul>
	 * <li>{@link Struct#DATABASE}: Forces drop of named databases.</li>
	 * <li>{@link Struct#COLLECTION}: Forces drop of collections within named databases.</li>
	 * <li>{@link Struct#INDEX}: Removes index within collections of named databases.</li>
	 * </ul>
	 * 
	 * @param dbNames
	 * @return
	 */
	public CleanSequoiadb useDatabases(String... dbNames) {

		this.dbNames.addAll(Arrays.asList(dbNames));
		return this;
	}

	/**
	 * Excludes the given {@link DB}s from being processed.
	 * 
	 * @param dbNames
	 * @return
	 */
	public CleanSequoiadb preserveDatabases(String... dbNames) {
		this.preserveDatabases.addAll(Arrays.asList(dbNames));
		return this;
	}

	/**
	 * Defines the {@link DBCollection}s to be used. <br />
	 * Impact along with {@link CleanSequoiadb#clean(Struct...)}:
	 * <ul>
	 * <li>{@link Struct#COLLECTION}: Forces drop of named collections.</li>
	 * <li>{@link Struct#INDEX}: Removes index within named collections.</li>
	 * </ul>
	 * 
	 * @param collectionNames
	 * @return
	 */
	public CleanSequoiadb useCollections(String... collectionNames) {
		return useCollections(Arrays.asList(collectionNames));
	}

	private CleanSequoiadb useCollections(Collection<String> collectionNames) {
		return useCollections("", collectionNames);
	}

	/**
	 * Defines the {@link DBCollection}s and {@link DB} to be used. <br />
	 * Impact along with {@link CleanSequoiadb#clean(Struct...)}:
	 * <ul>
	 * <li>{@link Struct#COLLECTION}: Forces drop of named collections in given db.</li>
	 * <li>{@link Struct#INDEX}: Removes index within named collections in given db.</li>
	 * </ul>
	 * 
	 * @param collectionNames
	 * @return
	 */
	public CleanSequoiadb useCollections(String db, Collection<String> collectionNames) {

		if (StringUtils.hasText(db)) {
			this.dbNames.add(db);
		}

		if (!CollectionUtils.isEmpty(collectionNames)) {
			this.collectionNames.addAll(collectionNames);
		}
		return this;
	}

	Statement apply() {
		return apply(null, null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.junit.rules.TestRule#apply(org.junit.runners.model.Statement, org.junit.runner.Description)
	 */
	public Statement apply(Statement base, Description description) {
		return new SequoiadbCleanStatement(base);
	}

	private void doClean() {

		Collection<String> dbNamesToUse = initDbNames();

		for (String dbName : dbNamesToUse) {

			if (isPreserved(dbName) || dropDbIfRequired(dbName)) {
				continue;
			}

			DB db = client.getDB(dbName);
			dropCollectionsOrIndexIfRequried(db, initCollectionNames(db));
		}
	}

	private boolean dropDbIfRequired(String dbName) {

		if (!types.contains(Struct.DATABASE)) {
			return false;
		}

		client.dropDatabase(dbName);
		LOGGER.debug("Dropping DB '{}'. ", dbName);
		return true;
	}

	private void dropCollectionsOrIndexIfRequried(DB db, Collection<String> collectionsToUse) {

		for (String collectionName : collectionsToUse) {

			if (db.collectionExists(collectionName)) {

				DBCollection collection = db.getCollectionFromString(collectionName);
				if (collection != null) {

					if (types.contains(Struct.COLLECTION)) {
						collection.drop();
						LOGGER.debug("Dropping collection '{}' for DB '{}'. ", collectionName, db.getName());
					} else if (types.contains(Struct.INDEX)) {
						collection.dropIndexes();
						LOGGER.debug("Dropping indexes in collection '{}' for DB '{}'. ", collectionName, db.getName());
					}
				}
			}
		}
	}

	private boolean isPreserved(String dbName) {
		return preserveDatabases.contains(dbName.toLowerCase());
	}

	private Collection<String> initDbNames() {

		Collection<String> dbNamesToUse = dbNames;
		if (dbNamesToUse.isEmpty()) {
			dbNamesToUse = client.getDatabaseNames();
		}
		return dbNamesToUse;
	}

	private Collection<String> initCollectionNames(DB db) {

		Collection<String> collectionsToUse = collectionNames;
		if (CollectionUtils.isEmpty(collectionsToUse)) {
			collectionsToUse = db.getCollectionNames();
		}
		return collectionsToUse;
	}

	/**

	 * @since 1.6
	 */
	private class SequoiadbCleanStatement extends Statement {

		private final Statement base;

		public SequoiadbCleanStatement(Statement base) {
			this.base = base;
		}

		@Override
		public void evaluate() throws Throwable {

			if (base != null) {
				base.evaluate();
			}

			boolean isInternal = false;
			if (client == null) {
				client = new SdbClient();
				isInternal = true;
			}

			doClean();

			if (isInternal) {
				client.close();
				client = null;
			}
		}
	}
}
