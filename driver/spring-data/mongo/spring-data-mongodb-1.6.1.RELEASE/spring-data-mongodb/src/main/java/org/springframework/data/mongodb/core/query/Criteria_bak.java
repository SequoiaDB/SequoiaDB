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
package org.springframework.data.mongodb.core.query;

import org.bson.BSON;
import org.springframework.data.geo.Circle;
import org.springframework.data.geo.Point;
import org.springframework.data.geo.Shape;
import org.springframework.data.mongodb.InvalidMongoDbApiUsageException;
import org.springframework.data.mongodb.assist.BasicDBList;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.core.geo.Sphere;
import org.springframework.util.Assert;
import org.springframework.util.CollectionUtils;
import org.springframework.util.ObjectUtils;
import org.springframework.util.StringUtils;

import java.util.*;
import java.util.regex.Pattern;

import static org.springframework.util.ObjectUtils.nullSafeHashCode;

/**
 * Central class for creating queries. It follows a fluent API style so that you can easily chain together multiple
 * criteria. Static import of the 'Criteria.where' method will improve readability.
 * 
 * @author Thomas Risberg
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public class Criteria_bak implements CriteriaDefinition {

	/**
	 * Custom "not-null" object as we have to be able to work with {@literal null} values as well.
	 */
	private static final Object NOT_SET = new Object();

	private String key;
	private List<Criteria_bak> criteriaChain;
	private LinkedHashMap<String, Object> criteria = new LinkedHashMap<String, Object>();
	private Object isValue = NOT_SET;

	public Criteria_bak() {
		this.criteriaChain = new ArrayList<Criteria_bak>();
	}

	public Criteria_bak(String key) {
		this.criteriaChain = new ArrayList<Criteria_bak>();
		this.criteriaChain.add(this);
		this.key = key;
	}

	protected Criteria_bak(List<Criteria_bak> criteriaChain, String key) {
		this.criteriaChain = criteriaChain;
		this.criteriaChain.add(this);
		this.key = key;
	}

	/**
	 * Static factory method to create a Criteria using the provided key
	 * 
	 * @param key
	 * @return
	 */
	public static Criteria_bak where(String key) {
		return new Criteria_bak(key);
	}

	/**
	 * Static factory method to create a Criteria using the provided key
	 * 
	 * @return
	 */
	public Criteria_bak and(String key) {
		return new Criteria_bak(this.criteriaChain, key);
	}

	/**
	 * Creates a criterion using equality
	 * 
	 * @param o
	 * @return
	 */
	public Criteria_bak is(Object o) {

		if (!isValue.equals(NOT_SET)) {
			throw new InvalidMongoDbApiUsageException(
					"Multiple 'is' values declared. You need to use 'and' with multiple criteria");
		}

		if (lastOperatorWasNot()) {
			throw new InvalidMongoDbApiUsageException("Invalid query: 'not' can't be used with 'is' - use 'ne' instead.");
		}

		this.isValue = o;
		return this;
	}

	private boolean lastOperatorWasNot() {
		return this.criteria.size() > 0 && "$not".equals(this.criteria.keySet().toArray()[this.criteria.size() - 1]);
	}

	/**
	 * Creates a criterion using the {@literal $ne} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/ne/
	 * @param o
	 * @return
	 */
	public Criteria_bak ne(Object o) {
		criteria.put("$ne", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $lt} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/lt/
	 * @param o
	 * @return
	 */
	public Criteria_bak lt(Object o) {
		criteria.put("$lt", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $lte} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/lte/
	 * @param o
	 * @return
	 */
	public Criteria_bak lte(Object o) {
		criteria.put("$lte", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $gt} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/gt/
	 * @param o
	 * @return
	 */
	public Criteria_bak gt(Object o) {
		criteria.put("$gt", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $gte} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/gte/
	 * @param o
	 * @return
	 */
	public Criteria_bak gte(Object o) {
		criteria.put("$gte", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $in} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/in/
	 * @param o the values to match against
	 * @return
	 */
	public Criteria_bak in(Object... o) {
		if (o.length > 1 && o[1] instanceof Collection) {
			throw new InvalidMongoDbApiUsageException("You can only pass in one argument of type "
					+ o[1].getClass().getName());
		}
		criteria.put("$in", Arrays.asList(o));
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $in} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/in/
	 * @param c the collection containing the values to match against
	 * @return
	 */
	public Criteria_bak in(Collection<?> c) {
		criteria.put("$in", c);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $nin} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/nin/
	 * @param o
	 * @return
	 */
	public Criteria_bak nin(Object... o) {
		return nin(Arrays.asList(o));
	}

	/**
	 * Creates a criterion using the {@literal $nin} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/nin/
	 * @param o
	 * @return
	 */
	public Criteria_bak nin(Collection<?> o) {
		criteria.put("$nin", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $mod} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/mod/
	 * @param value
	 * @param remainder
	 * @return
	 */
	public Criteria_bak mod(Number value, Number remainder) {
		List<Object> l = new ArrayList<Object>();
		l.add(value);
		l.add(remainder);
		criteria.put("$mod", l);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $all} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/all/
	 * @param o
	 * @return
	 */
	public Criteria_bak all(Object... o) {
		return all(Arrays.asList(o));
	}

	/**
	 * Creates a criterion using the {@literal $all} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/all/
	 * @param o
	 * @return
	 */
	public Criteria_bak all(Collection<?> o) {
		criteria.put("$all", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $size} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/size/
	 * @param s
	 * @return
	 */
	public Criteria_bak size(int s) {
		criteria.put("$size", s);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $exists} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/exists/
	 * @param b
	 * @return
	 */
	public Criteria_bak exists(boolean b) {
		criteria.put("$exists", b);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $type} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/type/
	 * @param t
	 * @return
	 */
	public Criteria_bak type(int t) {
		criteria.put("$type", t);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $not} meta operator which affects the clause directly following
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/not/
	 * @return
	 */
	public Criteria_bak not() {
		return not(null);
	}

	/**
	 * Creates a criterion using the {@literal $not} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/not/
	 * @param value
	 * @return
	 */
	private Criteria_bak not(Object value) {
		criteria.put("$not", value);
		return this;
	}

	/**
	 * Creates a criterion using a {@literal $regex} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/regex/
	 * @param re
	 * @return
	 */
	public Criteria_bak regex(String re) {
		return regex(re, null);
	}

	/**
	 * Creates a criterion using a {@literal $regex} and {@literal $options} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/regex/
	 * @see http://docs.mongodb.org/manual/reference/operator/query/regex/#op._S_options
	 * @param re
	 * @param options
	 * @return
	 */
	public Criteria_bak regex(String re, String options) {
		return regex(toPattern(re, options));
	}

	/**
	 * Syntactical sugar for {@link #is(Object)} making obvious that we create a regex predicate.
	 * 
	 * @param pattern
	 * @return
	 */
	public Criteria_bak regex(Pattern pattern) {

		Assert.notNull(pattern);

		if (lastOperatorWasNot()) {
			return not(pattern);
		}

		this.isValue = pattern;
		return this;
	}

	private Pattern toPattern(String regex, String options) {
		Assert.notNull(regex);
		return Pattern.compile(regex, options == null ? 0 : BSON.regexFlags(options));
	}

	/**
	 * Creates a geospatial criterion using a {@literal $within $centerSphere} operation. This is only available for Mongo
	 * 1.7 and higher.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/geoWithin/
	 * @see http://docs.mongodb.org/manual/reference/operator/query/centerSphere/
	 * @param circle must not be {@literal null}
	 * @return
	 */
	public Criteria_bak withinSphere(Circle circle) {
		Assert.notNull(circle);
		criteria.put("$within", new GeoCommand(new Sphere(circle)));
		return this;
	}

	/**
	 * Creates a geospatial criterion using a {@literal $within} operation.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/geoWithin/
	 * @param shape
	 * @return
	 */
	public Criteria_bak within(Shape shape) {

		Assert.notNull(shape);
		criteria.put("$within", new GeoCommand(shape));
		return this;
	}

	/**
	 * Creates a geospatial criterion using a {@literal $near} operation.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/near/
	 * @param point must not be {@literal null}
	 * @return
	 */
	public Criteria_bak near(Point point) {
		Assert.notNull(point);
		criteria.put("$near", point);
		return this;
	}

	/**
	 * Creates a geospatial criterion using a {@literal $nearSphere} operation. This is only available for Mongo 1.7 and
	 * higher.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/nearSphere/
	 * @param point must not be {@literal null}
	 * @return
	 */
	public Criteria_bak nearSphere(Point point) {
		Assert.notNull(point);
		criteria.put("$nearSphere", point);
		return this;
	}

	/**
	 * Creates a geospatical criterion using a {@literal $maxDistance} operation, for use with $near
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/maxDistance/
	 * @param maxDistance
	 * @return
	 */
	public Criteria_bak maxDistance(double maxDistance) {
		criteria.put("$maxDistance", maxDistance);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $elemMatch} operator
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/elemMatch/
	 * @param c
	 * @return
	 */
	public Criteria_bak elemMatch(Criteria_bak c) {
		criteria.put("$elemMatch", c.getCriteriaObject());
		return this;
	}

	/**
	 * Creates an 'or' criteria using the $or operator for all of the provided criteria
	 * <p>
	 * Note that mongodb doesn't support an $or operator to be wrapped in a $not operator.
	 * <p>
	 * 
	 * @throws IllegalArgumentException if {@link #orOperator(Criteria_bak...)} follows a not() call directly.
	 * @param criteria
	 */
	public Criteria_bak orOperator(Criteria_bak... criteria) {
		BasicDBList bsonList = createCriteriaList(criteria);
		return registerCriteriaChainElement(new Criteria_bak("$or").is(bsonList));
	}

	/**
	 * Creates a 'nor' criteria using the $nor operator for all of the provided criteria.
	 * <p>
	 * Note that mongodb doesn't support an $nor operator to be wrapped in a $not operator.
	 * <p>
	 * 
	 * @throws IllegalArgumentException if {@link #norOperator(Criteria_bak...)} follows a not() call directly.
	 * @param criteria
	 */
	public Criteria_bak norOperator(Criteria_bak... criteria) {
		BasicDBList bsonList = createCriteriaList(criteria);
		return registerCriteriaChainElement(new Criteria_bak("$nor").is(bsonList));
	}

	/**
	 * Creates an 'and' criteria using the $and operator for all of the provided criteria.
	 * <p>
	 * Note that mongodb doesn't support an $and operator to be wrapped in a $not operator.
	 * <p>
	 * 
	 * @throws IllegalArgumentException if {@link #andOperator(Criteria_bak...)} follows a not() call directly.
	 * @param criteria
	 */
	public Criteria_bak andOperator(Criteria_bak... criteria) {
		BasicDBList bsonList = createCriteriaList(criteria);
		return registerCriteriaChainElement(new Criteria_bak("$and").is(bsonList));
	}

	private Criteria_bak registerCriteriaChainElement(Criteria_bak criteria) {

		if (lastOperatorWasNot()) {
			throw new IllegalArgumentException("operator $not is not allowed around criteria chain element: "
					+ criteria.getCriteriaObject());
		} else {
			criteriaChain.add(criteria);
		}
		return this;
	}

	public String getKey() {
		return this.key;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.query.CriteriaDefinition#getCriteriaObject()
	 */
	public DBObject getCriteriaObject() {

		if (this.criteriaChain.size() == 1) {
			return criteriaChain.get(0).getSingleCriteriaObject();
		} else if (CollectionUtils.isEmpty(this.criteriaChain) && !CollectionUtils.isEmpty(this.criteria)) {
			return getSingleCriteriaObject();
		} else {
			DBObject criteriaObject = new BasicDBObject();
			for (Criteria_bak c : this.criteriaChain) {
				DBObject dbo = c.getSingleCriteriaObject();
				for (String k : dbo.keySet()) {
					setValue(criteriaObject, k, dbo.get(k));
				}
			}
			return criteriaObject;
		}
	}

	protected DBObject getSingleCriteriaObject() {

		DBObject dbo = new BasicDBObject();
		boolean not = false;

		for (String k : this.criteria.keySet()) {
			Object value = this.criteria.get(k);
			if (not) {
				DBObject notDbo = new BasicDBObject();
				notDbo.put(k, value);
				dbo.put("$not", notDbo);
				not = false;
			} else {
				if ("$not".equals(k) && value == null) {
					not = true;
				} else {
					dbo.put(k, value);
				}
			}
		}

		if (!StringUtils.hasText(this.key)) {
			if (not) {
				return new BasicDBObject("$not", dbo);
			}
			return dbo;
		}

		DBObject queryCriteria = new BasicDBObject();

		if (!NOT_SET.equals(isValue)) {
			queryCriteria.put(this.key, this.isValue);
			queryCriteria.putAll(dbo);
		} else {
			queryCriteria.put(this.key, dbo);
		}

		return queryCriteria;
	}

	private BasicDBList createCriteriaList(Criteria_bak[] criteria) {
		BasicDBList bsonList = new BasicDBList();
		for (Criteria_bak c : criteria) {
			bsonList.add(c.getCriteriaObject());
		}
		return bsonList;
	}

	private void setValue(DBObject dbo, String key, Object value) {
		Object existing = dbo.get(key);
		if (existing == null) {
			dbo.put(key, value);
		} else {
			throw new InvalidMongoDbApiUsageException("Due to limitations of the org.springframework.data.mongodb.assist.BasicDBObject, "
					+ "you can't add a second '" + key + "' expression specified as '" + key + " : " + value + "'. "
					+ "Criteria already contains '" + key + " : " + existing + "'.");
		}
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

		if (obj == null || !getClass().equals(obj.getClass())) {
			return false;
		}

		Criteria_bak that = (Criteria_bak) obj;

		if (this.criteriaChain.size() != that.criteriaChain.size()) {
			return false;
		}

		for (int i = 0; i < this.criteriaChain.size(); i++) {

			Criteria_bak left = this.criteriaChain.get(i);
			Criteria_bak right = that.criteriaChain.get(i);

			if (!simpleCriteriaEquals(left, right)) {
				return false;
			}
		}

		return true;
	}

	private boolean simpleCriteriaEquals(Criteria_bak left, Criteria_bak right) {

		boolean keyEqual = left.key == null ? right.key == null : left.key.equals(right.key);
		boolean criteriaEqual = left.criteria.equals(right.criteria);
		boolean valueEqual = isEqual(left.isValue, right.isValue);

		return keyEqual && criteriaEqual && valueEqual;
	}

	/**
	 * Checks the given objects for equality. Handles {@link Pattern} and arrays correctly.
	 * 
	 * @param left
	 * @param right
	 * @return
	 */
	private boolean isEqual(Object left, Object right) {

		if (left == null) {
			return right == null;
		}

		if (left instanceof Pattern) {
			return right instanceof Pattern ? ((Pattern) left).pattern().equals(((Pattern) right).pattern()) : false;
		}

		return ObjectUtils.nullSafeEquals(left, right);
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {

		int result = 17;

		result += nullSafeHashCode(key);
		result += criteria.hashCode();
		result += nullSafeHashCode(isValue);

		return result;
	}
}
