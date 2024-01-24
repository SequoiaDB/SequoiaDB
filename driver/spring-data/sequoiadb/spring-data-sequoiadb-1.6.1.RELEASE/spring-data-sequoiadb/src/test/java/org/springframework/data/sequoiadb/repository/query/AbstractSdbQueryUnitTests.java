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
package org.springframework.data.sequoiadb.repository.query;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
//import java.util.Optional;

import org.bson.BSONObject;
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
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.Person;
import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.BasicSequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.query.BasicQuery;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.repository.Meta;
import org.springframework.data.sequoiadb.repository.SequoiadbRepository;
import org.springframework.data.repository.core.RepositoryMetadata;

import org.springframework.data.sequoiadb.assist.BasicBSONObjectBuilder;

import org.springframework.data.sequoiadb.assist.WriteResult;

/**
 * Unit tests for {@link AbstractSequoiadbQuery}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class AbstractSdbQueryUnitTests {

	@Mock RepositoryMetadata metadataMock;
	@Mock
    SequoiadbOperations sequoiadbOperationsMock;
	@Mock @SuppressWarnings("rawtypes")
	BasicSequoiadbPersistentEntity persitentEntityMock;
	@Mock
	SequoiadbMappingContext mappingContextMock;
	@Mock WriteResult writeResultMock;

	@Before
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void setUp() {

		when(metadataMock.getDomainType()).thenReturn((Class) Person.class);
		when(metadataMock.getReturnedDomainClass(Matchers.any(Method.class))).thenReturn((Class) Person.class);
		when(persitentEntityMock.getCollection()).thenReturn("persons");
		when(mappingContextMock.getPersistentEntity(Matchers.any(Class.class))).thenReturn(persitentEntityMock);
		when(persitentEntityMock.getType()).thenReturn(Person.class);

		DbRefResolver dbRefResolver = new DefaultDbRefResolver(mock(SequoiadbFactory.class));
		MappingSequoiadbConverter converter = new MappingSequoiadbConverter(dbRefResolver, mappingContextMock);
		converter.afterPropertiesSet();

		when(sequoiadbOperationsMock.getConverter()).thenReturn(converter);
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@SuppressWarnings("unchecked")
	@Test
	public void testDeleteExecutionCallsRemoveCorreclty() {

		createQueryForMethod("deletePersonByLastname", String.class).setDeleteQuery(true).execute(new Object[] { "booh" });

		verify(this.sequoiadbOperationsMock, times(1)).remove(Matchers.any(Query.class), Matchers.eq(Person.class),
				Matchers.eq("persons"));
		verify(this.sequoiadbOperationsMock, times(0)).find(Matchers.any(Query.class), Matchers.any(Class.class),
				Matchers.anyString());
	}

	/**
	 * @see DATA_JIRA-566
	 * @see DATA_JIRA-1040
	 */
	@SuppressWarnings("unchecked")
	@Test
	public void testDeleteExecutionLoadsListOfRemovedDocumentsWhenReturnTypeIsCollectionLike() {

		when(this.sequoiadbOperationsMock.find(Matchers.any(Query.class), Matchers.any(Class.class), Matchers.anyString()))
				.thenReturn(Arrays.asList(new Person(new ObjectId(new Date()), "bar")));

		createQueryForMethod("deleteByLastname", String.class).setDeleteQuery(true).execute(new Object[] { "booh" });

		verify(this.sequoiadbOperationsMock, times(1)).findAllAndRemove(Matchers.any(Query.class), Matchers.eq(Person.class),
				Matchers.eq("persons"));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	public void testDeleteExecutionReturnsZeroWhenWriteResultIsNull() {

		SdbQueryFake query = createQueryForMethod("deletePersonByLastname", String.class);
		query.setDeleteQuery(true);

		assertThat(query.execute(new Object[] { "fake" }), Is.<Object> is(0L));
	}

	/**
	 * @see DATA_JIRA-566
	 * @see DATA_JIRA-978
	 */
	@Test
	public void testDeleteExecutionReturnsNrDocumentsDeletedFromWriteResult() {

		when(writeResultMock.getN()).thenReturn(100);
		when(this.sequoiadbOperationsMock.remove(Matchers.any(Query.class), Matchers.eq(Person.class), Matchers.eq("persons")))
				.thenReturn(writeResultMock);

		SdbQueryFake query = createQueryForMethod("deletePersonByLastname", String.class);
		query.setDeleteQuery(true);

		assertThat(query.execute(new Object[] { "fake" }), is((Object) 100L));
		verify(this.sequoiadbOperationsMock, times(1)).remove(Matchers.any(Query.class), Matchers.eq(Person.class),
				Matchers.eq("persons"));
	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void metadataShouldNotBeAddedToQueryWhenNotPresent() {

		SdbQueryFake query = createQueryForMethod("findByFirstname", String.class);
		query.execute(new Object[] { "fake" });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(1))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

//		assertThat(captor.getValue().getMeta().getComment(), nullValue());
	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void metadataShouldBeAddedToQueryCorrectly() {

		SdbQueryFake query = createQueryForMethod("findByFirstname", String.class, Pageable.class);
		query.execute(new Object[] { "fake", new PageRequest(0, 10) });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(1))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));
