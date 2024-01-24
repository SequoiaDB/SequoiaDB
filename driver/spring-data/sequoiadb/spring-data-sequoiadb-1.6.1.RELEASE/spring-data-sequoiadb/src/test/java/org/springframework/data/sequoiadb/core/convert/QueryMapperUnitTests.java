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
import static org.springframework.data.sequoiadb.core.DBObjectTestUtils.*;
import static org.springframework.data.sequoiadb.core.query.Criteria.*;
import static org.springframework.data.sequoiadb.core.query.Query.*;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.annotation.Id;
import org.springframework.data.domain.Sort;
import org.springframework.data.domain.Sort.Direction;
import org.springframework.data.sequoiadb.SequoiadbFactory;



import org.springframework.data.sequoiadb.assist.BasicBSONObjectBuilder;
import org.springframework.data.sequoiadb.core.DBObjectTestUtils;
import org.springframework.data.sequoiadb.core.Person;
import org.springframework.data.sequoiadb.core.mapping.*;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.query.BasicQuery;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.Query;

/**
 * Unit tests for {@link QueryMapper}.
 * 




 */
@RunWith(MockitoJUnitRunner.class)
public class QueryMapperUnitTests {

	QueryMapper mapper;
	SequoiadbMappingContext context;
	MappingSequoiadbConverter converter;

	@Mock
	SequoiadbFactory factory;

	@Before
	public void setUp() {

		this.context = new SequoiadbMappingContext();

		this.converter = new MappingSequoiadbConverter(new DefaultDbRefResolver(factory), context);
		this.converter.afterPropertiesSet();

		this.mapper = new QueryMapper(converter);
	}

	@Test
	public void translatesIdPropertyIntoIdKey() {

		BSONObject query = new BasicBSONObject("foo", "value");
		SequoiadbPersistentEntity<?> entity = context.getPersistentEntity(Sample.class);

		BSONObject result = mapper.getMappedObject(query, entity);
		assertThat(result.get("_id"), is(notNullValue()));
		assertThat(result.get("foo"), is(nullValue()));
	}

	@Test
	public void convertsStringIntoObjectId() {

		BSONObject query = new BasicBSONObject("_id", new ObjectId().toString());
		BSONObject result = mapper.getMappedObject(query, context.getPersistentEntity(IdWrapper.class));
		assertThat(result.get("_id"), is(instanceOf(ObjectId.class)));
	}

	@Test
	public void handlesBigIntegerIdsCorrectly() {

		BSONObject dbObject = new BasicBSONObject("id", new BigInteger("1"));
		BSONObject result = mapper.getMappedObject(dbObject, context.getPersistentEntity(IdWrapper.class));
		assertThat(result.get("_id"), is((Object) "1"));
	}

	@Test
	public void handlesObjectIdCapableBigIntegerIdsCorrectly() {

		ObjectId id = new ObjectId();
		BSONObject dbObject = new BasicBSONObject("id", new BigInteger(id.toString(), 16));
		BSONObject result = mapper.getMappedObject(dbObject, context.getPersistentEntity(IdWrapper.class));
		assertThat(result.get("_id"), is((Object) id));
	}

	/**
	 * @see DATA_JIRA-278
	 */
	@Test
	public void translates$NeCorrectly() {

		Criteria criteria = where("foo").ne(new ObjectId().toString());

		BSONObject result = mapper.getMappedObject(criteria.getCriteriaObject(), context.getPersistentEntity(Sample.class));
		Object object = result.get("_id");
		assertThat(object, is(instanceOf(BSONObject.class)));
		BSONObject dbObject = (BSONObject) object;
		assertThat(dbObject.get("$ne"), is(instanceOf(ObjectId.class)));
	}

	/**
	 * @see DATA_JIRA-326
	 */
	@Test
	public void handlesEnumsCorrectly() {
		Query query = query(where("foo").is(Enum.INSTANCE));
		BSONObject result = mapper.getMappedObject(query.getQueryObject(), null);
		Object object = result.get("foo");
		assertThat(object, is(instanceOf(String.class)));
	}

