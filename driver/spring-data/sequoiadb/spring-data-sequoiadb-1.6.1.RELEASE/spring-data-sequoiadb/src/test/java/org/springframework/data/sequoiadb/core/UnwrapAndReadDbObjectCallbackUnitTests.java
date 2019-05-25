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
package org.springframework.data.sequoiadb.core;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.SequoiadbFactory;

import org.springframework.data.sequoiadb.core.SequoiadbTemplate.UnwrapAndReadDbObjectCallback;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;


/**
 * Unit tests for {@link UnwrapAndReadDbObjectCallback}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class UnwrapAndReadDbObjectCallbackUnitTests {

	@Mock
	SequoiadbFactory factory;

	UnwrapAndReadDbObjectCallback<Target> callback;

	@Before
	public void setUp() {

		SequoiadbTemplate template = new SequoiadbTemplate(factory);
		MappingSequoiadbConverter converter = new MappingSequoiadbConverter(new DefaultDbRefResolver(factory),
				new SequoiadbMappingContext());

		this.callback = template.new UnwrapAndReadDbObjectCallback<Target>(converter, Target.class);
	}

	@Test
	public void usesFirstLevelValues() {

		Target target = callback.doWith(new BasicBSONObject("foo", "bar"));

		assertThat(target.id, is(nullValue()));
		assertThat(target.foo, is("bar"));
	}

	@Test
	public void unwrapsUnderscoreIdIfBasicDBObject() {

		Target target = callback.doWith(new BasicBSONObject("_id", new BasicBSONObject("foo", "bar")));

		assertThat(target.id, is(nullValue()));
		assertThat(target.foo, is("bar"));
	}

	@Test
	public void firstLevelPropertiesTrumpNestedOnes() {
		BSONObject obj = new BasicBSONObject("_id", new BasicBSONObject("foo", "bar"));
		obj.put("foo", "foobar");
		Target target = callback.doWith(obj);

		assertThat(target.id, is(nullValue()));
		assertThat(target.foo, is("foobar"));
	}

	@Test
	public void keepsUnderscoreIdIfScalarValue() {

		BSONObject obj = new BasicBSONObject("_id", "bar");
		obj.put("foo", "foo");
		Target target = callback.doWith(obj);

		assertThat(target.id, is("bar"));
		assertThat(target.foo, is("foo"));
	}

	static class Target {

		String id;
		String foo;
	}
}
