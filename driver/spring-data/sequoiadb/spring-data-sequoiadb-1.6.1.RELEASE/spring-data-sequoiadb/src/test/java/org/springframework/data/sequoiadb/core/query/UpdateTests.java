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
package org.springframework.data.sequoiadb.core.query;

import static org.hamcrest.CoreMatchers.*;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.Collections;
import java.util.Map;

import org.bson.BasicBSONObject;
import org.joda.time.DateTime;
import org.junit.Test;



/**
 * Test cases for {@link Update}.
 *
 */
public class UpdateTests {

    @Test
    public void sdbUpdateMultiOperator() {
        Update update = new Update().set("a",1)
                .inc("b",2)
                .addToSet("c",1,2,3)
                .pull("d",4)
                .pullAllBy("arr1",1,2,3)
                .pullAllBy("arr2",4,5,6)
                .pullAllBy("arr3", Arrays.asList(7,8,9))
                .replace("f", new org.bson.BasicBSONObject("name","sam"));
        System.out.println(update.getUpdateObject());
        assertThat(update.getUpdateObject().toString(),
                equalTo("{ \"$set\" : { \"a\" : 1 } , \"$inc\" : { \"b\" : 2 } , \"$addtoset\" : { \"c\" : [ 1 , 2 , 3 ] } , \"$pull\" : { \"d\" : 4 } , \"$pull_all_by\" : { \"arr1\" : [ 1 , 2 , 3 ] , \"arr2\" : [ 4 , 5 , 6 ] , \"arr3\" : [ 7 , 8 , 9 ] } , \"$replace\" : { \"f\" : { \"name\" : \"sam\" } } }"));
    }