	@Test
	public void handlesEnumsInNotEqualCorrectly() {
		Query query = query(where("foo").ne(Enum.INSTANCE));
		BSONObject result = mapper.getMappedObject(query.getQueryObject(), null);
		Object object = result.get("foo");
		assertThat(object, is(instanceOf(BSONObject.class)));

		Object ne = ((BSONObject) object).get("$ne");
		assertThat(ne, is(instanceOf(String.class)));
		assertThat(ne.toString(), is(Enum.INSTANCE.name()));
	}

	@Test
	public void handlesEnumsIn$InCorrectly() {

		Query query = query(where("foo").in(Enum.INSTANCE));
		BSONObject result = mapper.getMappedObject(query.getQueryObject(), null);

		Object object = result.get("foo");
		assertThat(object, is(instanceOf(BSONObject.class)));

		Object in = ((BSONObject) object).get("$in");
		assertThat(in, is(instanceOf(BasicBSONList.class)));

		BasicBSONList list = (BasicBSONList) in;
		assertThat(list.size(), is(1));
		assertThat(list.get(0), is(instanceOf(String.class)));
		assertThat(list.get(0).toString(), is(Enum.INSTANCE.name()));
	}

	/**
	 * @see DATA_JIRA-373
	 */
	@Test
	public void handlesNativelyBuiltQueryCorrectly() {

//		BSONObject query = new QueryBuilder().or(new BasicBSONObject("foo", "bar")).get();
//		mapper.getMappedObject(query, null);
	}

	/**
	 * @see DATA_JIRA-369
	 */
	@Test
	public void handlesAllPropertiesIfDBObject() {

		BSONObject query = new BasicBSONObject();
		query.put("foo", new BasicBSONObject("$in", Arrays.asList(1, 2)));
		query.put("bar", new Person());

		BSONObject result = mapper.getMappedObject(query, null);
		assertThat(result.get("bar"), is(notNullValue()));
	}

	/**
	 * @see DATA_JIRA-429
	 */
	@Test
	public void transformsArraysCorrectly() {

		Query query = new BasicQuery("{ 'tags' : { '$all' : [ 'green', 'orange']}}");

		BSONObject result = mapper.getMappedObject(query.getQueryObject(), null);
		assertThat(result, is(query.getQueryObject()));
	}

	@Test
	public void doesHandleNestedFieldsWithDefaultIdNames() {

		BasicBSONObject dbObject = new BasicBSONObject("id", new ObjectId().toString());
		dbObject.put("nested", new BasicBSONObject("id", new ObjectId().toString()));

		SequoiadbPersistentEntity<?> entity = context.getPersistentEntity(ClassWithDefaultId.class);

		BSONObject result = mapper.getMappedObject(dbObject, entity);
		assertThat(result.get("_id"), is(instanceOf(ObjectId.class)));
		assertThat(((BSONObject) result.get("nested")).get("_id"), is(instanceOf(ObjectId.class)));
	}

	/**
	 * @see DATA_JIRA-493
	 */
	@Test
	public void doesNotTranslateNonIdPropertiesFor$NeCriteria() {

		ObjectId accidentallyAnObjectId = new ObjectId();

		Query query = Query.query(Criteria.where("id").is("id_value").and("publishers")
				.ne(accidentallyAnObjectId.toString()));

		BSONObject dbObject = mapper.getMappedObject(query.getQueryObject(), context.getPersistentEntity(UserEntity.class));
		assertThat(dbObject.get("publishers"), is(instanceOf(BSONObject.class)));

		BSONObject publishers = (BSONObject) dbObject.get("publishers");
		assertThat(publishers.containsField("$ne"), is(true));
		assertThat(publishers.get("$ne"), is(instanceOf(String.class)));
	}

	/**
	 * @see DATA_JIRA-494
	 */
	@Test
	public void usesEntityMetadataInOr() {

		Query query = query(new Criteria().orOperator(where("foo").is("bar")));
		BSONObject result = mapper.getMappedObject(query.getQueryObject(), context.getPersistentEntity(Sample.class));

		assertThat(result.keySet(), hasSize(1));
		assertThat(result.keySet(), hasItem("$or"));

		BasicBSONList ors = getAsDBList(result, "$or");
		assertThat(ors, hasSize(1));
		BSONObject criterias = getAsDBObject(ors, 0);
		assertThat(criterias.keySet(), hasSize(1));
		assertThat(criterias.get("_id"), is(notNullValue()));
		assertThat(criterias.get("foo"), is(nullValue()));
	}

