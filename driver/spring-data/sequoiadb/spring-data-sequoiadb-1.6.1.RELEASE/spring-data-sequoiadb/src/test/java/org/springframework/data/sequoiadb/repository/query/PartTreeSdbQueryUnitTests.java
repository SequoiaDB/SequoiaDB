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
import static org.mockito.Mockito.*;
import static org.springframework.data.sequoiadb.core.query.IsTextQuery.*;

import java.lang.reflect.Method;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.BasicBSONObjectBuilder;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.TextCriteria;
import org.springframework.data.sequoiadb.repository.SequoiadbRepository;
import org.springframework.data.sequoiadb.repository.Person;
import org.springframework.data.sequoiadb.repository.Query;
import org.springframework.data.repository.core.RepositoryMetadata;

/**
 * Unit tests for {@link PartTreeSequoiadbQuery}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class PartTreeSdbQueryUnitTests {

	@Mock RepositoryMetadata metadataMock;
	@Mock
    SequoiadbOperations sequoiadbOperationsMock;

	SequoiadbMappingContext mappingContext;

	public @Rule ExpectedException exception = ExpectedException.none();

	@Before
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void setUp() {

		when(metadataMock.getDomainType()).thenReturn((Class) Person.class);
		when(metadataMock.getReturnedDomainClass(Matchers.any(Method.class))).thenReturn((Class) Person.class);
		mappingContext = new SequoiadbMappingContext();
		DbRefResolver dbRefResolver = new DefaultDbRefResolver(mock(SequoiadbFactory.class));
		SequoiadbConverter converter = new MappingSequoiadbConverter(dbRefResolver, mappingContext);

		when(sequoiadbOperationsMock.getConverter()).thenReturn(converter);
	}

	/**
	 * @see DATAMOGO-952
	 */
	@Test
	public void rejectsInvalidFieldSpecification() {

		exception.expect(IllegalStateException.class);
		exception.expectMessage("findByLastname");

		deriveQueryFromMethod("findByLastname", new Object[] { "foo" });
	}

	/**
	 * @see DATAMOGO-952
	 */
	@Test
	public void singleFieldJsonIncludeRestrictionShouldBeConsidered() {

		org.springframework.data.sequoiadb.core.query.Query query = deriveQueryFromMethod("findByFirstname",
				new Object[] { "foo" });

		assertThat(query.getFieldsObject(), is(new BasicBSONObjectBuilder().add("firstname", 1).get()));
	}

	/**
	 * @see DATAMOGO-952
	 */
	@Test
	public void multiFieldJsonIncludeRestrictionShouldBeConsidered() {

		org.springframework.data.sequoiadb.core.query.Query query = deriveQueryFromMethod("findByFirstnameAndLastname",
				new Object[] { "foo", "bar" });

		assertThat(query.getFieldsObject(), is(new BasicBSONObjectBuilder().add("firstname", 1).add("lastname", 1).get()));
	}

	/**
	 * @see DATAMOGO-952
	 */
	@Test
	public void multiFieldJsonExcludeRestrictionShouldBeConsidered() {

		org.springframework.data.sequoiadb.core.query.Query query = deriveQueryFromMethod("findPersonByFirstnameAndLastname",
				new Object[] { "foo", "bar" });

		assertThat(query.getFieldsObject(), is(new BasicBSONObjectBuilder().add("firstname", 0).add("lastname", 0).get()));
	}

	/**
	 * @see DATAMOGO-973
	 */
	@Test
	public void shouldAddFullTextParamCorrectlyToDerivedQuery() {

		org.springframework.data.sequoiadb.core.query.Query query = deriveQueryFromMethod("findPersonByFirstname",
				new Object[] { "text", TextCriteria.forDefaultLanguage().matching("search") });

		assertThat(query, isTextQuery().searchingFor("search").where(new Criteria("firstname").is("text")));
	}

	private org.springframework.data.sequoiadb.core.query.Query deriveQueryFromMethod(String method, Object[] args) {

		Class<?>[] types = new Class<?>[args.length];

		for (int i = 0; i < args.length; i++) {
			types[i] = args[i].getClass();
		}

		PartTreeSequoiadbQuery partTreeQuery = createQueryForMethod(method, types);

		SequoiadbParameterAccessor accessor = new SequoiadbParametersParameterAccessor(partTreeQuery.getQueryMethod(), args);
		return partTreeQuery.createQuery(new ConvertingParameterAccessor(sequoiadbOperationsMock.getConverter(), accessor));
	}

	private PartTreeSequoiadbQuery createQueryForMethod(String methodName, Class<?>... paramTypes) {

		try {

			Method method = Repo.class.getMethod(methodName, paramTypes);
			SequoiadbQueryMethod queryMethod = new SequoiadbQueryMethod(method, metadataMock, mappingContext);

			return new PartTreeSequoiadbQuery(queryMethod, sequoiadbOperationsMock);
		} catch (NoSuchMethodException e) {
			throw new IllegalArgumentException(e.getMessage(), e);
		} catch (SecurityException e) {
			throw new IllegalArgumentException(e.getMessage(), e);
		}
	}

	interface Repo extends SequoiadbRepository<Person, Long> {

		@Query(fields = "firstname")
		Person findByLastname(String lastname);

		@Query(fields = "{ 'firstname' : 1 }")
		Person findByFirstname(String lastname);

		@Query(fields = "{ 'firstname' : 1, 'lastname' : 1 }")
		Person findByFirstnameAndLastname(String firstname, String lastname);

		@Query(fields = "{ 'firstname' : 0, 'lastname' : 0 }")
		Person findPersonByFirstnameAndLastname(String firstname, String lastname);

		Person findPersonByFirstname(String firstname, TextCriteria fullText);
	}
}
