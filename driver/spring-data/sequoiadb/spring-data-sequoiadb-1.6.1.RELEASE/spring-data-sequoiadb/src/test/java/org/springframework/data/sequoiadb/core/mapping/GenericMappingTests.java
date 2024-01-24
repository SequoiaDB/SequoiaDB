/*
 * Copyright (c) 2011 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.springframework.data.sequoiadb.core.mapping;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.util.Collections;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;


import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;

/**
 * Unit tests for testing the mapping works with generic types.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class GenericMappingTests {

	SequoiadbMappingContext context;
	SequoiadbConverter converter;

	@Mock DbRefResolver resolver;

	@Before
	public void setUp() throws Exception {

		context = new SequoiadbMappingContext();
		context.setInitialEntitySet(Collections.singleton(StringWrapper.class));
		context.initialize();

		converter = new MappingSequoiadbConverter(resolver, context);
	}

	@Test
	public void writesGenericTypeCorrectly() {

		StringWrapper wrapper = new StringWrapper();
		wrapper.container = new Container<String>();
		wrapper.container.content = "Foo!";

		BSONObject dbObject = new BasicBSONObject();
		converter.write(wrapper, dbObject);

		Object container = dbObject.get("container");
		assertThat(container, is(notNullValue()));
		assertTrue(container instanceof BSONObject);

		Object content = ((BSONObject) container).get("content");
		assertTrue(content instanceof String);
		assertThat((String) content, is("Foo!"));
	}

	@Test
	public void readsGenericTypeCorrectly() {

		BSONObject content = new BasicBSONObject("content", "Foo!");
		BasicBSONObject container = new BasicBSONObject("container", content);

		StringWrapper result = converter.read(StringWrapper.class, container);
		assertThat(result.container, is(notNullValue()));
		assertThat(result.container.content, is("Foo!"));
	}

	static class StringWrapper extends Wrapper<String> {

	}

	static class Wrapper<S> {
		Container<S> container;
	}

	static class Container<T> {
		T content;
	}
}
