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
import org.bson.util.JSON;
import org.bson.BasicBSONObject;


import org.springframework.data.sequoiadb.assist.Helper;

/**
 * Custom {@link Query} implementation to setup a basic query from some arbitrary JSON query string.
 * 



 */
public class BasicQuery extends Query {

	private final BSONObject queryObject;
	private BSONObject fieldsObject;
	private BSONObject sortObject;

	public BasicQuery(String query) {
		this(new BasicBSONObject((BasicBSONObject)JSON.parse(query)));
	}

	public BasicQuery(BSONObject queryObject) {
		this(queryObject, null);
	}

	public BasicQuery(String query, String fields) {
		this.queryObject = (BasicBSONObject)JSON.parse(query);
		this.fieldsObject = (BasicBSONObject)JSON.parse(fields);
	}

	public BasicQuery(BSONObject queryObject, BSONObject fieldsObject) {
		this.queryObject = queryObject;
		this.fieldsObject = fieldsObject;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.query.Query#addCriteria(org.springframework.data.sequoiadb.core.query.CriteriaDefinition)
	 */
	@Override
	public Query addCriteria(CriteriaDefinition criteria) {
		this.queryObject.putAll(criteria.getCriteriaObject());
		return this;
	}

	@Override
	public BSONObject getQueryObject() {
		return this.queryObject;
	}

	@Override
	public BSONObject getFieldsObject() {
		return fieldsObject;
	}

	@Override
	public BSONObject getSortObject() {

		BasicBSONObject result = new BasicBSONObject();
		if (sortObject != null) {
			result.putAll(sortObject);
		}

		BSONObject overrides = super.getSortObject();
		if (overrides != null) {
			result.putAll(overrides);
		}

		return result;
	}

	public void setSortObject(BSONObject sortObject) {
		this.sortObject = sortObject;
	}

	/**
	 * @since 1.6
	 * @param fieldsObject
	 */
	protected void setFieldsObject(BSONObject fieldsObject) {
		this.fieldsObject = fieldsObject;
	}
}
