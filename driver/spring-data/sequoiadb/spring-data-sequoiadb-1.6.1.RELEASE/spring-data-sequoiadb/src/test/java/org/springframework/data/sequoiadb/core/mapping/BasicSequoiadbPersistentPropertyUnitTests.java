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

import java.lang.reflect.Field;
import java.util.Locale;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.springframework.data.annotation.Id;
import org.springframework.data.mapping.PersistentProperty;
import org.springframework.data.mapping.model.FieldNamingStrategy;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.mapping.model.PropertyNameFieldNamingStrategy;
import org.springframework.data.mapping.model.SimpleTypeHolder;
import org.springframework.data.util.ClassTypeInformation;
import org.springframework.util.ReflectionUtils;

/**
 * Unit test for {@link BasicSequoiadbPersistentProperty}.
 * 


 */
public class BasicSequoiadbPersistentPropertyUnitTests {

	SequoiadbPersistentEntity<Person> entity;

	@Rule public ExpectedException exception = ExpectedException.none();

	@Before
	public void setup() {
		entity = new BasicSequoiadbPersistentEntity<Person>(ClassTypeInformation.from(Person.class));
	}

	@Test
	public void usesAnnotatedFieldName() {

		Field field = ReflectionUtils.findField(Person.class, "firstname");
		assertThat(getPropertyFor(field).getFieldName(), is("foo"));
	}

	@Test
	public void returns_IdForIdProperty() {
		Field field = ReflectionUtils.findField(Person.class, "id");
		SequoiadbPersistentProperty property = getPropertyFor(field);
		assertThat(property.isIdProperty(), is(true));
		assertThat(property.getFieldName(), is("_id"));
	}

	@Test
	public void returnsPropertyNameForUnannotatedProperties() {

		Field field = ReflectionUtils.findField(Person.class, "lastname");
		assertThat(getPropertyFor(field).getFieldName(), is("lastname"));
	}

	@Test
	public void preventsNegativeOrder() {
		getPropertyFor(ReflectionUtils.findField(Person.class, "ssn"));
	}

	/**
	 * @see DATA_JIRA-553
	 */
	@Test
	public void usesPropertyAccessForThrowableCause() {

		SequoiadbPersistentProperty property = getPropertyFor(ReflectionUtils.findField(Throwable.class, "cause"));
		assertThat(property.usePropertyAccess(), is(true));
	}

	/**
	 * @see DATA_JIRA-607
	 */
	@Test
	public void usesCustomFieldNamingStrategyByDefault() throws Exception {

		Field field = ReflectionUtils.findField(Person.class, "lastname");

		SequoiadbPersistentProperty property = new BasicSequoiadbPersistentProperty(field, null, entity, new SimpleTypeHolder(),
				UppercaseFieldNamingStrategy.INSTANCE);
		assertThat(property.getFieldName(), is("LASTNAME"));

		field = ReflectionUtils.findField(Person.class, "firstname");

		property = new BasicSequoiadbPersistentProperty(field, null, entity, new SimpleTypeHolder(),
				UppercaseFieldNamingStrategy.INSTANCE);
		assertThat(property.getFieldName(), is("foo"));
	}

	/**
	 * @see DATA_JIRA-607
	 */
	@Test
	public void rejectsInvalidValueReturnedByFieldNamingStrategy() {

		Field field = ReflectionUtils.findField(Person.class, "lastname");
		SequoiadbPersistentProperty property = new BasicSequoiadbPersistentProperty(field, null, entity, new SimpleTypeHolder(),
				InvalidFieldNamingStrategy.INSTANCE);

		exception.expect(MappingException.class);
		exception.expectMessage(InvalidFieldNamingStrategy.class.getName());
		exception.expectMessage(property.toString());

		property.getFieldName();
	}

	/**
	 * @see DATA_JIRA-937
	 */
	@Test
	public void shouldDetectAnnotatedLanguagePropertyCorrectly() {

		BasicSequoiadbPersistentEntity<DocumentWithLanguageProperty> persistentEntity = new BasicSequoiadbPersistentEntity<DocumentWithLanguageProperty>(
				ClassTypeInformation.from(DocumentWithLanguageProperty.class));

		SequoiadbPersistentProperty property = getPropertyFor(persistentEntity, "lang");
		assertThat(property.isLanguageProperty(), is(true));
	}

	/**
	 * @see DATA_JIRA-937
	 */
	@Test
	public void shouldDetectIplicitLanguagePropertyCorrectly() {

		BasicSequoiadbPersistentEntity<DocumentWithImplicitLanguageProperty> persistentEntity = new BasicSequoiadbPersistentEntity<DocumentWithImplicitLanguageProperty>(
				ClassTypeInformation.from(DocumentWithImplicitLanguageProperty.class));

		SequoiadbPersistentProperty property = getPropertyFor(persistentEntity, "language");
		assertThat(property.isLanguageProperty(), is(true));
	}

	/**
	 * @see DATA_JIRA-976
	 */
	@Test
	public void shouldDetectTextScorePropertyCorrectly() {

		BasicSequoiadbPersistentEntity<DocumentWithTextScoreProperty> persistentEntity = new BasicSequoiadbPersistentEntity<DocumentWithTextScoreProperty>(
				ClassTypeInformation.from(DocumentWithTextScoreProperty.class));

		SequoiadbPersistentProperty property = getPropertyFor(persistentEntity, "score");
		assertThat(property.isTextScoreProperty(), is(true));
	}

	/**
	 * @see DATA_JIRA-976
	 */
	@Test
	public void shouldDetectTextScoreAsReadOnlyProperty() {

		BasicSequoiadbPersistentEntity<DocumentWithTextScoreProperty> persistentEntity = new BasicSequoiadbPersistentEntity<DocumentWithTextScoreProperty>(
				ClassTypeInformation.from(DocumentWithTextScoreProperty.class));

		SequoiadbPersistentProperty property = getPropertyFor(persistentEntity, "score");
		assertThat(property.isWritable(), is(false));
	}

	private SequoiadbPersistentProperty getPropertyFor(Field field) {
		return getPropertyFor(entity, field);
	}

	private SequoiadbPersistentProperty getPropertyFor(SequoiadbPersistentEntity<?> persistentEntity, String fieldname) {
		return getPropertyFor(persistentEntity, ReflectionUtils.findField(persistentEntity.getType(), fieldname));
	}

	private SequoiadbPersistentProperty getPropertyFor(SequoiadbPersistentEntity<?> persistentEntity, Field field) {
		return new BasicSequoiadbPersistentProperty(field, null, persistentEntity, new SimpleTypeHolder(),
				PropertyNameFieldNamingStrategy.INSTANCE);
	}

	class Person {

		@Id String id;

		@org.springframework.data.sequoiadb.core.mapping.Field("foo") String firstname;
		String lastname;

		@org.springframework.data.sequoiadb.core.mapping.Field(order = -20) String ssn;
	}

	enum UppercaseFieldNamingStrategy implements FieldNamingStrategy {

		INSTANCE;

		public String getFieldName(PersistentProperty<?> property) {
			return property.getName().toUpperCase(Locale.US);
		}
	}

	enum InvalidFieldNamingStrategy implements FieldNamingStrategy {

		INSTANCE;

		public String getFieldName(PersistentProperty<?> property) {
			return null;
		}
	}

	static class DocumentWithLanguageProperty {

		@Language String lang;
	}

	static class DocumentWithImplicitLanguageProperty {

		String language;
	}

	static class DocumentWithTextScoreProperty {
		@TextScore Float score;
	}
}
