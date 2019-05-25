/*
 * Copyright 2010-2013 the original author or authors.
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
package org.springframework.data.mongodb.core.query;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.util.Assert;
import org.springframework.util.ObjectUtils;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * @author Thomas Risberg
 * @author Oliver Gierke
 * @author Patryk Wasik
 */
public class Field_bak {

	private final Map<String, Integer> criteria = new HashMap<String, Integer>();
	private final Map<String, Object> slices = new HashMap<String, Object>();
	private final Map<String, Criteria> elemMatchs = new HashMap<String, Criteria>();
	private String postionKey;
	private int positionValue;
	private boolean include;


	public Field_bak include(String key) {
		include = true;
		criteria.put(key, Integer.valueOf(1));
		return this;
	}

	public Field_bak exclude(String key) {
		include = false;
		criteria.put(key, Integer.valueOf(0));
		return this;
	}

	public Field_bak slice(String key, int size) {
		slices.put(key, Integer.valueOf(size));
		return this;
	}

	public Field_bak slice(String key, int offset, int size) {
		slices.put(key, new Integer[] { Integer.valueOf(offset), Integer.valueOf(size) });
		return this;
	}

	public Field_bak elemMatch(String key, Criteria elemMatchCriteria) {
		elemMatchs.put(key, elemMatchCriteria);
		return this;
	}

	/**
	 * The array field must appear in the query. Only one positional {@code $} operator can appear in the projection and
	 * only one array field can appear in the query.
	 * 
	 * @param field query array field, must not be {@literal null} or empty.
	 * @param value
	 * @return
	 */
	public Field_bak position(String field, int value) {

		Assert.hasText(field, "DocumentField must not be null or empty!");

		postionKey = field;
		positionValue = value;

		return this;
	}

	public DBObject getFieldsObject() {

		DBObject dbo = new BasicDBObject();

		if (criteria.size() > 0) {
			if (include) {
				if (!criteria.containsKey("_id")) {
					criteria.put("_id", Integer.valueOf(1));
				}
			}
			for (String k : criteria.keySet()) {
				DBObject newValue = new BasicDBObject();
				newValue.put("$include", criteria.get(k));
				dbo.put(k, newValue);
			}
		}

		for (String k : slices.keySet()) {
			dbo.put(k, new BasicDBObject("$slice", slices.get(k)));
		}

		for (Entry<String, Criteria> entry : elemMatchs.entrySet()) {
			DBObject dbObject = new BasicDBObject("$elemMatch", entry.getValue().getCriteriaObject());
			dbo.put(entry.getKey(), dbObject);
		}

		if (postionKey != null) {
			dbo.put(postionKey + ".$", positionValue);
		}

		return dbo;
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object object) {

		if (this == object) {
			return true;
		}

		if (!(object instanceof Field_bak)) {
			return false;
		}

		Field_bak that = (Field_bak) object;

		if (!this.criteria.equals(that.criteria)) {
			return false;
		}

		if (!this.slices.equals(that.slices)) {
			return false;
		}

		if (!this.elemMatchs.equals(that.elemMatchs)) {
			return false;
		}

		boolean samePositionKey = this.postionKey == null ? that.postionKey == null : this.postionKey
				.equals(that.postionKey);
		boolean samePositionValue = this.positionValue == that.positionValue;

		return samePositionKey && samePositionValue;
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {

		int result = 17;

		result += 31 * ObjectUtils.nullSafeHashCode(this.criteria);
		result += 31 * ObjectUtils.nullSafeHashCode(this.elemMatchs);
		result += 31 * ObjectUtils.nullSafeHashCode(this.slices);
		result += 31 * ObjectUtils.nullSafeHashCode(this.postionKey);
		result += 31 * ObjectUtils.nullSafeHashCode(this.positionValue);

		return result;
	}
}
