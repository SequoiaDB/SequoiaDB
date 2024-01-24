/*
 * Copyright 2010-2013 the original author or authors.
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
package org.springframework.data.mongodb.core.query;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.springframework.data.mongodb.core.query.Criteria.*;
import static org.springframework.data.mongodb.core.query.Query.*;

import java.util.Arrays;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.springframework.data.domain.Sort;
import org.springframework.data.domain.Sort.Direction;
import org.springframework.data.mongodb.InvalidMongoDbApiUsageException;
import org.springframework.data.mongodb.core.SpecialDoc;

/**
 * Unit tests for {@link Query}.
 * 
 * @author Thomas Risberg
 * @author Oliver Gierke
 * @author Patryk Wasik
 * @author Thomas Darimont
 */
public class QueryTests {

	@Rule public ExpectedException exception = ExpectedException.none();

	@Test
	public void testQueryWithAnd() {
		Query q = new Query(Criteria.where("name").is("Thomas").
				andOperator(Criteria.where("age").gte(10), Criteria.where("age").lte(100)));
		Query q2 = new Query(Criteria.where("age").gte(10).lte(100));
		System.out.println("q is: " + q.getQueryObject().toString());
		System.out.println("q2 is: " + q2.getQueryObject().toString());


//		String expected = "{ \"name\" : \"Thomas\" , \"age\" : { \"$lt\" : 80 } }";
//		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testSimpleQuery() {
		Query q = new Query(where("name").is("Thomas").and("age").lt(80));
		String expected = "{ \"name\" : \"Thomas\" , \"age\" : { \"$lt\" : 80 } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testQueryWithNot() {
//		Query q = new Query(where("name").is("Thomas").and("age").not().mod(10, 0));
		Query q = new Query(where("name").is("Thomas").notOperator(where("age").mod(10, 0)));
//		String expected = "{ \"name\" : \"Thomas\" , \"age\" : { \"$not\" : { \"$mod\" : [ 10 , 0 ] } } }";
		String expected = "{ \"name\" : \"Thomas\" , \"$not\" : [ { \"age\" : { \"$mod\" : [ 10 , 0 ] } } ] }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testInvalidQueryWithNotIs() {
//			new Query(where("name").not().is("Thomas"));
        System.out.println(new Query(new Criteria().notOperator(where("name").is("Thomas"))).getQueryObject().toString());
	}

	@Test
	public void testOrQuery() {
		Query q = new Query(new Criteria().orOperator(where("name").is("Sven").and("age").lt(50), where("age").lt(50),
				where("name").is("Thomas")));
		String expected = "{ \"$or\" : [ { \"name\" : \"Sven\" , \"age\" : { \"$lt\" : 50 } } , { \"age\" : { \"$lt\" : 50 } } , { \"name\" : \"Thomas\" } ] }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testAndQuery() {
		Query q = new Query(new Criteria().andOperator(where("name").is("Sven"), where("age").lt(50)));
		String expected = "{ \"$and\" : [ { \"name\" : \"Sven\" } , { \"age\" : { \"$lt\" : 50 } } ] }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testNotQuery() {
		Query q = new Query(new Criteria().notOperator(where("name").is("Sven"), where("age").lt(50),
				where("name").is("Thomas")));
		String expected = "{ \"$not\" : [ { \"name\" : \"Sven\" } , { \"age\" : { \"$lt\" : 50 } } , { \"name\" : \"Thomas\" } ] }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testQueryWithLimit() {
//		Query q = new Query(where("name").gte("M").lte("T").and("age").not().gt(22));
		Query q = new Query(where("name").gte("M").lte("T").notOperator(where("age").gt(22)));
		q.limit(50);
//		String expected = "{ \"name\" : { \"$gte\" : \"M\" , \"$lte\" : \"T\" } , \"age\" : { \"$not\" : { \"$gt\" : 22 } } }";
		String expected = "{ \"name\" : { \"$gte\" : \"M\" , \"$lte\" : \"T\" } , \"$not\" : [ { \"age\" : { \"$gt\" : 22 } } ] }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
		Assert.assertEquals(50, q.getLimit());
	}

	@Test
	public void testQueryWithFieldsAndSlice() {
//		Query q = new Query(where("name").gte("M").lte("T").and("age").not().gt(22));
		Query q = new Query(where("name").gte("M").lte("T").notOperator(where("age").gt(22)));
		q.fields().include("address", 0).include("name", 1);

//		String expected = "{ \"name\" : { \"$gte\" : \"M\" , \"$lte\" : \"T\" } , \"age\" : { \"$not\" : { \"$gt\" : 22 } } }";
		String expected = "{ \"name\" : { \"$gte\" : \"M\" , \"$lte\" : \"T\" } , \"$not\" : [ { \"age\" : { \"$gt\" : 22 } } ] }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
//		String expectedFields = "{ \"address\" : 0 , \"name\" : 1 , \"orders\" : { \"$slice\" : 10 } }";
		String expectedFields = "{ \"address\" : { \"$include\" : 0 } , \"name\" : { \"$include\" : 1 } }";
		Assert.assertEquals(expectedFields, q.getFieldsObject().toString());
	}

