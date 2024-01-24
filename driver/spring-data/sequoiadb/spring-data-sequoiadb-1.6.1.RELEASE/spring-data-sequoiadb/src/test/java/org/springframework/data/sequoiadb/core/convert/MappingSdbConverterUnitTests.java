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
package org.springframework.data.sequoiadb.core.convert;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;
import static org.springframework.data.sequoiadb.core.DBObjectTestUtils.*;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.joda.time.LocalDate;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.aop.framework.ProxyFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.ApplicationContext;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.PersistenceConstructor;
import org.springframework.data.annotation.TypeAlias;
import org.springframework.data.geo.Box;
import org.springframework.data.geo.Circle;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.Metrics;
import org.springframework.data.geo.Point;
import org.springframework.data.geo.Polygon;
import org.springframework.data.geo.Shape;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.mapping.model.MappingInstantiationException;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.assist.DBRef;
import org.springframework.data.sequoiadb.core.DBObjectTestUtils;
import org.springframework.data.sequoiadb.core.convert.DBObjectAccessorUnitTests.NestedType;
import org.springframework.data.sequoiadb.core.convert.DBObjectAccessorUnitTests.ProjectingType;
import org.springframework.data.sequoiadb.core.geo.Sphere;
import org.springframework.data.sequoiadb.core.mapping.*;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.util.ClassTypeInformation;
import org.springframework.test.util.ReflectionTestUtils;



/**
 * Unit tests for {@link MappingSequoiadbConverter}.
 * 



 */
@RunWith(MockitoJUnitRunner.class)
public class MappingSdbConverterUnitTests {

	MappingSequoiadbConverter converter;
	SequoiadbMappingContext mappingContext;
	@Mock ApplicationContext context;
	@Mock DbRefResolver resolver;

	public @Rule ExpectedException exception = ExpectedException.none();

	@Before
	public void setUp() {

		mappingContext = new SequoiadbMappingContext();
		mappingContext.setApplicationContext(context);
		mappingContext.afterPropertiesSet();

		converter = new MappingSequoiadbConverter(resolver, mappingContext);
		converter.afterPropertiesSet();
	}

	@Test
	public void convertsAddressCorrectly() {

		Address address = new Address();
		address.city = "New York";
		address.street = "Broadway";

		BSONObject dbObject = new BasicBSONObject();

		converter.write(address, dbObject);

		assertThat(dbObject.get("city").toString(), is("New York"));
		assertThat(dbObject.get("street").toString(), is("Broadway"));
	}

	@Test
	public void convertsJodaTimeTypesCorrectly() {

		converter = new MappingSequoiadbConverter(resolver, mappingContext);
		converter.afterPropertiesSet();

		Person person = new Person();
		person.birthDate = new LocalDate();

		BSONObject dbObject = new BasicBSONObject();
		converter.write(person, dbObject);

		assertThat(dbObject.get("birthDate"), is(instanceOf(Date.class)));

		Person result = converter.read(Person.class, dbObject);
		assertThat(result.birthDate, is(notNullValue()));
	}

	@Test
	public void convertsCustomTypeOnConvertToSequoiadbType() {

		converter = new MappingSequoiadbConverter(resolver, mappingContext);
		converter.afterPropertiesSet();

		LocalDate date = new LocalDate();
		converter.convertToSequoiadbType(date);
	}

	/**
	 * @see DATA_JIRA-130
	 */
	@Test
	public void writesMapTypeCorrectly() {

		Map<Locale, String> map = Collections.singletonMap(Locale.US, "Foo");

		BasicBSONObject dbObject = new BasicBSONObject();
		converter.write(map, dbObject);

		assertThat(dbObject.get(Locale.US.toString()).toString(), is("Foo"));
	}

