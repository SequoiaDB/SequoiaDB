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
package org.springframework.data.sequoiadb.repository.query;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;
import static org.springframework.data.sequoiadb.core.query.Criteria.*;
import static org.springframework.data.sequoiadb.core.query.Query.*;
import static org.springframework.data.sequoiadb.repository.query.StubParameterAccessor.*;

import java.lang.reflect.Method;
import java.util.List;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.Point;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.sequoiadb.core.Person;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.DBRef;
import org.springframework.data.sequoiadb.core.mapping.Field;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.core.support.DefaultRepositoryMetadata;
import org.springframework.data.repository.query.parser.PartTree;
import org.springframework.data.util.TypeInformation;

/**
 * Unit test for {@link sequoiadbQueryCreator}.
 * 



 */
@RunWith(MockitoJUnitRunner.class)
public class SdbQueryCreatorUnitTests {

	Method findByFirstname, findByFirstnameAndFriend, findByFirstnameNotNull;

	@Mock
    SequoiadbConverter converter;

	MappingContext<?, SequoiadbPersistentProperty> context;

	@Rule public ExpectedException expection = ExpectedException.none();

	@Before
	public void setUp() throws SecurityException, NoSuchMethodException {

		context = new SequoiadbMappingContext();

		doAnswer(new Answer<Object>() {
			public Object answer(InvocationOnMock invocation) throws Throwable {
				return invocation.getArguments()[0];
			}
		}).when(converter).convertToSequoiadbType(any(), Mockito.any(TypeInformation.class));
	}