	@Test
	public void translatesPropertyReferenceCorrectly() {

		Query query = query(where("field").is(new CustomizedField()));
		BSONObject result = mapper
				.getMappedObject(query.getQueryObject(), context.getPersistentEntity(CustomizedField.class));

		assertThat(result.containsField("foo"), is(true));
		assertThat(result.keySet().size(), is(1));
	}

	@Test
	public void translatesNestedPropertyReferenceCorrectly() {

		Query query = query(where("field.field").is(new CustomizedField()));
		BSONObject result = mapper
				.getMappedObject(query.getQueryObject(), context.getPersistentEntity(CustomizedField.class));

		assertThat(result.containsField("foo.foo"), is(true));
		assertThat(result.keySet().size(), is(1));
	}

	@Test
	public void returnsOriginalKeyIfNoPropertyReference() {

		Query query = query(where("bar").is(new CustomizedField()));
		BSONObject result = mapper
				.getMappedObject(query.getQueryObject(), context.getPersistentEntity(CustomizedField.class));

		assertThat(result.containsField("bar"), is(true));
		assertThat(result.keySet().size(), is(1));
	}

	@Test
	public void convertsAssociationCorrectly() {

		Reference reference = new Reference();
		reference.id = 5L;

		Query query = query(where("reference").is(reference));
		BSONObject object = mapper.getMappedObject(query.getQueryObject(), context.getPersistentEntity(WithDBRef.class));

		Object referenceObject = object.get("reference");

		assertThat(referenceObject, is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
	}

	@Test
	public void convertsNestedAssociationCorrectly() {

		Reference reference = new Reference();
		reference.id = 5L;

		Query query = query(where("withDbRef.reference").is(reference));
		BSONObject object = mapper.getMappedObject(query.getQueryObject(),
				context.getPersistentEntity(WithDBRefWrapper.class));

		Object referenceObject = object.get("withDbRef.reference");

		assertThat(referenceObject, is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
	}

	@Test
	public void convertsInKeywordCorrectly() {

		Reference first = new Reference();
		first.id = 5L;

		Reference second = new Reference();
		second.id = 6L;

		Query query = query(where("reference").in(first, second));
		BSONObject result = mapper.getMappedObject(query.getQueryObject(), context.getPersistentEntity(WithDBRef.class));

		BSONObject reference = DBObjectTestUtils.getAsDBObject(result, "reference");

		BasicBSONList inClause = getAsDBList(reference, "$in");
		assertThat(inClause, hasSize(2));
		assertThat(inClause.get(0), is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
		assertThat(inClause.get(1), is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
	}

	/**
	 * @see DATA_JIRA-570
	 */
	@Test
	public void correctlyConvertsNullReference() {

		Query query = query(where("reference").is(null));
		BSONObject object = mapper.getMappedObject(query.getQueryObject(), context.getPersistentEntity(WithDBRef.class));

		assertThat(object.get("reference"), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-629
	 */
	@Test
	public void doesNotMapIdIfNoEntityMetadataAvailable() {

		String id = new ObjectId().toString();
		Query query = query(where("id").is(id));

		BSONObject object = mapper.getMappedObject(query.getQueryObject(), null);

		assertThat(object.containsField("id"), is(true));
		assertThat(object.get("id"), is((Object) id));
		assertThat(object.containsField("_id"), is(false));
	}

	/**
	 * @see DATA_JIRA-677
	 */
	@Test
	public void handleMapWithDBRefCorrectly() {

		BSONObject mapDbObject = new BasicBSONObject();
		mapDbObject.put("test", new org.springframework.data.sequoiadb.assist.DBRef(null, "test", "test"));
		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("mapWithDBRef", mapDbObject);

		BSONObject mapped = mapper.getMappedObject(dbObject, context.getPersistentEntity(WithMapDBRef.class));

		assertThat(mapped.containsField("mapWithDBRef"), is(true));
		assertThat(mapped.get("mapWithDBRef"), instanceOf(BasicBSONObject.class));
		assertThat(((BasicBSONObject) mapped.get("mapWithDBRef")).containsField("test"), is(true));
		assertThat(((BasicBSONObject) mapped.get("mapWithDBRef")).get("test"), instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class));
	}

	@Test
	public void convertsUnderscoreIdValueWithoutMetadata() {

		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("_id", new ObjectId().toString());

		BSONObject mapped = mapper.getMappedObject(dbObject, null);
		assertThat(mapped.containsField("_id"), is(true));
		assertThat(mapped.get("_id"), is(instanceOf(ObjectId.class)));
	}

	/**
	 * @see DATA_JIRA-705
	 */
	@Test
	public void convertsDBRefWithExistsQuery() {

		Query query = query(where("reference").exists(0));

		BasicSequoiadbPersistentEntity<?> entity = context.getPersistentEntity(WithDBRef.class);
		BSONObject mappedObject = mapper.getMappedObject(query.getQueryObject(), entity);

		BSONObject reference = getAsDBObject(mappedObject, "reference");
		assertThat(reference.containsField("$exists"), is(true));
		assertThat(reference.get("$exists"), is((Object) false));
	}

	/**
	 * @see DATA_JIRA-706
	 */
	@Test
	public void convertsNestedDBRefsCorrectly() {

		Reference reference = new Reference();
		reference.id = 5L;

		Query query = query(where("someString").is("foo").andOperator(where("reference").in(reference)));

		BasicSequoiadbPersistentEntity<?> entity = context.getPersistentEntity(WithDBRef.class);
		BSONObject mappedObject = mapper.getMappedObject(query.getQueryObject(), entity);

		assertThat(mappedObject.get("someString"), is((Object) "foo"));

		BasicBSONList andClause = getAsDBList(mappedObject, "$and");
		assertThat(andClause, hasSize(1));

		BasicBSONList inClause = getAsDBList(getAsDBObject(getAsDBObject(andClause, 0), "reference"), "$in");
		assertThat(inClause, hasSize(1));
		assertThat(inClause.get(0), is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
	}

	/**
	 * @see DATA_JIRA-752
	 */
	@Test
	public void mapsSimpleValuesStartingWith$Correctly() {

		Query query = query(where("myvalue").is("$334"));

		BSONObject result = mapper.getMappedObject(query.getQueryObject(), null);

		assertThat(result.keySet(), hasSize(1));
		assertThat(result.get("myvalue"), is((Object) "$334"));
	}

	/**
	 * @see DATA_JIRA-752
	 */
	@Test
	public void mapsKeywordAsSimpleValuesCorrectly() {

		Query query = query(where("myvalue").is("$center"));

		BSONObject result = mapper.getMappedObject(query.getQueryObject(), null);

		assertThat(result.keySet(), hasSize(1));
		assertThat(result.get("myvalue"), is((Object) "$center"));
	}

	/**
	 * @DATA_JIRA-805
	 */
	@Test
	public void shouldExcludeDBRefAssociation() {

		Query query = query(where("someString").is("foo"));
		query.fields().include("reference", 0);

		BasicSequoiadbPersistentEntity<?> entity = context.getPersistentEntity(WithDBRef.class);
		BSONObject queryResult = mapper.getMappedObject(query.getQueryObject(), entity);
		BSONObject fieldsResult = mapper.getMappedObject(query.getFieldsObject(), entity);

		assertThat(queryResult.get("someString"), is((Object) "foo"));
		assertThat(fieldsResult.get("reference"), is((Object) 0));
	}

	/**
	 * @see DATA_JIRA-686
	 */
	@Test
	public void queryMapperShouldNotChangeStateInGivenQueryObjectWhenIdConstrainedByInList() {

		BasicSequoiadbPersistentEntity<?> persistentEntity = context.getPersistentEntity(Sample.class);
		String idPropertyName = persistentEntity.getIdProperty().getName();
		BSONObject queryObject = query(where(idPropertyName).in("42")).getQueryObject();

		Object idValuesBefore = getAsDBObject(queryObject, idPropertyName).get("$in");
		mapper.getMappedObject(queryObject, persistentEntity);
		Object idValuesAfter = getAsDBObject(queryObject, idPropertyName).get("$in");

		assertThat(idValuesAfter, is(idValuesBefore));
	}

	/**
	 * @see DATA_JIRA-821
	 */
	@Test
	public void queryMapperShouldNotTryToMapDBRefListPropertyIfNestedInsideDBObjectWithinDBObject() {

		BSONObject queryObject = query(
				where("referenceList").is(new BasicBSONObject("$nested", new BasicBSONObject("$keys", 0L)))).getQueryObject();

		BSONObject mappedObject = mapper.getMappedObject(queryObject, context.getPersistentEntity(WithDBRefList.class));
		BSONObject referenceObject = getAsDBObject(mappedObject, "referenceList");
		BSONObject nestedObject = getAsDBObject(referenceObject, "$nested");

		assertThat(nestedObject, is((BSONObject) new BasicBSONObject("$keys", 0L)));
	}

	/**
	 * @see DATA_JIRA-821
	 */
	@Test
	public void queryMapperShouldNotTryToMapDBRefPropertyIfNestedInsideDBObjectWithinDBObject() {

		BSONObject queryObject = query(where("reference").is(new BasicBSONObject("$nested", new BasicBSONObject("$keys", 0L))))
				.getQueryObject();

		BSONObject mappedObject = mapper.getMappedObject(queryObject, context.getPersistentEntity(WithDBRef.class));
		BSONObject referenceObject = getAsDBObject(mappedObject, "reference");
		BSONObject nestedObject = getAsDBObject(referenceObject, "$nested");

		assertThat(nestedObject, is((BSONObject) new BasicBSONObject("$keys", 0L)));
	}

	/**
	 * @see DATA_JIRA-821
	 */
	@Test
	public void queryMapperShouldMapDBRefPropertyIfNestedInDBObject() {

		Reference sample = new Reference();
		sample.id = 321L;
		BSONObject queryObject = query(where("reference").is(new BasicBSONObject("$in", Arrays.asList(sample))))
				.getQueryObject();

		BSONObject mappedObject = mapper.getMappedObject(queryObject, context.getPersistentEntity(WithDBRef.class));

		BSONObject referenceObject = getAsDBObject(mappedObject, "reference");
		BasicBSONList inObject = getAsDBList(referenceObject, "$in");

		assertThat(inObject.get(0), is(instanceOf(org.springframework.data.sequoiadb.assist.DBRef.class)));
	}

	/**
	 * @see DATA_JIRA-773
	 */
	@Test
	public void queryMapperShouldBeAbleToProcessQueriesThatIncludeDbRefFields() {

		BasicSequoiadbPersistentEntity<?> persistentEntity = context.getPersistentEntity(WithDBRef.class);

		Query qry = query(where("someString").is("abc"));
		qry.fields().include("reference", 1);

		BSONObject mappedFields = mapper.getMappedObject(qry.getFieldsObject(), persistentEntity);
		assertThat(mappedFields, is(notNullValue()));
	}

	/**
	 * @see DATA_JIRA-893
	 */
	@Test
	public void classInformationShouldNotBePresentInDBObjectUsedInFinderMethods() {

		EmbeddedClass embedded = new EmbeddedClass();
		embedded.id = "1";

		EmbeddedClass embedded2 = new EmbeddedClass();
		embedded2.id = "2";
		Query query = query(where("embedded").in(Arrays.asList(embedded, embedded2)));

		BSONObject dbo = mapper.getMappedObject(query.getQueryObject(), context.getPersistentEntity(Foo.class));
		assertThat(dbo.toString(), equalTo("{ \"embedded\" : { \"$in\" : [ { \"_id\" : \"1\"} , { \"_id\" : \"2\"}]}}"));
	}

	/**
	 * @see DATA_JIRA-647
	 */
	@Test
	public void customizedFieldNameShouldBeMappedCorrectlyWhenApplyingSort() {

		Query query = query(where("field").is("bar")).with(new Sort(Direction.DESC, "field"));
		BSONObject dbo = mapper.getMappedObject(query.getSortObject(), context.getPersistentEntity(CustomizedField.class));
		assertThat(dbo, equalTo(new BasicBSONObjectBuilder().add("foo", -1).get()));
	}

	/**
	 * @see DATA_JIRA-973
	 */
	@Test
	public void getMappedFieldsAppendsTextScoreFieldProperlyCorrectlyWhenNotPresent() {

		Query query = new Query();

		BSONObject dbo = mapper.getMappedFields(query.getFieldsObject(),
				context.getPersistentEntity(WithTextScoreProperty.class));

		assertThat(dbo, equalTo(new BasicBSONObjectBuilder().add("score", new BasicBSONObject("$meta", "textScore")).get()));
	}

	/**
	 * @see DATA_JIRA-973
	 */
	@Test
	public void getMappedFieldsReplacesTextScoreFieldProperlyCorrectlyWhenPresent() {

		Query query = new Query();
		query.fields().include("textScore",1);

		BSONObject dbo = mapper.getMappedFields(query.getFieldsObject(),
				context.getPersistentEntity(WithTextScoreProperty.class));

		assertThat(dbo, equalTo(new BasicBSONObjectBuilder().add("score", new BasicBSONObject("$meta", "textScore")).get()));
	}

	/**
	 * @see DATA_JIRA-973
	 */
	@Test
	public void getMappedSortAppendsTextScoreProperlyWhenSortedByScore() {

		Query query = new Query().with(new Sort("textScore"));

		BSONObject dbo = mapper
				.getMappedSort(query.getSortObject(), context.getPersistentEntity(WithTextScoreProperty.class));

		assertThat(dbo, equalTo(new BasicBSONObjectBuilder().add("score", new BasicBSONObject("$meta", "textScore")).get()));
	}

	/**
	 * @see DATA_JIRA-973
	 */
	@Test
	public void getMappedSortIgnoresTextScoreWhenNotSortedByScore() {

		Query query = new Query().with(new Sort("id"));

		BSONObject dbo = mapper
				.getMappedSort(query.getSortObject(), context.getPersistentEntity(WithTextScoreProperty.class));

		assertThat(dbo, equalTo(new BasicBSONObjectBuilder().add("_id", 1).get()));
	}

	/**
	 * @see DATA_JIRA-1070
	 */
	@Test
	public void mapsIdReferenceToDBRefCorrectly() {

		ObjectId id = new ObjectId();

		BSONObject query = new BasicBSONObject("reference.id", new org.springframework.data.sequoiadb.assist.DBRef(null, "reference", id.toString()));
		BSONObject result = mapper.getMappedObject(query, context.getPersistentEntity(WithDBRef.class));

		assertThat(result.containsField("reference"), is(true));
		org.springframework.data.sequoiadb.assist.DBRef reference = getTypedValue(result, "reference", org.springframework.data.sequoiadb.assist.DBRef.class);
		assertThat(reference.getId(), is(instanceOf(ObjectId.class)));
	}

	@Document
	public class Foo {
		@Id private ObjectId id;
		EmbeddedClass embedded;
	}

	public class EmbeddedClass {
		public String id;
	}

	class IdWrapper {
		Object id;
	}

	class ClassWithEmbedded {
		@Id String id;
		Sample sample;
	}

	class ClassWithDefaultId {

		String id;
		ClassWithDefaultId nested;
	}

	class Sample {

		@Id private String foo;
	}

	class BigIntegerId {

		@Id private BigInteger id;
	}

	enum Enum {
		INSTANCE;
	}

	class UserEntity {
		String id;
		List<String> publishers = new ArrayList<String>();
	}

	class CustomizedField {

		@Field("foo") CustomizedField field;
	}

	class WithDBRef {

		String someString;
		@DBRef Reference reference;
	}

	class WithDBRefList {

		String someString;
		@DBRef List<Reference> referenceList;
	}

	class Reference {

		Long id;
	}

	class WithDBRefWrapper {

		WithDBRef withDbRef;
	}

	class WithMapDBRef {

		@DBRef Map<String, Sample> mapWithDBRef;
	}

	class WithTextScoreProperty {

		@Id String id;
		@TextScore @Field("score") Float textScore;
	}
}
