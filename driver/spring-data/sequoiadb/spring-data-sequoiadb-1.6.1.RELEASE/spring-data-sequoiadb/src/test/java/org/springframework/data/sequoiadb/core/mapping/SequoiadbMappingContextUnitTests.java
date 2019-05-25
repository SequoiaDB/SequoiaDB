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

import java.util.AbstractMap;
import java.util.Collections;
import java.util.Locale;
import java.util.Map;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.context.ApplicationContext;
import org.springframework.data.annotation.Id;
import org.springframework.data.mapping.PersistentProperty;
import org.springframework.data.mapping.model.FieldNamingStrategy;
import org.springframework.data.mapping.model.MappingException;

import org.springframework.data.sequoiadb.assist.DBRef;

/**
 * Unit tests for {@link SequoiadbMappingContext}.
 * 



 */
@RunWith(MockitoJUnitRunner.class)
public class SequoiadbMappingContextUnitTests {

	@Mock ApplicationContext applicationContext;

	@Rule public ExpectedException exception = ExpectedException.none();

	@Test
	public void addsSelfReferencingPersistentEntityCorrectly() throws Exception {

		SequoiadbMappingContext context = new SequoiadbMappingContext();

		context.setInitialEntitySet(Collections.singleton(SampleClass.class));
		context.initialize();
	}

	@Test
	public void doesNotReturnPersistentEntityForSequoiadbSimpleType() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		assertThat(context.getPersistentEntity(DBRef.class), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-638
	 */
	@Test
	public void doesNotCreatePersistentEntityForAbstractMap() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		assertThat(context.getPersistentEntity(AbstractMap.class), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-607
	 */
	@Test
	public void populatesPersistentPropertyWithCustomFieldNamingStrategy() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.setApplicationContext(applicationContext);
		context.setFieldNamingStrategy(new FieldNamingStrategy() {

			public String getFieldName(PersistentProperty<?> property) {
				return property.getName().toUpperCase(Locale.US);
			}
		});

		SequoiadbPersistentEntity<?> entity = context.getPersistentEntity(Person.class);
		assertThat(entity.getPersistentProperty("firstname").getFieldName(), is("FIRSTNAME"));
	}

	/**
	 * @see DATA_JIRA-607
	 */
	@Test
	public void rejectsClassWithAmbiguousFieldMappings() {

		exception.expect(MappingException.class);
		exception.expectMessage("firstname");
		exception.expectMessage("lastname");
		exception.expectMessage("foo");

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.setApplicationContext(applicationContext);
		context.getPersistentEntity(InvalidPerson.class);
	}

	/**
	 * @see DATA_JIRA-694
	 */
	@Test
	public void doesNotConsiderOverrridenAccessorANewField() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.setApplicationContext(applicationContext);
		context.getPersistentEntity(Child.class);
	}

	/**
	 * @see DATA_JIRA-688
	 */
	@Test
	public void mappingContextShouldAcceptClassWithImplicitIdProperty() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		BasicSequoiadbPersistentEntity<?> pe = context.getPersistentEntity(ClassWithImplicitId.class);

		assertThat(pe, is(not(nullValue())));
		assertThat(pe.isIdProperty(pe.getPersistentProperty("id")), is(true));
	}

	/**
	 * @see DATA_JIRA-688
	 */
	@Test
	public void mappingContextShouldAcceptClassWithExplicitIdProperty() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		BasicSequoiadbPersistentEntity<?> pe = context.getPersistentEntity(ClassWithExplicitId.class);

		assertThat(pe, is(not(nullValue())));
		assertThat(pe.isIdProperty(pe.getPersistentProperty("myId")), is(true));
	}

	/**
	 * @see DATA_JIRA-688
	 */
	@Test
	public void mappingContextShouldAcceptClassWithExplicitAndImplicitIdPropertyByGivingPrecedenceToExplicitIdProperty() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		BasicSequoiadbPersistentEntity<?> pe = context.getPersistentEntity(ClassWithExplicitIdAndImplicitId.class);
		assertThat(pe, is(not(nullValue())));
	}

	/**
	 * @see DATA_JIRA-688
	 */
	@Test(expected = MappingException.class)
	public void rejectsClassWithAmbiguousExplicitIdPropertyFieldMappings() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.getPersistentEntity(ClassWithMultipleExplicitIds.class);
	}

	/**
	 * @see DATA_JIRA-688
	 */
	@Test(expected = MappingException.class)
	public void rejectsClassWithAmbiguousImplicitIdPropertyFieldMappings() {

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.getPersistentEntity(ClassWithMultipleImplicitIds.class);
	}

	/**
	 * @see DATA_JIRA-976
	 */
	@Test
	public void shouldRejectClassWithInvalidTextScoreProperty() {

		exception.expect(MappingException.class);
		exception.expectMessage("score");
		exception.expectMessage("Float");
		exception.expectMessage("Double");

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.getPersistentEntity(ClassWithInvalidTextScoreProperty.class);
	}

	public class SampleClass {

		Map<String, SampleClass> children;
	}

	class Person {

		String firstname, lastname;
	}

	class InvalidPerson {

		@org.springframework.data.sequoiadb.core.mapping.Field("foo") String firstname, lastname;
	}

	class Parent {

		String name;

		public String getName() {
			return name;
		}
	}

	class Child extends Parent {

		@Override
		public String getName() {
			return super.getName();
		}
	}

	class ClassWithImplicitId {

		String field;
		String id;
	}

	class ClassWithExplicitId {

		@Id String myId;
		String field;
	}

	class ClassWithExplicitIdAndImplicitId {

		@Id String myId;
		String id;
	}

	class ClassWithMultipleExplicitIds {

		@Id String myId;
		@Id String id;
	}

	class ClassWithMultipleImplicitIds {

		String _id;
		String id;
	}

	class ClassWithInvalidTextScoreProperty {

		@TextScore Locale score;
	}
}
