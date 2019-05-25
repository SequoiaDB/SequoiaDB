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

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.dao.InvalidDataAccessApiUsageException;


import org.springframework.util.Assert;
import org.springframework.util.StringUtils;

import java.util.*;

import static org.springframework.util.ObjectUtils.nullSafeEquals;
import static org.springframework.util.ObjectUtils.nullSafeHashCode;

/**
 * Class to easily construct SequoiaDB update clauses.
 *
 */
public class Update_bak {

	public enum Position {
		LAST, FIRST
	}

	private Set<String> keysToUpdate = new HashSet<String>();
	private Map<String, Object> modifierOps = new LinkedHashMap<String, Object>();
	private Map<String, PushOperatorBuilder> pushCommandBuilders = new LinkedHashMap<String, PushOperatorBuilder>(1);

	/**
	 * Static factory method to create an Update using the provided key
	 * 
	 * @param key
	 * @return
	 */
	public static Update_bak update(String key, Object value) {
		return new Update_bak().set(key, value);
	}

	/**
	 * Creates an {@link Update_bak} instance from the given {@link BSONObject}. Allows to explicitly exlude fields from making
	 * it into the created {@link Update_bak} object. Note, that this will set attributes directly and <em>not</em> use
	 * {@literal $set}. This means fields not given in the {@link BSONObject} will be nulled when executing the update. To
	 * create an only-updating {@link Update_bak} instance of a {@link BSONObject}, call {@link #set(String, Object)} for each
	 * value in it.
	 * 
	 * @param object the source {@link BSONObject} to create the update from.
	 * @param exclude the fields to exclude.
	 * @return
	 */
	public static Update_bak fromDBObject(BSONObject object, String... exclude) {

		Update_bak update = new Update_bak();
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
	 * Update using the {@literal $set} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/set/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update_bak set(String key, Object value) {
		addMultiFieldOperation("$set", key, value);
		return this;
	}

	/**
	 * Update using the {@literal $setOnInsert} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/setOnInsert/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update_bak setOnInsert(String key, Object value) {
		addMultiFieldOperation("$setOnInsert", key, value);
		return this;
	}

	/**
	 * Update using the {@literal $unset} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/unset/
	 * @param key
	 * @return
	 */
	public Update_bak unset(String key) {
		addMultiFieldOperation("$unset", key, 1);
		return this;
	}

	/**
	 * Update using the {@literal $inc} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/inc/
	 * @param key
	 * @param inc
	 * @return
	 */
	public Update_bak inc(String key, Number inc) {
		addMultiFieldOperation("$inc", key, inc);
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
	public Update_bak push(String key, Object value) {
		addMultiFieldOperation("$push", key, value);
		return this;
	}

	/**
	 * Update using {@code $push} modifier. <br/>
	 * Allows creation of {@code $push} command for single or multiple (using {@code $each}) values.
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/push/
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/each/
	 * @param key
	 * @return {@link PushOperatorBuilder} for given key
	 */
	public PushOperatorBuilder push(String key) {

		if (!pushCommandBuilders.containsKey(key)) {
			pushCommandBuilders.put(key, new PushOperatorBuilder(key));
		}
		return pushCommandBuilders.get(key);
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
	public Update_bak pushAll(String key, Object[] values) {

		Object[] convertedValues = new Object[values.length];
		for (int i = 0; i < values.length; i++) {
			convertedValues[i] = values[i];
		}
		addMultiFieldOperation("$push_all", key, convertedValues);
		return this;
	}

	/**
	 * Update using {@code $addToSet} modifier. <br/>
	 * Allows creation of {@code $push} command for single or multiple (using {@code $each}) values
	 * 
	 * @param key
	 * @return
	 * @since 1.5
	 */
	public AddToSetBuilder addToSet(String key) {
		return new AddToSetBuilder(key);
	}

	/**
	 * Update using the {@literal $addToSet} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/addToSet/
	 * @param key
	 * @param value
	 * @return
	 */
	public Update_bak addToSet(String key, Object value) {
		addMultiFieldOperation("$addToSet", key, value);
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
	public Update_bak pop(String key, Position pos) {
		addMultiFieldOperation("$pop", key, pos == Position.FIRST ? -1 : 1);
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
	public Update_bak pull(String key, Object value) {
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
	public Update_bak pullAll(String key, Object[] values) {

		Object[] convertedValues = new Object[values.length];
		for (int i = 0; i < values.length; i++) {
			convertedValues[i] = values[i];
		}
		addFieldOperation("$pullAll", key, convertedValues);
		return this;
	}

	public Update_bak replace(String key, Object value) {
		addMultiFieldOperation("$replace", key, value);
		return this;
	}

	/**
	 * Update using the {@literal $rename} update modifier
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/rename/
	 * @param oldName
	 * @param newName
	 * @return
	 */
	public Update_bak rename(String oldName, String newName) {
		addMultiFieldOperation("$rename", oldName, newName);
		return this;
	}

	/**
	 * Update given key to current date using {@literal $currentDate} modifier.
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/currentDate/
	 * @param key
	 * @return
	 * @since 1.6
	 */
	public Update_bak currentDate(String key) {

		addMultiFieldOperation("$currentDate", key, true);
		return this;
	}

	/**
	 * Update given key to current date using {@literal $currentDate : &#123; $type : "timestamp" &#125;} modifier.
	 * 
	 * @see http://docs.sequoiadb.org/manual/reference/operator/update/currentDate/
	 * @param key
	 * @return
	 * @since 1.6
	 */
	public Update_bak currentTimestamp(String key) {

		addMultiFieldOperation("$currentDate", key, new BasicBSONObject("$type", "timestamp"));
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

		Update_bak that = (Update_bak) obj;
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

	/**
	 * Implementation of {@link Modifier} representing {@code $each}.
	 * 


	 */
	private static class Each implements Modifier {

		private Object[] values;

		public Each(Object... values) {
			this.values = extractValues(values);
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
		 * @see org.springframework.data.sequoiadb.core.query.Update.Modifier#getKey()
		 */
		@Override
		public String getKey() {
			return "$each";
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.query.Update.Modifier#getValue()
		 */
		@Override
		public Object getValue() {
			return this.values;
		}

		/* 
		 * (non-Javadoc)
		 * @see java.lang.Object#hashCode()
		 */
		@Override
		public int hashCode() {
			return nullSafeHashCode(values);
		}

		/* 
		 * (non-Javadoc)
		 * @see java.lang.Object#equals(java.lang.Object)
		 */
		@Override
		public boolean equals(Object that) {

			if (this == that) {
				return true;
			}

			if (that == null || getClass() != that.getClass()) {
				return false;
			}

			return nullSafeEquals(values, ((Each) that).values);
		}
	}

	/**
	 * Builder for creating {@code $push} modifiers
	 * 


	 */
	public class PushOperatorBuilder {

		private final String key;
		private final Modifiers modifiers;

		PushOperatorBuilder(String key) {
			this.key = key;
			this.modifiers = new Modifiers();
		}

		/**
		 * Propagates {@code $each} to {@code $push}
		 * 
		 * @param values
		 * @return
		 */
		public Update_bak each(Object... values) {

			this.modifiers.addModifier(new Each(values));
			return Update_bak.this.push(key, this.modifiers);
		}

		/**
		 * Propagates {@link #value(Object)} to {@code $push}
		 * 
		 * @param values
		 * @return
		 */
		public Update_bak value(Object value) {
			return Update_bak.this.push(key, value);
		}

		/* 
		 * (non-Javadoc)
		 * @see java.lang.Object#hashCode()
		 */
		@Override
		public int hashCode() {

			int result = 17;

			result += 31 * result + getOuterType().hashCode();
			result += 31 * result + nullSafeHashCode(key);
			result += 31 * result + nullSafeHashCode(modifiers);

			return result;
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

			PushOperatorBuilder that = (PushOperatorBuilder) obj;

			if (!getOuterType().equals(that.getOuterType())) {
				return false;
			}

			return nullSafeEquals(this.key, that.key) && nullSafeEquals(this.modifiers, that.modifiers);
		}

		private Update_bak getOuterType() {
			return Update_bak.this;
		}
	}

	/**
	 * Builder for creating {@code $addToSet} modifier.
	 * 

	 * @since 1.5
	 */
	public class AddToSetBuilder {

		private final String key;

		public AddToSetBuilder(String key) {
			this.key = key;
		}

		/**
		 * Propagates {@code $each} to {@code $addToSet}
		 * 
		 * @param values
		 * @return
		 */
		public Update_bak each(Object... values) {
			return Update_bak.this.addToSet(this.key, new Each(values));
		}

		/**
		 * Propagates {@link #value(Object)} to {@code $addToSet}
		 * 
		 * @param values
		 * @return
		 */
		public Update_bak value(Object value) {
			return Update_bak.this.addToSet(this.key, value);
		}
	}
}
