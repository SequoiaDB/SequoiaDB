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

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;




/**
 * Helper classes to ease assertions on {@link BSONObject}s.
 * 

 */
public abstract class DBObjectTestUtils {

	private DBObjectTestUtils() {

	}

	/**
	 * Expects the field with the given key to be not {@literal null} and a {@link BSONObject} in turn and returns it.
	 * 
	 * @param source the {@link BSONObject} to lookup the nested one
	 * @param key the key of the field to lokup the nested {@link BSONObject}
	 * @return
	 */
	public static BSONObject getAsDBObject(BSONObject source, String key) {
		return getTypedValue(source, key, BSONObject.class);
	}

	/**
	 * Expects the field with the given key to be not {@literal null} and a {@link BasicBSONList}.
	 * 
	 * @param source the {@link BSONObject} to lookup the {@link BasicBSONList} in
	 * @param key the key of the field to find the {@link BasicBSONList} in
	 * @return
	 */
	public static BasicBSONList getAsDBList(BSONObject source, String key) {
		return getTypedValue(source, key, BasicBSONList.class);
	}

	/**
	 * Expects the list element with the given index to be a non-{@literal null} {@link BSONObject} and returns it.
	 * 
	 * @param source the {@link BasicBSONList} to look up the {@link BSONObject} element in
	 * @param index the index of the element expected to contain a {@link BSONObject}
	 * @return
	 */
	public static BSONObject getAsDBObject(BasicBSONList source, int index) {

		assertThat(source.size(), greaterThanOrEqualTo(index + 1));
		Object value = source.get(index);
		assertThat(value, is(instanceOf(BSONObject.class)));
		return (BSONObject) value;
	}

	@SuppressWarnings("unchecked")
	public static <T> T getTypedValue(BSONObject source, String key, Class<T> type) {

		Object value = source.get(key);
		assertThat(value, is(notNullValue()));
		assertThat(value, is(instanceOf(type)));

		return (T) value;
	}
}
