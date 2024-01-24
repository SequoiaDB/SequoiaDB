/*
 * Copyright 2011-2014 the original author or authors.
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

import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.List;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.dao.DataAccessException;
import org.springframework.data.geo.Point;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mongodb.core.convert.AbstractMongoConverter;
import org.springframework.data.mongodb.core.convert.MongoConverter;
import org.springframework.data.mongodb.core.convert.MongoTypeMapper;
import org.springframework.data.mongodb.core.mapping.MongoPersistentEntity;
import org.springframework.data.mongodb.core.mapping.MongoPersistentProperty;
import org.springframework.data.mongodb.core.query.NearQuery;
import org.springframework.data.util.TypeInformation;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.DBRef;

/**
 * Abstract base class for unit tests to specify behaviour we expect from {@link MongoOperations}. Subclasses return
 * instances of their implementation and thus can see if it correctly implements the {@link MongoOperations} interface.
 * 
 * @author Oliver Gierke
 * @author Thomas Darimont
 */
@RunWith(MockitoJUnitRunner.class)
public abstract class MongoOperationsUnitTests {

	@Mock CollectionCallback<Object> collectionCallback;
	@Mock DbCallback<Object> dbCallback;

	MongoConverter converter;
	Person person;
	List<Person> persons;

	@Before
	public final void operationsSetUp() {

		person = new Person("Oliver");
		persons = Arrays.asList(person);

		converter = new AbstractMongoConverter(null) {

			public void write(Object t, DBObject dbo) {
				dbo.put("firstName", person.getFirstName());
			}

			@SuppressWarnings("unchecked")
			public <S extends Object> S read(Class<S> clazz, DBObject dbo) {
				return (S) person;
			}

			public MappingContext<? extends MongoPersistentEntity<?>, MongoPersistentProperty> getMappingContext() {
				return null;
			}

			public Object convertToMongoType(Object obj, TypeInformation<?> typeInformation) {
				return null;
			}

			public DBRef toDBRef(Object object, MongoPersistentProperty referingProperty) {
				return null;
			}

			@Override
			public MongoTypeMapper getTypeMapper() {
				return null;
			}
		};
	}

	@Test(expected = IllegalArgumentException.class)
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void rejectsNullForCollectionCallback() {

		getOperations().execute("test", (CollectionCallback) null);
	}

	@Test(expected = IllegalArgumentException.class)
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void rejectsNullForCollectionCallback2() {
		getOperations().execute("collection", (CollectionCallback) null);
	}

	@Test(expected = IllegalArgumentException.class)
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void rejectsNullForDbCallback() {
		getOperations().execute((DbCallback) null);
	}

	@Test
	public void convertsExceptionForCollectionExists() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.collectionExists("foo");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForCreateCollection() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.createCollection("foo");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForCreateCollection2() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.createCollection("foo", new CollectionOptions.Builder().build());
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForDropCollection() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.dropCollection("foo");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForExecuteCollectionCallback() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.execute("test", collectionCallback);
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForExecuteDbCallback() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.execute(dbCallback);
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForExecuteCollectionCallbackAndCollection() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.execute("collection", collectionCallback);
			}
		}.assertDataAccessException();
	}

	@Test
	@Ignore // not support executeCommand in sdb
	public void convertsExceptionForExecuteCommand() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.executeCommand(new BasicDBObject());
			}
		}.assertDataAccessException();
	}

	@Test
	@Ignore // not support executeCommand
	public void convertsExceptionForExecuteStringCommand() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.executeCommand("");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForExecuteInSession() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.executeInSession(dbCallback);
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForGetCollection() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.findAll(Object.class);
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForGetCollectionWithCollectionName() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.getCollection("collection");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForGetCollectionWithCollectionNameAndType() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.findAll(Object.class, "collection");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForGetCollectionWithCollectionNameTypeAndReader() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.findAll(Object.class, "collection");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForGetCollectionNames() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.getCollectionNames();
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForInsert() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.insert(person);
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForInsert2() {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.insert(person, "collection");
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForInsertList() throws Exception {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.insertAll(persons);
			}
		}.assertDataAccessException();
	}

	@Test
	public void convertsExceptionForGetInsertList2() throws Exception {
		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.insert(persons, "collection");
			}
		}.assertDataAccessException();
	}

	/**
	 * @see DATAMONGO-341
	 */
	@Test
	@Ignore // not support geo in sdb
	public void geoNearRejectsNullNearQuery() {

		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.geoNear(null, Person.class);
			}
		}.assertDataAccessException();
	}

	/**
	 * @see DATAMONGO-341
	 */
	@Test
	@Ignore // not support geo in sdb
	public void geoNearRejectsNullNearQueryifCollectionGiven() {

		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.geoNear(null, Person.class, "collection");
			}
		}.assertDataAccessException();
	}

	/**
	 * @see DATAMONGO-341
	 */
	@Test
	public void geoNearRejectsNullEntityClass() {

		final NearQuery query = NearQuery.near(new Point(10, 20));

		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.geoNear(query, null);
			}
		}.assertDataAccessException();
	}

	/**
	 * @see DATAMONGO-341
	 */
	@Test
	@Ignore // not support geo in sdb
	public void geoNearRejectsNullEntityClassIfCollectionGiven() {

		final NearQuery query = NearQuery.near(new Point(10, 20));

		new Execution() {
			@Override
			public void doWith(MongoOperations operations) {
				operations.geoNear(query, null, "collection");
			}
		}.assertDataAccessException();
	}

	private abstract class Execution {

		public void assertDataAccessException() {
			assertException(DataAccessException.class);
		}

		public void assertException(Class<? extends Exception> exception) {

			try {
				doWith(getOperationsForExceptionHandling());
				fail("Expected " + exception + " but completed without any!");
			} catch (Exception e) {
				assertTrue("Expected " + exception + " but got " + e, exception.isInstance(e));
			}
		}

		public abstract void doWith(MongoOperations operations);
	}

	/**
	 * Expects an {@link MongoOperations} instance that will be used to check that invoking methods on it will only cause
	 * {@link DataAccessException}s.
	 * 
	 * @return
	 */
	protected abstract MongoOperations getOperationsForExceptionHandling();

	/**
	 * Returns a plain {@link MongoOperations}.
	 * 
	 * @return
	 */
	protected abstract MongoOperations getOperations();

}
