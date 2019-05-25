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

import static org.springframework.data.mongodb.core.query.SerializationUtils.*;
import static org.springframework.data.mongodb.core.query.SerializationUtils.serializeToJsonSafely;
import static org.springframework.util.ObjectUtils.*;

import java.util.*;
import java.util.concurrent.TimeUnit;

import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Sort;
import org.springframework.data.domain.Sort.Order;
import org.springframework.data.mongodb.InvalidMongoDbApiUsageException;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.util.Assert;

/**
 * @author Thomas Risberg
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public class Query {

	private static final String RESTRICTED_TYPES_KEY = "_$RESTRICTED_TYPES";

	private final Set<Class<?>> restrictedTypes = new HashSet<Class<?>>();
	private final Map<String, CriteriaDefinition> criteria = new LinkedHashMap<String, CriteriaDefinition>();
	private Field fieldSpec;
	private Sort sort;
	private int skip = 0;
	private int limit = -1;
	private List<String> hintList;
	private int queryFlags;

	private Meta meta = new Meta();

	/**
	 * @brief Normally, query return bson object,
	 * when this flag is added, query return binary data stream
	 */
	public static final int FLG_QUERY_STRINGOUT = 0x00000001;

	/**
	 * @brief Force to use specified hint to query,
	 * if database have no index assigned by the hint, fail to query.
	 */
	public static final int FLG_QUERY_FORCE_HINT = 0x00000080;

	/**
	 * @brief Enable parallel sub query, each sub query will finish scanning diffent part of the data.
	 */
	public static final int FLG_QUERY_PARALLED = 0x00000100;

	/**
	 * @brief In general, query won't return data until cursor gets from database, when add this flag,
	 * return data in query response, it will be more high-performance.
	 */
	public static final int FLG_QUERY_WITH_RETURNDATA = 0x00000200;

	/**
	 * Static factory method to create a {@link Query} using the provided {@link CriteriaDefinition}.
	 * 
	 * @param criteriaDefinition must not be {@literal null}.
	 * @return
	 * @since 1.6
	 */
	public static Query query(CriteriaDefinition criteriaDefinition) {
		return new Query(criteriaDefinition);
	}

	public Query() {}

	/**
	 * Creates a new {@link Query} using the given {@link CriteriaDefinition}.
	 * 
	 * @param criteriaDefinition must not be {@literal null}.
	 * @since 1.6
	 */
	public Query(CriteriaDefinition criteriaDefinition) {
		addCriteria(criteriaDefinition);
	}

	/**
	 * Adds the given {@link CriteriaDefinition} to the current {@link Query}.
	 * 
	 * @param criteriaDefinition must not be {@literal null}.
	 * @return
	 * @since 1.6
	 */
	public Query addCriteria(CriteriaDefinition criteriaDefinition) {

		CriteriaDefinition existing = this.criteria.get(criteriaDefinition.getKey());
		String key = criteriaDefinition.getKey();

		if (existing == null) {
			this.criteria.put(key, criteriaDefinition);
		} else {
			throw new InvalidMongoDbApiUsageException("Due to limitations of the org.springframework.data.mongodb.assist.BasicDBObject, "
					+ "you can't add a second '" + key + "' criteria. " + "Query already contains '"
					+ existing.getCriteriaObject() + "'.");
		}

		return this;
	}

	public Field fields() {
		if (fieldSpec == null) {
			this.fieldSpec = new Field();
		}
		return this.fieldSpec;
	}

	/**
	 * Set number of documents to skip before returning results.
	 * 
	 * @param skip
	 * @return
	 */
	public Query skip(int skip) {
		this.skip = skip;
		return this;
	}

	/**
	 * Limit the number of returned documents to {@code limit}.
	 *
	 * @param limit     return the specified amount of documents,
	 *                   when limit is 0, return nothing,
	 *                   when limit is -1, return all the documents
	 * @return
	 */
	public Query limit(int limit) {
		this.limit = limit;
		return this;
	}

	/**
	 * Configures the query to use the given hint when being executed.
	 *
	 * @param firstIndexName   the name of the index. when firstIndexName is null, executing table scan.
	 *                          when firstIndexName is an empty string, the database will determine the hint by itself.
	 * @param otherIndexNames  the names of indexes. when offering several hints, database will
	 *                          choose the best one of them to use.
	 * @return
	 */
	public Query withHint(String firstIndexName, String... otherIndexNames) {

		if (firstIndexName == null) {
            this.hintList = new ArrayList<String>();
		} else if ("".equals(firstIndexName.trim())) {
            this.hintList = null;
		} else {
            this.hintList = new ArrayList<String>();
            this.hintList.add(firstIndexName);
            if (otherIndexNames != null && otherIndexNames.length > 0) {
                this.hintList.addAll(Arrays.asList(otherIndexNames));
            }
        }
		return this;
	}

	/**
	 * Specify the query flags
	 * @param flags     the query flags, default to be 0. Please see the definition
	 *                   of follow flags for more detail. Usage:
	 *                   e.g. set ( Query.FLG_QUERY_FORCE_HINT | Query.FLG_QUERY_WITH_RETURNDATA ) to param flag
	 *                   <ul>
	 *                   <li>Query.FLG_QUERY_STRINGOUT
	 *                   <li>Query.FLG_QUERY_FORCE_HINT
	 *                   <li>Query.FLG_QUERY_PARALLED
	 *                   <li>Query.FLG_QUERY_WITH_RETURNDATA
	 *                   </ul>
	 * @return
	 */
	public  Query queryFlags(int flags) {
		this.queryFlags = flags;
		return this;
	}

	/**
	 * Sets the given pagination information on the {@link Query} instance. Will transparently set {@code skip} and
	 * {@code limit} as well as applying the {@link Sort} instance defined with the {@link Pageable}.
	 * 
	 * @param pageable
	 * @return
	 */
	public Query with(Pageable pageable) {

		if (pageable == null) {
			return this;
		}

		this.limit = pageable.getPageSize();
		this.skip = pageable.getOffset();

		return with(pageable.getSort());
	}

	/**
	 * Adds a {@link Sort} to the {@link Query} instance.
	 * 
	 * @param sort
	 * @return
	 */
	public Query with(Sort sort) {

		if (sort == null) {
			return this;
		}

		for (Order order : sort) {
			if (order.isIgnoreCase()) {
				throw new IllegalArgumentException(String.format("Gven sort contained an Order for %s with ignore case! "
						+ "MongoDB does not support sorting ignoreing case currently!", order.getProperty()));
			}
		}

		if (this.sort == null) {
			this.sort = sort;
		} else {
			this.sort = this.sort.and(sort);
		}

		return this;
	}

	/**
	 * @return the restrictedTypes
	 */
	public Set<Class<?>> getRestrictedTypes() {
		return restrictedTypes == null ? Collections.<Class<?>> emptySet() : restrictedTypes;
	}

	/**
	 * Restricts the query to only return documents instances that are exactly of the given types.
	 *
	 * @param type may not be {@literal null}
	 * @param additionalTypes may not be {@literal null}
	 * @return
	 */
	public Query restrict(Class<?> type, Class<?>... additionalTypes) {

		Assert.notNull(type, "Type must not be null!");
		Assert.notNull(additionalTypes, "AdditionalTypes must not be null");

		restrictedTypes.add(type);
		for (Class<?> additionalType : additionalTypes) {
			restrictedTypes.add(additionalType);
		}

		return this;
	}

	public DBObject getQueryObject() {

		DBObject dbo = new BasicDBObject();

		for (String k : criteria.keySet()) {
			CriteriaDefinition c = criteria.get(k);
			DBObject cl = c.getCriteriaObject();
			dbo.putAll(cl);
		}

		if (!restrictedTypes.isEmpty()) {
			dbo.put(RESTRICTED_TYPES_KEY, getRestrictedTypes());
		}

		return dbo;
	}

	public DBObject getFieldsObject() {
		return this.fieldSpec == null ? null : fieldSpec.getFieldsObject();
	}

	public DBObject getSortObject() {

		if (this.sort == null) {
			return null;
		}

		DBObject dbo = new BasicDBObject();

		for (org.springframework.data.domain.Sort.Order order : this.sort) {
			dbo.put(order.getProperty(), order.isAscending() ? 1 : -1);
		}

		return dbo;
	}

	public DBObject getHintObject() {
        DBObject dbo = new BasicDBObject();
		if (this.hintList == null) {
            ;
		} else if (this.hintList.size() == 0) {
            dbo.put("", null);
        } else {
		    for(int i = 0; i < this.hintList.size(); i++) {
		        dbo.put("" + i, this.hintList.get(i));
            }
        }

		return dbo;
	}

	/**
	 * Get the number of documents to skip.
	 * 
	 * @return
	 */
	public int getSkip() {
		return this.skip;
	}

	/**
	 * Get the maximum number of documents to be return.
	 * 
	 * @return
	 */
	public int getLimit() {
		return this.limit;
	}

	/**
	 * @return
	 */
	public String getHint() {
	    if (hintList == null || hintList.size() == 0) {
	        return "";
        } else {
	        return hintList.get(0);
        }
	}

	/**
	 * @return
	 */
	public int getQueryFlags() {
		return this.queryFlags;
	}








	protected List<CriteriaDefinition> getCriteria() {
		return new ArrayList<CriteriaDefinition>(this.criteria.values());
	}

	/*
	 * (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		return String.format("Query: %s, Fields: %s, Sort: %s, Hint: %s, Limit: %d, Skip: %d, Flags: %d",
                serializeToJsonSafely(getQueryObject()),
				serializeToJsonSafely(getFieldsObject()),
                serializeToJsonSafely(getSortObject()),
                serializeToJsonSafely(getHintObject()),
                this.limit, this.skip, this.queryFlags);
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

		Query that = (Query) obj;

		boolean criteriaEqual = this.criteria.equals(that.criteria);
		boolean fieldsEqual = this.fieldSpec == null ? that.fieldSpec == null : this.fieldSpec.equals(that.fieldSpec);
		boolean sortEqual = this.sort == null ? that.sort == null : this.sort.equals(that.sort);
		boolean hintEqual = this.hintList == null ? that.hintList == null : this.hintList.equals(that.hintList);
		boolean skipEqual = this.skip == that.skip;
		boolean limitEqual = this.limit == that.limit;
		boolean queryFlag = this.queryFlags == that.queryFlags;
		boolean metaEqual = nullSafeEquals(this.meta, that.meta);

		return criteriaEqual && fieldsEqual && sortEqual && hintEqual && skipEqual && limitEqual && queryFlag && metaEqual;
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {

		int result = 17;

		result += 31 * criteria.hashCode();
		result += 31 * nullSafeHashCode(fieldSpec);
		result += 31 * nullSafeHashCode(sort);
		result += 31 * nullSafeHashCode(hintList);
		result += 31 * skip;
		result += 31 * limit;
		result += 31 * queryFlags;
		result += 31 * nullSafeHashCode(meta);

		return result;
	}

	/**
	 * Returns whether the given key is the one used to hold the type restriction information.
	 * 
	 * @deprecated don't call this method as the restricted type handling will undergo some significant changes going
	 *             forward.
	 * @param key
	 * @return
	 */
	@Deprecated
	public static boolean isRestrictedTypeKey(String key) {
		return RESTRICTED_TYPES_KEY.equals(key);
	}
}
