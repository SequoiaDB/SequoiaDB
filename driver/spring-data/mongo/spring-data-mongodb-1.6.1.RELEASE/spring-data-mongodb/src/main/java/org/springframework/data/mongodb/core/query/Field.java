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

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.data.mongodb.InvalidMongoDbApiUsageException;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.util.Assert;
import org.springframework.util.ObjectUtils;

import java.util.*;

/**
 * @author
 */
public class Field {

	private final Map<String, Object> fields = new LinkedHashMap<String, Object>();
	private String key;

	public Field select(String key, Object defaultValue) {
		this.key = key;
		fields.put(key, defaultValue);
		return this;
	}

	public Field include(String key, int include) {
		this.key = key;
		if (include == 0) {
			fields.put(key, new BasicDBObject("$include", Integer.valueOf(0)));
		} else {
			fields.put(key, new BasicDBObject("$include", Integer.valueOf(1)));
		}
		return this;
	}

	public Field defaultValue(String key, Object defaultValue) {
		this.key = key;
		fields.put(key, new BasicDBObject("$default", defaultValue));
		return this;
	}

	public Field elemMatch(String key, Criteria elemMatchCriteria) {
		Assert.notNull(elemMatchCriteria, "Criteria must not be null or empty!");
		this.key = key;
		fields.put(key, new BasicDBObject("$elemMatch", elemMatchCriteria.getCriteriaObject()));
		return this;
	}

	public Field elemMatchOne(String key, Criteria elemMatchCriteria) {
		Assert.notNull(elemMatchCriteria, "Criteria must not be null or empty!");
		this.key = key;
		fields.put(key, new BasicDBObject("$elemMatchOne", elemMatchCriteria.getCriteriaObject()));
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $abs} operator.
	 *
	 * @return
	 */
	public Field abs() {
		return funcOperator("$abs");
	}

	/**
	 * Creates a criterion using the {@literal $ceiling} operator.
	 *
	 * @return
	 */
	public Field ceiling() {
		return funcOperator("$ceiling");
	}

	/**
	 * Creates a criterion using the {@literal $floor} operator.
	 *
	 * @return
	 */
	public Field floor() {
		return funcOperator("$floor");
	}

	/**
	 * Creates a criterion using the {@literal $mod} operator.
	 *
	 * @param num the number to mod to
	 * @return
	 */
	public Field mod(Number num) {
		return funcOperator("$mod", num);
	}

	/**
	 * Creates a criterion using the {@literal $add} operator.
	 *
	 * @param num the number to add
	 * @return
	 */
	public Field add(Number num) {
		return funcOperator("$add", num);
	}

	/**
	 * Creates a criterion using the {@literal $subtract} operator.
	 *
	 * @return
	 */
	public Field subtract(Number num) {
		return funcOperator("$subtract", num);
	}

	/**
	 * Creates a criterion using the {@literal $multiply} operator.
	 *
	 * @return
	 */
	public Field multiply(Number num) {
		return funcOperator("$multiply", num);
	}

	/**
	 * Creates a criterion using the {@literal $divide} operator.
	 *
	 * @return
	 */
	public Field divide(Number num) {
		return funcOperator("$divide", num);
	}

	/**
	 * Creates a criterion using the {@literal $substr} operator.
	 *
	 * @param startIndex 	the start index, begin at 0
	 * @param  length the length of sub string
	 * @return
	 */
	public Field substr(int startIndex, int length) {
		return funcOperator("$substr", Arrays.asList(startIndex, length));
	}

	/**
	 * Creates a criterion using the {@literal $strlen} operator.
	 *
	 * @return
	 */
	public Field strlen() {
		return funcOperator("$strlen");
	}

	/**
	 * Creates a criterion using the {@literal $lower} operator.
	 *
	 * @return
	 */
	public Field lower() {
		return funcOperator("$lower");
	}

