/*
 * Copyright 2012-2014 the original author or authors.
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

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import java.util.Collections;
import java.util.Date;

import org.bson.BSONObject;
import org.hamcrest.core.IsEqual;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.context.ApplicationContext;
import org.springframework.data.geo.Point;
import org.springframework.data.mapping.context.MappingContextEvent;
import org.springframework.data.sequoiadb.SequoiadbFactory;

import org.springframework.data.sequoiadb.assist.BasicBSONObjectBuilder;
import org.springframework.data.sequoiadb.core.mapping.*;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;

import org.springframework.data.sequoiadb.assist.DB;
import org.springframework.data.sequoiadb.assist.DBCollection;

/**
 * Unit tests for {@link SequoiadbPersistentEntityIndexCreator}.
 * 





 */
@RunWith(MockitoJUnitRunner.class)
public class SequoiadbPersistentEntityIndexCreatorUnitTests {

	private @Mock
	SequoiadbFactory factory;
	private @Mock ApplicationContext context;
	private @Mock DB db;
	private @Mock DBCollection collection;

	ArgumentCaptor<BSONObject> keysCaptor;
	ArgumentCaptor<BSONObject> optionsCaptor;
	ArgumentCaptor<String> collectionCaptor;

	@Before
	public void setUp() {

		keysCaptor = ArgumentCaptor.forClass(BSONObject.class);
		optionsCaptor = ArgumentCaptor.forClass(BSONObject.class);
		collectionCaptor = ArgumentCaptor.forClass(String.class);

		when(factory.getDb()).thenReturn(db);
		when(db.getCollection(collectionCaptor.capture())).thenReturn(collection);

		doNothing().when(collection).createIndex(keysCaptor.capture(), optionsCaptor.capture());
	}

	@Test
	public void buildsIndexDefinitionUsingFieldName() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(Person.class);

		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		assertThat(keysCaptor.getValue(), is(notNullValue()));
		assertThat(keysCaptor.getValue().keySet(), hasItem("fieldname"));
		assertThat(optionsCaptor.getValue().get("name").toString(), is("indexName"));
		assertThat(optionsCaptor.getValue().get("background"), nullValue());
		assertThat(optionsCaptor.getValue().get("expireAfterSeconds"), nullValue());
	}

	@Test
	public void doesNotCreateIndexForEntityComingFromDifferentMappingContext() {

		SequoiadbMappingContext mappingContext = new SequoiadbMappingContext();
		SequoiadbMappingContext personMappingContext = prepareMappingContext(Person.class);

		SequoiadbPersistentEntityIndexCreator creator = new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		SequoiadbPersistentEntity<?> entity = personMappingContext.getPersistentEntity(Person.class);
		MappingContextEvent<SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> event = new MappingContextEvent<SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty>(
				personMappingContext, entity);

		creator.onApplicationEvent(event);

		verifyZeroInteractions(collection);
	}

	/**
	 * @see DATA_JIRA-530
	 */
	@Test
	public void isIndexCreatorForMappingContextHandedIntoConstructor() {

		SequoiadbMappingContext mappingContext = new SequoiadbMappingContext();
		mappingContext.initialize();

		SequoiadbPersistentEntityIndexCreator creator = new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);
		assertThat(creator.isIndexCreatorFor(mappingContext), is(true));
		assertThat(creator.isIndexCreatorFor(new SequoiadbMappingContext()), is(false));
	}

	/**
	 * @see DATA_JIRA-554
	 */
	@Test
	public void triggersBackgroundIndexingIfConfigured() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(AnotherPerson.class);
		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		assertThat(keysCaptor.getValue(), is(notNullValue()));
		assertThat(keysCaptor.getValue().keySet(), hasItem("lastname"));
		assertThat(optionsCaptor.getValue().get("name").toString(), is("lastname"));
		assertThat(optionsCaptor.getValue().get("background"), IsEqual.<Object> equalTo(true));
		assertThat(optionsCaptor.getValue().get("expireAfterSeconds"), nullValue());
	}

	/**
	 * @see DATA_JIRA-544
	 */
	@Test
	public void expireAfterSecondsIfConfigured() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(Milk.class);
		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		assertThat(keysCaptor.getValue(), is(notNullValue()));
		assertThat(keysCaptor.getValue().keySet(), hasItem("expiry"));
		assertThat(optionsCaptor.getValue().get("expireAfterSeconds"), IsEqual.<Object> equalTo(60L));
	}

	/**
	 * @see DATA_JIRA-899
	 */
	@Test
	public void createsNotNestedGeoSpatialIndexCorrectly() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(Wrapper.class);
		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		assertThat(keysCaptor.getValue(), equalTo(new BasicBSONObjectBuilder().add("company.address.location", "2d").get()));
		assertThat(optionsCaptor.getValue(), equalTo(new BasicBSONObjectBuilder().add("name", "company.address.location")
				.add("min", -180).add("max", 180).add("bits", 26).get()));
	}

	/**
	 * @see DATA_JIRA-827
	 */
	@Test
	public void autoGeneratedIndexNameShouldGenerateNoName() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(EntityWithGeneratedIndexName.class);
		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		assertThat(keysCaptor.getValue().containsField("name"), is(false));
		assertThat(keysCaptor.getValue().keySet(), hasItem("lastname"));
		assertThat(optionsCaptor.getValue(), is(new BasicBSONObjectBuilder().get()));
	}

	/**
	 * @see DATA_JIRA-367
	 */
	@Test
	public void indexCreationShouldNotCreateNewCollectionForNestedGeoSpatialIndexStructures() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(Wrapper.class);
		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		ArgumentCaptor<String> collectionNameCapturer = ArgumentCaptor.forClass(String.class);

		verify(db, times(1)).getCollection(collectionNameCapturer.capture());
		assertThat(collectionNameCapturer.getValue(), equalTo("wrapper"));
	}

	/**
	 * @see DATA_JIRA-367
	 */
	@Test
	public void indexCreationShouldNotCreateNewCollectionForNestedIndexStructures() {

		SequoiadbMappingContext mappingContext = prepareMappingContext(IndexedDocumentWrapper.class);
		new SequoiadbPersistentEntityIndexCreator(mappingContext, factory);

		ArgumentCaptor<String> collectionNameCapturer = ArgumentCaptor.forClass(String.class);

		verify(db, times(1)).getCollection(collectionNameCapturer.capture());
		assertThat(collectionNameCapturer.getValue(), equalTo("indexedDocumentWrapper"));
	}

	private static SequoiadbMappingContext prepareMappingContext(Class<?> type) {

		SequoiadbMappingContext mappingContext = new SequoiadbMappingContext();
		mappingContext.setInitialEntitySet(Collections.singleton(type));
		mappingContext.initialize();

		return mappingContext;
	}

	@Document
	static class Person {

		@Indexed(name = "indexName")//
		@Field("fieldname")//
		String field;

	}

	@Document
	static class AnotherPerson {

		@Indexed(background = true) String lastname;
	}

	@Document
	static class Milk {

		@Indexed(expireAfterSeconds = 60) Date expiry;
	}

	@Document
	static class Wrapper {

		String id;
		Company company;

	}

	static class Company {

		String name;
		Address address;
	}

	static class Address {

		String street;
		String city;

		@GeoSpatialIndexed Point location;
	}

	@Document
	static class IndexedDocumentWrapper {

		IndexedDocument indexedDocument;
	}

	static class IndexedDocument {

		@Indexed String indexedValue;
	}

	@Document
	class EntityWithGeneratedIndexName {

		@Indexed(useGeneratedName = true, name = "ignored") String lastname;
	}
}
