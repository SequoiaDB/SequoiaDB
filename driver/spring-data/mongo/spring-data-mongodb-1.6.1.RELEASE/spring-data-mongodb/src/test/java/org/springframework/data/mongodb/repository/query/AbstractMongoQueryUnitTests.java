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
package org.springframework.data.mongodb.repository.query;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

import org.bson.types.ObjectId;
import org.hamcrest.core.Is;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Slice;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.data.mongodb.core.MongoOperations;
import org.springframework.data.mongodb.core.Person;
import org.springframework.data.mongodb.core.convert.DbRefResolver;
import org.springframework.data.mongodb.core.convert.DefaultDbRefResolver;
import org.springframework.data.mongodb.core.convert.MappingMongoConverter;
import org.springframework.data.mongodb.core.mapping.BasicMongoPersistentEntity;
import org.springframework.data.mongodb.core.mapping.MongoMappingContext;
import org.springframework.data.mongodb.core.query.BasicQuery;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.repository.Meta;
import org.springframework.data.mongodb.repository.MongoRepository;
import org.springframework.data.repository.core.RepositoryMetadata;

import org.springframework.data.mongodb.assist.BasicDBObjectBuilder;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.WriteResult;

/**
 * Unit tests for {@link AbstractMongoQuery}.
 * 
 * @author Christoph Strobl
 * @author Oliver Gierke
 */
@RunWith(MockitoJUnitRunner.class)
public class AbstractMongoQueryUnitTests {

	@Mock RepositoryMetadata metadataMock;
	@Mock MongoOperations mongoOperationsMock;
	@Mock @SuppressWarnings("rawtypes") BasicMongoPersistentEntity persitentEntityMock;
	@Mock MongoMappingContext mappingContextMock;
	@Mock WriteResult writeResultMock;

	@Before
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void setUp() {

		when(metadataMock.getDomainType()).thenReturn((Class) Person.class);
		when(metadataMock.getReturnedDomainClass(Matchers.any(Method.class))).thenReturn((Class) Person.class);
		when(persitentEntityMock.getCollection()).thenReturn("persons");
		when(mappingContextMock.getPersistentEntity(Matchers.any(Class.class))).thenReturn(persitentEntityMock);
		when(persitentEntityMock.getType()).thenReturn(Person.class);

		DbRefResolver dbRefResolver = new DefaultDbRefResolver(mock(MongoDbFactory.class));
		MappingMongoConverter converter = new MappingMongoConverter(dbRefResolver, mappingContextMock);
		converter.afterPropertiesSet();

		when(mongoOperationsMock.getConverter()).thenReturn(converter);
	}

	/**
	 * @see DATAMONGO-566
	 */
	@SuppressWarnings("unchecked")
	@Test
	public void testDeleteExecutionCallsRemoveCorreclty() {

		createQueryForMethod("deletePersonByLastname", String.class).setDeleteQuery(true).execute(new Object[] { "booh" });

		verify(this.mongoOperationsMock, times(1)).remove(Matchers.any(Query.class), Matchers.eq(Person.class),
				Matchers.eq("persons"));
		verify(this.mongoOperationsMock, times(0)).find(Matchers.any(Query.class), Matchers.any(Class.class),
				Matchers.anyString());
	}

	/**
	 * @see DATAMONGO-566
	 * @see DATAMONGO-1040
	 */
	@SuppressWarnings("unchecked")
	@Test
	public void testDeleteExecutionLoadsListOfRemovedDocumentsWhenReturnTypeIsCollectionLike() {

		when(this.mongoOperationsMock.find(Matchers.any(Query.class), Matchers.any(Class.class), Matchers.anyString()))
				.thenReturn(Arrays.asList(new Person(new ObjectId(new Date()), "bar")));

		createQueryForMethod("deleteByLastname", String.class).setDeleteQuery(true).execute(new Object[] { "booh" });

		verify(this.mongoOperationsMock, times(1)).findAllAndRemove(Matchers.any(Query.class), Matchers.eq(Person.class),
				Matchers.eq("persons"));
	}

	/**
	 * @see DATAMONGO-566
	 */
	@Test
	public void testDeleteExecutionReturnsZeroWhenWriteResultIsNull() {

		MongoQueryFake query = createQueryForMethod("deletePersonByLastname", String.class);
		query.setDeleteQuery(true);

		assertThat(query.execute(new Object[] { "fake" }), Is.<Object> is(0L));
	}

	/**
	 * @see DATAMONGO-566
	 * @see DATAMONGO-978
	 */
	@Test
	public void testDeleteExecutionReturnsNrDocumentsDeletedFromWriteResult() {

		when(writeResultMock.getN()).thenReturn(100);
		when(this.mongoOperationsMock.remove(Matchers.any(Query.class), Matchers.eq(Person.class), Matchers.eq("persons")))
				.thenReturn(writeResultMock);

		MongoQueryFake query = createQueryForMethod("deletePersonByLastname", String.class);
		query.setDeleteQuery(true);

		assertThat(query.execute(new Object[] { "fake" }), is((Object) 100L));
		verify(this.mongoOperationsMock, times(1)).remove(Matchers.any(Query.class), Matchers.eq(Person.class),
				Matchers.eq("persons"));
	}

