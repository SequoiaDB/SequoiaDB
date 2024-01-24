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

import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.util.Assert;




/**
 * Wrapper value object for a {@link BasicBSONObject} to be able to access raw values by {@link SequoiadbPersistentProperty}
 * references. The accessors will transparently resolve nested document values that a {@link SequoiadbPersistentProperty}
 * might refer to through a path expression in field names.
 * 

 */
class DBObjectAccessor {

	private final BSONObject dbObject;

	/**
	 * Creates a new {@link DBObjectAccessor} for the given {@link BSONObject}.
	 * 
	 * @param dbObject must be a {@link BasicBSONObject} effectively, must not be {@literal null}.
	 */
	public DBObjectAccessor(BSONObject dbObject) {

		Assert.notNull(dbObject, "BSONObject must not be null!");
		Assert.isInstanceOf(BasicBSONObject.class, dbObject, "Given BSONObject must be a BasicBSONObject!");

		this.dbObject = dbObject;
	}

	/**
	 * Puts the given value into the backing {@link BSONObject} based on the coordinates defined through the given
	 * {@link SequoiadbPersistentProperty}. By default this will be the plain field name. But field names might also consist
	 * of path traversals so we might need to create intermediate {@link BasicBSONObject}s.
	 * 
	 * @param prop must not be {@literal null}.
	 * @param value
	 */
	public void put(SequoiadbPersistentProperty prop, Object value) {

		Assert.notNull(prop, "SequoiadbPersistentProperty must not be null!");
		String fieldName = prop.getFieldName();

		Iterator<String> parts = Arrays.asList(fieldName.split("\\.")).iterator();
		BSONObject dbObject = this.dbObject;

		while (parts.hasNext()) {

			String part = parts.next();

			if (parts.hasNext()) {
				BasicBSONObject nestedDbObject = new BasicBSONObject();
				dbObject.put(part, nestedDbObject);
				dbObject = nestedDbObject;
			} else {
				dbObject.put(part, value);
			}
		}
	}

	/**
	 * Returns the value the given {@link SequoiadbPersistentProperty} refers to. By default this will be a direct field but
	 * the method will also transparently resolve nested values the {@link SequoiadbPersistentProperty} might refer to through
	 * a path expression in the field name metadata.
	 * 
	 * @param property must not be {@literal null}.
	 * @return
	 */
	@SuppressWarnings("unchecked")
	public Object get(SequoiadbPersistentProperty property) {

		String fieldName = property.getFieldName();
		Iterator<String> parts = Arrays.asList(fieldName.split("\\.")).iterator();
		Map<Object, Object> source = this.dbObject.toMap();
		Object result = null;

		while (source != null && parts.hasNext()) {

			result = source.get(parts.next());

			if (parts.hasNext()) {
				source = getAsMap(result);
			}
		}

		return result;
	}

	@SuppressWarnings("unchecked")
	private Map<Object, Object> getAsMap(Object source) {

		if (source instanceof BasicBSONObject) {
			return ((BSONObject) source).toMap();
		}

		if (source instanceof Map) {
			return (Map<Object, Object>) source;
		}

		return null;
	}
}
