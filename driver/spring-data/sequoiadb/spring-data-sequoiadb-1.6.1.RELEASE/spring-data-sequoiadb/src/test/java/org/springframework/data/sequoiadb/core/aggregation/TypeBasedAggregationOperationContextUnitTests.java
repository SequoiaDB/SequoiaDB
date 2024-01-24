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
package org.springframework.data.sequoiadb.core.aggregation;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.springframework.data.sequoiadb.core.aggregation.Aggregation.*;

import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.core.convert.converter.Converter;
import org.springframework.core.convert.support.GenericConversionService;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.PersistenceConstructor;
import org.springframework.data.mapping.model.MappingException;


import org.springframework.data.sequoiadb.core.aggregation.ExposedFields.ExposedField;
import org.springframework.data.sequoiadb.core.aggregation.ExposedFields.FieldReference;
import org.springframework.data.sequoiadb.core.convert.CustomConversions;
import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.QueryMapper;
import org.springframework.data.sequoiadb.core.mapping.Document;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.query.Criteria;

/**
 * Unit tests for {@link TypeBasedAggregationOperationContext}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class TypeBasedAggregationOperationContextUnitTests {

	SequoiadbMappingContext context;
	MappingSequoiadbConverter converter;
	QueryMapper mapper;

	@Mock DbRefResolver dbRefResolver;

	@Before
	public void setUp() {

		this.context = new SequoiadbMappingContext();
		this.converter = new MappingSequoiadbConverter(dbRefResolver, context);
		this.mapper = new QueryMapper(converter);
	}

	@Test
	public void findsSimpleReference() {
		assertThat(getContext(Foo.class).getReference("bar"), is(notNullValue()));
	}

	@Test(expected = MappingException.class)
	public void rejectsInvalidFieldReference() {
		getContext(Foo.class).getReference("foo");
	}

	/**
	 * @see DATA_JIRA-741
	 */
	@Test
	public void returnsReferencesToNestedFieldsCorrectly() {

		AggregationOperationContext context = getContext(Foo.class);

		Field field = Fields.field("bar.name");

		assertThat(context.getReference("bar.name"), is(notNullValue()));
		assertThat(context.getReference(field), is(notNullValue()));
		assertThat(context.getReference(field), is(context.getReference("bar.name")));
	}

	/**
	 * @see DATA_JIRA-806
	 */
	@Test
	public void aliasesIdFieldCorrectly() {

		AggregationOperationContext context = getContext(Foo.class);
		assertThat(context.getReference("id"), is(new FieldReference(new ExposedField(Fields.field("id", "_id"), true))));
	}

	/**
	 * @see DATA_JIRA-912
	 */
	@Test
	public void shouldUseCustomConversionIfPresentAndConversionIsRequiredInFirstStage() {

		CustomConversions customConversions = customAgeConversions();
		converter.setCustomConversions(customConversions);
		customConversions.registerConvertersIn((GenericConversionService) converter.getConversionService());

		AggregationOperationContext context = getContext(FooPerson.class);

		MatchOperation matchStage = match(Criteria.where("age").is(new Age(10)));
		ProjectionOperation projectStage = project("age", "name");

		BSONObject agg = newAggregation(matchStage, projectStage).toDbObject("test", context);

		BSONObject age = getValue((BSONObject) getValue(getPipelineElementFromAggregationAt(agg, 0), "$match"), "age");
		assertThat(age, is((BSONObject) new BasicBSONObject("v", 10)));
	}

	/**
	 * @see DATA_JIRA-912
	 */
	@Test
	public void shouldUseCustomConversionIfPresentAndConversionIsRequiredInLaterStage() {

		CustomConversions customConversions = customAgeConversions();
		converter.setCustomConversions(customConversions);
		customConversions.registerConvertersIn((GenericConversionService) converter.getConversionService());

		AggregationOperationContext context = getContext(FooPerson.class);

		MatchOperation matchStage = match(Criteria.where("age").is(new Age(10)));
		ProjectionOperation projectStage = project("age", "name");

		BSONObject agg = newAggregation(projectStage, matchStage).toDbObject("test", context);

		BSONObject age = getValue((BSONObject) getValue(getPipelineElementFromAggregationAt(agg, 1), "$match"), "age");
		assertThat(age, is((BSONObject) new BasicBSONObject("v", 10)));
	}

	/**
	 * @see DATA_JIRA-960
	 */
	@Test
	public void rendersAggregationOptionsInTypedAggregationContextCorrectly() {

		AggregationOperationContext context = getContext(FooPerson.class);
		TypedAggregation<FooPerson> agg = newAggregation(FooPerson.class, project("name", "age")) //
				.withOptions(
						newAggregationOptions().allowDiskUse(true).explain(true).cursor(new BasicBSONObject("foo", 1)).build());

		BSONObject dbo = agg.toDbObject("person", context);

		BSONObject projection = getPipelineElementFromAggregationAt(dbo, 0);
		assertThat(projection.containsField("$project"), is(true));

		assertThat(projection.get("$project"), is((Object) new BasicBSONObject("name", 1).append("age", 1)));

		assertThat(dbo.get("allowDiskUse"), is((Object) true));
		assertThat(dbo.get("explain"), is((Object) true));
		assertThat(dbo.get("cursor"), is((Object) new BasicBSONObject("foo", 1)));
	}

	@Document(collection = "person")
	public static class FooPerson {

		final ObjectId id;
		final String name;
		final Age age;

		@PersistenceConstructor
		FooPerson(ObjectId id, String name, Age age) {
			this.id = id;
			this.name = name;
			this.age = age;
		}
	}

	public static class Age {

		final int value;

		Age(int value) {
			this.value = value;
		}
	}

	public CustomConversions customAgeConversions() {
		return new CustomConversions(Arrays.<Converter<?, ?>> asList(ageWriteConverter(), ageReadConverter()));
	}

	Converter<Age, BSONObject> ageWriteConverter() {
		return new Converter<Age, BSONObject>() {
			@Override
			public BSONObject convert(Age age) {
				return new BasicBSONObject("v", age.value);
			}
		};
	}

	Converter<BSONObject, Age> ageReadConverter() {
		return new Converter<BSONObject, Age>() {
			@Override
			public Age convert(BSONObject dbObject) {
				return new Age(((Integer) dbObject.get("v")));
			}
		};
	}

	@SuppressWarnings("unchecked")
	static BSONObject getPipelineElementFromAggregationAt(BSONObject agg, int index) {
		return ((List<BSONObject>) agg.get("pipeline")).get(index);
	}

	@SuppressWarnings("unchecked")
	static <T> T getValue(BSONObject o, String key) {
		return (T) o.get(key);
	}

	private TypeBasedAggregationOperationContext getContext(Class<?> type) {
		return new TypeBasedAggregationOperationContext(type, context, mapper);
	}

	static class Foo {

		@Id String id;
		Bar bar;
	}

	static class Bar {

		String name;
	}
}
