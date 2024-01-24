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

import static org.springframework.util.ObjectUtils.*;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.dao.InvalidDataAccessApiUsageException;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;




/**
 * Class to easily construct SequoiaDB update clauses.
 *
 */
public class Update {

	private Set<String> keysToUpdate = new HashSet<String>();
	private Map<String, Object> modifierOps = new LinkedHashMap<String, Object>();

	/**
	 * Static factory method to create an Update using the provided key
	 * 
	 * @param key
	 * @return
	 */
	public static Update update(String key, Object value) {
		return new Update().set(key, value);
	}

	/**
	 * Creates an {@link Update} instance from the given {@link BSONObject}. Allows to explicitly exlude fields from making
	 * it into the created {@link Update} object. Note, that this will set attributes directly and <em>not</em> use
	 * {@literal $set}. This means fields not given in the {@link BSONObject} will be nulled when executing the update. To
	 * create an only-updating {@link Update} instance of a {@link BSONObject}, call {@link #set(String, Object)} for each
	 * value in it.
	 * 
	 * @param object the source {@link BSONObject} to create the update from.
	 * @param exclude the fields to exclude.
	 * @return
	 */
	public static Update fromDBObject(BSONObject object, String... exclude) {

		Update update = new Update();
		List<String> excludeList = Arrays.asList(exclude);

		for (String key : object.keySet()) {

			if (excludeList.contains(key)) {
				continue;
			}

			Object value = object.get(key);
			update.modifierOps.put(key, value);
			if (isKeyword(key) && value instanceof BSONObject) {
				update.keysToUpdate.addAll(((BSONObject) value).keySet());
			} else {
				update.keysToUpdate.add(key);
			}
		}

		return update;
	}


	/**
	 * Update using the {@literal $inc} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/inc/
	 * @param key
	 * @param inc
	 * @return
	 */
	public Update inc(String key, Number inc) {
		addMultiFieldOperation("$inc", key, inc);
		return this;
	}

	/**
	 * Update using the {@literal $set} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/set/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update set(String key, Object value) {
		addMultiFieldOperation("$set", key, value);
		return this;
	}

	/**
	 * Update using the {@literal $unset} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/unset/
	 * @param key
	 * @return
	 */
	public Update unset(String key) {
		addMultiFieldOperation("$unset", key, "");
		return this;
	}

	/**
	 * Update using the {@literal $addtoset} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/addToSet/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update addToSet(String key, Object... values) {
		addMultiFieldOperation("$addtoset", key, extractValues(values));
		return this;
	}

	/**
	 * Update using the {@literal $pop} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/pop/
	 * @param key
	 * @param pos
	 * @return
	 */
	public Update pop(String key, int pos) {
		addMultiFieldOperation("$pop", key, pos);
		return this;
	}

	/**
	 * Update using the {@literal $pull} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/pull/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update pull(String key, Object value) {
		addMultiFieldOperation("$pull", key, value);
		return this;
	}

	/**
	 * Update using the {@literal $pullAll} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/pullAll/
	 * @param key
	 * @param values
	 * @return
	 */
	public Update pullAll(String key, Object... values) {
		addMultiFieldOperation("$pull_all", key, extractValues(values));
		return this;
	}

	/**
	 * Update using the {@literal $pullBy} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/pull/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update pullBy(String key, Object value) {
		addMultiFieldOperation("$pull_by", key, value);
		return this;
	}

	/**
	 * Update using the {@literal $pullAllBy} update modifier
	 *
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/pullAll/
	 * @param key
	 * @param values
	 * @return
	 */
	public Update pullAllBy(String key, Object... values) {
		addMultiFieldOperation("$pull_all_by", key, extractValues(values));
		return this;
	}

	/**
	 * Update using the {@literal $push} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/push/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update push(String key, Object value) {
		addMultiFieldOperation("$push", key, value);
		return this;
	}

	/**
	 * Update using the {@code $pushAll} update modifier. <br>
	 * <b>Note</b>: In sequoiadb 2.4 the usage of {@code $pushAll} has been deprecated in favor of {@code $push $each}.
	 * {@link #push(String)}) returns a builder that can be used to populate the {@code $each} object.
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/pushAll/
	 * @param key
	 * @param values
	 * @return
	 */
	public Update pushAll(String key, Object... values) {
		addMultiFieldOperation("$push_all", key, extractValues(values));
		return this;
	}

