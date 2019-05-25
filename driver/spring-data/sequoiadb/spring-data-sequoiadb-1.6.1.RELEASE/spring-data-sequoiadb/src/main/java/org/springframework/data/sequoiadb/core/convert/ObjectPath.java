/*
 * Copyright 2014 the original author or authors.
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;



/**
 * A path of objects nested into each other. The type allows access to all parent objects currently in creation even
 * when resolving more nested objects. This allows to avoid re-resolving object instances that are logically equivalent
 * to already resolved ones.
 * <p>
 * An immutable ordered set of target objects for {@link BSONObject} to {@link Object} conversions. Object paths can be
 * constructed by the {@link #toObjectPath(Object)} method and extended via {@link #push(Object)}.
 * 


 * @since 1.6
 */
class ObjectPath {

	public static final ObjectPath ROOT = new ObjectPath();

	private final List<ObjectPathItem> items;

	private ObjectPath() {
		this.items = Collections.emptyList();
	}

	/**
	 * Creates a new {@link ObjectPath} from the given parent {@link ObjectPath} by adding the provided
	 * {@link ObjectPathItem} to it.
	 * 
	 * @param parent can be {@literal null}.
	 * @param item
	 */
	private ObjectPath(ObjectPath parent, ObjectPath.ObjectPathItem item) {

		List<ObjectPath.ObjectPathItem> items = new ArrayList<ObjectPath.ObjectPathItem>(parent.items);
		items.add(item);

		this.items = Collections.unmodifiableList(items);
	}

	/**
	 * Returns a copy of the {@link ObjectPath} with the given {@link Object} as current object.
	 *
	 * @param object must not be {@literal null}.
	 * @param entity must not be {@literal null}.
	 * @param id must not be {@literal null}.
	 * @return
	 */
	public ObjectPath push(Object object, SequoiadbPersistentEntity<?> entity, Object id) {

		Assert.notNull(object, "Object must not be null!");
		Assert.notNull(entity, "SequoiadbPersistentEntity must not be null!");

		ObjectPathItem item = new ObjectPathItem(object, id, entity.getCollection());
		return new ObjectPath(this, item);
	}

	/**
	 * Returns the object with the given id and stored in the given collection if it's contained in the {@link ObjectPath}
	 * .
	 * 
	 * @param id must not be {@literal null}.
	 * @param collection must not be {@literal null} or empty.
	 * @return
	 */
	public Object getPathItem(Object id, String collection) {

		Assert.notNull(id, "Id must not be null!");
		Assert.hasText(collection, "Collection name must not be null!");

		for (ObjectPathItem item : items) {

			Object object = item.getObject();

			if (object == null) {
				continue;
			}

			if (item.getIdValue() == null) {
				continue;
			}

			if (collection.equals(item.getCollection()) && id.equals(item.getIdValue())) {
				return object;
			}
		}

		return null;
	}

	/**
	 * Returns the current object of the {@link ObjectPath} or {@literal null} if the path is empty.
	 * 
	 * @return
	 */
	public Object getCurrentObject() {
		return items.isEmpty() ? null : items.get(items.size() - 1).getObject();
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {

		if (items.isEmpty()) {
			return "[empty]";
		}

		List<String> strings = new ArrayList<String>(items.size());

		for (ObjectPathItem item : items) {
			strings.add(item.object.toString());
		}

		return StringUtils.collectionToDelimitedString(strings, " -> ");
	}

	/**
	 * An item in an {@link ObjectPath}.
	 * 


	 */
	private static class ObjectPathItem {

		private final Object object;
		private final Object idValue;
		private final String collection;

		/**
		 * Creates a new {@link ObjectPathItem}.
		 * 
		 * @param object
		 * @param idValue
		 * @param collection
		 */
		ObjectPathItem(Object object, Object idValue, String collection) {

			this.object = object;
			this.idValue = idValue;
			this.collection = collection;
		}

		public Object getObject() {
			return object;
		}

		public Object getIdValue() {
			return idValue;
		}

		public String getCollection() {
			return collection;
		}
	}
}