	/**
	 * @see DATAMONGO-652
	 */
	@Test
	public void testQueryWithFieldsElemMatchAndPositionalOperator() {

//		Query query = query(where("name").gte("M").lte("T").and("age").not().gt(22));
		Query query = query(where("name").gte("M").lte("T").notOperator(where("age").gt(22)));
//		query.fields().elemMatch("products", where("name").is("milk")).position("comments", 2);
		query.fields().elemMatch("products", where("name").is("milk"));

//		String expected = "{ \"name\" : { \"$gte\" : \"M\" , \"$lte\" : \"T\" } , \"age\" : { \"$not\" : { \"$gt\" : 22 } } }";
		String expected = "{ \"name\" : { \"$gte\" : \"M\" , \"$lte\" : \"T\" } , \"$not\" : [ { \"age\" : { \"$gt\" : 22 } } ] }";
		assertThat(query.getQueryObject().toString(), is(expected));
//		String expectedFields = "{ \"products\" : { \"$elemMatch\" : { \"name\" : \"milk\" } } , \"comments.$\" : 2 }";
		String expectedFields = "{ \"products\" : { \"$elemMatch\" : { \"name\" : \"milk\" } } }";
		assertThat(query.getFieldsObject().toString(), is(expectedFields));
	}

	@Test
	public void testSimpleQueryWithChainedCriteria() {
		Query q = new Query(where("name").is("Thomas").and("age").lt(80));
		String expected = "{ \"name\" : \"Thomas\" , \"age\" : { \"$lt\" : 80 } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testComplexQueryWithMultipleChainedCriteria() {
		Query q = new Query(where("name").regex("^T.*").and("age").gt(20).lt(80).and("city")
				.in("Stockholm", "London", "New York"));
		String expected = "{ \"name\" : { \"$regex\" : \"^T.*\" } , \"age\" : { \"$gt\" : 20 , \"$lt\" : 80 } , "
				+ "\"city\" : { \"$in\" : [ \"Stockholm\" , \"London\" , \"New York\" ] } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testAddCriteriaWithComplexQueryWithMultipleChainedCriteria() {
		Query q1 = new Query(where("name").regex("^T.*").and("age").gt(20).lt(80).and("city")
				.in("Stockholm", "London", "New York"));
		Query q2 = new Query(where("name").regex("^T.*").and("age").gt(20).lt(80)).addCriteria(where("city").in(
				"Stockholm", "London", "New York"));
		Assert.assertEquals(q1.getQueryObject().toString(), q2.getQueryObject().toString());
		Query q3 = new Query(where("name").regex("^T.*")).addCriteria(where("age").gt(20).lt(80)).addCriteria(
				where("city").in("Stockholm", "London", "New York"));
		Assert.assertEquals(q1.getQueryObject().toString(), q3.getQueryObject().toString());
	}

	@Test
	public void testQueryWithElemMatch() {
		Query q = new Query(where("openingHours").elemMatch(where("dayOfWeek").is("Monday").and("open").lte("1800")));
		String expected = "{ \"openingHours\" : { \"$elemMatch\" : { \"dayOfWeek\" : \"Monday\" , \"open\" : { \"$lte\" : \"1800\" } } } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testQueryWithIn() {
		Query q = new Query(where("state").in("NY", "NJ", "PA"));
		String expected = "{ \"state\" : { \"$in\" : [ \"NY\" , \"NJ\" , \"PA\" ] } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testQueryWithRegex() {
		Query q = new Query(where("name").regex("b.*"));
		String expected = "{ \"name\" : { \"$regex\" : \"b.*\" } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	@Test
	public void testQueryWithRegexAndOption() {
		Query q = new Query(where("name").regex("b.*", "i"));
		String expected = "{ \"name\" : { \"$regex\" : \"b.*\" , \"$options\" : \"i\" } }";
		Assert.assertEquals(expected, q.getQueryObject().toString());
	}

	/**
	 * @see DATAMONGO-538
	 */
	@Test
	public void addsSortCorrectly() {

		Query query = new Query().with(new Sort(Direction.DESC, "foo"));
		assertThat(query.getSortObject().toString(), is("{ \"foo\" : -1 }"));
	}

	@Test
	public void rejectsOrderWithIgnoreCase() {

		exception.expect(IllegalArgumentException.class);
		exception.expectMessage("foo");

		new Query().with(new Sort(new Sort.Order("foo").ignoreCase()));
	}

	/**
	 * @see DATAMONGO-709
	 */
	@Test
	@SuppressWarnings("unchecked")
	public void shouldReturnClassHierarchyOfRestrictedTypes() {

		Query query = new Query(where("name").is("foo")).restrict(SpecialDoc.class);
		assertThat(
				query.toString(),
				is("Query: { \"name\" : \"foo\", \"_$RESTRICTED_TYPES\" : [ { $java : class org.springframework.data.mongodb.core.SpecialDoc } ] }, Fields: null, Sort: null, Hint: {  }, Limit: -1, Skip: 0, Flags: 0"));
		assertThat(query.getRestrictedTypes(), is(notNullValue()));
		assertThat(query.getRestrictedTypes().size(), is(1));
		assertThat(query.getRestrictedTypes(), hasItems(Arrays.asList(SpecialDoc.class).toArray(new Class<?>[0])));
	}
}
