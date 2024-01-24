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

import org.bson.util.JSON;
import org.bson.BasicBSONObject;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.Helper;

/**
 * Custom {@link Query} implementation to setup a basic query from some arbitrary JSON query string.
 * 
 * @author Thomas Risberg
 * @author Oliver Gierke
 * @author Christoph Strobl
 */
public class BasicQuery extends Query {

	private final DBObject queryObject;
	private DBObject fieldsObject;
	private DBObject sortObject;

	public BasicQuery(String query) {
		this(new BasicDBObject((BasicBSONObject)JSON.parse(query)));
	}

	public BasicQuery(DBObject queryObject) {
		this(queryObject, null);
	}

	public BasicQuery(String query, String fields) {
		this.queryObject = Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject)JSON.parse(query));
		this.fieldsObject = Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject)JSON.parse(fields));
	}

	public BasicQuery(DBObject queryObject, DBObject fieldsObject) {
		this.queryObject = queryObject;
		this.fieldsObject = fieldsObject;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.query.Query#addCriteria(org.springframework.data.mongodb.core.query.CriteriaDefinition)
	 */
	@Override
	public Query addCriteria(CriteriaDefinition criteria) {
		this.queryObject.putAll(criteria.getCriteriaObject());
		return this;
	}

	@Override
	public DBObject getQueryObject() {
		return this.queryObject;
	}

	@Override
	public DBObject getFieldsObject() {
		return fieldsObject;
	}

	@Override
	public DBObject getSortObject() {

		BasicDBObject result = new BasicDBObject();
		if (sortObject != null) {
			result.putAll(sortObject);
		}

		DBObject overrides = super.getSortObject();
		if (overrides != null) {
			result.putAll(overrides);
		}

		return result;
	}

	public void setSortObject(DBObject sortObject) {
		this.sortObject = sortObject;
	}

	/**
	 * @since 1.6
	 * @param fieldsObject
	 */
	protected void setFieldsObject(DBObject fieldsObject) {
		this.fieldsObject = fieldsObject;
	}
}