	@Test
	public void createsQueryCorrectly() throws Exception {

		PartTree tree = new PartTree("findByFirstName", Person.class);

		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "Oliver"), context);
		Query query = creator.createQuery();
		assertThat(query, is(query(where("firstName").is("Oliver"))));
	}

	/**
	 * @see DATA_JIRA-469
	 */
	@Test
	public void createsAndQueryCorrectly() {

		Person person = new Person();
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(new PartTree("findByFirstNameAndFriend", Person.class),
				getAccessor(converter, "Oliver", person), context);
		Query query = creator.createQuery();

		assertThat(query, is(query(where("firstName").is("Oliver").and("friend").is(person))));
	}

	@Test
	public void createsNotNullQueryCorrectly() {

		PartTree tree = new PartTree("findByFirstNameNotNull", Person.class);
		Query query = new sequoiadbQueryCreator(tree, getAccessor(converter), context).createQuery();

		assertThat(query, is(new Query(Criteria.where("firstName").ne(null))));
	}

	@Test
	public void createsIsNullQueryCorrectly() {

		PartTree tree = new PartTree("findByFirstNameIsNull", Person.class);
		Query query = new sequoiadbQueryCreator(tree, getAccessor(converter), context).createQuery();

		assertThat(query, is(new Query(Criteria.where("firstName").is(null))));
	}

	@Test
	public void bindsMetricDistanceParameterToNearSphereCorrectly() throws Exception {

	}

	@Test
	public void bindsDistanceParameterToNearCorrectly() throws Exception {

	}

	@Test
	public void createsLessThanEqualQueryCorrectly() throws Exception {

		PartTree tree = new PartTree("findByAgeLessThanEqual", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, 18), context);

		Query reference = query(where("age").lte(18));
		assertThat(creator.createQuery(), is(reference));
	}

	@Test
	public void createsGreaterThanEqualQueryCorrectly() throws Exception {

		PartTree tree = new PartTree("findByAgeGreaterThanEqual", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, 18), context);

		Query reference = query(where("age").gte(18));
		assertThat(creator.createQuery(), is(reference));
	}

	/**
	 * @see DATA_JIRA-338
	 */
	@Test
	public void createsExistsClauseCorrectly() {

		PartTree tree = new PartTree("findByAgeExists", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, true), context);
		Query query = query(where("age").exists(1));
		assertThat(creator.createQuery(), is(query));
	}

	/**
	 * @see DATA_JIRA-338
	 */
	@Test
	public void createsRegexClauseCorrectly() {

		PartTree tree = new PartTree("findByFirstNameRegex", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, ".*"), context);
		Query query = query(where("firstName").regex(".*"));
		assertThat(creator.createQuery(), is(query));
	}

	/**
	 * @see DATA_JIRA-338
	 */
	@Test
	public void createsTrueClauseCorrectly() {

		PartTree tree = new PartTree("findByActiveTrue", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter), context);
		Query query = query(where("active").is(true));
		assertThat(creator.createQuery(), is(query));
	}

	/**
	 * @see DATA_JIRA-338
	 */
	@Test
	public void createsFalseClauseCorrectly() {

		PartTree tree = new PartTree("findByActiveFalse", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter), context);
		Query query = query(where("active").is(false));
		assertThat(creator.createQuery(), is(query));
	}

	/**
	 * @see DATA_JIRA-413
	 */
	@Test
	public void createsOrQueryCorrectly() {

		PartTree tree = new PartTree("findByFirstNameOrAge", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "Dave", 42), context);

		Query query = creator.createQuery();
		assertThat(query, is(query(new Criteria().orOperator(where("firstName").is("Dave"), where("age").is(42)))));
	}

	/**
	 * @see DATA_JIRA-347
	 */
	@Test
	public void createsQueryReferencingADBRefCorrectly() {

		User user = new User();
		org.springframework.data.sequoiadb.assist.DBRef dbref = new org.springframework.data.sequoiadb.assist.DBRef(null, "user", "id");
		when(converter.toDBRef(eq(user), Mockito.any(SequoiadbPersistentProperty.class))).thenReturn(dbref);

		PartTree tree = new PartTree("findByCreator", User.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, user), context);
		Query query = creator.createQuery();

		assertThat(query, is(query(where("creator").is(dbref))));
	}

	/**
	 * @see DATA_JIRA-418
	 */
	@Test
	public void createsQueryWithStartingWithPredicateCorrectly() {

		PartTree tree = new PartTree("findByUsernameStartingWith", User.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "Matt"), context);
		Query query = creator.createQuery();

		assertThat(query, is(query(where("username").regex("^Matt"))));
	}

	/**
	 * @see DATA_JIRA-418
	 */
	@Test
	public void createsQueryWithEndingWithPredicateCorrectly() {

		PartTree tree = new PartTree("findByUsernameEndingWith", User.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "ews"), context);
		Query query = creator.createQuery();

		assertThat(query, is(query(where("username").regex("ews$"))));
	}

	/**
	 * @see DATA_JIRA-418
	 */
	@Test
	public void createsQueryWithContainingPredicateCorrectly() {

		PartTree tree = new PartTree("findByUsernameContaining", User.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "thew"), context);
		Query query = creator.createQuery();

		assertThat(query, is(query(where("username").regex(".*thew.*"))));
	}

	private void assertBindsDistanceToQuery(Point point, Distance distance, Query reference) throws Exception {

		when(converter.convertToSequoiadbType("Dave")).thenReturn("Dave");

		PartTree tree = new PartTree("findByLocationNearAndFirstname",
				org.springframework.data.sequoiadb.repository.Person.class);
		Method method = PersonRepository.class.getMethod("findByLocationNearAndFirstname", Point.class, Distance.class,
				String.class);
		SequoiadbQueryMethod queryMethod = new SequoiadbQueryMethod(method, new DefaultRepositoryMetadata(PersonRepository.class),
				new SequoiadbMappingContext());
		SequoiadbParameterAccessor accessor = new SequoiadbParametersParameterAccessor(queryMethod, new Object[] { point, distance,
				"Dave" });

		Query query = new sequoiadbQueryCreator(tree, new ConvertingParameterAccessor(converter, accessor), context)
				.createQuery();
		assertThat(query, is(query));
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void createsQueryWithFindByIgnoreCaseCorrectly() {

		PartTree tree = new PartTree("findByfirstNameIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave"), context);

		Query query = creator.createQuery();
		assertThat(query, is(query(where("firstName").regex("^dave$", "i"))));
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void createsQueryWithFindByNotIgnoreCaseCorrectly() {

		PartTree tree = new PartTree("findByFirstNameNotIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave"), context);

		Query query = creator.createQuery();
		String str = query(new Criteria().notOperator(where("firstName").regex("^dave$", "i"))).toString();
		assertThat(query.toString(), is(str));
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void createsQueryWithFindByStartingWithIgnoreCaseCorrectly() {

		PartTree tree = new PartTree("findByFirstNameStartingWithIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave"), context);

		Query query = creator.createQuery();
		assertThat(query, is(query(where("firstName").regex("^dave", "i"))));
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void createsQueryWithFindByEndingWithIgnoreCaseCorrectly() {

		PartTree tree = new PartTree("findByFirstNameEndingWithIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave"), context);

		Query query = creator.createQuery();
		assertThat(query, is(query(where("firstName").regex("dave$", "i"))));
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void createsQueryWithFindByContainingIgnoreCaseCorrectly() {

		PartTree tree = new PartTree("findByFirstNameContainingIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave"), context);

		Query query = creator.createQuery();
		assertThat(query, is(query(where("firstName").regex(".*dave.*", "i"))));
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void shouldThrowExceptionForQueryWithFindByIgnoreCaseOnNonStringProperty() {

		expection.expect(IllegalArgumentException.class);
		expection.expectMessage("must be of type String");

		PartTree tree = new PartTree("findByFirstNameAndAgeIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "foo", 42), context);

		creator.createQuery();
	}

	/**
	 * @see DATA_JIRA-770
	 */
	@Test
	public void shouldOnlyGenerateLikeExpressionsForStringPropertiesIfAllIgnoreCase() {

		PartTree tree = new PartTree("findByFirstNameAndAgeAllIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave", 42), context);

		Query query = creator.createQuery();
		assertThat(query, is(query(where("firstName").regex("^dave$", "i").and("age").is(42))));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	public void shouldCreateDeleteByQueryCorrectly() {

		PartTree tree = new PartTree("deleteByFirstName", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave", 42), context);

		Query query = creator.createQuery();

		assertThat(tree.isDelete(), is(true));
		assertThat(query, is(query(where("firstName").is("dave"))));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	public void shouldCreateDeleteByQueryCorrectlyForMultipleCriteriaAndCaseExpressions() {

		PartTree tree = new PartTree("deleteByFirstNameAndAgeAllIgnoreCase", Person.class);
		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, getAccessor(converter, "dave", 42), context);

		Query query = creator.createQuery();

		assertThat(tree.isDelete(), is(true));
		assertThat(query, is(query(where("firstName").regex("^dave$", "i").and("age").is(42))));
	}

	interface PersonRepository extends Repository<Person, Long> {

		List<Person> findByLocationNearAndFirstname(Point location, Distance maxDistance, String firstname);
	}

	class User {

		@Field("foo") String username;

		@DBRef User creator;
	}
}
