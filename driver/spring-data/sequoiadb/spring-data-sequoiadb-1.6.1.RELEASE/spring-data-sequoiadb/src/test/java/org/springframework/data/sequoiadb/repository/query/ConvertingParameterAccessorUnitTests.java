/*
 * Copyright 2011-2013 the original author or authors.
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

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import java.util.Arrays;
import java.util.Collection;

import org.bson.types.BasicBSONList;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.DBRef;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.repository.query.ConvertingParameterAccessor.PotentiallyConvertingIterator;



/**
 * Unit tests for {@link ConvertingParameterAccessor}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class ConvertingParameterAccessorUnitTests {

	@Mock
	SequoiadbFactory factory;
	@Mock
	SequoiadbParameterAccessor accessor;

	SequoiadbMappingContext context;
	MappingSequoiadbConverter converter;
	DbRefResolver resolver;

	@Before
	public void setUp() {

		this.context = new SequoiadbMappingContext();
		this.resolver = new DefaultDbRefResolver(factory);
		this.converter = new MappingSequoiadbConverter(resolver, context);
	}

	@SuppressWarnings("deprecation")
	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullSequoiadbFactory() {
		new MappingSequoiadbConverter((SequoiadbFactory) null, context);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullDbRefResolver() {
		new MappingSequoiadbConverter((DbRefResolver) null, context);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullContext() {
		new MappingSequoiadbConverter(resolver, null);
	}

	@Test
	public void convertsCollectionUponAccess() {

		when(accessor.getBindableValue(0)).thenReturn(Arrays.asList("Foo"));

		ConvertingParameterAccessor parameterAccessor = new ConvertingParameterAccessor(converter, accessor);
		Object result = parameterAccessor.getBindableValue(0);

		BasicBSONList reference = new BasicBSONList();
		reference.add("Foo");

		assertThat(result, is((Object) reference));
	}

	/**
	 * @see DATA_JIRA-505
	 */
	@Test
	public void convertsAssociationsToDBRef() {

		Property property = new Property();
		property.id = 5L;

		Object result = setupAndConvert(property);

		assertThat(result, is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
		org.springframework.data.sequoiadb.assist.DBRef dbRef = (org.springframework.data.sequoiadb.assist.DBRef) result;
		assertThat(dbRef.getRef(), is("property"));
		assertThat(dbRef.getId(), is((Object) 5L));
	}

	/**
	 * @see DATA_JIRA-505
	 */
	@Test
	public void convertsAssociationsToDBRefForCollections() {

		Property property = new Property();
		property.id = 5L;

		Object result = setupAndConvert(Arrays.asList(property));

		assertThat(result, is(instanceOf(Collection.class)));
		Collection<?> collection = (Collection<?>) result;

		assertThat(collection, hasSize(1));
		Object element = collection.iterator().next();

		assertThat(element, is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
		org.springframework.data.sequoiadb.assist.DBRef dbRef = (org.springframework.data.sequoiadb.assist.DBRef) element;
		assertThat(dbRef.getRef(), is("property"));
		assertThat(dbRef.getId(), is((Object) 5L));
	}

	private Object setupAndConvert(Object... parameters) {

		SequoiadbParameterAccessor delegate = new StubParameterAccessor(parameters);
		PotentiallyConvertingIterator iterator = new ConvertingParameterAccessor(converter, delegate).iterator();

		SequoiadbPersistentEntity<?> entity = context.getPersistentEntity(Entity.class);
		SequoiadbPersistentProperty property = entity.getPersistentProperty("property");

		return iterator.nextConverted(property);
	}

	static class Entity {

		@DBRef Property property;
	}

	static class Property {

		Long id;
	}
}
