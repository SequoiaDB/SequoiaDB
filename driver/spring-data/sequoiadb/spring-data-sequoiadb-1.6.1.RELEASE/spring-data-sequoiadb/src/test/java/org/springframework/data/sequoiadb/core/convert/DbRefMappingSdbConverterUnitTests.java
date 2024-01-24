/*
 * Copyright 2013-2014 the original author or authors.
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

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;
import static org.springframework.data.sequoiadb.core.convert.LazyLoadingTestUtils.*;

import java.io.Serializable;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.annotation.AccessType;
import org.springframework.data.annotation.AccessType.Type;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.PersistenceConstructor;
import org.springframework.data.mapping.PropertyPath;
import org.springframework.data.mapping.model.BeanWrapper;
import org.springframework.data.sequoiadb.SequoiadbFactory;


import org.springframework.data.sequoiadb.core.SequoiadbExceptionTranslator;
import org.springframework.data.sequoiadb.core.convert.MappingSdbConverterUnitTests.Person;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.test.util.ReflectionTestUtils;
import org.springframework.util.SerializationUtils;

import org.springframework.data.sequoiadb.assist.DBRef;

/**
 * Unit tests dor {@link DbRefMappingSdbConverterUnitTests}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class DbRefMappingSdbConverterUnitTests {

	MappingSequoiadbConverter converter;
	SequoiadbMappingContext mappingContext;

	@Mock
	SequoiadbFactory dbFactory;

	@Before
	public void setUp() {

		when(dbFactory.getExceptionTranslator()).thenReturn(new SequoiadbExceptionTranslator());

		this.mappingContext = new SequoiadbMappingContext();
		this.converter = new MappingSequoiadbConverter(new DefaultDbRefResolver(dbFactory), mappingContext);
	}

	/**
	 * @see DATA_JIRA-347
	 */
	@Test
	public void createsSimpleDBRefCorrectly() {

		Person person = new Person();
		person.id = "foo";

		DBRef dbRef = converter.toDBRef(person, null);
		assertThat(dbRef.getId(), is((Object) "foo"));
		assertThat(dbRef.getRef(), is("person"));
	}

	/**
	 * @see DATA_JIRA-657
	 */
	@Test
	public void convertDocumentWithMapDBRef() {

		MapDBRef mapDBRef = new MapDBRef();

		MapDBRefVal val = new MapDBRefVal();
		val.id = BigInteger.ONE;

		Map<String, MapDBRefVal> mapVal = new HashMap<String, MapDBRefVal>();
		mapVal.put("test", val);

		mapDBRef.map = mapVal;

		BasicBSONObject dbObject = new BasicBSONObject();
		converter.write(mapDBRef, dbObject);

		BSONObject map = (BSONObject) dbObject.get("map");

		assertThat(map.get("test"), instanceOf(DBRef.class));

		BSONObject mapValDBObject = new BasicBSONObject();
		mapValDBObject.put("_id", BigInteger.ONE);

		DBRef dbRef = mock(DBRef.class);
		when(dbRef.fetch()).thenReturn(mapValDBObject);

		((BSONObject) dbObject.get("map")).put("test", dbRef);

		MapDBRef read = converter.read(MapDBRef.class, dbObject);

		assertThat(read.map.get("test").id, is(BigInteger.ONE));
	}

	/**
	 * @see DATA_JIRA-347
	 */
	@Test
	public void createsDBRefWithClientSpecCorrectly() {

		PropertyPath path = PropertyPath.from("person", PersonClient.class);
		SequoiadbPersistentProperty property = mappingContext.getPersistentPropertyPath(path).getLeafProperty();

		Person person = new Person();
		person.id = "foo";

		DBRef dbRef = converter.toDBRef(person, property);
		assertThat(dbRef.getId(), is((Object) "foo"));
		assertThat(dbRef.getRef(), is("person"));
	}

	/**
	 * @see DATA_JIRA-348
	 */
	@Test
	public void lazyLoadingProxyForLazyDbRefOnInterface() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		ClassWithLazyDbRefs lazyDbRefs = new ClassWithLazyDbRefs();
		lazyDbRefs.dbRefToInterface = new LinkedList<LazyDbRefTarget>(Arrays.asList(new LazyDbRefTarget("1")));
		converterSpy.write(lazyDbRefs, dbo);

		ClassWithLazyDbRefs result = converterSpy.read(ClassWithLazyDbRefs.class, dbo);

		assertProxyIsResolved(result.dbRefToInterface, false);
		assertThat(result.dbRefToInterface.get(0).getId(), is(id));
		assertProxyIsResolved(result.dbRefToInterface, true);
		assertThat(result.dbRefToInterface.get(0).getValue(), is(value));
	}

	/**
	 * @see DATA_JIRA-348
	 */
	@Test
	public void lazyLoadingProxyForLazyDbRefOnConcreteCollection() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		ClassWithLazyDbRefs lazyDbRefs = new ClassWithLazyDbRefs();
		lazyDbRefs.dbRefToConcreteCollection = new ArrayList<LazyDbRefTarget>(Arrays.asList(new LazyDbRefTarget(id, value)));
		converterSpy.write(lazyDbRefs, dbo);

		ClassWithLazyDbRefs result = converterSpy.read(ClassWithLazyDbRefs.class, dbo);

		assertProxyIsResolved(result.dbRefToConcreteCollection, false);
		assertThat(result.dbRefToConcreteCollection.get(0).getId(), is(id));
		assertProxyIsResolved(result.dbRefToConcreteCollection, true);
		assertThat(result.dbRefToConcreteCollection.get(0).getValue(), is(value));
	}

	/**
	 * @see DATA_JIRA-348
	 */
	@Test
	public void lazyLoadingProxyForLazyDbRefOnConcreteType() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		ClassWithLazyDbRefs lazyDbRefs = new ClassWithLazyDbRefs();
		lazyDbRefs.dbRefToConcreteType = new LazyDbRefTarget(id, value);
		converterSpy.write(lazyDbRefs, dbo);

		ClassWithLazyDbRefs result = converterSpy.read(ClassWithLazyDbRefs.class, dbo);

		assertProxyIsResolved(result.dbRefToConcreteType, false);
		assertThat(result.dbRefToConcreteType.getId(), is(id));
		assertProxyIsResolved(result.dbRefToConcreteType, true);
		assertThat(result.dbRefToConcreteType.getValue(), is(value));
	}

	/**
	 * @see DATA_JIRA-348
	 */
	@Test
	public void lazyLoadingProxyForLazyDbRefOnConcreteTypeWithPersistenceConstructor() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		ClassWithLazyDbRefs lazyDbRefs = new ClassWithLazyDbRefs();
		lazyDbRefs.dbRefToConcreteTypeWithPersistenceConstructor = new LazyDbRefTargetWithPeristenceConstructor(
				(Object) id, (Object) value);
		converterSpy.write(lazyDbRefs, dbo);

		ClassWithLazyDbRefs result = converterSpy.read(ClassWithLazyDbRefs.class, dbo);

		assertProxyIsResolved(result.dbRefToConcreteTypeWithPersistenceConstructor, false);
		assertThat(result.dbRefToConcreteTypeWithPersistenceConstructor.getId(), is(id));
		assertProxyIsResolved(result.dbRefToConcreteTypeWithPersistenceConstructor, true);
		assertThat(result.dbRefToConcreteTypeWithPersistenceConstructor.getValue(), is(value));
	}

	/**
	 * @see DATA_JIRA-348
	 */
	@Test
	public void lazyLoadingProxyForLazyDbRefOnConcreteTypeWithPersistenceConstructorButWithoutDefaultConstructor() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		ClassWithLazyDbRefs lazyDbRefs = new ClassWithLazyDbRefs();
		lazyDbRefs.dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor = new LazyDbRefTargetWithPeristenceConstructorWithoutDefaultConstructor(
				(Object) id, (Object) value);
		converterSpy.write(lazyDbRefs, dbo);

		ClassWithLazyDbRefs result = converterSpy.read(ClassWithLazyDbRefs.class, dbo);

		assertProxyIsResolved(result.dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor, false);
		assertThat(result.dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor.getId(), is(id));
		assertProxyIsResolved(result.dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor, true);
		assertThat(result.dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor.getValue(), is(value));
	}

	/**
	 * @see DATA_JIRA-348
	 */
	@Test
	public void lazyLoadingProxyForSerializableLazyDbRefOnConcreteType() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		SerializableClassWithLazyDbRefs lazyDbRefs = new SerializableClassWithLazyDbRefs();
		lazyDbRefs.dbRefToSerializableTarget = new SerializableLazyDbRefTarget(id, value);
		converterSpy.write(lazyDbRefs, dbo);

		SerializableClassWithLazyDbRefs result = converterSpy.read(SerializableClassWithLazyDbRefs.class, dbo);

		SerializableClassWithLazyDbRefs deserializedResult = (SerializableClassWithLazyDbRefs) transport(result);

		assertThat(deserializedResult.dbRefToSerializableTarget.getId(), is(id));
		assertProxyIsResolved(deserializedResult.dbRefToSerializableTarget, true);
		assertThat(deserializedResult.dbRefToSerializableTarget.getValue(), is(value));
	}

	/**
	 * @see DATA_JIRA-884
	 */
	@Test
	public void lazyLoadingProxyForToStringObjectMethodOverridingDbref() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		WithObjectMethodOverrideLazyDbRefs lazyDbRefs = new WithObjectMethodOverrideLazyDbRefs();
		lazyDbRefs.dbRefToToStringObjectMethodOverride = new ToStringObjectMethodOverrideLazyDbRefTarget(id, value);
		converterSpy.write(lazyDbRefs, dbo);

		WithObjectMethodOverrideLazyDbRefs result = converterSpy.read(WithObjectMethodOverrideLazyDbRefs.class, dbo);

		assertThat(result.dbRefToToStringObjectMethodOverride, is(notNullValue()));
		assertProxyIsResolved(result.dbRefToToStringObjectMethodOverride, false);
		assertThat(result.dbRefToToStringObjectMethodOverride.toString(), is(id + ":" + value));
		assertProxyIsResolved(result.dbRefToToStringObjectMethodOverride, true);
	}

	/**
	 * @see DATA_JIRA-884
	 */
	@Test
	public void callingToStringObjectMethodOnLazyLoadingDbrefShouldNotInitializeProxy() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		WithObjectMethodOverrideLazyDbRefs lazyDbRefs = new WithObjectMethodOverrideLazyDbRefs();
		lazyDbRefs.dbRefToPlainObject = new LazyDbRefTarget(id, value);
		converterSpy.write(lazyDbRefs, dbo);

		WithObjectMethodOverrideLazyDbRefs result = converterSpy.read(WithObjectMethodOverrideLazyDbRefs.class, dbo);

		assertThat(result.dbRefToPlainObject, is(notNullValue()));
		assertProxyIsResolved(result.dbRefToPlainObject, false);

		// calling Object#toString does not initialize the proxy.
		String proxyString = result.dbRefToPlainObject.toString();
		assertThat(proxyString, is("lazyDbRefTarget" + ":" + id + "$LazyLoadingProxy"));
		assertProxyIsResolved(result.dbRefToPlainObject, false);

		// calling another method not declared on object triggers proxy initialization.
		assertThat(result.dbRefToPlainObject.getValue(), is(value));
		assertProxyIsResolved(result.dbRefToPlainObject, true);
	}

	/**
	 * @see DATA_JIRA-884
	 */
	@Test
	public void equalsObjectMethodOnLazyLoadingDbrefShouldNotInitializeProxy() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		WithObjectMethodOverrideLazyDbRefs lazyDbRefs = new WithObjectMethodOverrideLazyDbRefs();
		lazyDbRefs.dbRefToPlainObject = new LazyDbRefTarget(id, value);
		lazyDbRefs.dbRefToToStringObjectMethodOverride = new ToStringObjectMethodOverrideLazyDbRefTarget(id, value);
		converterSpy.write(lazyDbRefs, dbo);

		WithObjectMethodOverrideLazyDbRefs result = converterSpy.read(WithObjectMethodOverrideLazyDbRefs.class, dbo);

		assertThat(result.dbRefToPlainObject, is(notNullValue()));
		assertProxyIsResolved(result.dbRefToPlainObject, false);

		assertThat(result.dbRefToPlainObject, is(equalTo(result.dbRefToPlainObject)));
		assertThat(result.dbRefToPlainObject, is(not(equalTo(null))));
		assertThat(result.dbRefToPlainObject, is(not(equalTo((Object) lazyDbRefs.dbRefToToStringObjectMethodOverride))));

		assertProxyIsResolved(result.dbRefToPlainObject, false);
	}

	/**
	 * @see DATA_JIRA-884
	 */
	@Test
	public void hashcodeObjectMethodOnLazyLoadingDbrefShouldNotInitializeProxy() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		WithObjectMethodOverrideLazyDbRefs lazyDbRefs = new WithObjectMethodOverrideLazyDbRefs();
		lazyDbRefs.dbRefToPlainObject = new LazyDbRefTarget(id, value);
		lazyDbRefs.dbRefToToStringObjectMethodOverride = new ToStringObjectMethodOverrideLazyDbRefTarget(id, value);
		converterSpy.write(lazyDbRefs, dbo);

		WithObjectMethodOverrideLazyDbRefs result = converterSpy.read(WithObjectMethodOverrideLazyDbRefs.class, dbo);

		assertThat(result.dbRefToPlainObject, is(notNullValue()));
		assertProxyIsResolved(result.dbRefToPlainObject, false);

		assertThat(result.dbRefToPlainObject.hashCode(), is(311365444));

		assertProxyIsResolved(result.dbRefToPlainObject, false);
	}

	/**
	 * @see DATA_JIRA-884
	 */
	@Test
	public void lazyLoadingProxyForEqualsAndHashcodeObjectMethodOverridingDbref() {

		String id = "42";
		String value = "bubu";
		MappingSequoiadbConverter converterSpy = spy(converter);
		doReturn(new BasicBSONObject("_id", id).append("value", value)).when(converterSpy).readRef((DBRef) any());

		BasicBSONObject dbo = new BasicBSONObject();
		WithObjectMethodOverrideLazyDbRefs lazyDbRefs = new WithObjectMethodOverrideLazyDbRefs();
		lazyDbRefs.dbRefEqualsAndHashcodeObjectMethodOverride1 = new EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget(
				id, value);
		lazyDbRefs.dbRefEqualsAndHashcodeObjectMethodOverride2 = new EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget(
				id, value);
		converterSpy.write(lazyDbRefs, dbo);

		WithObjectMethodOverrideLazyDbRefs result = converterSpy.read(WithObjectMethodOverrideLazyDbRefs.class, dbo);

		assertProxyIsResolved(result.dbRefEqualsAndHashcodeObjectMethodOverride1, false);
		assertThat(result.dbRefEqualsAndHashcodeObjectMethodOverride1, is(notNullValue()));
		result.dbRefEqualsAndHashcodeObjectMethodOverride1.equals(null);
		assertProxyIsResolved(result.dbRefEqualsAndHashcodeObjectMethodOverride1, true);

		assertProxyIsResolved(result.dbRefEqualsAndHashcodeObjectMethodOverride2, false);
		assertThat(result.dbRefEqualsAndHashcodeObjectMethodOverride2, is(notNullValue()));
		result.dbRefEqualsAndHashcodeObjectMethodOverride2.hashCode();
		assertProxyIsResolved(result.dbRefEqualsAndHashcodeObjectMethodOverride2, true);
	}

	/**
	 * @see DATA_JIRA-987
	 */
	@Test
	public void shouldNotGenerateLazyLoadingProxyForNullValues() {

		BSONObject dbo = new BasicBSONObject();
		ClassWithLazyDbRefs lazyDbRefs = new ClassWithLazyDbRefs();
		lazyDbRefs.id = "42";
		converter.write(lazyDbRefs, dbo);

		ClassWithLazyDbRefs result = converter.read(ClassWithLazyDbRefs.class, dbo);

		assertThat(result.id, is(lazyDbRefs.id));
		assertThat(result.dbRefToInterface, is(nullValue()));
		assertThat(result.dbRefToConcreteCollection, is(nullValue()));
		assertThat(result.dbRefToConcreteType, is(nullValue()));
		assertThat(result.dbRefToConcreteTypeWithPersistenceConstructor, is(nullValue()));
		assertThat(result.dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor, is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-1005
	 */
	@Test
	public void shouldBeAbleToStoreDirectReferencesToSelf() {

		BSONObject dbo = new BasicBSONObject();

		ClassWithDbRefField o = new ClassWithDbRefField();
		o.id = "123";
		o.reference = o;
		converter.write(o, dbo);

		ClassWithDbRefField found = converter.read(ClassWithDbRefField.class, dbo);

		assertThat(found, is(notNullValue()));
		assertThat(found.reference, is(found));
	}

	/**
	 * @see DATA_JIRA-1005
	 */
	@Test
	public void shouldBeAbleToStoreNestedReferencesToSelf() {

		BSONObject dbo = new BasicBSONObject();

		ClassWithNestedDbRefField o = new ClassWithNestedDbRefField();
		o.id = "123";
		o.nested = new NestedReferenceHolder();
		o.nested.reference = o;

		converter.write(o, dbo);

		ClassWithNestedDbRefField found = converter.read(ClassWithNestedDbRefField.class, dbo);

		assertThat(found, is(notNullValue()));
		assertThat(found.nested, is(notNullValue()));
		assertThat(found.nested.reference, is(found));
	}

	/**
	 * @see DATA_JIRA-1012
	 */
	@Test
	public void shouldEagerlyResolveIdPropertyWithFieldAccess() {

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(ClassWithLazyDbRefs.class);
		SequoiadbPersistentProperty property = entity.getPersistentProperty("dbRefToConcreteType");

		String idValue = new ObjectId().toString();
		DBRef dbRef = converter.toDBRef(new LazyDbRefTarget(idValue), property);

		BSONObject object = new BasicBSONObject("dbRefToConcreteType", dbRef);

		ClassWithLazyDbRefs result = converter.read(ClassWithLazyDbRefs.class, object);

		BeanWrapper<LazyDbRefTarget> wrapper = BeanWrapper.create(result.dbRefToConcreteType, null);
		SequoiadbPersistentProperty idProperty = mappingContext.getPersistentEntity(LazyDbRefTarget.class).getIdProperty();

		assertThat(wrapper.getProperty(idProperty), is(notNullValue()));
		assertProxyIsResolved(result.dbRefToConcreteType, false);
	}

	/**
	 * @see DATA_JIRA-1012
	 */
	@Test
	public void shouldNotEagerlyResolveIdPropertyWithPropertyAccess() {

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(ClassWithLazyDbRefs.class);
		SequoiadbPersistentProperty property = entity.getPersistentProperty("dbRefToConcreteTypeWithPropertyAccess");

		String idValue = new ObjectId().toString();
		DBRef dbRef = converter.toDBRef(new LazyDbRefTargetPropertyAccess(idValue), property);

		BSONObject object = new BasicBSONObject("dbRefToConcreteTypeWithPropertyAccess", dbRef);

		ClassWithLazyDbRefs result = converter.read(ClassWithLazyDbRefs.class, object);

		LazyDbRefTargetPropertyAccess proxy = result.dbRefToConcreteTypeWithPropertyAccess;
		assertThat(ReflectionTestUtils.getField(proxy, "id"), is(nullValue()));
		assertProxyIsResolved(proxy, false);
	}

	/**
	 * @see DATA_JIRA-1076
	 */
	@Test
	public void shouldNotTriggerResolvingOfLazyLoadedProxyWhenFinalizeMethodIsInvoked() throws Exception {

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(WithObjectMethodOverrideLazyDbRefs.class);
		SequoiadbPersistentProperty property = entity.getPersistentProperty("dbRefToConcreteTypeWithPropertyAccess");

		String idValue = new ObjectId().toString();
		DBRef dbRef = converter.toDBRef(new LazyDbRefTargetPropertyAccess(idValue), property);

		WithObjectMethodOverrideLazyDbRefs result = converter.read(WithObjectMethodOverrideLazyDbRefs.class,
				new BasicBSONObject("dbRefToPlainObject", dbRef));

		ReflectionTestUtils.invokeMethod(result.dbRefToPlainObject, "finalize");

		assertProxyIsResolved(result.dbRefToPlainObject, false);
	}

	private Object transport(Object result) {
		return SerializationUtils.deserialize(SerializationUtils.serialize(result));
	}

	class MapDBRef {
		@org.springframework.data.sequoiadb.core.mapping.DBRef Map<String, MapDBRefVal> map;
	}

	class MapDBRefVal {
		BigInteger id;
	}

	class PersonClient {
		@org.springframework.data.sequoiadb.core.mapping.DBRef Person person;
	}

	static class ClassWithLazyDbRefs {

		@Id String id;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) List<LazyDbRefTarget> dbRefToInterface;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) ArrayList<LazyDbRefTarget> dbRefToConcreteCollection;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) LazyDbRefTarget dbRefToConcreteType;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) LazyDbRefTargetPropertyAccess dbRefToConcreteTypeWithPropertyAccess;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) LazyDbRefTargetWithPeristenceConstructor dbRefToConcreteTypeWithPersistenceConstructor;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) LazyDbRefTargetWithPeristenceConstructorWithoutDefaultConstructor dbRefToConcreteTypeWithPersistenceConstructorWithoutDefaultConstructor;
	}

	static class SerializableClassWithLazyDbRefs implements Serializable {

		private static final long serialVersionUID = 1L;

		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) SerializableLazyDbRefTarget dbRefToSerializableTarget;
	}

	static class LazyDbRefTarget implements Serializable {

		private static final long serialVersionUID = 1L;

		@Id String id;
		String value;

		public LazyDbRefTarget() {
			this(null);
		}

		public LazyDbRefTarget(String id) {
			this(id, null);
		}

		public LazyDbRefTarget(String id, String value) {
			this.id = id;
			this.value = value;
		}

		public String getId() {
			return id;
		}

		public String getValue() {
			return value;
		}
	}

	static class LazyDbRefTargetPropertyAccess implements Serializable {

		private static final long serialVersionUID = 1L;

		@Id @AccessType(Type.PROPERTY) String id;

		public LazyDbRefTargetPropertyAccess(String id) {
			this.id = id;
		}

		public String getId() {
			return id;
		}
	}

	@SuppressWarnings("serial")
	static class LazyDbRefTargetWithPeristenceConstructor extends LazyDbRefTarget {

		boolean persistenceConstructorCalled;

		public LazyDbRefTargetWithPeristenceConstructor() {}

		@PersistenceConstructor
		public LazyDbRefTargetWithPeristenceConstructor(String id, String value) {
			super(id, value);
			this.persistenceConstructorCalled = true;
		}

		public LazyDbRefTargetWithPeristenceConstructor(Object id, Object value) {
			super(id.toString(), value.toString());
		}
	}

	@SuppressWarnings("serial")
	static class LazyDbRefTargetWithPeristenceConstructorWithoutDefaultConstructor extends LazyDbRefTarget {

		boolean persistenceConstructorCalled;

		@PersistenceConstructor
		public LazyDbRefTargetWithPeristenceConstructorWithoutDefaultConstructor(String id, String value) {
			super(id, value);
			this.persistenceConstructorCalled = true;
		}

		public LazyDbRefTargetWithPeristenceConstructorWithoutDefaultConstructor(Object id, Object value) {
			super(id.toString(), value.toString());
		}
	}

	static class SerializableLazyDbRefTarget extends LazyDbRefTarget implements Serializable {

		public SerializableLazyDbRefTarget() {}

		public SerializableLazyDbRefTarget(String id, String value) {
			super(id, value);
		}

		private static final long serialVersionUID = 1L;
	}

	static class ToStringObjectMethodOverrideLazyDbRefTarget extends LazyDbRefTarget {

		private static final long serialVersionUID = 1L;

		public ToStringObjectMethodOverrideLazyDbRefTarget() {}

		public ToStringObjectMethodOverrideLazyDbRefTarget(String id, String value) {
			super(id, value);
		}

		/* 
		 * (non-Javadoc)
		 * @see java.lang.Object#toString()
		 */
		@Override
		public String toString() {
			return this.id + ":" + this.value;
		}
	}

	static class EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget extends LazyDbRefTarget {

		private static final long serialVersionUID = 1L;

		public EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget() {}

		public EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget(String id, String value) {
			super(id, value);
		}

		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((id == null) ? 0 : id.hashCode());
			result = prime * result + ((value == null) ? 0 : value.hashCode());
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget other = (EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget) obj;
			if (id == null) {
				if (other.id != null)
					return false;
			} else if (!id.equals(other.id))
				return false;
			if (value == null) {
				if (other.value != null)
					return false;
			} else if (!value.equals(other.value))
				return false;
			return true;
		}
	}

	static class WithObjectMethodOverrideLazyDbRefs {

		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) LazyDbRefTarget dbRefToPlainObject;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) ToStringObjectMethodOverrideLazyDbRefTarget dbRefToToStringObjectMethodOverride;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget dbRefEqualsAndHashcodeObjectMethodOverride2;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) EqualsAndHashCodeObjectMethodOverrideLazyDbRefTarget dbRefEqualsAndHashcodeObjectMethodOverride1;
	}

	class ClassWithDbRefField {

		String id;
		@org.springframework.data.sequoiadb.core.mapping.DBRef ClassWithDbRefField reference;
	}

	static class NestedReferenceHolder {

		String id;
		@org.springframework.data.sequoiadb.core.mapping.DBRef ClassWithNestedDbRefField reference;
	}

	static class ClassWithNestedDbRefField {

		String id;
		NestedReferenceHolder nested;
	}
}