    @Test
    public void sdbUpdateIncOperator() {
        Update update1 = new Update();
        update1.inc("age", 5);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$inc\" : { \"age\" : 5 } }"));

        Update update2 = new Update();
        update2.inc("age", 10).inc("ID", 1);
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(),equalTo("{ \"$inc\" : { \"age\" : 10 , \"ID\" : 1 } }"));
    }

    @Test
    public void sdbUpdateSetOperator() {
        Update update1 = new Update();
        update1.set("age", 5);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$set\" : { \"age\" : 5 } }"));

        Update update2 = new Update();
        update2.set("age", 10).set("ID", 10).set("obj", new BasicBSONObject().append("a",1));
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$set\" : { \"age\" : 10 , \"ID\" : 10 , \"obj\" : { \"a\" : 1 } } }"));
    }

    @Test
    public void sdbUpdateUnsetOperator() {
        Update update1 = new Update();
        update1.unset("age");
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$unset\" : { \"age\" : \"\" } }"));

        Update update2 = new Update();
        update2.unset("age").unset("ID").unset("obj");
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$unset\" : { \"age\" : \"\" , \"ID\" : \"\" , \"obj\" : \"\" } }"));
    }

    @Test
    public void sdbUpdateAddToSetOperator() {
        Update update1 = new Update();
        update1.addToSet("arr", 1,2,3,4);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$addtoset\" : { \"arr\" : [ 1 , 2 , 3 , 4 ] } }"));

        Update update2 = new Update();
        update2.addToSet("arr1", 1,2,3,4,5).addToSet("arr2", "1","2","3",4);
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$addtoset\" : { \"arr1\" : [ 1 , 2 , 3 , 4 , 5 ] , \"arr2\" : [ \"1\" , \"2\" , \"3\" , 4 ] } }"));
    }

    @Test
    public void sdbUpdatePopOperator() {
        Update update1 = new Update();
        update1.pop("arr", 2);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$pop\" : { \"arr\" : 2 } }"));

        Update update2 = new Update();
        update2.pop("arr1", 10).pop("arr2", -2);
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$pop\" : { \"arr1\" : 10 , \"arr2\" : -2 } }"));
    }

    @Test
    public void sdbUpdatePullOperator() {
        Update update1 = new Update();
        update1.pull("arr",2);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$pull\" : { \"arr\" : 2 } }"));

        Update update2 = new Update();
        update2.pull("arr", 10).pull("name","Mile");
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$pull\" : { \"arr\" : 10 , \"name\" : \"Mile\" } }"));
    }

    @Test
    public void sdbUpdatePullAllOperator() {
        Update update1 = new Update();
        update1.pullAll("arr", 1,2,3,4);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$pull_all\" : { \"arr\" : [ 1 , 2 , 3 , 4 ] } }"));

        Update update2 = new Update();
        update2.pullAll("arr1", 1,2,3,4,5).pullAll("arr2","1","2", new BasicBSONObject().append("a",1));
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$pull_all\" : { \"arr1\" : [ 1 , 2 , 3 , 4 , 5 ] , \"arr2\" : [ \"1\" , \"2\" , { \"a\" : 1 } ] } }"));
    }

    @Test
    public void sdbUpdatePullByOperator() {
        Update update1 = new Update();
        update1.pullBy("arr",2);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$pull_by\" : { \"arr\" : 2 } }"));

        Update update2 = new Update();
        update2.pullBy("arr", 10).pullBy("name","Mile");
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$pull_by\" : { \"arr\" : 10 , \"name\" : \"Mile\" } }"));
    }

    @Test
    public void sdbUpdatePullAllByOperator() {
        Update update1 = new Update();
        update1.pullAllBy("arr", 1,2,3,4);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$pull_all_by\" : { \"arr\" : [ 1 , 2 , 3 , 4 ] } }"));

        Update update2 = new Update();
        update2.pullAllBy("arr1", 1,2,3,4,5).pullAllBy("arr2","1","2", new BasicBSONObject().append("a",1));
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$pull_all_by\" : { \"arr1\" : [ 1 , 2 , 3 , 4 , 5 ] , \"arr2\" : [ \"1\" , \"2\" , { \"a\" : 1 } ] } }"));
    }

    @Test
    public void sdbUpdatePushOperator() {
        Update update1 = new Update();
        update1.push("arr",2);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$push\" : { \"arr\" : 2 } }"));

        Update update2 = new Update();
        update2.push("arr", 10).push("name","Mile");
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$push\" : { \"arr\" : 10 , \"name\" : \"Mile\" } }"));
    }

    @Test
    public void sdbUpdatePushAllOperator() {
        Update update1 = new Update();
        update1.pushAll("arr", 1,2,3,4);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$push_all\" : { \"arr\" : [ 1 , 2 , 3 , 4 ] } }"));

        Update update2 = new Update();
        update2.pushAll("arr1", 1,2,3,4,5).pushAll("arr2","1","2", new BasicBSONObject().append("a",1));
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$push_all\" : { \"arr1\" : [ 1 , 2 , 3 , 4 , 5 ] , \"arr2\" : [ \"1\" , \"2\" , { \"a\" : 1 } ] } }"));
    }

    @Test
    public void sdbUpdateReplaceOperator() {
        Update update1 = new Update();
        update1.replace("arr",2);
        System.out.println(update1.getUpdateObject());
        assertThat(update1.getUpdateObject().toString(), equalTo("{ \"$replace\" : { \"arr\" : 2 } }"));

        Update update2 = new Update();
        update2.replace("arr", 10).replace("name","Mile");
        System.out.println(update2.getUpdateObject());
        assertThat(update2.getUpdateObject().toString(), equalTo("{ \"$replace\" : { \"arr\" : 10 , \"name\" : \"Mile\" } }"));
    }


	@Test
	public void testSet() {

		Update u = new Update().set("directory", "/Users/Test/Desktop");
		assertThat(u.getUpdateObject().toString(), is("{ \"$set\" : { \"directory\" : \"/Users/Test/Desktop\" } }"));
	}

	@Test
	public void testSetSet() {

		Update u = new Update().set("directory", "/Users/Test/Desktop").set("size", 0);
		assertThat(u.getUpdateObject().toString(),
				is("{ \"$set\" : { \"directory\" : \"/Users/Test/Desktop\" , \"size\" : 0 } }"));
	}

	@Test
	public void testInc() {

		Update u = new Update().inc("size", 1);
		assertThat(u.getUpdateObject().toString(), is("{ \"$inc\" : { \"size\" : 1 } }"));
	}

	@Test
	public void testIncInc() {

		Update u = new Update().inc("size", 1).inc("count", 1);
		assertThat(u.getUpdateObject().toString(), is("{ \"$inc\" : { \"size\" : 1 , \"count\" : 1 } }"));
	}

	@Test
	public void testIncAndSet() {

		Update u = new Update().inc("size", 1).set("directory", "/Users/Test/Desktop");
		assertThat(u.getUpdateObject().toString(),
				is("{ \"$inc\" : { \"size\" : 1 } , \"$set\" : { \"directory\" : \"/Users/Test/Desktop\" } }"));
	}

	@Test
	public void testUnset() {

		Update u = new Update().unset("directory");
		assertThat(u.getUpdateObject().toString(), is("{ \"$unset\" : { \"directory\" : \"\" } }"));
	}

	@Test
	public void testPush() {

		Update u = new Update().push("authors", Collections.singletonMap("name", "Sven"));
		assertThat(u.getUpdateObject().toString(), is("{ \"$push\" : { \"authors\" : { \"name\" : \"Sven\" } } }"));
	}

	@Test
	public void testPushAll() {

		Map<String, String> m1 = Collections.singletonMap("name", "Sven");
		Map<String, String> m2 = Collections.singletonMap("name", "Maria");

		Update u = new Update().pushAll("authors", new Object[] { m1, m2 });
		assertThat(u.getUpdateObject().toString(),
				is("{ \"$push_all\" : { \"authors\" : [ { \"name\" : \"Sven\" } , { \"name\" : \"Maria\" } ] } }"));
	}

	/**
	 * @see DATA_JIRA-354
	 */
	@Test
	public void testMultiplePushAllShouldBePossibleWhenUsingDifferentFields() {

		Map<String, String> m1 = Collections.singletonMap("name", "Sven");
		Map<String, String> m2 = Collections.singletonMap("name", "Maria");

		Update u = new Update().pushAll("authors", new Object[] { m1, m2 });
		u.pushAll("books", new Object[] { "Spring in Action" });

		assertThat(
				u.getUpdateObject().toString(),
				is("{ \"$push_all\" : { \"authors\" : [ { \"name\" : \"Sven\" } , { \"name\" : \"Maria\" } ] , \"books\" : [ \"Spring in Action\" ] } }"));
	}

	@Test
	public void testAddToSet() {

		Update u = new Update().addToSet("authors", Collections.singletonMap("name", "Sven"));
		assertThat(u.getUpdateObject().toString(), is("{ \"$addtoset\" : { \"authors\" : [ { \"name\" : \"Sven\" } ] } }"));
	}

	@Test
	public void testPop() {

		Update u = new Update().pop("authors", -1);
		assertThat(u.getUpdateObject().toString(), is("{ \"$pop\" : { \"authors\" : -1 } }"));

		u = new Update().pop("authors", 1);
		assertThat(u.getUpdateObject().toString(), is("{ \"$pop\" : { \"authors\" : 1 } }"));
	}

	@Test
	public void testPull() {

		Update u = new Update().pull("authors", Collections.singletonMap("name", "Sven"));
		assertThat(u.getUpdateObject().toString(), is("{ \"$pull\" : { \"authors\" : { \"name\" : \"Sven\" } } }"));
	}

	@Test
	public void testPullAll() {

		Map<String, String> m1 = Collections.singletonMap("name", "Sven");
		Map<String, String> m2 = Collections.singletonMap("name", "Maria");

		Update u = new Update().pullAll("authors", new Object[] { m1, m2 });
		assertThat(u.getUpdateObject().toString(),
				is("{ \"$pull_all\" : { \"authors\" : [ { \"name\" : \"Sven\" } , { \"name\" : \"Maria\" } ] } }"));
	}

	@Test
	public void testPullAllWithSomeFields() {
//		Update u = new Update().set("a",new BasicBSONObject("id",2)).set("b",new BasicBSONObject("id",4));
//		System.out.println("obj is: " + u.getUpdateObject());
	}

	@Test
	public void testRename() {

//		Update u = new Update().rename("directory", "folder");
//		assertThat(u.getUpdateObject().toString(), is("{ \"$rename\" : { \"directory\" : \"folder\"}}"));
	}

	@Test
	public void testBasicUpdateInc() {

		Update u = new Update().inc("size", 1);
		assertThat(u.getUpdateObject().toString(), is("{ \"$inc\" : { \"size\" : 1 } }"));
	}

	@Test
	public void testBasicUpdateIncAndSet() {

		Update u = new BasicUpdate("{ \"$inc\" : { \"size\" : 1 } }").set("directory", "/Users/Test/Desktop");
		assertThat(u.getUpdateObject().toString(),
				is("{ \"$inc\" : { \"size\" : 1 } , \"$set\" : { \"directory\" : \"/Users/Test/Desktop\" } }"));
	}

	/**
	 * @see DATA_JIRA-630
	 */
	@Test
	public void testSetOnInsert() {

//		Update u = new Update().setOnInsert("size", 1);
//		assertThat(u.getUpdateObject().toString(), is("{ \"$setOnInsert\" : { \"size\" : 1}}"));
	}

	/**
	 * @see DATA_JIRA-630
	 */
	@Test
	public void testSetOnInsertSetOnInsert() {
//
//		Update u = new Update().setOnInsert("size", 1).setOnInsert("count", 1);
//		assertThat(u.getUpdateObject().toString(), is("{ \"$setOnInsert\" : { \"size\" : 1 , \"count\" : 1}}"));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnTrueWhenMultiFieldOperationAddedForField() {

		Update update = new Update().set("foo", "bar");
		assertThat(update.modifies("foo"), is(true));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnFalseWhenMultiFieldOperationAddedForField() {

		Update update = new Update().set("foo", "bar");
		assertThat(update.modifies("oof"), is(false));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnTrueWhenSingleFieldOperationAddedForField() {

		Update update = new Update().pullAll("foo", new Object[] { "bar" });
		assertThat(update.modifies("foo"), is(true));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnFalseWhenSingleFieldOperationAddedForField() {

		Update update = new Update().pullAll("foo", new Object[] { "bar" });
		assertThat(update.modifies("oof"), is(false));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnFalseWhenCalledOnEmptyUpdate() {
		assertThat(new Update().modifies("foo"), is(false));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnTrueWhenUpdateWithKeyCreatedFromDbObject() {

		Update update = new Update().set("foo", "bar");
		Update clone = Update.fromDBObject(update.getUpdateObject());

		assertThat(clone.modifies("foo"), is(true));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void testUpdateAffectsFieldShouldReturnFalseWhenUpdateWithoutKeyCreatedFromDbObject() {

		Update update = new Update().set("foo", "bar");
		Update clone = Update.fromDBObject(update.getUpdateObject());

		assertThat(clone.modifies("oof"), is(false));
	}

	/**
	 * @see DATA_JIRA-853
	 */
	@Test(expected = IllegalArgumentException.class)
	public void testAddingMultiFieldOperationThrowsExceptionWhenCalledWithNullKey() {
		new Update().addMultiFieldOperation("$op", null, "exprected to throw IllegalArgumentException.");
	}

	/**
	 * @see DATA_JIRA-853
	 */
	@Test(expected = IllegalArgumentException.class)
	public void testAddingSingleFieldOperationThrowsExceptionWhenCalledWithNullKey() {
		new Update().addFieldOperation("$op", null, "exprected to throw IllegalArgumentException.");
	}

	/**
	 * @see DATA_JIRA-853
	 */
	@Test(expected = IllegalArgumentException.class)
	public void testCreatingUpdateWithNullKeyThrowsException() {
		Update.update(null, "value");
	}

	/**
	 * @see DATA_JIRA-953
	 */
	@Test
	public void testEquality() {

		Update actualUpdate = new Update() //
				.inc("size", 1) //
				.set("nl", null) //
				.set("directory", "/Users/Test/Desktop") //
				.push("authors", Collections.singletonMap("name", "Sven")) //
				.pop("authors", -1) //
				.set("foo", "bar");

		Update expectedUpdate = new Update() //
				.inc("size", 1) //
				.set("nl", null) //
				.set("directory", "/Users/Test/Desktop") //
				.push("authors", Collections.singletonMap("name", "Sven")) //
				.pop("authors", -1) //
				.set("foo", "bar");

		assertThat(actualUpdate, is(equalTo(actualUpdate)));
		assertThat(actualUpdate.hashCode(), is(equalTo(actualUpdate.hashCode())));
		assertEquals(actualUpdate, actualUpdate);
		assertThat(actualUpdate, is(equalTo(expectedUpdate)));
		assertThat(actualUpdate.hashCode(), is(equalTo(expectedUpdate.hashCode())));
	}

	/**
	 * @see DATA_JIRA-953
	 */
	@Test
	public void testToString() {

		Update actualUpdate = new Update() //
				.inc("size", 1) //
				.set("nl", null) //
				.set("directory", "/Users/Test/Desktop") //
				.push("authors", Collections.singletonMap("name", "Sven")) //
				.pop("authors", -1) //
				.set("foo", "bar");

		Update expectedUpdate = new Update() //
				.inc("size", 1) //
				.set("nl", null) //
				.set("directory", "/Users/Test/Desktop") //
				.push("authors", Collections.singletonMap("name", "Sven")) //
				.pop("authors", -1) //
				.set("foo", "bar");

		assertThat(actualUpdate.toString(), is(equalTo(expectedUpdate.toString())));
		assertThat(actualUpdate.toString(), is("{ \"$inc\" : { \"size\" : 1 } ," //
				+ " \"$set\" : { \"nl\" :  null  , \"directory\" : \"/Users/Test/Desktop\" , \"foo\" : \"bar\" } , " //
				+ "\"$push\" : { \"authors\" : { \"name\" : \"Sven\" } } " //
				+ ", \"$pop\" : { \"authors\" : -1 } }")); //
	}

	/**
	 * @see DATA_JIRA-944
	 */
	@Test
	public void getUpdateObjectShouldReturnCurrentDateCorrectlyForSingleFieldWhenUsingDate() {

//		Update update = new Update().currentDate("foo");
//		assertThat(update.getUpdateObject(),
//				equalTo(new BasicBSONObjectBuilder().add("$currentDate", new BasicBSONObject("foo", true)).get()));
	}

	/**
	 * @see DATA_JIRA-944
	 */
	@Test
	public void getUpdateObjectShouldReturnCurrentDateCorrectlyForMultipleFieldsWhenUsingDate() {

//		Update update = new Update().currentDate("foo").currentDate("bar");
//		assertThat(update.getUpdateObject(),
//				equalTo(new BasicBSONObjectBuilder().add("$currentDate", new BasicBSONObject("foo", true).append("bar", true))
//						.get()));
	}

	/**
	 * @see DATA_JIRA-944
	 */
	@Test
	public void getUpdateObjectShouldReturnCurrentDateCorrectlyForSingleFieldWhenUsingTimestamp() {
//
//		Update update = new Update().currentTimestamp("foo");
//		assertThat(
//				update.getUpdateObject(),
//				equalTo(new BasicBSONObjectBuilder().add("$currentDate",
//						new BasicBSONObject("foo", new BasicBSONObject("$type", "timestamp"))).get()));
	}

	/**
	 * @see DATA_JIRA-944
	 */
	@Test
	public void getUpdateObjectShouldReturnCurrentDateCorrectlyForMultipleFieldsWhenUsingTimestamp() {

//		Update update = new Update().currentTimestamp("foo").currentTimestamp("bar");
//		assertThat(
//				update.getUpdateObject(),
//				equalTo(new BasicBSONObjectBuilder().add(
//						"$currentDate",
//						new BasicBSONObject("foo", new BasicBSONObject("$type", "timestamp")).append("bar", new BasicBSONObject("$type",
//								"timestamp"))).get()));
	}

	/**
	 * @see DATA_JIRA-944
	 */
	@Test
	public void getUpdateObjectShouldReturnCurrentDateCorrectlyWhenUsingMixedDateAndTimestamp() {
//
//		Update update = new Update().currentDate("foo").currentTimestamp("bar");
//		assertThat(
//				update.getUpdateObject(),
//				equalTo(new BasicBSONObjectBuilder().add("$currentDate",
//						new BasicBSONObject("foo", true).append("bar", new BasicBSONObject("$type", "timestamp"))).get()));
	}

	/**
	 * @see DATA_JIRA-1002
	 */
	@Test
	public void toStringWorksForUpdateWithComplexObject() {

		Update update = new Update().addToSet("key", new DateTime());
		assertThat(update.toString(), is(notNullValue()));
	}
}