	public Update replace(String key, Object value) {
		addMultiFieldOperation("$replace", key, value);
		return this;
	}

	public BSONObject getUpdateObject() {

		BSONObject dbo = new BasicBSONObject();
		for (String k : modifierOps.keySet()) {
			dbo.put(k, modifierOps.get(k));
		}
		return dbo;
	}

	protected void addFieldOperation(String operator, String key, Object value) {

		Assert.hasText(key, "Key/Path for update must not be null or blank.");
		modifierOps.put(operator, new BasicBSONObject(key, value));
		this.keysToUpdate.add(key);
	}

	protected void addMultiFieldOperation(String operator, String key, Object value) {

		Assert.hasText(key, "Key/Path for update must not be null or blank.");
		Object existingValue = this.modifierOps.get(operator);
		BSONObject keyValueMap;

		if (existingValue == null) {
			keyValueMap = new BasicBSONObject();
			this.modifierOps.put(operator, keyValueMap);
		} else {
			if (existingValue instanceof BasicBSONObject) {
				keyValueMap = (BasicBSONObject) existingValue;
			} else {
				throw new InvalidDataAccessApiUsageException("Modifier Operations should be a LinkedHashMap but was "
						+ existingValue.getClass());
			}
		}

		keyValueMap.put(key, value);
		this.keysToUpdate.add(key);
	}

	/**
	 * Determine if a given {@code key} will be touched on execution.
	 * 
	 * @param key
	 * @return
	 */
	public boolean modifies(String key) {
		return this.keysToUpdate.contains(key);
	}

	/**
	 * Inspects given {@code key} for '$'.
	 *
	 * @param key
	 * @return
	 */
	private static boolean isKeyword(String key) {
		return StringUtils.startsWithIgnoreCase(key, "$");
	}

	private Object[] extractValues(Object[] values) {

		if (values == null || values.length == 0) {
			return values;
		}

		if (values.length == 1 && values[0] instanceof Collection) {
			return ((Collection<?>) values[0]).toArray();
		}

		Object[] convertedValues = new Object[values.length];

		for (int i = 0; i < values.length; i++) {
			convertedValues[i] = values[i];
		}

		return convertedValues;
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return getUpdateObject().hashCode();
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj) {

		if (this == obj) {
			return true;
		}

		if (obj == null || getClass() != obj.getClass()) {
			return false;
		}

		Update that = (Update) obj;
		return this.getUpdateObject().equals(that.getUpdateObject());
	}

	/*
	 * (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		return SerializationUtils.serializeToJsonSafely(getUpdateObject());
	}

	/**
	 * Modifiers holds a distinct collection of {@link Modifier}
	 *


	 */
	public static class Modifiers {

		private Map<String, Modifier> modifiers;

		public Modifiers() {
			this.modifiers = new LinkedHashMap<String, Modifier>(1);
		}

		public Collection<Modifier> getModifiers() {
			return Collections.unmodifiableCollection(this.modifiers.values());
		}

		public void addModifier(Modifier modifier) {
			this.modifiers.put(modifier.getKey(), modifier);
		}

		/* (non-Javadoc)
		 * @see java.lang.Object#hashCode()
		 */
		@Override
		public int hashCode() {
			return nullSafeHashCode(modifiers);
		}

		/* (non-Javadoc)
		 * @see java.lang.Object#equals(java.lang.Object)
		 */
		@Override
		public boolean equals(Object obj) {

			if (this == obj) {
				return true;
			}

			if (obj == null || getClass() != obj.getClass()) {
				return false;
			}

			Modifiers that = (Modifiers) obj;

			return this.modifiers.equals(that.modifiers);
		}
	}

	/**
	 * Marker interface of nested commands.
	 *

	 */
	public static interface Modifier {

		/**
		 * @return the command to send eg. {@code $push}
		 */
		String getKey();

		/**
		 * @return value to be sent with command
		 */
		Object getValue();
	}

}
