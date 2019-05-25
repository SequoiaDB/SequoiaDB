/*
 * Copyright 2011 the original author or authors.
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

import java.net.UnknownHostException;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.test.util.ReflectionTestUtils;

import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoURI;

/**
 * Unit tests for {@link SimpleMongoDbFactory}.
 * 
 * @author Oliver Gierke
 */
@RunWith(MockitoJUnitRunner.class)
public class SimpleMongoDbFactoryUnitTests {

	@Mock Mongo mongo;

	/**
	 * @see DATADOC-254
	 */
	@Test
	public void rejectsIllegalDatabaseNames() {
		rejectsDatabaseName("foo.bar");
		rejectsDatabaseName("foo!bar");
	}

	/**
	 * @see DATADOC-254
	 */
	@Test
	public void allowsDatabaseNames() {
		new SimpleMongoDbFactory(mongo, "foo-bar");
		new SimpleMongoDbFactory(mongo, "foo_bar");
		new SimpleMongoDbFactory(mongo, "foo01231bar");
	}

	/**
	 * @see DATADOC-295
	 * @throws UnknownHostException
	 */
	@Test
	@SuppressWarnings("deprecation")
	public void mongoUriConstructor() throws UnknownHostException {

		MongoURI mongoURI = new MongoURI("mongodb://myUsername:myPassword@localhost/myDatabase.myCollection");
		MongoDbFactory mongoDbFactory = new SimpleMongoDbFactory(mongoURI);

		assertThat(ReflectionTestUtils.getField(mongoDbFactory, "credentials"), is((Object) new UserCredentials(
				"myUsername", "myPassword")));
		assertThat(ReflectionTestUtils.getField(mongoDbFactory, "databaseName").toString(), is("myDatabase"));
		assertThat(ReflectionTestUtils.getField(mongoDbFactory, "databaseName").toString(), is("myDatabase"));
	}

	/**
	 * @see DATAMONGO-789
	 */
	@Test
	public void defaultsAuthenticationDatabaseToDatabase() {

		SimpleMongoDbFactory factory = new SimpleMongoDbFactory(mongo, "foo");
		assertThat(ReflectionTestUtils.getField(factory, "authenticationDatabaseName"), is((Object) "foo"));
	}

	private void rejectsDatabaseName(String databaseName) {

		try {
			new SimpleMongoDbFactory(mongo, databaseName);
			fail("Expected database name " + databaseName + " to be rejected!");
		} catch (IllegalArgumentException ex) {

		}
	}
}
