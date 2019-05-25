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
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import java.util.List;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.springframework.transaction.support.TransactionSynchronization;
import org.springframework.transaction.support.TransactionSynchronizationManager;
import org.springframework.transaction.support.TransactionSynchronizationUtils;

import org.springframework.data.mongodb.assist.DB;
import org.springframework.data.mongodb.assist.Mongo;

/**
 * Unit tests for {@link MongoDbUtils}.
 * 
 * @author Oliver Gierke
 * @author Randy Watler
 */
@RunWith(MockitoJUnitRunner.class)
public class MongoDbUtilsUnitTests {

	@Mock Mongo mongo;

	@Before
	public void setUp() throws Exception {

		when(mongo.getDB(anyString())).then(new Answer<DB>() {
			public DB answer(InvocationOnMock invocation) throws Throwable {
				return mock(DB.class);
			}
		});

		TransactionSynchronizationManager.initSynchronization();
	}

	@After
	public void tearDown() {

		for (Object key : TransactionSynchronizationManager.getResourceMap().keySet()) {
			TransactionSynchronizationManager.unbindResource(key);
		}

		TransactionSynchronizationManager.clearSynchronization();
	}

	@Test
	public void returnsNewInstanceForDifferentDatabaseName() {

		DB first = MongoDbUtils.getDB(mongo, "first");
		DB second = MongoDbUtils.getDB(mongo, "second");
		assertThat(second, is(not(first)));
	}

	@Test
	public void returnsSameInstanceForSameDatabaseName() {

		DB first = MongoDbUtils.getDB(mongo, "first");
		assertThat(first, is(notNullValue()));
		assertThat(MongoDbUtils.getDB(mongo, "first"), is(sameInstance(first)));
	}

	/**
	 * @see DATAMONGO-737
	 */
	@Test
	public void handlesTransactionSynchronizationLifecycle() {

		assertThat(TransactionSynchronizationManager.getSynchronizations().isEmpty(), is(true));
		assertThat(TransactionSynchronizationManager.getResourceMap().isEmpty(), is(true));

		MongoDbUtils.getDB(mongo, "first");

		assertThat(TransactionSynchronizationManager.getSynchronizations().isEmpty(), is(false));
		assertThat(TransactionSynchronizationManager.getResourceMap().isEmpty(), is(false));

		try {
			simulateTransactionCompletion();
		} catch (Exception e) {
			fail("Unexpected exception thrown during transaction completion: " + e);
		}

		assertThat(TransactionSynchronizationManager.getResourceMap().isEmpty(), is(true));
	}

	/**
	 * @see DATAMONGO-737
	 */
	@Test
	public void handlesTransactionSynchronizationsLifecycle() {

		assertThat(TransactionSynchronizationManager.getSynchronizations().isEmpty(), is(true));
		assertThat(TransactionSynchronizationManager.getResourceMap().isEmpty(), is(true));

		MongoDbUtils.getDB(mongo, "first");
		MongoDbUtils.getDB(mongo, "second");

		assertThat(TransactionSynchronizationManager.getSynchronizations().isEmpty(), is(false));
		assertThat(TransactionSynchronizationManager.getResourceMap().isEmpty(), is(false));

		try {
			simulateTransactionCompletion();
		} catch (Exception e) {
			fail("Unexpected exception thrown during transaction completion: " + e);
		}

		assertThat(TransactionSynchronizationManager.getResourceMap().isEmpty(), is(true));
	}

	/**
	 * Simulate transaction rollback/commit completion protocol on managed transaction synchronizations which will unbind
	 * managed transaction resources. Does not swallow exceptions for testing purposes.
	 * 
	 * @see TransactionSynchronizationUtils#triggerBeforeCompletion()
	 * @see TransactionSynchronizationUtils#triggerAfterCompletion(int)
	 */
	private void simulateTransactionCompletion() {

		List<TransactionSynchronization> synchronizations = TransactionSynchronizationManager.getSynchronizations();
		for (TransactionSynchronization synchronization : synchronizations) {
			synchronization.beforeCompletion();
		}

		List<TransactionSynchronization> remainingSynchronizations = TransactionSynchronizationManager
				.getSynchronizations();
		if (remainingSynchronizations != null) {
			for (TransactionSynchronization remainingSynchronization : remainingSynchronizations) {
				remainingSynchronization.afterCompletion(TransactionSynchronization.STATUS_ROLLED_BACK);
			}
		}
	}
}
