/*
 * Copyright 2012-2013 the original author or authors.
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

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.springframework.dao.DataAccessException;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.mongodb.assist.MongoClient;
import org.springframework.scheduling.concurrent.ThreadPoolExecutorFactoryBean;

import org.springframework.data.mongodb.assist.DB;
import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoException;

/**
 * Integration tests for {@link MongoDbUtils}.
 * 
 * @author Oliver Gierke
 * @author Thomas Darimont
 */
public class MongoDbUtilsIntegrationTests {

	static final String AUTHENTICATION_DATABASE_NAME = "admin";
	static final String DATABASE_NAME = "dbAuthTests";
	static final UserCredentials CREDENTIALS = new UserCredentials("admin", "admin");

	static Mongo mongo;
	static MongoTemplate template;
	static ThreadPoolExecutorFactoryBean factory;
	static ExecutorService service;

	Exception exception;

	@BeforeClass
	public static void setUp() throws Exception {

		mongo = new MongoClient();
		template = new MongoTemplate(mongo, DATABASE_NAME);

		factory = new ThreadPoolExecutorFactoryBean();
		factory.setCorePoolSize(2);
		factory.setMaxPoolSize(10);
		factory.setWaitForTasksToCompleteOnShutdown(true);
		factory.afterPropertiesSet();

		service = factory.getObject();
	}

	@AfterClass
	public static void tearDown() {

		factory.destroy();


		template.execute(new DbCallback<Void>() {
			public Void doInDB(DB db) throws MongoException, DataAccessException {
				db.dropDatabase();
				return null;
			}
		});
	}

	/**
	 * @see DATAMONGO-585
	 */
	@Test
	public void authenticatesCorrectlyInMultithreadedEnvironment() throws Exception {

		template.execute(new DbCallback<Void>() {
			public Void doInDB(DB db) throws MongoException, DataAccessException {
				db.addUser("admin", "admin".toCharArray());
				return null;
			}
		});

		Callable<Void> callable = new Callable<Void>() {
			public Void call() throws Exception {

				try {
					DB db = MongoDbUtils.getDB(mongo, DATABASE_NAME, CREDENTIALS);
					assertThat(db, is(notNullValue()));
				} catch (Exception o_O) {
					MongoDbUtilsIntegrationTests.this.exception = o_O;
				}

				return null;
			}
		};

		List<Callable<Void>> callables = new ArrayList<Callable<Void>>();

		for (int i = 0; i < 10; i++) {
			callables.add(callable);
		}

		service.invokeAll(callables);

		if (exception != null) {
			fail("Exception occurred!" + exception);
		}
	}

	/**
	 * @see DATAMONGO-789
	 */
	@Test
	public void authenticatesCorrectlyWithAuthenticationDB() throws Exception {

		template.execute(new DbCallback<Void>() {
			public Void doInDB(DB db) throws MongoException, DataAccessException {
				db.getSisterDB("admin").addUser("admin", "admin".toCharArray());
				return null;
			}
		});

		Callable<Void> callable = new Callable<Void>() {
			public Void call() throws Exception {

				try {
					DB db = MongoDbUtils.getDB(mongo, DATABASE_NAME, CREDENTIALS, AUTHENTICATION_DATABASE_NAME);
					assertThat(db, is(notNullValue()));
				} catch (Exception o_O) {
					MongoDbUtilsIntegrationTests.this.exception = o_O;
				}

				return null;
			}
		};

		List<Callable<Void>> callables = new ArrayList<Callable<Void>>();

		for (int i = 0; i < 10; i++) {
			callables.add(callable);
		}

		service.invokeAll(callables);

		if (exception != null) {
			fail("Exception occurred!" + exception);
		}
	}
}
