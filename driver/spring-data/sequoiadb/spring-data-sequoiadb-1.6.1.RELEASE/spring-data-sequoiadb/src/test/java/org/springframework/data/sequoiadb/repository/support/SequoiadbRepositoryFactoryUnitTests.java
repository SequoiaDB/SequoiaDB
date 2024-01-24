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
package org.springframework.data.sequoiadb.repository.support;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import java.io.Serializable;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.repository.Person;
import org.springframework.data.sequoiadb.repository.query.SequoiadbEntityInformation;
import org.springframework.data.repository.Repository;

/**
 * Unit test for {@link SequoiadbRepositoryFactory}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class SequoiadbRepositoryFactoryUnitTests {

	@Mock
    SequoiadbTemplate template;

	@Mock
    SequoiadbConverter converter;

	@Mock
	@SuppressWarnings("rawtypes")
	MappingContext mappingContext;

	@Mock
	@SuppressWarnings("rawtypes")
	SequoiadbPersistentEntity entity;

	@Before
	@SuppressWarnings("unchecked")
	public void setUp() {
		when(template.getConverter()).thenReturn(converter);
		when(converter.getMappingContext()).thenReturn(mappingContext);
	}

	@Test
	@SuppressWarnings("unchecked")
	public void usesMappingSequoiadbEntityInformationIfMappingContextSet() {

		when(mappingContext.getPersistentEntity(Person.class)).thenReturn(entity);
		when(entity.getType()).thenReturn(Person.class);

		SequoiadbRepositoryFactory factory = new SequoiadbRepositoryFactory(template);
		SequoiadbEntityInformation<Person, Serializable> entityInformation = factory.getEntityInformation(Person.class);
		assertTrue(entityInformation instanceof MappingSequoiadbEntityInformation);
	}

	/**
	 * @see DATA_JIRA-385
	 */
	@Test
	@SuppressWarnings("unchecked")
	public void createsRepositoryWithIdTypeLong() {

		when(mappingContext.getPersistentEntity(Person.class)).thenReturn(entity);
		when(entity.getType()).thenReturn(Person.class);

		SequoiadbRepositoryFactory factory = new SequoiadbRepositoryFactory(template);
		MyPersonRepository repository = factory.getRepository(MyPersonRepository.class);
		assertThat(repository, is(notNullValue()));
	}

	interface MyPersonRepository extends Repository<Person, Long> {

	}
}