	/**
	 * Creates a criterion using the {@literal $upper} operator.
	 *
	 * @return
	 */
	public Field upper() {
		return funcOperator("$upper");
	}

	/**
	 * Creates a criterion using the {@literal $ltrim} operator.
	 *
	 * @return
	 */
	public Field ltrim() {
		return funcOperator("$ltrim");
	}

	/**
	 * Creates a criterion using the {@literal $rtrim} operator.
	 *
	 * @return
	 */
	public Field rtrim() {
		return funcOperator("$rtrim");
	}

	/**
	 * Creates a criterion using the {@literal $trim} operator.
	 *
	 * @return
	 */
	public Field trim() {
		return funcOperator("$trim");
	}

	/**
	 * Creates a criterion using the {@literal $cast} operator.
	 *
	 * @return
	 */
	public Field cast(Object type) {
		return funcCastOperator(type);
	}

	/**
	 * Creates a criterion using the {@literal $size} operator.
	 *
	 * @return
	 */
	public Field size() {
		return funcOperator("$size");
	}

	/**
	 * Creates a criterion using the {@literal $type} operator.
	 *
	 * @param t the format of return value type
	 * @return
	 */
	public Field type(int t) {
		if (t != 1 && t != 2) {
			throw new InvalidMongoDbApiUsageException("the input should be 1 or 2");
		}
		return funcOperator("$type", t);
	}

	/**
	 * Creates a criterion using the {@literal $slice} operator.
	 *
	 * @param startIndex the start index
	 * @param length the count of value to get
	 * @return
	 */
	public Field slice(int startIndex, int length) {
		return funcOperator("$slice", Arrays.asList(startIndex, length));
	}

	public DBObject getFieldsObject() {
		DBObject dbo = new BasicDBObject();
		for (String k : fields.keySet()) {
			dbo.put(k, fields.get(k));
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

		if (!(object instanceof Field)) {
			return false;
		}

		Field that = (Field) object;
		if (!this.fields.equals(that.fields)) {
			return false;
		}
		return true;
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		int result = 17;
		result += 31 * ObjectUtils.nullSafeHashCode(this.fields);
		return result;
	}

	private boolean isCommandOperation(DBObject obj) {
		Assert.notNull(obj);
		for(String key : obj.keySet()) {
			if(key.startsWith("$")) {
				return true;
			}
		}
		return false;
	}

	private Field funcCastOperator(Object type) {
		if (fields.containsKey(this.key)) {
			Object obj = fields.get(this.key);
			DBObject doc;
			if (obj instanceof DBObject && isCommandOperation((BasicDBObject)obj)) {
				doc = (BasicDBObject)obj;
			} else {
				doc = new BasicDBObject();
			}
			// add contents
			if (type instanceof Integer) {
				doc.put("$cast", (Integer)type);
			} else if (type instanceof String) {
				doc.put("$cast", type);
			} else {
				throw new InvalidMongoDbApiUsageException("expect string or int type value");
			}
			fields.put(this.key, doc);
		}
		return this;
	}

	private Field funcOperator(String operator, List<Integer> list) {
		if (fields.containsKey(this.key)) {
			Object obj = fields.get(this.key);
			if (obj instanceof DBObject && isCommandOperation((BasicDBObject)obj)) {
				DBObject doc = (BasicDBObject)obj;
				doc.put(operator, list);
			} else {
				fields.put(this.key, new BasicDBObject(operator, list));
			}
		}
		return this;
	}

	private Field funcOperator(String operator, Number num) {
		if (fields.containsKey(this.key)) {
			Object obj = fields.get(this.key);
			if (obj instanceof DBObject && isCommandOperation((BasicDBObject)obj)) {
				DBObject doc = (BasicDBObject)obj;
				doc.put(operator, num);
			} else {
				fields.put(this.key, new BasicDBObject(operator, num));
			}
		}
		return this;
	}

	private Field funcOperator(String operator) {
		return funcOperator(operator, 1);
	}
}
