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
package org.springframework.data.mongodb.repository.query;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.lang.reflect.Method;
import java.util.List;

import org.hamcrest.core.IsNull;
import org.junit.Test;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.Metrics;
import org.springframework.data.geo.Point;
import org.springframework.data.mongodb.core.mapping.MongoMappingContext;
import org.springframework.data.mongodb.core.query.TextCriteria;
import org.springframework.data.mongodb.repository.Person;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.core.RepositoryMetadata;
import org.springframework.data.repository.core.support.DefaultRepositoryMetadata;

/**
 * Unit tests for {@link MongoParametersParameterAccessor}.
 * 
 * @author Oliver Gierke
 * @author Christoph Strobl
 */
public class MongoParametersParameterAccessorUnitTests {

	private static final Distance DISTANCE = new Distance(2.5, Metrics.KILOMETERS);
	private static final RepositoryMetadata metadata = new DefaultRepositoryMetadata(PersonRepository.class);
	private static final MongoMappingContext context = new MongoMappingContext();

	@Test
	public void returnsNullForDistanceIfNoneAvailable() throws NoSuchMethodException, SecurityException {

		Method method = PersonRepository.class.getMethod("findByLocationNear", Point.class);
		MongoQueryMethod queryMethod = new MongoQueryMethod(method, metadata, context);

		MongoParameterAccessor accessor = new MongoParametersParameterAccessor(queryMethod,
				new Object[] { new Point(10, 20) });
		assertThat(accessor.getMaxDistance(), is(nullValue()));
	}

	@Test
	public void returnsDistanceIfAvailable() throws NoSuchMethodException, SecurityException {

		Method method = PersonRepository.class.getMethod("findByLocationNear", Point.class, Distance.class);
		MongoQueryMethod queryMethod = new MongoQueryMethod(method, metadata, context);

		MongoParameterAccessor accessor = new MongoParametersParameterAccessor(queryMethod, new Object[] {
				new Point(10, 20), DISTANCE });
		assertThat(accessor.getMaxDistance(), is(DISTANCE));
	}

	/**
	 * @see DATAMONGO-973
	 */
	@Test
	public void shouldReturnAsFullTextStringWhenNoneDefinedForMethod() throws NoSuchMethodException, SecurityException {

		Method method = PersonRepository.class.getMethod("findByLocationNear", Point.class, Distance.class);
		MongoQueryMethod queryMethod = new MongoQueryMethod(method, metadata, context);

		MongoParameterAccessor accessor = new MongoParametersParameterAccessor(queryMethod, new Object[] {
				new Point(10, 20), DISTANCE });
		assertThat(accessor.getFullText(), IsNull.nullValue());
	}

	/**
	 * @see DATAMONGO-973
	 */
	@Test
	public void shouldProperlyConvertTextCriteria() throws NoSuchMethodException, SecurityException {

		Method method = PersonRepository.class.getMethod("findByFirstname", String.class, TextCriteria.class);
		MongoQueryMethod queryMethod = new MongoQueryMethod(method, metadata, context);

		MongoParameterAccessor accessor = new MongoParametersParameterAccessor(queryMethod, new Object[] { "spring",
				TextCriteria.forDefaultLanguage().matching("data") });
		assertThat(accessor.getFullText().getCriteriaObject().toString(),
				equalTo("{ \"$text\" : { \"$search\" : \"data\"}}"));
	}

	interface PersonRepository extends Repository<Person, Long> {

		List<Person> findByLocationNear(Point point);

		List<Person> findByLocationNear(Point point, Distance distance);

		List<Person> findByFirstname(String firstname, TextCriteria fullText);
	}
}
