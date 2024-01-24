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
package org.springframework.data.mongodb.core.query;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.bson.types.BSONTimestamp;
import org.junit.Test;
import org.springframework.data.mongodb.InvalidMongoDbApiUsageException;

import org.springframework.data.mongodb.assist.BasicDBList;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.BasicDBObjectBuilder;
import org.springframework.data.mongodb.assist.DBObject;

import java.util.Date;

/**
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public class CriteriaTests {

	@Test
	public void testFuncOperators() {
	    // $abs
		Criteria criteria = new Criteria("a").abs().et(1);
		String actualStr = criteria.getCriteriaObject().toString();
		String expectStr = "{ \"a\" : { \"$abs\" : 1 , \"$et\" : 1 } }";
		System.out.println(actualStr);
		assertEquals(expectStr, actualStr);

		// $ceiling
        criteria = new Criteria("a").is(1).and("b").ceiling().et(10);
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : 1 , \"b\" : { \"$ceiling\" : 1 , \"$et\" : 10 } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // $floor $mod $add
        criteria = new Criteria("a").floor().mod(10).add(100).gt(1000);
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$floor\" : 1 , \"$mod\" : 10 , \"$add\" : 100 , \"$gt\" : 1000 } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

		// $subtract $multiply $divide $substr $strlen $lower
        criteria = new Criteria("a").subtract(10).multiply(10).divide(1.2).substr(0,5).strlen().lower().gt(1000);
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$subtract\" : 10 , \"$multiply\" : 10 , \"$divide\" : 1.2 , \"$substr\" : [ 0 , 5 ] , \"$strlen\" : 1 , \"$lower\" : 1 , \"$gt\" : 1000 } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // $upper $ltrim $rtrim $trim $cast $size $type $slice
        criteria = new Criteria("a").upper().ltrim().rtrim().trim().cast(1).size().type(2).slice(0, 10).gt(1000);
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$upper\" : 1 , \"$ltrim\" : 1 , \"$rtrim\" : 1 , \"$trim\" : 1 , \"$cast\" : 1 , \"$size\" : 1 , \"$type\" : 2 , \"$slice\" : [ 0 , 10 ] , \"$gt\" : 1000 } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);
	}

    // compare
    @Test
    public void testCompareOperators() {
        Criteria criteria = new Criteria("a").gt("a")
                .and("b").gte("b")
                .and("c").lt("c")
                .and("d").lte("d")
                .and("e").ne("e")
                .and("f").et("f")
                .and("g").in("g")
                .and("gg").in("gg1", "gg2","gg3")
                .and("h").nin("h")
                .and("hh").nin("hh1", "hh2", "hh3");
        String actualStr = criteria.getCriteriaObject().toString();
        String expectStr = "{ \"a\" : { \"$gt\" : \"a\" } , \"b\" : { \"$gte\" : \"b\" } , \"c\" : { \"$lt\" : \"c\" } , \"d\" : { \"$lte\" : \"d\" } , \"e\" : { \"$ne\" : \"e\" } , \"f\" : { \"$et\" : \"f\" } , \"g\" : { \"$in\" : [ \"g\" ] } , \"gg\" : { \"$in\" : [ \"gg1\" , \"gg2\" , \"gg3\" ] } , \"h\" : { \"$nin\" : [ \"h\" ] } , \"hh\" : { \"$nin\" : [ \"hh1\" , \"hh2\" , \"hh3\" ] } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);
    }

    @Test
    public void testLogicalOperators()
    {
        // or
        Criteria criteria =
                new Criteria().orOperator(Criteria.where("age").is(20), Criteria.where("price").lt(10)).and("name").is("Tom");
        String actualStr = criteria.getCriteriaObject().toString();
        String expectStr = "{ \"$or\" : [ { \"age\" : 20 } , { \"price\" : { \"$lt\" : 10 } } ] , \"name\" : \"Tom\" }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // and
        criteria =
                new Criteria("name").is("Tom").andOperator(Criteria.where("age").is(20), Criteria.where("price").lt(10));
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"name\" : \"Tom\" , \"$and\" : [ { \"age\" : 20 } , { \"price\" : { \"$lt\" : 10 } } ] }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // not
        // case 1:
        Date date1 = new Date(2010 -1900, 0,1);
        Date date2 = new Date(2020 -1900, 0,1);
        BSONTimestamp time1 = new BSONTimestamp((int)(date1.getTime() / 1000), (int)(date1.getTime() % 1000) * 1000);
        BSONTimestamp time2 = new BSONTimestamp((int)(date2.getTime() / 1000), (int)(date2.getTime() % 1000) * 1000);
        criteria = new Criteria("name").is("Tom")
                        .notOperator(Criteria.where("date").gt(date1).lt(date2),
                                Criteria.where("time").gt(time1).lt(time2));
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"name\" : \"Tom\" , \"$not\" : [ { \"date\" : { \"$gt\" : { \"$date\" : \"2010-01-01\" } , \"$lt\" : { \"$date\" : \"2020-01-01\" } } } , { \"time\" : { \"$gt\" : { \"$ts\" : 1262275200 , \"$inc\" : 0 } , \"$lt\" : { \"$ts\" : 1577808000 , \"$inc\" : 0 } } } ] }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);
        // case 2:
        criteria = new Criteria()
                .notOperator(new Criteria().andOperator(Criteria.where("date").gt(date1), Criteria.where("date").lt(date2)),
                        Criteria.where("time").gt(time1).lt(time2));
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"$not\" : [ { \"$and\" : [ { \"date\" : { \"$gt\" : { \"$date\" : \"2010-01-01\" } } } , { \"date\" : { \"$lt\" : { \"$date\" : \"2020-01-01\" } } } ] } , { \"time\" : { \"$gt\" : { \"$ts\" : 1262275200 , \"$inc\" : 0 } , \"$lt\" : { \"$ts\" : 1577808000 , \"$inc\" : 0 } } } ] }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // case 3:
        BasicDBObject notObj = new BasicDBObject();
        BasicDBList notList = new BasicDBList();
        BasicDBObject andObj = new BasicDBObject();
        BasicDBList andList = new BasicDBList();
        andList.put("0", new BasicDBObject("date", new BasicDBObject("$gt", date1)));
        andList.put("1", new BasicDBObject("date", new BasicDBObject("$lt", date2)));
        andObj.put("$and", andList);
        BasicDBObject obj = new BasicDBObject();
        obj.append("time", new BasicDBObject("$gt", time1).append("$lt", time2));
        notList.put("0", andObj);
        notList.put("1", obj);
        notObj.append("$not", notList);
        expectStr = notObj.toString();
        System.out.println(expectStr);
        assertEquals(expectStr, actualStr);

    }

    // elements
    @Test
    public void testElementsOperators()
    {
        Criteria criteria = new Criteria("a").exists(1).and("b").exists(0)
                .and("c").isNull(1).and("d").isNull(0)
                .and("e").field("ee").and("f").field("$gt", "ff").field("$lt", "fff");

        String actualStr = criteria.getCriteriaObject().toString();
        String expectStr = "{ \"a\" : { \"$exists\" : 1 } , \"b\" : { \"$exists\" : 0 } , \"c\" : { \"$isnull\" : 1 } , \"d\" : { \"$isnull\" : 0 } , \"e\" : { \"$field\" : \"ee\" } , \"f\" : { \"$gt\" : { \"$field\" : \"ff\" } , \"$lt\" : { \"$field\" : \"fff\" } } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);
    }

    @Test
    public void testOperatingOperators()
    {
        // $mod
        Criteria criteria = new Criteria("a").mod(100,10);
        String actualStr = criteria.getCriteriaObject().toString();
        String expectStr = "{ \"a\" : { \"$mod\" : [ 100 , 10 ] } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // $regex
        criteria = new Criteria("a").regex("dh.*fj","i");
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$regex\" : \"dh.*fj\" , \"$options\" : \"i\" } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);
    }

    @Test
    public void testArrayOperators()
    {
        // $all
        Criteria criteria = new Criteria("a").all("a", "b", "c");
        String actualStr = criteria.getCriteriaObject().toString();
        String expectStr = "{ \"a\" : { \"$all\" : [ \"a\" , \"b\" , \"c\" ] } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // $elemMatch
        criteria = new Criteria("a").elemMatch(Criteria.where("name").is("Jack").and("phone").is("123"));
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$elemMatch\" : { \"name\" : \"Jack\" , \"phone\" : \"123\" } } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // $expand
        criteria = new Criteria("a").expand();
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$expand\" : 1 } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);

        // $returnMatch
        criteria = new Criteria("a").returnMatch(0,10).and("b").returnMatch(2);
        actualStr = criteria.getCriteriaObject().toString();
        expectStr = "{ \"a\" : { \"$returnMatch\" : [ 0 , 10 ] } , \"b\" : { \"$returnMatch\" : 2 } }";
        System.out.println(actualStr);
        assertEquals(expectStr, actualStr);
    }

    /*

    @Test
    public void testOperators()
    {
        Criteria criteria = new Criteria();
        String actualStr = criteria.getCriteriaObject().toString();
//        String expectStr =
        System.out.println(actualStr);
//        assertEquals(expectStr, actualStr);
    }


    */

	@Test
	public void testCriteriaForSdb() {
		Criteria criteria = new Criteria() //
							.orOperator(Criteria.where("delete").is(true).and("_id").is(42)); //
		Criteria criteria2 = new Criteria().orOperator(Criteria.where("age").is(20), Criteria.where("price").lt(10));
		System.out.println("criteria is: " + criteria.getCriteriaObject());
		System.out.println("criteria2 is: " + criteria2.getCriteriaObject());
	}

	@Test
	public void testNotOperationForSdb() {
		Criteria criteria = new Criteria().notOperator(Criteria.where("age").is(20), Criteria.where("price").lt(10));
		System.out.println("criteria is: " + criteria.getCriteriaObject());
		assertEquals("{ \"$not\" : [ { \"age\" : 20 } , { \"price\" : { \"$lt\" : 10 } } ] }",
				criteria.getCriteriaObject().toString());
	}

	/////////////////////////////////////////////////////////////////
	@Test
	public void testSimpleCriteria() {
		Criteria c = new Criteria("name").is("Bubba");
		assertEquals("{ \"name\" : \"Bubba\" }", c.getCriteriaObject().toString());
	}

	@Test
	public void testNotEqualCriteria() {
		Criteria c = new Criteria("name").ne("Bubba");
		assertEquals("{ \"name\" : { \"$ne\" : \"Bubba\" } }", c.getCriteriaObject().toString());
	}

	@Test
	public void buildsIsNullCriteriaCorrectly() {

		DBObject reference = new BasicDBObject("name", null);

		Criteria criteria = new Criteria("name").is(null);
		assertThat(criteria.getCriteriaObject(), is(reference));
	}

	@Test
	public void testChainedCriteria() {
		Criteria c = new Criteria("name").is("Bubba").and("age").lt(21);
		assertEquals("{ \"name\" : \"Bubba\" , \"age\" : { \"$lt\" : 21 } }", c.getCriteriaObject().toString());
	}

	@Test(expected = InvalidMongoDbApiUsageException.class)
	public void testCriteriaWithMultipleConditionsForSameKey() {
		Criteria c = new Criteria("name").gte("M").and("name").ne("A");
		c.getCriteriaObject();
	}

	@Test
	public void equalIfCriteriaMatches() {

		Criteria left = new Criteria("name").is("Foo").and("lastname").is("Bar");
		Criteria right = new Criteria("name").is("Bar").and("lastname").is("Bar");

		assertThat(left, is(not(right)));
		assertThat(right, is(not(left)));
	}

	/**
	 * @see DATAMONGO-507
	 */
