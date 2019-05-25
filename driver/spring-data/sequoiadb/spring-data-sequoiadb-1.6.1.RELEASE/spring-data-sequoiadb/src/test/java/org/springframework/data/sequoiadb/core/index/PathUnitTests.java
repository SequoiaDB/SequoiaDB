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
package org.springframework.data.sequoiadb.core.index;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.core.index.SequoiadbPersistentEntityIndexResolver.CycleGuard.Path;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;

/**
 * Unit tests for {@link Path}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class PathUnitTests {

	@Mock
    SequoiadbPersistentEntity<?> entityMock;

	@Before
	@SuppressWarnings({ "rawtypes", "unchecked" })
	public void setUp() {
		when(entityMock.getType()).thenReturn((Class) Object.class);
	}

	/**
	 * @see DATA_JIRA-962
	 */
	@Test
	public void shouldIdentifyCycleForOwnerOfSameTypeAndMatchingPath() {

		SequoiadbPersistentProperty property = createPersistentPropertyMock(entityMock, "foo");
		assertThat(new Path(property, "foo.bar").cycles(property, "foo.bar.bar"), is(true));
	}

	/**
	 * @see DATA_JIRA-962
	 */
	@Test
	@SuppressWarnings("rawtypes")
	public void shouldAllowMatchingPathForDifferentOwners() {

		SequoiadbPersistentProperty existing = createPersistentPropertyMock(entityMock, "foo");

		SequoiadbPersistentEntity entityOfDifferentType = Mockito.mock(SequoiadbPersistentEntity.class);
		when(entityOfDifferentType.getType()).thenReturn(String.class);
		SequoiadbPersistentProperty toBeVerified = createPersistentPropertyMock(entityOfDifferentType, "foo");

		assertThat(new Path(existing, "foo.bar").cycles(toBeVerified, "foo.bar.bar"), is(false));
	}

	/**
	 * @see DATA_JIRA-962
	 */
	@Test
	public void shouldAllowEqaulPropertiesOnDifferentPaths() {

		SequoiadbPersistentProperty property = createPersistentPropertyMock(entityMock, "foo");
		assertThat(new Path(property, "foo.bar").cycles(property, "foo2.bar.bar"), is(false));
	}

	@SuppressWarnings({ "rawtypes", "unchecked" })
	private SequoiadbPersistentProperty createPersistentPropertyMock(SequoiadbPersistentEntity owner, String fieldname) {

		SequoiadbPersistentProperty property = Mockito.mock(SequoiadbPersistentProperty.class);
		when(property.getOwner()).thenReturn(owner);
		when(property.getFieldName()).thenReturn(fieldname);
		return property;
	}
}