	/**
	 * @see DATA_JIRA-130
	 */
	@Test
	public void readsMapWithCustomKeyTypeCorrectly() {

		BSONObject mapObject = new BasicBSONObject(Locale.US.toString(), "Value");
		BSONObject dbObject = new BasicBSONObject("map", mapObject);

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, dbObject);
		assertThat(result.map.get(Locale.US), is("Value"));
	}

	/**
	 * @see DATA_JIRA-128
	 */
	@Test
	public void usesDocumentsStoredTypeIfSubtypeOfRequest() {

		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("birthDate", new LocalDate());
		dbObject.put(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, Person.class.getName());

		assertThat(converter.read(Contact.class, dbObject), is(instanceOf(Person.class)));
	}

	/**
	 * @see DATA_JIRA-128
	 */
	@Test
	public void ignoresDocumentsStoredTypeIfCompletelyDifferentTypeRequested() {

		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("birthDate", new LocalDate());
		dbObject.put(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, Person.class.getName());

		assertThat(converter.read(BirthDateContainer.class, dbObject), is(instanceOf(BirthDateContainer.class)));
	}

	@Test
	public void writesTypeDiscriminatorIntoRootObject() {

		Person person = new Person();

		BSONObject result = new BasicBSONObject();
		converter.write(person, result);

		assertThat(result.containsField(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(true));
		assertThat(result.get(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY).toString(), is(Person.class.getName()));
	}

	/**
	 * @see DATA_JIRA-136
	 */
	@Test
	public void writesEnumsCorrectly() {

		ClassWithEnumProperty value = new ClassWithEnumProperty();
		value.sampleEnum = SampleEnum.FIRST;

		BSONObject result = new BasicBSONObject();
		converter.write(value, result);

		assertThat(result.get("sampleEnum"), is(instanceOf(String.class)));
		assertThat(result.get("sampleEnum").toString(), is("FIRST"));
	}

	/**
	 * @see DATA_JIRA-209
	 */
	@Test
	public void writesEnumCollectionCorrectly() {

		ClassWithEnumProperty value = new ClassWithEnumProperty();
		value.enums = Arrays.asList(SampleEnum.FIRST);

		BSONObject result = new BasicBSONObject();
		converter.write(value, result);

		assertThat(result.get("enums"), is(instanceOf(BasicBSONList.class)));

		BasicBSONList enums = (BasicBSONList) result.get("enums");
		assertThat(enums.size(), is(1));
		assertThat((String) enums.get(0), is("FIRST"));
	}

	/**
	 * @see DATA_JIRA-136
	 */
	@Test
	public void readsEnumsCorrectly() {
		BSONObject dbObject = new BasicBSONObject("sampleEnum", "FIRST");
		ClassWithEnumProperty result = converter.read(ClassWithEnumProperty.class, dbObject);

		assertThat(result.sampleEnum, is(SampleEnum.FIRST));
	}

	/**
	 * @see DATA_JIRA-209
	 */
	@Test
	public void readsEnumCollectionsCorrectly() {

		BasicBSONList enums = new BasicBSONList();
		enums.add("FIRST");
		BSONObject dbObject = new BasicBSONObject("enums", enums);

		ClassWithEnumProperty result = converter.read(ClassWithEnumProperty.class, dbObject);

		assertThat(result.enums, is(instanceOf(List.class)));
		assertThat(result.enums.size(), is(1));
		assertThat(result.enums, hasItem(SampleEnum.FIRST));
	}

	/**
	 * @see DATA_JIRA-144
	 */
	@Test
	public void considersFieldNameWhenWriting() {

		Person person = new Person();
		person.firstname = "Oliver";

		BSONObject result = new BasicBSONObject();
		converter.write(person, result);

		assertThat(result.containsField("foo"), is(true));
		assertThat(result.containsField("firstname"), is(false));
	}

	/**
	 * @see DATA_JIRA-144
	 */
	@Test
	public void considersFieldNameWhenReading() {

		BSONObject dbObject = new BasicBSONObject("foo", "Oliver");
		Person result = converter.read(Person.class, dbObject);

		assertThat(result.firstname, is("Oliver"));
	}

	@Test
	public void resolvesNestedComplexTypeForConstructorCorrectly() {

		BSONObject address = new BasicBSONObject("street", "110 Southwark Street");
		address.put("city", "London");

		BasicBSONList addresses = new BasicBSONList();
		addresses.add(address);

		BSONObject person = new BasicBSONObject("firstname", "Oliver");
		person.put("addresses", addresses);

		Person result = converter.read(Person.class, person);
		assertThat(result.addresses, is(notNullValue()));
	}

	/**
	 * @see DATA_JIRA-145
	 */
	@Test
	public void writesCollectionWithInterfaceCorrectly() {

		Person person = new Person();
		person.firstname = "Oliver";

		CollectionWrapper wrapper = new CollectionWrapper();
		wrapper.contacts = Arrays.asList((Contact) person);

		BasicBSONObject dbObject = new BasicBSONObject();
		converter.write(wrapper, dbObject);

		Object result = dbObject.get("contacts");
		assertThat(result, is(instanceOf(BasicBSONList.class)));
		BasicBSONList contacts = (BasicBSONList) result;
		BSONObject personDbObject = (BSONObject) contacts.get(0);
		assertThat(personDbObject.get("foo").toString(), is("Oliver"));
		assertThat((String) personDbObject.get(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(Person.class.getName()));
	}

	/**
	 * @see DATA_JIRA-145
	 */
	@Test
	public void readsCollectionWithInterfaceCorrectly() {

		BasicBSONObject person = new BasicBSONObject(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, Person.class.getName());
		person.put("foo", "Oliver");

		BasicBSONList contacts = new BasicBSONList();
		contacts.add(person);

		CollectionWrapper result = converter.read(CollectionWrapper.class, new BasicBSONObject("contacts", contacts));
		assertThat(result.contacts, is(notNullValue()));
		assertThat(result.contacts.size(), is(1));
		Contact contact = result.contacts.get(0);
		assertThat(contact, is(instanceOf(Person.class)));
		assertThat(((Person) contact).firstname, is("Oliver"));
	}

	@Test
	public void convertsLocalesOutOfTheBox() {
		LocaleWrapper wrapper = new LocaleWrapper();
		wrapper.locale = Locale.US;

		BSONObject dbObject = new BasicBSONObject();
		converter.write(wrapper, dbObject);

		Object localeField = dbObject.get("locale");
		assertThat(localeField, is(instanceOf(String.class)));
		assertThat((String) localeField, is("en_US"));

		LocaleWrapper read = converter.read(LocaleWrapper.class, dbObject);
		assertThat(read.locale, is(Locale.US));
	}

	/**
	 * @see DATA_JIRA-161
	 */
	@Test
	public void readsNestedMapsCorrectly() {

		Map<String, String> secondLevel = new HashMap<String, String>();
		secondLevel.put("key1", "value1");
		secondLevel.put("key2", "value2");

		Map<String, Map<String, String>> firstLevel = new HashMap<String, Map<String, String>>();
		firstLevel.put("level1", secondLevel);
		firstLevel.put("level2", secondLevel);

		ClassWithNestedMaps maps = new ClassWithNestedMaps();
		maps.nestedMaps = new LinkedHashMap<String, Map<String, Map<String, String>>>();
		maps.nestedMaps.put("afield", firstLevel);

		BSONObject dbObject = new BasicBSONObject();
		converter.write(maps, dbObject);

		ClassWithNestedMaps result = converter.read(ClassWithNestedMaps.class, dbObject);
		Map<String, Map<String, Map<String, String>>> nestedMap = result.nestedMaps;
		assertThat(nestedMap, is(notNullValue()));
		assertThat(nestedMap.get("afield"), is(firstLevel));
	}

	/**
	 * @see DATACMNS-42, DATA_JIRA-171
	 */
	@Test
	public void writesClassWithBigDecimal() {

		BigDecimalContainer container = new BigDecimalContainer();
		container.value = BigDecimal.valueOf(2.5d);
		container.map = Collections.singletonMap("foo", container.value);

		BSONObject dbObject = new BasicBSONObject();
		converter.write(container, dbObject);

		assertThat(dbObject.get("value"), is(instanceOf(String.class)));
		assertThat((String) dbObject.get("value"), is("2.5"));
		assertThat(((BSONObject) dbObject.get("map")).get("foo"), is(instanceOf(String.class)));
	}

	/**
	 * @see DATACMNS-42, DATA_JIRA-171
	 */
	@Test
	public void readsClassWithBigDecimal() {

		BSONObject dbObject = new BasicBSONObject("value", "2.5");
		dbObject.put("map", new BasicBSONObject("foo", "2.5"));

		BasicBSONList list = new BasicBSONList();
		list.add("2.5");
		dbObject.put("collection", list);
		BigDecimalContainer result = converter.read(BigDecimalContainer.class, dbObject);

		assertThat(result.value, is(BigDecimal.valueOf(2.5d)));
		assertThat(result.map.get("foo"), is(BigDecimal.valueOf(2.5d)));
		assertThat(result.collection.get(0), is(BigDecimal.valueOf(2.5d)));
	}

	@Test
	@SuppressWarnings("unchecked")
	public void writesNestedCollectionsCorrectly() {

		CollectionWrapper wrapper = new CollectionWrapper();
		wrapper.strings = Arrays.asList(Arrays.asList("Foo"));

		BSONObject dbObject = new BasicBSONObject();
		converter.write(wrapper, dbObject);

		Object outerStrings = dbObject.get("strings");
		assertThat(outerStrings, is(instanceOf(BasicBSONList.class)));

		BasicBSONList typedOuterString = (BasicBSONList) outerStrings;
		assertThat(typedOuterString.size(), is(1));
	}

	/**
	 * @see DATA_JIRA-192
	 */
	@Test
	public void readsEmptySetsCorrectly() {

		Person person = new Person();
		person.addresses = Collections.emptySet();

		BSONObject dbObject = new BasicBSONObject();
		converter.write(person, dbObject);
		converter.read(Person.class, dbObject);
	}

	@Test
	public void convertsObjectIdStringsToObjectIdCorrectly() {
		PersonPojoStringId p1 = new PersonPojoStringId("1234567890", "Text-1");
		BSONObject dbo1 = new BasicBSONObject();

		converter.write(p1, dbo1);
		assertThat(dbo1.get("_id"), is(instanceOf(String.class)));

		PersonPojoStringId p2 = new PersonPojoStringId(new ObjectId().toString(), "Text-1");
		BSONObject dbo2 = new BasicBSONObject();

		converter.write(p2, dbo2);
		assertThat(dbo2.get("_id"), is(instanceOf(ObjectId.class)));
	}

	/**
	 * @see DATA_JIRA-207
	 */
	@Test
	public void convertsCustomEmptyMapCorrectly() {

		BSONObject map = new BasicBSONObject();
		BSONObject wrapper = new BasicBSONObject("map", map);

		ClassWithSortedMap result = converter.read(ClassWithSortedMap.class, wrapper);

		assertThat(result, is(instanceOf(ClassWithSortedMap.class)));
		assertThat(result.map, is(instanceOf(SortedMap.class)));
	}

	/**
	 * @see DATA_JIRA-211
	 */
	@Test
	public void maybeConvertHandlesNullValuesCorrectly() {
		assertThat(converter.convertToSequoiadbType(null), is(nullValue()));
	}

	@Test
	public void writesGenericTypeCorrectly() {

		GenericType<Address> type = new GenericType<Address>();
		type.content = new Address();
		type.content.city = "London";

		BasicBSONObject result = new BasicBSONObject();
		converter.write(type, result);

		BSONObject content = (BSONObject) result.get("content");
		assertThat(content.get("_class"), is(notNullValue()));
		assertThat(content.get("city"), is(notNullValue()));
	}

	@Test
	public void readsGenericTypeCorrectly() {

		BSONObject address = new BasicBSONObject("_class", Address.class.getName());
		address.put("city", "London");

		GenericType<?> result = converter.read(GenericType.class, new BasicBSONObject("content", address));
		assertThat(result.content, is(instanceOf(Address.class)));

	}

	/**
	 * @see DATA_JIRA-228
	 */
	@Test
	public void writesNullValuesForMaps() {

		ClassWithMapProperty foo = new ClassWithMapProperty();
		foo.map = Collections.singletonMap(Locale.US, null);

		BSONObject result = new BasicBSONObject();
		converter.write(foo, result);

		Object map = result.get("map");
		assertThat(map, is(instanceOf(BSONObject.class)));
		assertThat(((BSONObject) map).keySet(), hasItem("en_US"));
	}

	@Test
	public void writesBigIntegerIdCorrectly() {

		ClassWithBigIntegerId foo = new ClassWithBigIntegerId();
		foo.id = BigInteger.valueOf(23L);

		BSONObject result = new BasicBSONObject();
		converter.write(foo, result);

		assertThat(result.get("_id"), is(instanceOf(String.class)));
	}

	public void convertsObjectsIfNecessary() {

		ObjectId id = new ObjectId();
		assertThat(converter.convertToSequoiadbType(id), is((Object) id));
	}

	/**
	 * @see DATA_JIRA-235
	 */
	@Test
	public void writesMapOfListsCorrectly() {

		ClassWithMapProperty input = new ClassWithMapProperty();
		input.mapOfLists = Collections.singletonMap("Foo", Arrays.asList("Bar"));

		BasicBSONObject result = new BasicBSONObject();
		converter.write(input, result);

		Object field = result.get("mapOfLists");
		assertThat(field, is(instanceOf(BSONObject.class)));

		BSONObject map = (BSONObject) field;
		Object foo = map.get("Foo");
		assertThat(foo, is(instanceOf(BasicBSONList.class)));

		BasicBSONList value = (BasicBSONList) foo;
		assertThat(value.size(), is(1));
		assertThat((String) value.get(0), is("Bar"));
	}

	/**
	 * @see DATA_JIRA-235
	 */
	@Test
	public void readsMapListValuesCorrectly() {

		BasicBSONList list = new BasicBSONList();
		list.add("Bar");
		BSONObject source = new BasicBSONObject("mapOfLists", new BasicBSONObject("Foo", list));

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, source);
		assertThat(result.mapOfLists, is(not(nullValue())));
	}

	/**
	 * @see DATA_JIRA-235
	 */
	@Test
	public void writesMapsOfObjectsCorrectly() {

		ClassWithMapProperty input = new ClassWithMapProperty();
		input.mapOfObjects = new HashMap<String, Object>();
		input.mapOfObjects.put("Foo", Arrays.asList("Bar"));

		BasicBSONObject result = new BasicBSONObject();
		converter.write(input, result);

		Object field = result.get("mapOfObjects");
		assertThat(field, is(instanceOf(BSONObject.class)));

		BSONObject map = (BSONObject) field;
		Object foo = map.get("Foo");
		assertThat(foo, is(instanceOf(BasicBSONList.class)));

		BasicBSONList value = (BasicBSONList) foo;
		assertThat(value.size(), is(1));
		assertThat((String) value.get(0), is("Bar"));
	}

	/**
	 * @see DATA_JIRA-235
	 */
	@Test
	public void readsMapOfObjectsListValuesCorrectly() {

		BasicBSONList list = new BasicBSONList();
		list.add("Bar");
		BSONObject source = new BasicBSONObject("mapOfObjects", new BasicBSONObject("Foo", list));

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, source);
		assertThat(result.mapOfObjects, is(not(nullValue())));
	}

	/**
	 * @see DATA_JIRA-245
	 */
	@Test
	public void readsMapListNestedValuesCorrectly() {

		BasicBSONList list = new BasicBSONList();
		list.add(new BasicBSONObject("Hello", "World"));
		BSONObject source = new BasicBSONObject("mapOfObjects", new BasicBSONObject("Foo", list));

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, source);
		Object firstObjectInFoo = ((List<?>) result.mapOfObjects.get("Foo")).get(0);
		assertThat(firstObjectInFoo, is(instanceOf(Map.class)));
		assertThat((String) ((Map<?, ?>) firstObjectInFoo).get("Hello"), is(equalTo("World")));
	}

	/**
	 * @see DATA_JIRA-245
	 */
	@Test
	public void readsMapDoublyNestedValuesCorrectly() {

		BasicBSONObject nested = new BasicBSONObject();
		BasicBSONObject doubly = new BasicBSONObject();
		doubly.append("Hello", "World");
		nested.append("nested", doubly);
		BSONObject source = new BasicBSONObject("mapOfObjects", new BasicBSONObject("Foo", nested));

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, source);
		Object foo = result.mapOfObjects.get("Foo");
		assertThat(foo, is(instanceOf(Map.class)));
		Object doublyNestedObject = ((Map<?, ?>) foo).get("nested");
		assertThat(doublyNestedObject, is(instanceOf(Map.class)));
		assertThat((String) ((Map<?, ?>) doublyNestedObject).get("Hello"), is(equalTo("World")));
	}

	/**
	 * @see DATA_JIRA-245
	 */
	@Test
	public void readsMapListDoublyNestedValuesCorrectly() {

		BasicBSONList list = new BasicBSONList();
		BasicBSONObject nested = new BasicBSONObject();
		BasicBSONObject doubly = new BasicBSONObject();
		doubly.append("Hello", "World");
		nested.append("nested", doubly);
		list.add(nested);
		BSONObject source = new BasicBSONObject("mapOfObjects", new BasicBSONObject("Foo", list));

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, source);
		Object firstObjectInFoo = ((List<?>) result.mapOfObjects.get("Foo")).get(0);
		assertThat(firstObjectInFoo, is(instanceOf(Map.class)));
		Object doublyNestedObject = ((Map<?, ?>) firstObjectInFoo).get("nested");
		assertThat(doublyNestedObject, is(instanceOf(Map.class)));
		assertThat((String) ((Map<?, ?>) doublyNestedObject).get("Hello"), is(equalTo("World")));
	}

	/**
	 * @see DATA_JIRA-259
	 */
	@Test
	public void writesListOfMapsCorrectly() {

		Map<String, Locale> map = Collections.singletonMap("Foo", Locale.ENGLISH);

		CollectionWrapper wrapper = new CollectionWrapper();
		wrapper.listOfMaps = new ArrayList<Map<String, Locale>>();
		wrapper.listOfMaps.add(map);

		BSONObject result = new BasicBSONObject();
		converter.write(wrapper, result);

		BasicBSONList list = (BasicBSONList) result.get("listOfMaps");
		assertThat(list, is(notNullValue()));
		assertThat(list.size(), is(1));

		BSONObject dbObject = (BSONObject) list.get(0);
		assertThat(dbObject.containsField("Foo"), is(true));
		assertThat((String) dbObject.get("Foo"), is(Locale.ENGLISH.toString()));
	}

	/**
	 * @see DATA_JIRA-259
	 */
	@Test
	public void readsListOfMapsCorrectly() {

		BSONObject map = new BasicBSONObject("Foo", "en");

		BasicBSONList list = new BasicBSONList();
		list.add(map);

		BSONObject wrapperSource = new BasicBSONObject("listOfMaps", list);

		CollectionWrapper wrapper = converter.read(CollectionWrapper.class, wrapperSource);

		assertThat(wrapper.listOfMaps, is(notNullValue()));
		assertThat(wrapper.listOfMaps.size(), is(1));
		assertThat(wrapper.listOfMaps.get(0), is(notNullValue()));
		assertThat(wrapper.listOfMaps.get(0).get("Foo"), is(Locale.ENGLISH));
	}

	/**
	 * @see DATA_JIRA-259
	 */
	@Test
	public void writesPlainMapOfCollectionsCorrectly() {

		Map<String, List<Locale>> map = Collections.singletonMap("Foo", Arrays.asList(Locale.US));
		BSONObject result = new BasicBSONObject();
		converter.write(map, result);

		assertThat(result.containsField("Foo"), is(true));
		assertThat(result.get("Foo"), is(notNullValue()));
		assertThat(result.get("Foo"), is(instanceOf(BasicBSONList.class)));

		BasicBSONList list = (BasicBSONList) result.get("Foo");

		assertThat(list.size(), is(1));
		assertThat(list.get(0), is((Object) Locale.US.toString()));
	}

	/**
	 * @see DATA_JIRA-285
	 */
	@Test
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void testSaveMapWithACollectionAsValue() {

		Map<String, Object> keyValues = new HashMap<String, Object>();
		keyValues.put("string", "hello");
		List<String> list = new ArrayList<String>();
		list.add("ping");
		list.add("pong");
		keyValues.put("list", list);

		BSONObject dbObject = new BasicBSONObject();
		converter.write(keyValues, dbObject);

		Map<String, Object> keyValuesFromSequoiadb = converter.read(Map.class, dbObject);

		assertEquals(keyValues.size(), keyValuesFromSequoiadb.size());
		assertEquals(keyValues.get("string"), keyValuesFromSequoiadb.get("string"));
		assertTrue(List.class.isAssignableFrom(keyValuesFromSequoiadb.get("list").getClass()));
		List<String> listFromSequoiadb = (List) keyValuesFromSequoiadb.get("list");
		assertEquals(list.size(), listFromSequoiadb.size());
		assertEquals(list.get(0), listFromSequoiadb.get(0));
		assertEquals(list.get(1), listFromSequoiadb.get(1));
	}

	/**
	 * @see DATA_JIRA-309
	 */
	@Test
	@SuppressWarnings({ "unchecked" })
	public void writesArraysAsMapValuesCorrectly() {

		ClassWithMapProperty wrapper = new ClassWithMapProperty();
		wrapper.mapOfObjects = new HashMap<String, Object>();
		wrapper.mapOfObjects.put("foo", new String[] { "bar" });

		BSONObject result = new BasicBSONObject();
		converter.write(wrapper, result);

		Object mapObject = result.get("mapOfObjects");
		assertThat(mapObject, is(instanceOf(BasicBSONObject.class)));

		BSONObject map = (BSONObject) mapObject;
		Object valueObject = map.get("foo");
		assertThat(valueObject, is(instanceOf(BasicBSONList.class)));

		List<Object> list = (List<Object>) valueObject;
		assertThat(list.size(), is(1));
		assertThat(list, hasItem((Object) "bar"));
	}

	/**
	 * @see DATA_JIRA-324
	 */
	@Test
	public void writesDbObjectCorrectly() {

		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("foo", "bar");

		BSONObject result = new BasicBSONObject();

		converter.write(dbObject, result);

		result.removeField(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY);
		assertThat(dbObject, is(result));
	}

	/**
	 * @see DATA_JIRA-324
	 */
	@Test
	public void readsDbObjectCorrectly() {

		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("foo", "bar");

		BSONObject result = converter.read(BSONObject.class, dbObject);

		assertThat(result, is(dbObject));
	}

	/**
	 * @see DATA_JIRA-329
	 */
	@Test
	public void writesMapAsGenericFieldCorrectly() {

		Map<String, A<String>> objectToSave = new HashMap<String, A<String>>();
		objectToSave.put("test", new A<String>("testValue"));

		A<Map<String, A<String>>> a = new A<Map<String, A<String>>>(objectToSave);
		BSONObject result = new BasicBSONObject();

		converter.write(a, result);

		assertThat((String) result.get(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(A.class.getName()));
		assertThat((String) result.get("valueType"), is(HashMap.class.getName()));

		BSONObject object = (BSONObject) result.get("value");
		assertThat(object, is(notNullValue()));

		BSONObject inner = (BSONObject) object.get("test");
		assertThat(inner, is(notNullValue()));
		assertThat((String) inner.get(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY), is(A.class.getName()));
		assertThat((String) inner.get("valueType"), is(String.class.getName()));
		assertThat((String) inner.get("value"), is("testValue"));
	}

	@Test
	public void writesIntIdCorrectly() {

		ClassWithIntId value = new ClassWithIntId();
		value.id = 5;

		BSONObject result = new BasicBSONObject();
		converter.write(value, result);

		assertThat(result.get("_id"), is((Object) 5));
	}

	/**
	 * @see DATA_JIRA-368
	 */
	@Test
	@SuppressWarnings("unchecked")
	public void writesNullValuesForCollection() {

		CollectionWrapper wrapper = new CollectionWrapper();
		wrapper.contacts = Arrays.<Contact> asList(new Person(), null);

		BSONObject result = new BasicBSONObject();
		converter.write(wrapper, result);

		Object contacts = result.get("contacts");
		assertThat(contacts, is(instanceOf(Collection.class)));
		assertThat(((Collection<?>) contacts).size(), is(2));
		assertThat((Collection<Object>) contacts, hasItem(nullValue()));
	}

	/**
	 * @see DATA_JIRA-379
	 */
	@Test
	public void considersDefaultingExpressionsAtConstructorArguments() {

		BSONObject dbObject = new BasicBSONObject("foo", "bar");
		dbObject.put("foobar", 2.5);

		DefaultedConstructorArgument result = converter.read(DefaultedConstructorArgument.class, dbObject);
		assertThat(result.bar, is(-1));
	}

	/**
	 * @see DATA_JIRA-379
	 */
	@Test
	public void usesDocumentFieldIfReferencedInAtValue() {

		BSONObject dbObject = new BasicBSONObject("foo", "bar");
		dbObject.put("something", 37);
		dbObject.put("foobar", 2.5);

		DefaultedConstructorArgument result = converter.read(DefaultedConstructorArgument.class, dbObject);
		assertThat(result.bar, is(37));
	}

	/**
	 * @see DATA_JIRA-379
	 */
	@Test(expected = MappingInstantiationException.class)
	public void rejectsNotFoundConstructorParameterForPrimitiveType() {

		BSONObject dbObject = new BasicBSONObject("foo", "bar");

		converter.read(DefaultedConstructorArgument.class, dbObject);
	}

	/**
	 * @see DATA_JIRA-358
	 */
	@Test
	public void writesListForObjectPropertyCorrectly() {

		Attribute attribute = new Attribute();
		attribute.key = "key";
		attribute.value = Arrays.asList("1", "2");

		Item item = new Item();
		item.attributes = Arrays.asList(attribute);

		BSONObject result = new BasicBSONObject();

		converter.write(item, result);

		Item read = converter.read(Item.class, result);
		assertThat(read.attributes.size(), is(1));
		assertThat(read.attributes.get(0).key, is(attribute.key));
		assertThat(read.attributes.get(0).value, is(instanceOf(Collection.class)));

		@SuppressWarnings("unchecked")
		Collection<String> values = (Collection<String>) read.attributes.get(0).value;

		assertThat(values.size(), is(2));
		assertThat(values, hasItems("1", "2"));
	}

	/**
	 * @see DATA_JIRA-380
	 */
	@Test(expected = MappingException.class)
	public void rejectsMapWithKeyContainingDotsByDefault() {
		converter.write(Collections.singletonMap("foo.bar", "foobar"), new BasicBSONObject());
	}

	/**
	 * @see DATA_JIRA-380
	 */
	@Test
	public void escapesDotInMapKeysIfReplacementConfigured() {

		converter.setMapKeyDotReplacement("~");

		BSONObject dbObject = new BasicBSONObject();
		converter.write(Collections.singletonMap("foo.bar", "foobar"), dbObject);

		assertThat((String) dbObject.get("foo~bar"), is("foobar"));
		assertThat(dbObject.containsField("foo.bar"), is(false));
	}

	/**
	 * @see DATA_JIRA-380
	 */
	@Test
	@SuppressWarnings("unchecked")
	public void unescapesDotInMapKeysIfReplacementConfigured() {

		converter.setMapKeyDotReplacement("~");

		BSONObject dbObject = new BasicBSONObject("foo~bar", "foobar");
		Map<String, String> result = converter.read(Map.class, dbObject);

		assertThat(result.get("foo.bar"), is("foobar"));
		assertThat(result.containsKey("foobar"), is(false));
	}

	/**
	 * @see DATA_JIRA-382
	 */
	@Test
	public void convertsSetToBasicDBList() {

		Address address = new Address();
		address.city = "London";
		address.street = "Foo";

		Object result = converter.convertToSequoiadbType(Collections.singleton(address), ClassTypeInformation.OBJECT);
		assertThat(result, is(instanceOf(BasicBSONList.class)));

		Set<?> readResult = converter.read(Set.class, (BasicBSONList) result);
		assertThat(readResult.size(), is(1));
		assertThat(readResult.iterator().next(), is(instanceOf(Address.class)));
	}

	/**
	 * @see DATA_JIRA-402
	 */
	@Test
	public void readsMemberClassCorrectly() {

		BSONObject dbObject = new BasicBSONObject("inner", new BasicBSONObject("value", "FOO!"));

		Outer outer = converter.read(Outer.class, dbObject);
		assertThat(outer.inner, is(notNullValue()));
		assertThat(outer.inner.value, is("FOO!"));
		assertSyntheticFieldValueOf(outer.inner, outer);
	}

	/**
	 * @see DATA_JIRA-458
	 */
	@Test
	public void readEmptyCollectionIsModifiable() {

		BSONObject dbObject = new BasicBSONObject("contactsSet", new BasicBSONList());
		CollectionWrapper wrapper = converter.read(CollectionWrapper.class, dbObject);

		assertThat(wrapper.contactsSet, is(notNullValue()));
		wrapper.contactsSet.add(new Contact() {});
	}

	/**
	 * @see DATA_JIRA-424
	 */
	@Test
	public void readsPlainDBRefObject() {

		DBRef dbRef = new DBRef(mock(DB.class), "foo", 2);
		BSONObject dbObject = new BasicBSONObject("ref", dbRef);

		DBRefWrapper result = converter.read(DBRefWrapper.class, dbObject);
		assertThat(result.ref, is(dbRef));
	}

	/**
	 * @see DATA_JIRA-424
	 */
	@Test
	public void readsCollectionOfDBRefs() {

		DBRef dbRef = new DBRef(mock(DB.class), "foo", 2);
		BasicBSONList refs = new BasicBSONList();
		refs.add(dbRef);

		BSONObject dbObject = new BasicBSONObject("refs", refs);

		DBRefWrapper result = converter.read(DBRefWrapper.class, dbObject);
		assertThat(result.refs, hasSize(1));
		assertThat(result.refs, hasItem(dbRef));
	}

	/**
	 * @see DATA_JIRA-424
	 */
	@Test
	public void readsDBRefMap() {

		DBRef dbRef = mock(DBRef.class);
		BasicBSONObject refMap = new BasicBSONObject("foo", dbRef);
		BSONObject dbObject = new BasicBSONObject("refMap", refMap);

		DBRefWrapper result = converter.read(DBRefWrapper.class, dbObject);

		assertThat(result.refMap.entrySet(), hasSize(1));
		assertThat(result.refMap.values(), hasItem(dbRef));
	}

	/**
	 * @see DATA_JIRA-424
	 */
	@Test
	@SuppressWarnings({ "rawtypes", "unchecked" })
	public void resolvesDBRefMapValue() {

		DBRef dbRef = mock(DBRef.class);
		when(dbRef.fetch()).thenReturn(new BasicBSONObject());

		BasicBSONObject refMap = new BasicBSONObject("foo", dbRef);
		BSONObject dbObject = new BasicBSONObject("personMap", refMap);

		DBRefWrapper result = converter.read(DBRefWrapper.class, dbObject);

		Matcher isPerson = instanceOf(Person.class);

		assertThat(result.personMap.entrySet(), hasSize(1));
		assertThat(result.personMap.values(), hasItem(isPerson));
	}

	/**
	 * @see DATA_JIRA-462
	 */
	@Test
	public void writesURLsAsStringOutOfTheBox() throws Exception {

		URLWrapper wrapper = new URLWrapper();
		wrapper.url = new URL("http://springsource.org");
		BSONObject sink = new BasicBSONObject();

		converter.write(wrapper, sink);

		assertThat(sink.get("url"), is((Object) "http://springsource.org"));
	}

	/**
	 * @see DATA_JIRA-462
	 */
	@Test
	public void readsURLFromStringOutOfTheBox() throws Exception {
		BSONObject dbObject = new BasicBSONObject("url", "http://springsource.org");
		URLWrapper result = converter.read(URLWrapper.class, dbObject);
		assertThat(result.url, is(new URL("http://springsource.org")));
	}

	/**
	 * @see DATA_JIRA-485
	 */
	@Test
	public void writesComplexIdCorrectly() {

		ComplexId id = new ComplexId();
		id.innerId = 4711L;

		ClassWithComplexId entity = new ClassWithComplexId();
		entity.complexId = id;

		BSONObject dbObject = new BasicBSONObject();
		converter.write(entity, dbObject);

		Object idField = dbObject.get("_id");
		assertThat(idField, is(notNullValue()));
		assertThat(idField, is(instanceOf(BSONObject.class)));
		assertThat(((BSONObject) idField).get("innerId"), is((Object) 4711L));
	}

	/**
	 * @see DATA_JIRA-485
	 */
	@Test
	public void readsComplexIdCorrectly() {

		BSONObject innerId = new BasicBSONObject("innerId", 4711L);
		BSONObject entity = new BasicBSONObject("_id", innerId);

		ClassWithComplexId result = converter.read(ClassWithComplexId.class, entity);

		assertThat(result.complexId, is(notNullValue()));
		assertThat(result.complexId.innerId, is(4711L));
	}

	/**
	 * @see DATA_JIRA-489
	 */
	@Test
	public void readsArraysAsMapValuesCorrectly() {

		BasicBSONList list = new BasicBSONList();
		list.add("Foo");
		list.add("Bar");

		BSONObject map = new BasicBSONObject("key", list);
		BSONObject wrapper = new BasicBSONObject("mapOfStrings", map);

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, wrapper);
		assertThat(result.mapOfStrings, is(notNullValue()));

		String[] values = result.mapOfStrings.get("key");
		assertThat(values, is(notNullValue()));
		assertThat(values, is(arrayWithSize(2)));
	}

	/**
	 * @see DATA_JIRA-497
	 */
	@Test
	public void readsEmptyCollectionIntoConstructorCorrectly() {

		BSONObject source = new BasicBSONObject("attributes", new BasicBSONList());

		TypWithCollectionConstructor result = converter.read(TypWithCollectionConstructor.class, source);
		assertThat(result.attributes, is(notNullValue()));
	}

	private static void assertSyntheticFieldValueOf(Object target, Object expected) {

		for (int i = 0; i < 10; i++) {
			try {
				assertThat(ReflectionTestUtils.getField(target, "this$" + i), is(expected));
				return;
			} catch (IllegalArgumentException e) {
				// Suppress and try next
			}
		}

		fail(String.format("Didn't find synthetic field on %s!", target));
	}

	/**
	 * @see DATAMGONGO-508
	 */
	@Test
	public void eagerlyReturnsDBRefObjectIfTargetAlreadyIsOne() {

		DB db = mock(DB.class);
		DBRef dbRef = new DBRef(db, "collection", "id");

		SequoiadbPersistentProperty property = mock(SequoiadbPersistentProperty.class);

		assertThat(converter.createDBRef(dbRef, property), is(dbRef));
	}

	/**
	 * @see DATA_JIRA-523
	 */
	@Test
	public void considersTypeAliasAnnotation() {

		Aliased aliased = new Aliased();
		aliased.name = "foo";

		BSONObject result = new BasicBSONObject();
		converter.write(aliased, result);

		Object type = result.get("_class");
		assertThat(type, is(notNullValue()));
		assertThat(type.toString(), is("_"));
	}

	/**
	 * @see DATA_JIRA-533
	 */
	@Test
	public void marshalsThrowableCorrectly() {

		ThrowableWrapper wrapper = new ThrowableWrapper();
		wrapper.throwable = new Exception();

		BSONObject dbObject = new BasicBSONObject();
		converter.write(wrapper, dbObject);
	}

	/**
	 * @see DATA_JIRA-592
	 */
	@Test
	public void recursivelyConvertsSpELReadValue() {

		BSONObject input = (BSONObject) JSON
				.parse("{ \"_id\" : { \"$oid\" : \"50ca271c4566a2b08f2d667a\" }, \"_class\" : \"com.recorder.TestRecorder2$ObjectContainer\", \"property\" : { \"property\" : 100 } }");

		converter.read(ObjectContainer.class, input);
	}

	/**
	 * @see DATA_JIRA-724
	 */
	@Test
	@SuppressWarnings("unchecked")
	public void mappingConsidersCustomConvertersNotWritingTypeInformation() {

		Person person = new Person();
		person.firstname = "Dave";

		ClassWithMapProperty entity = new ClassWithMapProperty();
		entity.mapOfPersons = new HashMap<String, Person>();
		entity.mapOfPersons.put("foo", person);
		entity.mapOfObjects = new HashMap<String, Object>();
		entity.mapOfObjects.put("foo", person);

		CustomConversions conversions = new CustomConversions(Arrays.asList(new Converter<Person, BSONObject>() {

			@Override
			public BSONObject convert(Person source) {
				BSONObject obj = new BasicBSONObject();
				obj.put("firstname", source.firstname);
				obj.put("_class", Person.class.getName());
				return obj ;
			}

		}, new Converter<BSONObject, Person>() {

			@Override
			public Person convert(BSONObject source) {
				Person person = new Person();
				person.firstname = source.get("firstname").toString();
				person.lastname = "converter";
				return person;
			}
		}));

		SequoiadbMappingContext context = new SequoiadbMappingContext();
		context.setSimpleTypeHolder(conversions.getSimpleTypeHolder());
		context.afterPropertiesSet();

		MappingSequoiadbConverter sequoiadbConverter = new MappingSequoiadbConverter(resolver, context);
		sequoiadbConverter.setCustomConversions(conversions);
		sequoiadbConverter.afterPropertiesSet();

		BasicBSONObject dbObject = new BasicBSONObject();
		sequoiadbConverter.write(entity, dbObject);

		ClassWithMapProperty result = sequoiadbConverter.read(ClassWithMapProperty.class, dbObject);

		assertThat(result.mapOfPersons, is(notNullValue()));
		Person personCandidate = result.mapOfPersons.get("foo");
		assertThat(personCandidate, is(notNullValue()));
		assertThat(personCandidate.firstname, is("Dave"));

		assertThat(result.mapOfObjects, is(notNullValue()));
		Object value = result.mapOfObjects.get("foo");
		assertThat(value, is(notNullValue()));
		assertThat(value, is(instanceOf(Person.class)));
		assertThat(((Person) value).firstname, is("Dave"));
		assertThat(((Person) value).lastname, is("converter"));
	}

	/**
	 * @see DATA_JIRA-743
	 */
	@Test
	public void readsIntoStringsOutOfTheBox() {

		BSONObject dbObject = new BasicBSONObject("firstname", "Dave");
		assertThat(converter.read(String.class, dbObject), is("{ \"firstname\" : \"Dave\"}"));
	}

	/**
	 * @see DATA_JIRA-766
	 */
	@Test
	public void writesProjectingTypeCorrectly() {

		NestedType nested = new NestedType();
		nested.c = "C";

		ProjectingType type = new ProjectingType();
		type.name = "name";
		type.foo = "bar";
		type.a = nested;

		BasicBSONObject result = new BasicBSONObject();
		converter.write(type, result);

		assertThat(result.get("name"), is((Object) "name"));
		BSONObject aValue = DBObjectTestUtils.getAsDBObject(result, "a");
		assertThat(aValue.get("b"), is((Object) "bar"));
		assertThat(aValue.get("c"), is((Object) "C"));
	}

	/**
	 * @see DATA_JIRA-812
	 * @see DATA_JIRA-893
	 */
	@Test
	public void convertsListToBasicDBListAndRetainsTypeInformationForComplexObjects() {

		Address address = new Address();
		address.city = "London";
		address.street = "Foo";

		Object result = converter.convertToSequoiadbType(Collections.singletonList(address),
				ClassTypeInformation.from(InterfaceType.class));

		assertThat(result, is(instanceOf(BasicBSONList.class)));

		BasicBSONList dbList = (BasicBSONList) result;
		assertThat(dbList, hasSize(1));
		assertThat(getTypedValue(getAsDBObject(dbList, 0), "_class", String.class), equalTo(Address.class.getName()));
	}

	/**
	 * @see DATA_JIRA-812
	 */
	@Test
	public void convertsListToBasicDBListWithoutTypeInformationForSimpleTypes() {

		Object result = converter.convertToSequoiadbType(Collections.singletonList("foo"));

		assertThat(result, is(instanceOf(BasicBSONList.class)));

		BasicBSONList dbList = (BasicBSONList) result;
		assertThat(dbList, hasSize(1));
		assertThat(dbList.get(0), instanceOf(String.class));
	}

	/**
	 * @see DATA_JIRA-812
	 */
	@Test
	public void convertsArrayToBasicDBListAndRetainsTypeInformationForComplexObjects() {

		Address address = new Address();
		address.city = "London";
		address.street = "Foo";

		Object result = converter.convertToSequoiadbType(new Address[] { address }, ClassTypeInformation.OBJECT);

		assertThat(result, is(instanceOf(BasicBSONList.class)));

		BasicBSONList dbList = (BasicBSONList) result;
		assertThat(dbList, hasSize(1));
		assertThat(getTypedValue(getAsDBObject(dbList, 0), "_class", String.class), equalTo(Address.class.getName()));
	}

	/**
	 * @see DATA_JIRA-812
	 */
	@Test
	public void convertsArrayToBasicDBListWithoutTypeInformationForSimpleTypes() {

		Object result = converter.convertToSequoiadbType(new String[] { "foo" });

		assertThat(result, is(instanceOf(BasicBSONList.class)));

		BasicBSONList dbList = (BasicBSONList) result;
		assertThat(dbList, hasSize(1));
		assertThat(dbList.get(0), instanceOf(String.class));
	}

	/**
	 * @see DATA_JIRA-833
	 */
	@Test
	public void readsEnumSetCorrectly() {

		BasicBSONList enumSet = new BasicBSONList();
		enumSet.add("SECOND");
		BSONObject dbObject = new BasicBSONObject("enumSet", enumSet);

		ClassWithEnumProperty result = converter.read(ClassWithEnumProperty.class, dbObject);

		assertThat(result.enumSet, is(instanceOf(EnumSet.class)));
		assertThat(result.enumSet.size(), is(1));
		assertThat(result.enumSet, hasItem(SampleEnum.SECOND));
	}

	/**
	 * @see DATA_JIRA-833
	 */
	@Test
	public void readsEnumMapCorrectly() {

		BasicBSONObject enumMap = new BasicBSONObject("FIRST", "Dave");
		ClassWithEnumProperty result = converter.read(ClassWithEnumProperty.class, new BasicBSONObject("enumMap", enumMap));

		assertThat(result.enumMap, is(instanceOf(EnumMap.class)));
		assertThat(result.enumMap.size(), is(1));
		assertThat(result.enumMap.get(SampleEnum.FIRST), is("Dave"));
	}

	/**
	 * @see DATA_JIRA-887
	 */
	@Test
	public void readsTreeMapCorrectly() {

		BSONObject person = new BasicBSONObject("foo", "Dave");
		BSONObject treeMapOfPerson = new BasicBSONObject("key", person);
		BSONObject document = new BasicBSONObject("treeMapOfPersons", treeMapOfPerson);

		ClassWithMapProperty result = converter.read(ClassWithMapProperty.class, document);

		assertThat(result.treeMapOfPersons, is(notNullValue()));
		assertThat(result.treeMapOfPersons.get("key"), is(notNullValue()));
		assertThat(result.treeMapOfPersons.get("key").firstname, is("Dave"));
	}

	/**
	 * @see DATA_JIRA-887
	 */
	@Test
	public void writesTreeMapCorrectly() {

		Person person = new Person();
		person.firstname = "Dave";

		ClassWithMapProperty source = new ClassWithMapProperty();
		source.treeMapOfPersons = new TreeMap<String, Person>();
		source.treeMapOfPersons.put("key", person);

		BSONObject result = new BasicBSONObject();

		converter.write(source, result);

		BSONObject map = getAsDBObject(result, "treeMapOfPersons");
		BSONObject entry = getAsDBObject(map, "key");
		assertThat(entry.get("foo"), is((Object) "Dave"));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldWriteEntityWithGeoBoxCorrectly() {

		ClassWithGeoBox object = new ClassWithGeoBox();
		object.box = new Box(new Point(1, 2), new Point(3, 4));

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		assertThat(dbo, is(notNullValue()));
		assertThat(dbo.get("box"), is(instanceOf(BSONObject.class)));
		assertThat(dbo.get("box"), is((Object) new BasicBSONObject().append("first", toDbObject(object.box.getFirst()))
				.append("second", toDbObject(object.box.getSecond()))));
	}

	private static BSONObject toDbObject(Point point) {
		BSONObject obj = new BasicBSONObject();
		obj.put("x", point.getX());
		obj.put("y", point.getY());
		return obj;
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldReadEntityWithGeoBoxCorrectly() {

		ClassWithGeoBox object = new ClassWithGeoBox();
		object.box = new Box(new Point(1, 2), new Point(3, 4));

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		ClassWithGeoBox result = converter.read(ClassWithGeoBox.class, dbo);

		assertThat(result, is(notNullValue()));
		assertThat(result.box, is(object.box));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldWriteEntityWithGeoPolygonCorrectly() {

		ClassWithGeoPolygon object = new ClassWithGeoPolygon();
		object.polygon = new Polygon(new Point(1, 2), new Point(3, 4), new Point(4, 5));

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		assertThat(dbo, is(notNullValue()));

		assertThat(dbo.get("polygon"), is(instanceOf(BSONObject.class)));
		BSONObject polygonDbo = (BSONObject) dbo.get("polygon");

		@SuppressWarnings("unchecked")
		List<BSONObject> points = (List<BSONObject>) polygonDbo.get("points");

		assertThat(points, hasSize(3));
		assertThat(points, Matchers.<BSONObject> hasItems(toDbObject(object.polygon.getPoints().get(0)),
				toDbObject(object.polygon.getPoints().get(1)), toDbObject(object.polygon.getPoints().get(2))));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldReadEntityWithGeoPolygonCorrectly() {

		ClassWithGeoPolygon object = new ClassWithGeoPolygon();
		object.polygon = new Polygon(new Point(1, 2), new Point(3, 4), new Point(4, 5));

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		ClassWithGeoPolygon result = converter.read(ClassWithGeoPolygon.class, dbo);

		assertThat(result, is(notNullValue()));
		assertThat(result.polygon, is(object.polygon));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldWriteEntityWithGeoCircleCorrectly() {

		ClassWithGeoCircle object = new ClassWithGeoCircle();
		Circle circle = new Circle(new Point(1, 2), 3);
		Distance radius = circle.getRadius();
		object.circle = circle;

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		assertThat(dbo, is(notNullValue()));
		assertThat(dbo.get("circle"), is(instanceOf(BSONObject.class)));
		assertThat(
				dbo.get("circle"),
				is((Object) new BasicBSONObject("center", new BasicBSONObject("x", circle.getCenter().getX()).append("y", circle
						.getCenter().getY())).append("radius", radius.getNormalizedValue()).append("metric",
						radius.getMetric().toString())));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldReadEntityWithGeoCircleCorrectly() {

		ClassWithGeoCircle object = new ClassWithGeoCircle();
		object.circle = new Circle(new Point(1, 2), 3);

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		ClassWithGeoCircle result = converter.read(ClassWithGeoCircle.class, dbo);

		assertThat(result, is(notNullValue()));
		assertThat(result.circle, is(result.circle));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldWriteEntityWithGeoSphereCorrectly() {

		ClassWithGeoSphere object = new ClassWithGeoSphere();
		Sphere sphere = new Sphere(new Point(1, 2), 3);
		Distance radius = sphere.getRadius();
		object.sphere = sphere;

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		assertThat(dbo, is(notNullValue()));
		assertThat(dbo.get("sphere"), is(instanceOf(BSONObject.class)));
		assertThat(
				dbo.get("sphere"),
				is((Object) new BasicBSONObject("center", new BasicBSONObject("x", sphere.getCenter().getX()).append("y", sphere
						.getCenter().getY())).append("radius", radius.getNormalizedValue()).append("metric",
						radius.getMetric().toString())));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldWriteEntityWithGeoSphereWithMetricDistanceCorrectly() {

		ClassWithGeoSphere object = new ClassWithGeoSphere();
		Sphere sphere = new Sphere(new Point(1, 2), new Distance(3, Metrics.KILOMETERS));
		Distance radius = sphere.getRadius();
		object.sphere = sphere;

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		assertThat(dbo, is(notNullValue()));
		assertThat(dbo.get("sphere"), is(instanceOf(BSONObject.class)));
		assertThat(
				dbo.get("sphere"),
				is((Object) new BasicBSONObject("center", new BasicBSONObject("x", sphere.getCenter().getX()).append("y", sphere
						.getCenter().getY())).append("radius", radius.getNormalizedValue()).append("metric",
						radius.getMetric().toString())));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldReadEntityWithGeoSphereCorrectly() {

		ClassWithGeoSphere object = new ClassWithGeoSphere();
		object.sphere = new Sphere(new Point(1, 2), 3);

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		ClassWithGeoSphere result = converter.read(ClassWithGeoSphere.class, dbo);

		assertThat(result, is(notNullValue()));
		assertThat(result.sphere, is(object.sphere));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	public void shouldWriteEntityWithGeoShapeCorrectly() {

		ClassWithGeoShape object = new ClassWithGeoShape();
		Sphere sphere = new Sphere(new Point(1, 2), 3);
		Distance radius = sphere.getRadius();
		object.shape = sphere;

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		assertThat(dbo, is(notNullValue()));
		assertThat(dbo.get("shape"), is(instanceOf(BSONObject.class)));
		assertThat(
				dbo.get("shape"),
				is((Object) new BasicBSONObject("center", new BasicBSONObject("x", sphere.getCenter().getX()).append("y", sphere
						.getCenter().getY())).append("radius", radius.getNormalizedValue()).append("metric",
						radius.getMetric().toString())));
	}

	/**
	 * @DATA_JIRA-858
	 */
	@Test
	@Ignore
	public void shouldReadEntityWithGeoShapeCorrectly() {

		ClassWithGeoShape object = new ClassWithGeoShape();
		Sphere sphere = new Sphere(new Point(1, 2), 3);
		object.shape = sphere;

		BSONObject dbo = new BasicBSONObject();
		converter.write(object, dbo);

		ClassWithGeoShape result = converter.read(ClassWithGeoShape.class, dbo);

		assertThat(result, is(notNullValue()));
		assertThat(result.shape, is((Shape) sphere));
	}

	/**
	 * @see DATA_JIRA-976
	 */
	@Test
	public void shouldIgnoreTextScorePropertyWhenWriting() {

		ClassWithTextScoreProperty source = new ClassWithTextScoreProperty();
		source.score = Float.MAX_VALUE;

		BasicBSONObject dbo = new BasicBSONObject();
		converter.write(source, dbo);

		assertThat(dbo.get("score"), nullValue());
	}

	/**
	 * @see DATA_JIRA-976
	 */
	@Test
	public void shouldIncludeTextScorePropertyWhenReading() {

		ClassWithTextScoreProperty entity = converter
				.read(ClassWithTextScoreProperty.class, new BasicBSONObject("score", 5F));
		assertThat(entity.score, equalTo(5F));
	}

	/**
	 * @see DATA_JIRA-1001
	 */
	@Test
	public void shouldWriteCglibProxiedClassTypeInformationCorrectly() {

		ProxyFactory factory = new ProxyFactory();
		factory.setTargetClass(GenericType.class);
		factory.setProxyTargetClass(true);

		GenericType<?> proxied = (GenericType<?>) factory.getProxy();
		BasicBSONObject dbo = new BasicBSONObject();
		converter.write(proxied, dbo);

		assertThat(dbo.get("_class"), is((Object) GenericType.class.getName()));
	}

	/**
	 * @see DATA_JIRA-1001
	 */
	@Test
	public void shouldUseTargetObjectOfLazyLoadingProxyWhenWriting() {

		LazyLoadingProxy mock = mock(LazyLoadingProxy.class);

		BasicBSONObject dbo = new BasicBSONObject();
		converter.write(mock, dbo);

		verify(mock, times(1)).getTarget();
	}

	/**
	 * @see DATA_JIRA-1034
	 */
	@Test
	public void rejectsBasicDbListToBeConvertedIntoComplexType() {

		BasicBSONList inner = new BasicBSONList();
		inner.add("key");
		inner.add("value");

		BasicBSONList outer = new BasicBSONList();
		outer.add(inner);
		outer.add(inner);

		BasicBSONObject source = new BasicBSONObject("attributes", outer);

		exception.expect(MappingException.class);
		exception.expectMessage(Item.class.getName());
		exception.expectMessage(BasicBSONList.class.getName());

		converter.read(Item.class, source);
	}

	/**
	 * @see DATA_JIRA-1058
	 */
	@Test
	public void readShouldRespectExplicitFieldNameForDbRef() {

		BasicBSONObject source = new BasicBSONObject();
		source.append("explict-name-for-db-ref", new DBRef(mock(DB.class), "foo", "1"));

		converter.read(ClassWithExplicitlyNamedDBRefProperty.class, source);

		verify(resolver, times(1)).resolveDbRef(Mockito.any(SequoiadbPersistentProperty.class), Mockito.any(DBRef.class),
				Mockito.any(DbRefResolverCallback.class), Mockito.any(DbRefProxyHandler.class));
	}

	static class GenericType<T> {
		T content;
	}

	static class ClassWithEnumProperty {

		SampleEnum sampleEnum;
		List<SampleEnum> enums;
		EnumSet<SampleEnum> enumSet;
		EnumMap<SampleEnum, String> enumMap;
	}

	static enum SampleEnum {
		FIRST {
			@Override
			void method() {}
		},
		SECOND {
			@Override
			void method() {

			}
		};

		abstract void method();
	}

	static interface InterfaceType {

	}

	static class Address implements InterfaceType {
		String street;
		String city;
	}

	interface Contact {

	}

	static class Person implements Contact {

		@Id String id;

		LocalDate birthDate;

		@Field("foo") String firstname;
		String lastname;

		Set<Address> addresses;

		public Person() {

		}

		@PersistenceConstructor
		public Person(Set<Address> addresses) {
			this.addresses = addresses;
		}
	}

	static class ClassWithSortedMap {
		SortedMap<String, String> map;
	}

	static class ClassWithMapProperty {
		Map<Locale, String> map;
		Map<String, List<String>> mapOfLists;
		Map<String, Object> mapOfObjects;
		Map<String, String[]> mapOfStrings;
		Map<String, Person> mapOfPersons;
		TreeMap<String, Person> treeMapOfPersons;
	}

	static class ClassWithNestedMaps {
		Map<String, Map<String, Map<String, String>>> nestedMaps;
	}

	static class BirthDateContainer {
		LocalDate birthDate;
	}

	static class BigDecimalContainer {
		BigDecimal value;
		Map<String, BigDecimal> map;
		List<BigDecimal> collection;
	}

	static class CollectionWrapper {
		List<Contact> contacts;
		List<List<String>> strings;
		List<Map<String, Locale>> listOfMaps;
		Set<Contact> contactsSet;
	}

	static class LocaleWrapper {
		Locale locale;
	}

	static class ClassWithBigIntegerId {
		@Id BigInteger id;
	}

	static class A<T> {

		String valueType;
		T value;

		public A(T value) {
			this.valueType = value.getClass().getName();
			this.value = value;
		}
	}

	static class ClassWithIntId {

		@Id int id;
	}

	static class DefaultedConstructorArgument {

		String foo;
		int bar;
		double foobar;

		DefaultedConstructorArgument(String foo, @Value("#root.something ?: -1") int bar, double foobar) {
			this.foo = foo;
			this.bar = bar;
			this.foobar = foobar;
		}
	}

	static class Item {
		List<Attribute> attributes;
	}

	static class Attribute {
		String key;
		Object value;
	}

	static class Outer {

		class Inner {
			String value;
		}

		Inner inner;
	}

	static class DBRefWrapper {

		DBRef ref;
		List<DBRef> refs;
		Map<String, DBRef> refMap;
		Map<String, Person> personMap;
	}

	static class URLWrapper {
		URL url;
	}

	static class ClassWithComplexId {

		@Id ComplexId complexId;
	}

	static class ComplexId {
		Long innerId;
	}

	static class TypWithCollectionConstructor {

		List<Attribute> attributes;

		public TypWithCollectionConstructor(List<Attribute> attributes) {
			this.attributes = attributes;
		}
	}

	@TypeAlias("_")
	static class Aliased {
		String name;
	}

	static class ThrowableWrapper {

		Throwable throwable;
	}

	@Document
	static class PrimitiveContainer {

		@Field("property") private final int m_property;

		@PersistenceConstructor
		public PrimitiveContainer(@Value("#root.property") int a_property) {
			m_property = a_property;
		}

		public int property() {
			return m_property;
		}
	}

	@Document
	static class ObjectContainer {

		@Field("property") private final PrimitiveContainer m_property;

		@PersistenceConstructor
		public ObjectContainer(@Value("#root.property") PrimitiveContainer a_property) {
			m_property = a_property;
		}

		public PrimitiveContainer property() {
			return m_property;
		}
	}

	class ClassWithGeoBox {

		Box box;
	}

	class ClassWithGeoCircle {

		Circle circle;
	}

	class ClassWithGeoSphere {

		Sphere sphere;
	}

	class ClassWithGeoPolygon {

		Polygon polygon;
	}

	class ClassWithGeoShape {

		Shape shape;
	}

	class ClassWithTextScoreProperty {

		@TextScore Float score;
	}

	class ClassWithExplicitlyNamedDBRefProperty {

		@Field("explict-name-for-db-ref")//
		@org.springframework.data.sequoiadb.core.mapping.DBRef//
		ClassWithIntId dbRefProperty;

		public ClassWithIntId getDbRefProperty() {
			return dbRefProperty;
		}

	}
}
