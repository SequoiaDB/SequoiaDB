/*
 * Copyright 2011-2014 by the original author(s).
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
package org.springframework.data.sequoiadb.core.mapping;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.context.ApplicationContext;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.util.ClassTypeInformation;

/**
 * Unit tests for {@link BasicSequoiadbPersistentEntity}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class BasicSequoiadbPersistentEntityUnitTests {

	@Mock ApplicationContext context;
	@Mock
    SequoiadbPersistentProperty propertyMock;

	@Test
	public void subclassInheritsAtDocumentAnnotation() {

		BasicSequoiadbPersistentEntity<Person> entity = new BasicSequoiadbPersistentEntity<Person>(
				ClassTypeInformation.from(Person.class));
		assertThat(entity.getCollection(), is("contacts"));
	}

	@Test
	public void evaluatesSpELExpression() {

		SequoiadbPersistentEntity<Company> entity = new BasicSequoiadbPersistentEntity<Company>(
				ClassTypeInformation.from(Company.class));
		assertThat(entity.getCollection(), is("35"));
	}

	@Test
	public void collectionAllowsReferencingSpringBean() {

		CollectionProvider provider = new CollectionProvider();
		provider.collectionName = "reference";

		when(context.getBean("myBean")).thenReturn(provider);
		when(context.containsBean("myBean")).thenReturn(true);

		BasicSequoiadbPersistentEntity<DynamicallyMapped> entity = new BasicSequoiadbPersistentEntity<DynamicallyMapped>(
				ClassTypeInformation.from(DynamicallyMapped.class));
		entity.setApplicationContext(context);

		assertThat(entity.getCollection(), is("reference"));
	}

	/**
	 * @see DATA_JIRA-937
	 */
	@Test
	public void shouldDetectLanguageCorrectly() {

		BasicSequoiadbPersistentEntity<DocumentWithLanguage> entity = new BasicSequoiadbPersistentEntity<DocumentWithLanguage>(
				ClassTypeInformation.from(DocumentWithLanguage.class));
		assertThat(entity.getLanguage(), is("spanish"));
	}

	/**
	 * @see DATA_JIRA-1053
	 */
	@SuppressWarnings({ "unchecked", "rawtypes" })
	@Test(expected = MappingException.class)
	public void verifyShouldThrowExceptionForInvalidTypeOfExplicitLanguageProperty() {

		BasicSequoiadbPersistentEntity<AnyDocument> entity = new BasicSequoiadbPersistentEntity<AnyDocument>(
				ClassTypeInformation.from(AnyDocument.class));

		when(propertyMock.isExplicitLanguageProperty()).thenReturn(true);
		when(propertyMock.getActualType()).thenReturn((Class) Number.class);

		entity.addPersistentProperty(propertyMock);
		entity.verify();
	}

	/**
	 * @see DATA_JIRA-1053
	 */
	@SuppressWarnings({ "unchecked", "rawtypes" })
	@Test
	public void verifyShouldPassForStringAsExplicitLanguageProperty() {

		BasicSequoiadbPersistentEntity<AnyDocument> entity = new BasicSequoiadbPersistentEntity<AnyDocument>(
				ClassTypeInformation.from(AnyDocument.class));
		when(propertyMock.isExplicitLanguageProperty()).thenReturn(true);
		when(propertyMock.getActualType()).thenReturn((Class) String.class);
		entity.addPersistentProperty(propertyMock);

		entity.verify();

		verify(propertyMock, times(1)).isExplicitLanguageProperty();
		verify(propertyMock, times(1)).getActualType();
	}

	/**
	 * @see DATA_JIRA-1053
	 */
	@SuppressWarnings({ "unchecked", "rawtypes" })
	@Test
	public void verifyShouldIgnoreNonExplicitLanguageProperty() {

		BasicSequoiadbPersistentEntity<AnyDocument> entity = new BasicSequoiadbPersistentEntity<AnyDocument>(
				ClassTypeInformation.from(AnyDocument.class));
		when(propertyMock.isExplicitLanguageProperty()).thenReturn(false);
		when(propertyMock.getActualType()).thenReturn((Class) Number.class);
		entity.addPersistentProperty(propertyMock);

		entity.verify();

		verify(propertyMock, times(1)).isExplicitLanguageProperty();
		verify(propertyMock, never()).getActualType();
	}

	@Document(collection = "contacts")
	class Contact {

	}

	class Person extends Contact {

	}

	@Document(collection = "#{35}")
	class Company {

	}

	@Document(collection = "#{myBean.collectionName}")
	class DynamicallyMapped {

	}

	class CollectionProvider {
		String collectionName;

		public String getCollectionName() {
			return collectionName;
		}
	}

	@Document(language = "spanish")
	static class DocumentWithLanguage {

	}

	static class AnyDocument {

	}
}