//		assertThat(captor.getValue().getMeta().getComment(), is("comment"));
	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void metadataShouldBeAddedToCountQueryCorrectly() {

		SdbQueryFake query = createQueryForMethod("findByFirstname", String.class, Pageable.class);
		query.execute(new Object[] { "fake", new PageRequest(0, 10) });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(1)).count(captor.capture(), Matchers.eq("persons"));
//		assertThat(captor.getValue().getMeta().getComment(), is("comment"));
	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void metadataShouldBeAddedToStringBasedQueryCorrectly() {

		SdbQueryFake query = createQueryForMethod("findByAnnotatedQuery", String.class, Pageable.class);
		query.execute(new Object[] { "fake", new PageRequest(0, 10) });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(1))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));
//		assertThat(captor.getValue().getMeta().getComment(), is("comment"));
	}

	/**
	 * @see DATA_JIRA-1057
	 */
	@Test
	public void slicedExecutionShouldRetainNrOfElementsToSkip() {

		SdbQueryFake query = createQueryForMethod("findByLastname", String.class, Pageable.class);
		Pageable page1 = new PageRequest(0, 10);
		Pageable page2 = page1.next();

		query.execute(new Object[] { "fake", page1 });
		query.execute(new Object[] { "fake", page2 });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(2))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

		assertThat(captor.getAllValues().get(0).getSkip(), is(0));
		assertThat(captor.getAllValues().get(1).getSkip(), is(10));
	}

	/**
	 * @see DATA_JIRA-1057
	 */
	@Test
	public void slicedExecutionShouldIncrementLimitByOne() {

		SdbQueryFake query = createQueryForMethod("findByLastname", String.class, Pageable.class);
		Pageable page1 = new PageRequest(0, 10);
		Pageable page2 = page1.next();

		query.execute(new Object[] { "fake", page1 });
		query.execute(new Object[] { "fake", page2 });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(2))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

		assertThat(captor.getAllValues().get(0).getLimit(), is(11));
		assertThat(captor.getAllValues().get(1).getLimit(), is(11));
	}

	/**
	 * @see DATA_JIRA-1057
	 */
	@Test
	public void slicedExecutionShouldRetainSort() {

		SdbQueryFake query = createQueryForMethod("findByLastname", String.class, Pageable.class);
		Pageable page1 = new PageRequest(0, 10, Sort.Direction.DESC, "bar");
		Pageable page2 = page1.next();

		query.execute(new Object[] { "fake", page1 });
		query.execute(new Object[] { "fake", page2 });

		ArgumentCaptor<Query> captor = ArgumentCaptor.forClass(Query.class);

		verify(this.sequoiadbOperationsMock, times(2))
				.find(captor.capture(), Matchers.eq(Person.class), Matchers.eq("persons"));

		BSONObject expectedSortObject = new BasicBSONObjectBuilder().add("bar", -1).get();
		assertThat(captor.getAllValues().get(0).getSortObject(), is(expectedSortObject));
		assertThat(captor.getAllValues().get(1).getSortObject(), is(expectedSortObject));
	}

	/**
	 * @see DATA_JIRA-1080
	 */
	@Test
	public void doesNotTryToPostProcessQueryResultIntoWrapperType() {

		Person reference = new Person();
		when(sequoiadbOperationsMock.findOne(Mockito.any(Query.class), eq(Person.class), eq("persons"))).//
				thenReturn(reference);

		AbstractSequoiadbQuery query = createQueryForMethod("findByLastname", String.class);

		assertThat(query.execute(new Object[] { "lastname" }), is((Object) reference));
	}

	private SdbQueryFake createQueryForMethod(String methodName, Class<?>... paramTypes) {

		try {

			Method method = Repo.class.getMethod(methodName, paramTypes);
			SequoiadbQueryMethod queryMethod = new SequoiadbQueryMethod(method, metadataMock, mappingContextMock);

			return new SdbQueryFake(queryMethod, sequoiadbOperationsMock);

		} catch (NoSuchMethodException e) {
			throw new IllegalArgumentException(e.getMessage(), e);
		} catch (SecurityException e) {
			throw new IllegalArgumentException(e.getMessage(), e);
		}
	}

	private static class SdbQueryFake extends AbstractSequoiadbQuery {

		private boolean isCountQuery;
		private boolean isDeleteQuery;

		public SdbQueryFake(SequoiadbQueryMethod method, SequoiadbOperations operations) {
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

		public SdbQueryFake setDeleteQuery(boolean isDeleteQuery) {
			this.isDeleteQuery = isDeleteQuery;
			return this;
		}
	}

	private interface Repo extends SequoiadbRepository<Person, Long> {

		List<Person> deleteByLastname(String lastname);

		Long deletePersonByLastname(String lastname);

		List<Person> findByFirstname(String firstname);

		@Meta(comment = "comment")
		Page<Person> findByFirstname(String firstnanme, Pageable pageable);

		@Meta(comment = "comment")
		@org.springframework.data.sequoiadb.repository.Query("{}")
		Page<Person> findByAnnotatedQuery(String firstnanme, Pageable pageable);

		/** @see DATA_JIRA-1057 */
		Slice<Person> findByLastname(String lastname, Pageable page);

//		Optional<Person> findByLastname(String lastname);
	}
}
