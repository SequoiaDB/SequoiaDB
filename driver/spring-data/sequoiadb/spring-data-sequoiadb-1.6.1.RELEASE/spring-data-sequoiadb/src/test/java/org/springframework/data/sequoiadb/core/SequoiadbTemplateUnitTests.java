/*
 * Copyright 2010-2014 the original author or authors.
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
package org.springframework.data.sequoiadb.core;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import java.math.BigInteger;
import java.util.Collections;
import java.util.regex.Pattern;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.hamcrest.core.Is;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.context.support.GenericApplicationContext;
import org.springframework.core.convert.converter.Converter;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.InvalidDataAccessApiUsageException;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.Version;
import org.springframework.data.domain.Sort;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.convert.CustomConversions;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.QueryMapper;
import org.springframework.data.sequoiadb.core.index.SequoiadbPersistentEntityIndexCreator;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.query.BasicQuery;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.core.query.Update;
import org.springframework.test.util.ReflectionTestUtils;



/**
 * Unit tests for {@link SequoiadbTemplate}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class SequoiadbTemplateUnitTests extends SequoiadbOperationsUnitTests {

	SequoiadbTemplate template;

	@Mock
	SequoiadbFactory factory;
	@Mock
	Sdb sdb;
	@Mock DB db;
	@Mock DBCollection collection;
	@Mock DBCursor cursor;

	SequoiadbExceptionTranslator exceptionTranslator = new SequoiadbExceptionTranslator();
	MappingSequoiadbConverter converter;
	SequoiadbMappingContext mappingContext;

	@Before
	public void setUp() {

		when(cursor.copy()).thenReturn(cursor);
		when(factory.getDb()).thenReturn(db);
		when(factory.getExceptionTranslator()).thenReturn(exceptionTranslator);
		when(db.getCollection(Mockito.any(String.class))).thenReturn(collection);
		when(collection.find(Mockito.any(BSONObject.class))).thenReturn(cursor);
		when(cursor.limit(anyInt())).thenReturn(cursor);
		when(cursor.sort(Mockito.any(BSONObject.class))).thenReturn(cursor);
		when(cursor.hint(anyString())).thenReturn(cursor);

		this.mappingContext = new SequoiadbMappingContext();
		this.converter = new MappingSequoiadbConverter(new DefaultDbRefResolver(factory), mappingContext);
		this.template = new SequoiadbTemplate(factory, converter);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullDatabaseName() throws Exception {
		new SequoiadbTemplate(sdb, null);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullSequoiadb() throws Exception {
		new SequoiadbTemplate(null, "database");
	}

	@Test(expected = DataAccessException.class)
	public void removeHandlesBaseExceptionProperly() throws Exception {
		SequoiadbTemplate template = mockOutGetDb();
		when(db.getCollection("collection")).thenThrow(
				new BaseException(SDBError.SDB_DMS_NOTEXIST.getErrorCode(),"Exception!"));

		template.remove(null, "collection");
	}

	@Test
	public void defaultsConverterToMappingSequoiadbConverter() throws Exception {
		SequoiadbTemplate template = new SequoiadbTemplate(sdb, "database");
		assertTrue(ReflectionTestUtils.getField(template, "sequoiadbConverter") instanceof MappingSequoiadbConverter);
	}

	@Test(expected = InvalidDataAccessApiUsageException.class)
	@Ignore // sdb not support mapreduce
	public void rejectsNotFoundMapReduceResource() {

		GenericApplicationContext ctx = new GenericApplicationContext();
		ctx.refresh();
		template.setApplicationContext(ctx);
		template.mapReduce("foo", "classpath:doesNotExist.js", "function() {}", Person.class);
	}

	/**
	 * @see DATA_JIRA-322
	 */
	@Test(expected = InvalidDataAccessApiUsageException.class)
	public void rejectsEntityWithNullIdIfNotSupportedIdType() {

		Object entity = new NotAutogenerateableId();
		template.save(entity);
	}

	/**
	 * @see DATA_JIRA-322
	 */
	@Test
	public void storesEntityWithSetIdAlthoughNotAutogenerateable() {

		NotAutogenerateableId entity = new NotAutogenerateableId();
		entity.id = 1;

		template.save(entity);
	}

	/**
	 * @see DATA_JIRA-322
	 */
	@Test
	public void autogeneratesIdForEntityWithAutogeneratableId() {

		this.converter.afterPropertiesSet();

		SequoiadbTemplate template = spy(this.template);
		doReturn(new ObjectId()).when(template).saveDBObject(Mockito.any(String.class), Mockito.any(BSONObject.class),
				Mockito.any(Class.class));

		AutogenerateableId entity = new AutogenerateableId();
		template.save(entity);

		assertThat(entity.id, is(notNullValue()));
	}

	/**
	 * @see DATA_JIRA-374
	 */
	@Test
	public void convertsUpdateConstraintsUsingConverters() {

		CustomConversions conversions = new CustomConversions(Collections.singletonList(MyConverter.INSTANCE));
		this.converter.setCustomConversions(conversions);
		this.converter.afterPropertiesSet();

		Query query = new Query();
		Update update = new Update().set("foo", new AutogenerateableId());

		template.updateFirst(query, update, Wrapper.class);

		QueryMapper queryMapper = new QueryMapper(converter);
		BSONObject reference = queryMapper.getMappedObject(update.getUpdateObject(), null);

		verify(collection, times(1)).update(Mockito.any(BSONObject.class), eq(reference), anyBoolean(), anyBoolean());
	}

	/**
	 * @see DATA_JIRA-474
	 */
	@Test
	public void setsUnpopulatedIdField() {

		NotAutogenerateableId entity = new NotAutogenerateableId();

		template.populateIdIfNecessary(entity, 5);
		assertThat(entity.id, is(5));
	}

	/**
	 * @see DATA_JIRA-474
	 */
	@Test
	public void doesNotSetAlreadyPopulatedId() {

		NotAutogenerateableId entity = new NotAutogenerateableId();
		entity.id = 5;

		template.populateIdIfNecessary(entity, 7);
		assertThat(entity.id, is(5));
	}

	/**
	 * @see DATA_JIRA-868
	 */
	@Test
	public void findAndModifyShouldBumpVersionByOneWhenVersionFieldNotIncludedInUpdate() {

		VersionedEntity v = new VersionedEntity();
		v.id = 1;
		v.version = 0;

		ArgumentCaptor<BSONObject> captor = ArgumentCaptor.forClass(BSONObject.class);

		template.findAndModify(new Query(), new Update().set("id", "10"), VersionedEntity.class);
		verify(collection, times(1)).findAndModify(Matchers.any(BSONObject.class),
				org.mockito.Matchers.isNull(BSONObject.class), org.mockito.Matchers.isNull(BSONObject.class), eq(false),
				captor.capture(), eq(false), eq(false));

		Assert.assertThat(captor.getValue().get("$inc"), Is.<Object> is(new BasicBSONObject("version", 1L)));
	}

	/**
	 * @see DATA_JIRA-868
	 */
	@Test
	public void findAndModifyShouldNotBumpVersionByOneWhenVersionFieldAlreadyIncludedInUpdate() {

		VersionedEntity v = new VersionedEntity();
		v.id = 1;
		v.version = 0;

		ArgumentCaptor<BSONObject> captor = ArgumentCaptor.forClass(BSONObject.class);

		template.findAndModify(new Query(), new Update().set("version", 100), VersionedEntity.class);
		verify(collection, times(1)).findAndModify(Matchers.any(BSONObject.class), isNull(BSONObject.class),
				isNull(BSONObject.class), eq(false), captor.capture(), eq(false), eq(false));

		Assert.assertThat(captor.getValue().get("$set"), Is.<Object> is(new BasicBSONObject("version", 100)));
		Assert.assertThat(captor.getValue().get("$inc"), nullValue());
	}

	/**
	 * @see DATA_JIRA-533
	 */
	@Test
	public void registersDefaultEntityIndexCreatorIfApplicationContextHasOneForDifferentMappingContext() {

		GenericApplicationContext applicationContext = new GenericApplicationContext();
		applicationContext.getBeanFactory().registerSingleton("foo",
				new SequoiadbPersistentEntityIndexCreator(new SequoiadbMappingContext(), factory));
		applicationContext.refresh();

		GenericApplicationContext spy = spy(applicationContext);

		SequoiadbTemplate sequoiadbTemplate = new SequoiadbTemplate(factory, converter);
		sequoiadbTemplate.setApplicationContext(spy);

		verify(spy, times(1)).addApplicationListener(argThat(new ArgumentMatcher<SequoiadbPersistentEntityIndexCreator>() {

			@Override
			public boolean matches(Object argument) {

				if (!(argument instanceof SequoiadbPersistentEntityIndexCreator)) {
					return false;
				}

				return ((SequoiadbPersistentEntityIndexCreator) argument).isIndexCreatorFor(mappingContext);
			}
		}));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	public void findAllAndRemoveShouldRetrieveMatchingDocumentsPriorToRemoval() {

		BasicQuery query = new BasicQuery("{'foo':'bar'}");
		template.findAllAndRemove(query, VersionedEntity.class);
		verify(collection, times(1)).find(Matchers.eq(query.getQueryObject()));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	public void findAllAndRemoveShouldRemoveDocumentsReturedByFindQuery() {

		Mockito.when(cursor.hasNext()).thenReturn(true).thenReturn(true).thenReturn(false);
		Mockito.when(cursor.next()).thenReturn(new BasicBSONObject("_id", Integer.valueOf(0)))
				.thenReturn(new BasicBSONObject("_id", Integer.valueOf(1)));

		ArgumentCaptor<BSONObject> queryCaptor = ArgumentCaptor.forClass(BSONObject.class);
		BasicQuery query = new BasicQuery("{'foo':'bar'}");
		template.findAllAndRemove(query, VersionedEntity.class);

		verify(collection, times(1)).remove(queryCaptor.capture());

		BSONObject idField = DBObjectTestUtils.getAsDBObject(queryCaptor.getValue(), "_id");
		assertThat((Object[]) idField.get("$in"), is(new Object[] { Integer.valueOf(0), Integer.valueOf(1) }));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	public void findAllAndRemoveShouldNotTriggerRemoveIfFindResultIsEmpty() {

		template.findAllAndRemove(new BasicQuery("{'foo':'bar'}"), VersionedEntity.class);
		verify(collection, never()).remove(Mockito.any(BSONObject.class));
	}

	/**
	 * @see DATA_JIRA-948
	 */
	@Test
	public void sortShouldBeTakenAsIsWhenExecutingQueryWithoutSpecificTypeInformation() {

		Query query = Query.query(Criteria.where("foo").is("bar")).with(new Sort("foo"));
		template.executeQuery(query, "collection1", new DocumentCallbackHandler() {

			@Override
			public void processDocument(BSONObject dbObject) throws BaseException, DataAccessException {
			}
		});

		ArgumentCaptor<BSONObject> captor = ArgumentCaptor.forClass(BSONObject.class);
		verify(cursor, times(1)).sort(captor.capture());
		assertThat(captor.getValue(), equalTo(new BasicBSONObjectBuilder().add("foo", 1).get()));
	}

	class AutogenerateableId {

		@Id BigInteger id;
	}

	class NotAutogenerateableId {

		@Id Integer id;

		public Pattern getId() {
			return Pattern.compile(".");
		}
	}

	static class VersionedEntity {

		@Id Integer id;
		@Version Integer version;
	}

	enum MyConverter implements Converter<AutogenerateableId, String> {

		INSTANCE;

		public String convert(AutogenerateableId source) {
			return source.toString();
		}
	}

	class Wrapper {

		AutogenerateableId foo;
	}

	/**
	 * Mocks out the {@link SequoiadbTemplate#getDb()} method to return the {@link DB} mock instead of executing the actual
	 * behaviour.
	 * 
	 * @return
	 */
	private SequoiadbTemplate mockOutGetDb() {

		SequoiadbTemplate template = spy(this.template);
		stub(template.getDb()).toReturn(db);
		return template;
	}

	/* (non-Javadoc)
	  * @see org.springframework.data.sequoiadb.core.core.SequoiadbOperationsUnitTests#getOperations()
	  */
	@Override
	protected SequoiadbOperations getOperationsForExceptionHandling() {
		SequoiadbTemplate template = spy(this.template);
		stub(template.getDb()).toThrow(new BaseException(SDBError.SDB_SYS.getErrorCode(), "Error!"));
		return template;
	}

	/* (non-Javadoc)
	  * @see org.springframework.data.sequoiadb.core.core.SequoiadbOperationsUnitTests#getOperations()
	  */
	@Override
	protected SequoiadbOperations getOperations() {
		return this.template;
	}
}
