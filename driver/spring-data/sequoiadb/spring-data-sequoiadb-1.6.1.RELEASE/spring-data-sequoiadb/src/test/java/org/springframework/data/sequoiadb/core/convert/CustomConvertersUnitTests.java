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
package org.springframework.data.sequoiadb.core.convert;

import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import java.util.Arrays;
import java.util.HashSet;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.hamcrest.CoreMatchers;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.annotation.Id;
import org.springframework.data.sequoiadb.SequoiadbFactory;


import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;

/**
 * Test case to verify correct usage of custom {@link Converter} implementations to be used.
 * 

 * @see DATADOC-101
 */
@RunWith(MockitoJUnitRunner.class)
public class CustomConvertersUnitTests {

	MappingSequoiadbConverter converter;

	@Mock BarToDBObjectConverter barToDBObjectConverter;
	@Mock DBObjectToBarConverter dbObjectToBarConverter;
	@Mock
	SequoiadbFactory sequoiadbFactory;

	SequoiadbMappingContext context;
	SequoiadbPersistentEntity<Foo> fooEntity;
	SequoiadbPersistentEntity<Bar> barEntity;

	@Before
	@SuppressWarnings("unchecked")
	public void setUp() throws Exception {

		when(barToDBObjectConverter.convert(any(Bar.class))).thenReturn(new BasicBSONObject());
		when(dbObjectToBarConverter.convert(any(BSONObject.class))).thenReturn(new Bar());

		CustomConversions conversions = new CustomConversions(Arrays.asList(barToDBObjectConverter, dbObjectToBarConverter));

		context = new SequoiadbMappingContext();
		context.setInitialEntitySet(new HashSet<Class<?>>(Arrays.asList(Foo.class, Bar.class)));
		context.setSimpleTypeHolder(conversions.getSimpleTypeHolder());
		context.initialize();

		converter = new MappingSequoiadbConverter(new DefaultDbRefResolver(sequoiadbFactory), context);
		converter.setCustomConversions(conversions);
		converter.afterPropertiesSet();
	}

	@Test
	public void nestedToDBObjectConverterGetsInvoked() {

		Foo foo = new Foo();
		foo.bar = new Bar();

		converter.write(foo, new BasicBSONObject());
		verify(barToDBObjectConverter).convert(any(Bar.class));
	}

	@Test
	public void nestedFromDBObjectConverterGetsInvoked() {

		BasicBSONObject dbObject = new BasicBSONObject();
		dbObject.put("bar", new BasicBSONObject());

		converter.read(Foo.class, dbObject);
		verify(dbObjectToBarConverter).convert(any(BSONObject.class));
	}

	@Test
	public void toDBObjectConverterGetsInvoked() {

		converter.write(new Bar(), new BasicBSONObject());
		verify(barToDBObjectConverter).convert(any(Bar.class));
	}

	@Test
	public void fromDBObjectConverterGetsInvoked() {

		converter.read(Bar.class, new BasicBSONObject());
		verify(dbObjectToBarConverter).convert(any(BSONObject.class));
	}

	@Test
	public void foo() {
		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("foo", null);

		Assert.assertThat(dbObject.containsField("foo"), CoreMatchers.is(true));
	}

	public static class Foo {
		@Id public String id;
		public Bar bar;
	}

	public static class Bar {
		@Id public String id;
		public String foo;
	}

	private interface BarToDBObjectConverter extends Converter<Bar, BSONObject> {

	}

	private interface DBObjectToBarConverter extends Converter<BSONObject, Bar> {

	}
}
