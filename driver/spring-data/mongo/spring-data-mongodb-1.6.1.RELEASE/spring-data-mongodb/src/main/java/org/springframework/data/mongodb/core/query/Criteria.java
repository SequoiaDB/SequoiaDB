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

import static org.springframework.util.ObjectUtils.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.regex.Pattern;

import org.bson.BSON;
import org.springframework.data.geo.Circle;
import org.springframework.data.geo.Point;
import org.springframework.data.geo.Shape;
import org.springframework.data.mongodb.InvalidMongoDbApiUsageException;
import org.springframework.data.mongodb.core.geo.Sphere;
import org.springframework.util.Assert;
import org.springframework.util.CollectionUtils;
import org.springframework.util.ObjectUtils;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.BasicDBList;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;

/**
 * Central class for creating queries. It follows a fluent API style so that you can easily chain together multiple
 * criteria. Static import of the 'Criteria.where' method will improve readability.
 * 
 * @author Thomas Risberg
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public class Criteria implements CriteriaDefinition {

	/**
	 * Custom "not-null" object as we have to be able to work with {@literal null} values as well.
	 */
	private static final Object NOT_SET = new Object();

	private String key;
	private List<Criteria> criteriaChain;
	private LinkedHashMap<String, Object> criteria = new LinkedHashMap<String, Object>();
	private Object isValue = NOT_SET;

	public Criteria() {
		this.criteriaChain = new ArrayList<Criteria>();
	}

	public Criteria(String key) {
		this.criteriaChain = new ArrayList<Criteria>();
		this.criteriaChain.add(this);
		this.key = key;
	}

	protected Criteria(List<Criteria> criteriaChain, String key) {
		this.criteriaChain = criteriaChain;
		this.criteriaChain.add(this);
		this.key = key;
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
            for (Criteria c : this.criteriaChain) {
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

	/**
	 * Static factory method to create a Criteria using the provided key
	 * 
	 * @param key
	 * @return
	 */
	public static Criteria where(String key) {
		return new Criteria(key);
	}

    /**
     * Creates a criterion using equality
     *
     * @param o
     * @return
     */
    public Criteria is(Object o) {

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

	/**
	 * Static factory method to create a Criteria using the provided key
	 * 
	 * @return
	 */
	public Criteria and(String key) {
		return new Criteria(this.criteriaChain, key);
	}

	/**
	 * Creates a criterion using the {@literal $gt} operator.
	 *
	 * @see http://docs.mongodb.org/manual/reference/operator/query/gt/
	 * @param o
	 * @return
	 */
	public Criteria gt(Object o) {
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
	public Criteria gte(Object o) {
		criteria.put("$gte", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $lt} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/lt/
	 * @param o
	 * @return
	 */
	public Criteria lt(Object o) {
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
	public Criteria lte(Object o) {
		criteria.put("$lte", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $ne} operator.
	 *
	 * @see http://docs.mongodb.org/manual/reference/operator/query/ne/
	 * @param o
	 * @return
	 */
	public Criteria ne(Object o) {
		criteria.put("$ne", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $et} operator.
	 *
	 * @see http://docs.mongodb.org/manual/reference/operator/query/et/
	 * @param o
	 * @return
	 */
	public Criteria et(Object o) {
		criteria.put("$et", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $in} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/in/
	 * @param o the values to match against
	 * @return
	 */
	public Criteria in(Object... o) {
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
	public Criteria in(Collection<?> c) {
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
	public Criteria nin(Object... o) {
		return nin(Arrays.asList(o));
	}

	/**
	 * Creates a criterion using the {@literal $nin} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/nin/
	 * @param o
	 * @return
	 */
	public Criteria nin(Collection<?> o) {
		criteria.put("$nin", o);
		return this;
	}

	/**
	 * Creates an 'or' criteria using the $or operator for all of the provided criteria
	 * <p>
	 * Note that mongodb doesn't support an $or operator to be wrapped in a $not operator.
	 * <p>
	 *
	 * @throws IllegalArgumentException if {@link #orOperator(Criteria...)} follows a not() call directly.
	 * @param criteria
	 */
	public Criteria orOperator(Criteria... criteria) {
		BasicDBList bsonList = createCriteriaList(criteria);
		return registerCriteriaChainElement(new Criteria("$or").is(bsonList));
	}

	/**
	 * Creates an 'and' criteria using the $and operator for all of the provided criteria.
	 * <p>
	 * Note that mongodb doesn't support an $and operator to be wrapped in a $not operator.
	 * <p>
	 *
	 * @throws IllegalArgumentException if {@link #andOperator(Criteria...)} follows a not() call directly.
	 * @param criteria
	 */
	public Criteria andOperator(Criteria... criteria) {
		BasicDBList bsonList = createCriteriaList(criteria);
		return registerCriteriaChainElement(new Criteria("$and").is(bsonList));
	}

	/**
	 * Creates an 'not' criteria using the $not operator for all of the provided criteria.
	 *
	 * @throws IllegalArgumentException if {@link #andOperator(Criteria...)} follows a not() call directly.
	 * @param criteria
	 */
	public Criteria notOperator(Criteria... criteria) {
		BasicDBList bsonList = createCriteriaList(criteria);
		return registerCriteriaChainElement(new Criteria("$not").is(bsonList));
	}

	/**
	 * Creates a criterion using the {@literal $exists} operator.
	 *
	 * @see http://docs.mongodb.org/manual/reference/operator/query/exists/
	 * @param b
	 * @return
	 */
	public Criteria exists(int b) {
		criteria.put("$exists", b);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $isnull} operator.
	 *
	 * @param n
	 * @return
	 */
	public Criteria isNull(int n) {
		criteria.put("$isnull", n);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $field} operator.
	 *
	 * @param f
	 * @return
	 */
	public Criteria field(String f) {
		criteria.put("$field", f);
		return this;
	}

    /**
     * Creates a criterion using the {@literal $field} operator.
     *
     * @param operator the operator, e.g. "$gt", "$lt" and so on.
     * @param f
     * @return
     */
    public Criteria field(String operator, String f) {
        Assert.hasText(operator, "Hint must not be empty or null!");
        criteria.put(operator,new BasicDBObject("$field", f));
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
	public Criteria mod(Number value, Number remainder) {
		List<Object> l = new ArrayList<Object>();
		l.add(value);
		l.add(remainder);
		criteria.put("$mod", l);
		return this;
	}

	/**
	 * Creates a criterion using a {@literal $regex} operator.
	 *
	 * @see http://docs.mongodb.org/manual/reference/operator/query/regex/
	 * @param re
	 * @return
	 */
	public Criteria regex(String re) {
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
	public Criteria regex(String re, String options) {
		return regex(toPattern(re, options));
	}

	/**
	 * Syntactical sugar for {@link #is(Object)} making obvious that we create a regex predicate.
	 *
	 * @param pattern
	 * @return
	 */
	public Criteria regex(Pattern pattern) {

		Assert.notNull(pattern);

		if (lastOperatorWasNot()) {
			return not(pattern);
		}

		this.isValue = pattern;
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $all} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/all/
	 * @param o
	 * @return
	 */
	public Criteria all(Object... o) {
		return all(Arrays.asList(o));
	}

	/**
	 * Creates a criterion using the {@literal $all} operator.
	 * 
	 * @see http://docs.mongodb.org/manual/reference/operator/query/all/
	 * @param o
	 * @return
	 */
	public Criteria all(Collection<?> o) {
		criteria.put("$all", o);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $elemMatch} operator
	 *
	 * @see http://docs.mongodb.org/manual/reference/operator/query/elemMatch/
	 * @param c
	 * @return
	 */
	public Criteria elemMatch(Criteria c) {
		criteria.put("$elemMatch", c.getCriteriaObject());
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $expand} operator.
	 *
	 * @return
	 */
	public Criteria expand() {
		criteria.put("$expand", 1);
		return this;
	}

    /**
     * Creates a criterion using the {@literal $returnMatch} operator.
     *
     * @param startIndex
     * @return
     */
    public Criteria returnMatch(int startIndex) {
        criteria.put("$returnMatch", startIndex);
        return this;
    }

	/**
	 * Creates a criterion using the {@literal $returnMatch} operator.
	 *
	 * @param startIndex
	 * @param length
	 * @return
	 */
	public Criteria returnMatch(int startIndex, int length) {
		criteria.put("$returnMatch", Arrays.asList(startIndex, length));
		return this;
	}

//	 //TODO: not add $ operation yet
//	public void $Operation() {}

	/**
	 * Creates a criterion using the {@literal $abs} operator.
	 *
	 * @return
	 */
	public Criteria abs() {
		criteria.put("$abs", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $ceiling} operator.
	 *
	 * @return
	 */
	public Criteria ceiling() {
		criteria.put("$ceiling", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $floor} operator.
	 *
	 * @return
	 */
	public Criteria floor() {
		criteria.put("$floor", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $mod} operator.
	 *
	 * @param num the number to mod to
	 * @return
	 */
	public Criteria mod(Number num) {
		criteria.put("$mod", num);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $add} operator.
	 *
	 * @param num the number to add
	 * @return
	 */
	public Criteria add(Number num) {
		criteria.put("$add", num);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $subtract} operator.
	 *
	 * @return
	 */
	public Criteria subtract(Number num) {
		criteria.put("$subtract", num);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $multiply} operator.
	 *
	 * @return
	 */
	public Criteria multiply(Number num) {
		criteria.put("$multiply", num);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $divide} operator.
	 *
	 * @return
	 */
	public Criteria divide(Number num) {
		criteria.put("$divide", num);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $substr} operator.
	 *
	 * @param startIndex 	the start index, begin at 0
	 * @param  length the length of sub string
	 * @return
	 */
	public Criteria substr(int startIndex, int length) {
		criteria.put("$substr", Arrays.asList(startIndex, length));
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $strlen} operator.
	 *
	 * @return
	 */
	public Criteria strlen() {
		criteria.put("$strlen", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $lower} operator.
	 *
	 * @return
	 */
	public Criteria lower() {
		criteria.put("$lower", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $upper} operator.
	 *
	 * @return
	 */
	public Criteria upper() {
		criteria.put("$upper", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $ltrim} operator.
	 *
	 * @return
	 */
	public Criteria ltrim() {
		criteria.put("$ltrim", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $rtrim} operator.
	 *
	 * @return
	 */
	public Criteria rtrim() {
		criteria.put("$rtrim",1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $trim} operator.
	 *
	 * @return
	 */
	public Criteria trim() {
		criteria.put("$trim", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $cast} operator.
	 *
	 * @return
	 */
	public Criteria cast(Object type) {
		if (type instanceof Integer) {
			criteria.put("$cast", (Integer)type);
		} else if (type instanceof String) {
			criteria.put("$cast", type);
		} else {
			throw new InvalidMongoDbApiUsageException("expect string or int type value");
		}
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $size} operator.
	 *
	 * @return
	 */
	public Criteria size() {
		criteria.put("$size", 1);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $type} operator.
	 *
	 * @param t the format of return value type
	 * @return
	 */
	public Criteria type(int t) {
		if (t != 1 && t != 2) {
			throw new InvalidMongoDbApiUsageException("the input should be 1 or 2");
		}
		criteria.put("$type", t);
		return this;
	}

	/**
	 * Creates a criterion using the {@literal $slice} operator.
	 *
	 * @param startIndex the start index
	 * @param length the count of value to get
	 * @return
	 */
	public Criteria slice(int startIndex, int length) {
		criteria.put("$slice", Arrays.asList(startIndex, length));
		return this;
	}


	private Criteria not(Object value) {
//		criteria.put("$not", value);
		return this;
	}

    private boolean lastOperatorWasNot() {
//		return this.criteria.size() > 0 && "$not".equals(this.criteria.keySet().toArray()[this.criteria.size() - 1]);
        return false;
    }

	private Pattern toPattern(String regex, String options) {
		Assert.notNull(regex);
		return Pattern.compile(regex, options == null ? 0 : BSON.regexFlags(options));
	}

	private Criteria registerCriteriaChainElement(Criteria criteria) {

		if (lastOperatorWasNot()) {
			throw new IllegalArgumentException("operator $not is not allowed around criteria chain element: "
					+ criteria.getCriteriaObject());
		} else {
			criteriaChain.add(criteria);
		}
		return this;
	}

	private BasicDBList createCriteriaList(Criteria[] criteria) {
		BasicDBList bsonList = new BasicDBList();
		for (Criteria c : criteria) {
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

	private boolean simpleCriteriaEquals(Criteria left, Criteria right) {

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

        Criteria that = (Criteria) obj;

        if (this.criteriaChain.size() != that.criteriaChain.size()) {
            return false;
        }

        for (int i = 0; i < this.criteriaChain.size(); i++) {

            Criteria left = this.criteriaChain.get(i);
            Criteria right = that.criteriaChain.get(i);

            if (!simpleCriteriaEquals(left, right)) {
                return false;
            }
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

		result += nullSafeHashCode(key);
		result += criteria.hashCode();
		result += nullSafeHashCode(isValue);

		return result;
	}
}
