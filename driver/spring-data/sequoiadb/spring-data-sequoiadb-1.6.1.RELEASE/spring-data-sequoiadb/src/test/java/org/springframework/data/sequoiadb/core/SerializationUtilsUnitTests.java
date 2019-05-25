/*
 * Copyright 2012 the original author or authors.
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
import static org.springframework.data.sequoiadb.core.query.SerializationUtils.*;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.hamcrest.Matcher;
import org.junit.Test;


import org.springframework.data.sequoiadb.core.query.SerializationUtils;

/**
 * Unit tests for {@link SerializationUtils}.
 * 

 */
public class SerializationUtilsUnitTests {

	@Test
	public void writesSimpleDBObject() {

		BSONObject dbObject = new BasicBSONObject("foo", "bar");
		assertThat(serializeToJsonSafely(dbObject), is("{ \"foo\" : \"bar\" }"));
	}

	@Test
	public void writesComplexObjectAsPlainToString() {

		BSONObject dbObject = new BasicBSONObject("foo", new Complex());
		assertThat(serializeToJsonSafely(dbObject),
				startsWith("{ \"foo\" : { $java : org.springframework.data.sequoiadb.core.SerializationUtilsUnitTests$Complex"));
	}

	@Test
	public void writesCollection() {

		BSONObject dbObject = new BasicBSONObject("foo", Arrays.asList("bar", new Complex()));
		Matcher<String> expectedOutput = allOf(
				startsWith("{ \"foo\" : [ \"bar\", { $java : org.springframework.data.sequoiadb.core.SerializationUtilsUnitTests$Complex"),
				endsWith(" } ] }"));
		assertThat(serializeToJsonSafely(dbObject), is(expectedOutput));
	}

	static class Complex {

	}

}
