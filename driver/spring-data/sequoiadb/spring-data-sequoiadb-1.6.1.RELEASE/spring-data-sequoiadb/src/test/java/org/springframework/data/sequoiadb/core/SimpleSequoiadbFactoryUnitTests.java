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
package org.springframework.data.sequoiadb.core;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.net.UnknownHostException;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.test.util.ReflectionTestUtils;

import org.springframework.data.sequoiadb.assist.SequoiadbURI;

/**
 * Unit tests for {@link SimpleSequoiadbFactory}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class SimpleSequoiadbFactoryUnitTests {

	@Mock
	Sdb sdb;

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
		new SimpleSequoiadbFactory(sdb, "foo-bar");
		new SimpleSequoiadbFactory(sdb, "foo_bar");
		new SimpleSequoiadbFactory(sdb, "foo01231bar");
	}

	/**
	 * @see DATADOC-295
	 * @throws UnknownHostException
	 */
	@Test
	@SuppressWarnings("deprecation")
	public void sequoiadbUriConstructor() throws UnknownHostException {

		SequoiadbURI sequoiadbURI = new SequoiadbURI("sequoiadb://myUsername:myPassword@localhost/myDatabase.myCollection");
		SequoiadbFactory sequoiadbFactory = new SimpleSequoiadbFactory(sequoiadbURI);

		assertThat(ReflectionTestUtils.getField(sequoiadbFactory, "credentials"), is((Object) new UserCredentials(
				"myUsername", "myPassword")));
		assertThat(ReflectionTestUtils.getField(sequoiadbFactory, "databaseName").toString(), is("myDatabase"));
		assertThat(ReflectionTestUtils.getField(sequoiadbFactory, "databaseName").toString(), is("myDatabase"));
	}

	/**
	 * @see DATA_JIRA-789
	 */
	@Test
	public void defaultsAuthenticationDatabaseToDatabase() {

		SimpleSequoiadbFactory factory = new SimpleSequoiadbFactory(sdb, "foo");
		assertThat(ReflectionTestUtils.getField(factory, "authenticationDatabaseName"), is((Object) "foo"));
	}

	private void rejectsDatabaseName(String databaseName) {

		try {
			new SimpleSequoiadbFactory(sdb, databaseName);
			fail("Expected database name " + databaseName + " to be rejected!");
		} catch (IllegalArgumentException ex) {

		}
	}
}