//	@Test(expected = IllegalArgumentException.class)
	@Test()
	public void shouldThrowExceptionWhenTryingToNegateAndOperation() {

	    System.out.println(
		new Criteria() //
				.andOperator(Criteria.where("delete").is(true).and("_id").is(42)).getCriteriaObject().toString()
        );
	}

	/**
	 * @see DATAMONGO-507
	 */
//	@Test(expected = IllegalArgumentException.class)
	@Test
	public void shouldThrowExceptionWhenTryingToNegateOrOperation() {

//		System.out.println(
//		new Criteria()
//				.not()
//				.orOperator(Criteria.where("delete").is(true).and("_id").is(42)));

		Criteria criteria = new Criteria()
//				.not()
				.orOperator(Criteria.where("delete").is(true).and("_id").is(42));
		System.out.println(criteria.getCriteriaObject().toString());
	}

	/**
	 * @see DATAMONGO-507
	 */
//	@Test(expected = IllegalArgumentException.class)
	@Test
	public void shouldThrowExceptionWhenTryingToNegateNorOperation() {
		new Criteria() //
				.notOperator(Criteria.where("delete").is(true).and("_id").is(42)); //
	}

	/**
	 * @see DATAMONGO-507
	 */
	@Test
	public void shouldNegateFollowingSimpleExpression() {

//		Criteria c = Criteria.where("age").not().gt(18).and("status").is("student");
        Criteria c = new Criteria().notOperator(Criteria.where("age").gt(18)).and("status").is("student");
		DBObject co = c.getCriteriaObject();

		assertThat(co, is(notNullValue()));
//		assertThat(co.toString(), is("{ \"age\" : { \"$not\" : { \"$gt\" : 18 } } , \"status\" : \"student\" }"));
		assertThat(co.toString(), is("{ \"$not\" : [ { \"age\" : { \"$gt\" : 18 } } ] , \"status\" : \"student\" }"));
	}

	/**
	 * @see DATAMONGO-1068
	 */
	@Test
	public void getCriteriaObjectShouldReturnEmptyDBOWhenNoCriteriaSpecified() {

		DBObject dbo = new Criteria().getCriteriaObject();

		assertThat(dbo, equalTo(new BasicDBObjectBuilder().get()));
	}

	/**
	 * @see DATAMONGO-1068
	 */
	@Test
	public void getCriteriaObjectShouldUseCritieraValuesWhenNoKeyIsPresent() {

		DBObject dbo = new Criteria().lt("foo").getCriteriaObject();

		assertThat(dbo, equalTo(new BasicDBObjectBuilder().add("$lt", "foo").get()));
	}

	/**
	 * @see DATAMONGO-1068
	 */
	@Test
	public void getCriteriaObjectShouldUseCritieraValuesWhenNoKeyIsPresentButMultipleCriteriasPresent() {

		DBObject dbo = new Criteria().lt("foo").gt("bar").getCriteriaObject();

		assertThat(dbo, equalTo(new BasicDBObjectBuilder().add("$lt", "foo").add("$gt", "bar").get()));
	}

	/**
	 * @see DATAMONGO-1068
	 */
	@Test
	public void getCriteriaObjectShouldRespectNotWhenNoKeyPresent() {

		//DBObject dbo = new Criteria().lt("foo").not().getCriteriaObject();
        DBObject dbo = new Criteria().notOperator(new Criteria("a").lt("foo")).getCriteriaObject();
        DBObject target = new BasicDBObject();
        BasicDBList list = new BasicDBList();
        list.put("0", new BasicDBObject("a", new BasicDBObject("$lt", "foo")));
        target.put("$not", list);
//		assertThat(dbo, equalTo(new BasicDBObjectBuilder().add("$not", new BasicDBObject("$lt", "foo")).get()));
		assertThat(dbo, equalTo(target));
	}
}