	/**
	 * @see DATAMONGO-957
	 */
	@Test
	public void metadataShouldNotBeAddedToQueryWhenNotPresent() {

		MongoQueryFake query = createQueryForMethod("findByFirstname", String.class);
		query.execute(new Object[] { "fake" });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(1))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

	}

	/**
	 * @see DATAMONGO-957
	 */
	@Test
	public void metadataShouldBeAddedToQueryCorrectly() {

		MongoQueryFake query = createQueryForMethod("findByFirstname", String.class, Pageable.class);
		query.execute(new Object[] { "fake", new PageRequest(0, 10) });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(1))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));
	}

	/**
	 * @see DATAMONGO-957
	 */
	@Test
	public void metadataShouldBeAddedToCountQueryCorrectly() {

		MongoQueryFake query = createQueryForMethod("findByFirstname", String.class, Pageable.class);
		query.execute(new Object[] { "fake", new PageRequest(0, 10) });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(1)).count(captor.capture(), Matchers.eq("persons"));
	}

	/**
	 * @see DATAMONGO-957
	 */
	@Test
	public void metadataShouldBeAddedToStringBasedQueryCorrectly() {

		MongoQueryFake query = createQueryForMethod("findByAnnotatedQuery", String.class, Pageable.class);
		query.execute(new Object[] { "fake", new PageRequest(0, 10) });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(1))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));
	}

	/**
	 * @see DATAMONGO-1057
	 */
	@Test
	public void slicedExecutionShouldRetainNrOfElementsToSkip() {

		MongoQueryFake query = createQueryForMethod("findByLastname", String.class, Pageable.class);
		Pageable page1 = new PageRequest(0, 10);
		Pageable page2 = page1.next();

		query.execute(new Object[] { "fake", page1 });
		query.execute(new Object[] { "fake", page2 });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(2))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

		assertThat(captor.getAllValues().get(0).getSkip(), is(0));
		assertThat(captor.getAllValues().get(1).getSkip(), is(10));
	}

	/**
	 * @see DATAMONGO-1057
	 */
	@Test
	public void slicedExecutionShouldIncrementLimitByOne() {

		MongoQueryFake query = createQueryForMethod("findByLastname", String.class, Pageable.class);
		Pageable page1 = new PageRequest(0, 10);
		Pageable page2 = page1.next();

		query.execute(new Object[] { "fake", page1 });
		query.execute(new Object[] { "fake", page2 });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(2))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

		assertThat(captor.getAllValues().get(0).getLimit(), is(11));
		assertThat(captor.getAllValues().get(1).getLimit(), is(11));
	}

	/**
	 * @see DATAMONGO-1057
	 */
	@Test
	public void slicedExecutionShouldRetainSort() {

		MongoQueryFake query = createQueryForMethod("findByLastname", String.class, Pageable.class);
		Pageable page1 = new PageRequest(0, 10, Sort.Direction.DESC, "bar");
		Pageable page2 = page1.next();

		query.execute(new Object[] { "fake", page1 });
		query.execute(new Object[] { "fake", page2 });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.mongoOperationsMock, times(2))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

		DBObject expectedSortObject = new BasicDBObjectBuilder().add("bar", -1).get();
		assertThat(captor.getAllValues().get(0).getSortObject(), is(expectedSortObject));
		assertThat(captor.getAllValues().get(1).getSortObject(), is(expectedSortObject));
	}

	/**
	 * @see DATAMONGO-1080
	 */
	@Test
	public void doesNotTryToPostProcessQueryResultIntoWrapperType() {

		Person reference = new Person();
		when(mongoOperationsMock.findOne(Mockito.any(Query.class), eq(Person.class), eq("persons"))).//
				thenReturn(reference);

		AbstractMongoQuery query = createQueryForMethod("findByLastname", String.class);

		assertThat(query.execute(new Object[] { "lastname" }), is((Object) reference));
	}

	private MongoQueryFake createQueryForMethod(String methodName, Class<?>... paramTypes) {

		try {

			Method method = Repo.class.getMethod(methodName, paramTypes);
			MongoQueryMethod queryMethod = new MongoQueryMethod(method, metadataMock, mappingContextMock);

			return new MongoQueryFake(queryMethod, mongoOperationsMock);

		} catch (NoSuchMethodException e) {
			throw new IllegalArgumentException(e.getMessage(), e);
		} catch (SecurityException e) {
			throw new IllegalArgumentException(e.getMessage(), e);
		}
	}

	private static class MongoQueryFake extends AbstractMongoQuery {

		private boolean isCountQuery;
		private boolean isDeleteQuery;

		public MongoQueryFake(MongoQueryMethod method, MongoOperations operations) {
			super(method, operations);
		}

		@Override
		protected Query createQuery(ConvertingParameterAccessor accessor) {
			return new BasicQuery("{'foo':'bar'}");
		}

		@Override
		protected boolean isCountQuery() {
			return isCountQuery;
		}

		@Override
		protected boolean isDeleteQuery() {
			return isDeleteQuery;
		}

		public MongoQueryFake setDeleteQuery(boolean isDeleteQuery) {
			this.isDeleteQuery = isDeleteQuery;
			return this;
		}
	}

	private interface Repo extends MongoRepository<Person, Long> {

		List<Person> deleteByLastname(String lastname);

		Long deletePersonByLastname(String lastname);

		List<Person> findByFirstname(String firstname);

		@Meta(comment = "comment")
		Page<Person> findByFirstname(String firstnanme, Pageable pageable);

		@Meta(comment = "comment")
		@org.springframework.data.mongodb.repository.Query("{}")
		Page<Person> findByAnnotatedQuery(String firstnanme, Pageable pageable);

		/** @see DATAMONGO-1057 */
		Slice<Person> findByLastname(String lastname, Pageable page);

	}
}
