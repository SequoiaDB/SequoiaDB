/*
 * Copyright 2013 the original author or authors.
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

import org.junit.Assert;
import org.junit.Test;
import org.springframework.data.mongodb.assist.BasicDBObject;

/**
 * Unit tests for {@link DocumentField}.
 * 
 * @author Oliver Gierke
 */
public class FieldUnitTests {

	@Test
	public void funcOperatorWithOtherTest() {
		Field field = new Field();
		String actualStr;
		String expectStr;

		field = new Field().include("a", 1).abs();
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$include\" : 1 , \"$abs\" : 1 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().defaultValue("a", new BasicDBObject("aa", 1)).abs().divide(10);
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$default\" : { \"aa\" : 1 } , \"$abs\" : 1 , \"$divide\" : 10 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().elemMatch("a", Criteria.where("aa").is(100)).abs().divide(10);
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$elemMatch\" : { \"aa\" : 100 } , \"$abs\" : 1 , \"$divide\" : 10 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().elemMatchOne("a", Criteria.where("aa").is(100)).abs().divide(10);
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$elemMatchOne\" : { \"aa\" : 100 } , \"$abs\" : 1 , \"$divide\" : 10 } }";
		assertEquals(expectStr, actualStr);
	}

	@Test
	public void funcOperatorWithSelectorTest() {
		Field field = new Field();
		String actualStr;
		String expectStr;

		field = new Field().select("a", 1).abs();
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$abs\" : 1 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().select("a", 1).ceiling();
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$ceiling\" : 1 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().select("a", 1).floor().mod(10).add(100);
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$floor\" : 1 , \"$mod\" : 10 , \"$add\" : 100 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().select("a", 1).subtract(10).multiply(10).divide(1.2).substr(0,5).strlen().lower();
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$subtract\" : 10 , \"$multiply\" : 10 , \"$divide\" : 1.2 , \"$substr\" : [ 0 , 5 ] , \"$strlen\" : 1 , \"$lower\" : 1 } }";
		assertEquals(expectStr, actualStr);

		field = new Field().select("a", 1).upper().ltrim().rtrim().trim().cast(1).size().type(2).slice(0, 10);
		actualStr = field.getFieldsObject().toString();
		expectStr = "{ \"a\" : { \"$upper\" : 1 , \"$ltrim\" : 1 , \"$rtrim\" : 1 , \"$trim\" : 1 , \"$cast\" : 1 , \"$size\" : 1 , \"$type\" : 2 , \"$slice\" : [ 0 , 10 ] } }";
		assertEquals(expectStr, actualStr);
	}

	@Test
	public void multiFieldsTest() {
		Field f = new Field().select("a",1).include("b", 1).elemMatchOne("c", Criteria.where("foo").is("bar")).defaultValue("d", "d");
		Assert.assertEquals("{ \"a\" : 1 , \"b\" : { \"$include\" : 1 } , \"c\" : { \"$elemMatchOne\" : { \"foo\" : \"bar\" } } , \"d\" : { \"$default\" : \"d\" } }", f.getFieldsObject().toString());
	}

	@Test
	public void selectTest() {
		Field f = new Field().select("a", 1).select("b", new BasicDBObject("name","sam"));
		Assert.assertEquals("{ \"a\" : 1 , \"b\" : { \"name\" : \"sam\" } }", f.getFieldsObject().toString());
	}

	@Test
	public void defaultValueTest() {
		Field f1 = new Field().defaultValue("foo", "bar");
		Field f2 = new Field().defaultValue("foo1","bar1").defaultValue("foo2", "bar2");
		Assert.assertEquals("{ \"foo\" : { \"$default\" : \"bar\" } }", f1.getFieldsObject().toString());
		Assert.assertEquals("{ \"foo1\" : { \"$default\" : \"bar1\" } , \"foo2\" : { \"$default\" : \"bar2\" } }", f2.getFieldsObject().toString());
	}

	@Test
	public void elemMatchOneTest() {
		Field f1 = new Field().elemMatchOne("key", Criteria.where("foo").is("bar"));
		Field f2 = new Field().elemMatchOne("key1", Criteria.where("foo1").is("bar1")).elemMatchOne("key2", Criteria.where("foo2").is("bar2"));
		Assert.assertEquals("{ \"key\" : { \"$elemMatchOne\" : { \"foo\" : \"bar\" } } }", f1.getFieldsObject().toString());
		Assert.assertEquals("{ \"key1\" : { \"$elemMatchOne\" : { \"foo1\" : \"bar1\" } } , \"key2\" : { \"$elemMatchOne\" : { \"foo2\" : \"bar2\" } } }", f2.getFieldsObject().toString());
	}

	@Test
	public void elemMatchTest() {
		Field f1 = new Field().elemMatch("key", Criteria.where("foo").is("bar"));
		Field f2 = new Field().elemMatch("key1", Criteria.where("foo1").is("bar1")).elemMatch("key2", Criteria.where("foo2").is("bar2"));
		Assert.assertEquals("{ \"key\" : { \"$elemMatch\" : { \"foo\" : \"bar\" } } }", f1.getFieldsObject().toString());
		Assert.assertEquals("{ \"key1\" : { \"$elemMatch\" : { \"foo1\" : \"bar1\" } } , \"key2\" : { \"$elemMatch\" : { \"foo2\" : \"bar2\" } } }", f2.getFieldsObject().toString());
	}

	@Test
	public void includeTest() {
		Field f1 = new Field().include("a",1).include("b",1).include("c",1);
		Field f3 = new Field().include("a",0).include("b",0).include("c",0);
		Assert.assertEquals("{ \"a\" : { \"$include\" : 1 } , \"b\" : { \"$include\" : 1 } , \"c\" : { \"$include\" : 1 } }", f1.getFieldsObject().toString());
		Assert.assertEquals("{ \"a\" : { \"$include\" : 0 } , \"b\" : { \"$include\" : 0 } , \"c\" : { \"$include\" : 0 } }", f3.getFieldsObject().toString());
	}


	@Test
	public void sameObjectSetupCreatesEqualField() {

		Field left = new Field().elemMatch("key", Criteria.where("foo").is("bar"));
		Field right = new Field().elemMatch("key", Criteria.where("foo").is("bar"));

		assertThat(left, is(right));
		assertThat(right, is(left));
	}

	@Test
	public void differentObjectSetupCreatesEqualField() {

		Field left = new Field().elemMatch("key", Criteria.where("foo").is("bar"));
		Field right = new Field().elemMatch("key", Criteria.where("foo").is("foo"));

		assertThat(left, is(not(right)));
		assertThat(right, is(not(left)));
	}
}
