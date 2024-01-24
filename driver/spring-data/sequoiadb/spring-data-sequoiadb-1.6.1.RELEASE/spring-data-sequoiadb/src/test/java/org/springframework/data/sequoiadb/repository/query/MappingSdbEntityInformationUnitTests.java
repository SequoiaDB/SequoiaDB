/*
 * Copyright (c) 2011 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.repository.Person;
import org.springframework.data.sequoiadb.repository.support.MappingSequoiadbEntityInformation;

/**
 * Unit tests for {@link MappingSequoiadbEntityInformation}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class MappingSdbEntityInformationUnitTests {

	@Mock
    SequoiadbPersistentEntity<Person> info;

	@Before
	public void setUp() {
		when(info.getType()).thenReturn(Person.class);
		when(info.getCollection()).thenReturn("Person");
	}

	@Test
	public void usesEntityCollectionIfNoCustomOneGiven() {
		SequoiadbEntityInformation<Person, Long> information = new MappingSequoiadbEntityInformation<Person, Long>(info);
		assertThat(information.getCollectionName(), is("Person"));
	}

	@Test
	public void usesCustomCollectionIfGiven() {
		SequoiadbEntityInformation<Person, Long> information = new MappingSequoiadbEntityInformation<Person, Long>(info, "foobar");
		assertThat(information.getCollectionName(), is("foobar"));
	}
}
