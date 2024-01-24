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
package org.springframework.data.sequoiadb.core.convert;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.Before;
import org.junit.Test;
import org.springframework.data.convert.ConfigurableTypeInformationMapper;
import org.springframework.data.convert.SimpleTypeInformationMapper;



import org.springframework.data.sequoiadb.core.DBObjectTestUtils;
import org.springframework.data.util.TypeInformation;

/**
 * Unit tests for {@link DefaultSequoiadbTypeMapper}.
 * 

 */
public class DefaultSdbTypeMapperUnitTests {

	ConfigurableTypeInformationMapper configurableTypeInformationMapper;
	SimpleTypeInformationMapper simpleTypeInformationMapper;

	DefaultSequoiadbTypeMapper typeMapper;

	@Before
	public void setUp() {

		configurableTypeInformationMapper = new ConfigurableTypeInformationMapper(Collections.singletonMap(String.class,
				"1"));
		simpleTypeInformationMapper = SimpleTypeInformationMapper.INSTANCE;

		typeMapper = new DefaultSequoiadbTypeMapper();
	}

	@Test
	public void defaultInstanceWritesClasses() {

		writesTypeToField(new BasicBSONObject(), String.class, String.class.getName());
	}

	@Test
	public void defaultInstanceReadsClasses() {

		BSONObject dbObject = new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, String.class.getName());
		readsTypeFromField(dbObject, String.class);
	}

	@Test
	public void writesMapKeyForType() {

		typeMapper = new DefaultSequoiadbTypeMapper(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY,
				Arrays.asList(configurableTypeInformationMapper));

		writesTypeToField(new BasicBSONObject(), String.class, "1");
		writesTypeToField(new BasicBSONObject(), Object.class, null);
	}

	@Test
	public void writesClassNamesForUnmappedValuesIfConfigured() {

		typeMapper = new DefaultSequoiadbTypeMapper(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, Arrays.asList(
				configurableTypeInformationMapper, simpleTypeInformationMapper));

		writesTypeToField(new BasicBSONObject(), String.class, "1");
		writesTypeToField(new BasicBSONObject(), Object.class, Object.class.getName());
	}

	@Test
	public void readsTypeForMapKey() {

		typeMapper = new DefaultSequoiadbTypeMapper(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY,
				Arrays.asList(configurableTypeInformationMapper));

		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, "1"), String.class);
		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, "unmapped"), null);
	}

	@Test
	public void readsTypeLoadingClassesForUnmappedTypesIfConfigured() {

		typeMapper = new DefaultSequoiadbTypeMapper(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, Arrays.asList(
				configurableTypeInformationMapper, simpleTypeInformationMapper));

		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, "1"), String.class);
		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, Object.class.getName()), Object.class);
	}

	/**
	 * @see DATA_JIRA-709
	 */
	@Test
	public void writesTypeRestrictionsCorrectly() {

		BSONObject result = new BasicBSONObject();

		typeMapper = new DefaultSequoiadbTypeMapper();
		typeMapper.writeTypeRestrictions(result, Collections.<Class<?>> singleton(String.class));

		BSONObject typeInfo = DBObjectTestUtils.getAsDBObject(result, DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY);
		List<Object> aliases = DBObjectTestUtils.getAsDBList(typeInfo, "$in");
		assertThat(aliases, hasSize(1));
		assertThat(aliases.get(0), is((Object) String.class.getName()));
	}

	@Test
	public void addsFullyQualifiedClassNameUnderDefaultKeyByDefault() {
		writesTypeToField(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, new BasicBSONObject(), String.class);
	}

	@Test
	public void writesTypeToCustomFieldIfConfigured() {
		typeMapper = new DefaultSequoiadbTypeMapper("_custom");
		writesTypeToField("_custom", new BasicBSONObject(), String.class);
	}

	@Test
	public void doesNotWriteTypeInformationInCaseKeyIsSetToNull() {
		typeMapper = new DefaultSequoiadbTypeMapper(null);
		writesTypeToField(null, new BasicBSONObject(), String.class);
	}

	@Test
	public void readsTypeFromDefaultKeyByDefault() {
		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, String.class.getName()), String.class);
	}

	@Test
	public void readsTypeFromCustomFieldConfigured() {

		typeMapper = new DefaultSequoiadbTypeMapper("_custom");
		readsTypeFromField(new BasicBSONObject("_custom", String.class.getName()), String.class);
	}

	@Test
	public void returnsListForBasicDBLists() {
		readsTypeFromField(new BasicBSONList(), null);
	}

	@Test
	public void returnsNullIfNoTypeInfoInDBObject() {
		readsTypeFromField(new BasicBSONObject(), null);
		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, ""), null);
	}

	@Test
	public void returnsNullIfClassCannotBeLoaded() {
		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, "fooBar"), null);
	}

	@Test
	public void returnsNullIfTypeKeySetToNull() {
		typeMapper = new DefaultSequoiadbTypeMapper(null);
		readsTypeFromField(new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, String.class), null);
	}

	@Test
	public void returnsCorrectTypeKey() {

		assertThat(typeMapper.isTypeKey(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(true));

		typeMapper = new DefaultSequoiadbTypeMapper("_custom");
		assertThat(typeMapper.isTypeKey("_custom"), is(true));
		assertThat(typeMapper.isTypeKey(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(false));

		typeMapper = new DefaultSequoiadbTypeMapper(null);
		assertThat(typeMapper.isTypeKey("_custom"), is(false));
		assertThat(typeMapper.isTypeKey(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(false));
	}

	private void readsTypeFromField(BSONObject dbObject, Class<?> type) {

		TypeInformation<?> typeInfo = typeMapper.readType(dbObject);

		if (type != null) {
			assertThat(typeInfo, is(notNullValue()));
			assertThat(typeInfo.getType(), is(typeCompatibleWith(type)));
		} else {
			assertThat(typeInfo, is(nullValue()));
		}
	}

	private void writesTypeToField(String field, BSONObject dbObject, Class<?> type) {

		typeMapper.writeType(type, dbObject);

		if (field == null) {
			assertThat(dbObject.keySet().isEmpty(), is(true));
		} else {
			assertThat(dbObject.containsField(field), is(true));
			assertThat(dbObject.get(field), is((Object) type.getName()));
		}
	}

	private void writesTypeToField(BSONObject dbObject, Class<?> type, Object value) {

		typeMapper.writeType(type, dbObject);

		if (value == null) {
			assertThat(dbObject.keySet().isEmpty(), is(true));
		} else {
			assertThat(dbObject.containsField(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(true));
			assertThat(dbObject.get(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(value));
		}
	}
}
