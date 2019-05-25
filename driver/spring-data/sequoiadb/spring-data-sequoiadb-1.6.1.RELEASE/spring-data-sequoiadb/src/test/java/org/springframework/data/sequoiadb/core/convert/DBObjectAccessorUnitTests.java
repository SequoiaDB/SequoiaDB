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
package org.springframework.data.sequoiadb.core.convert;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.Test;


import org.springframework.data.sequoiadb.core.DBObjectTestUtils;
import org.springframework.data.sequoiadb.core.mapping.Field;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;


/**
 * Unit tests for {@link DbObjectAccessor}.
 * 
 * @see DATA_JIRA-766

 */
public class DBObjectAccessorUnitTests {

	SequoiadbMappingContext context = new SequoiadbMappingContext();
	SequoiadbPersistentEntity<?> projectingTypeEntity = context.getPersistentEntity(ProjectingType.class);
	SequoiadbPersistentProperty fooProperty = projectingTypeEntity.getPersistentProperty("foo");

	@Test
	public void putsNestedFieldCorrectly() {

		BSONObject dbObject = new BasicBSONObject();

		DBObjectAccessor accessor = new DBObjectAccessor(dbObject);
		accessor.put(fooProperty, "FooBar");

		BSONObject aDbObject = DBObjectTestUtils.getAsDBObject(dbObject, "a");
		assertThat(aDbObject.get("b"), is((Object) "FooBar"));
	}

	@Test
	public void getsNestedFieldCorrectly() {

		BSONObject source = new BasicBSONObject("a", new BasicBSONObject("b", "FooBar"));

		DBObjectAccessor accessor = new DBObjectAccessor(source);
		assertThat(accessor.get(fooProperty), is((Object) "FooBar"));
	}

	@Test
	public void returnsNullForNonExistingFieldPath() {

		DBObjectAccessor accessor = new DBObjectAccessor(new BasicBSONObject());
		assertThat(accessor.get(fooProperty), is(nullValue()));
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNonBasicDBObjects() {
		new DBObjectAccessor(new BasicBSONList());
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullDBObject() {
		new DBObjectAccessor(null);
	}

	static class ProjectingType {

		String name;
		@Field("a.b") String foo;
		NestedType a;
	}

	static class NestedType {
		String b;
		String c;
	}

}
