/*
 * Copyright 2011-2012 the original author or authors.
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
package org.springframework.data.sequoiadb.core;

import org.bson.BSONObject;
import org.springframework.util.Assert;


import org.springframework.data.sequoiadb.assist.WriteConcern;

/**
 * Represents an action taken against the collection. Used by {@link WriteConcernResolver} to determine a custom
 * {@link WriteConcern} based on this information.
 * <ul>
 * <li>INSERT, SAVE have null query</li>
 * <li>REMOVE has null document</li>
 * <li>INSERT_LIST has null entityType, document, and query</li>
 * </ul>
 * 


 */
public class SequoiadbAction {

	private final String collectionName;
	private final WriteConcern defaultWriteConcern;
	private final Class<?> entityType;
	private final SequoiadbActionOperation sequoiadbActionOperation;
	private final BSONObject query;
	private final BSONObject document;

	/**
	 * Create an instance of a {@link SequoiadbAction}.
	 * 
	 * @param defaultWriteConcern the default write concern.
	 * @param sequoiadbActionOperation action being taken against the collection
	 * @param collectionName the collection name, must not be {@literal null} or empty.
	 * @param entityType the POJO that is being operated against
	 * @param document the converted BSONObject from the POJO or Spring Update object
	 * @param query the converted DBOjbect from the Spring Query object
	 */
	public SequoiadbAction(WriteConcern defaultWriteConcern, SequoiadbActionOperation sequoiadbActionOperation,
						   String collectionName, Class<?> entityType, BSONObject document, BSONObject query) {

		Assert.hasText(collectionName, "Collection name must not be null or empty!");

		this.defaultWriteConcern = defaultWriteConcern;
		this.sequoiadbActionOperation = sequoiadbActionOperation;
		this.collectionName = collectionName;
		this.entityType = entityType;
		this.query = query;
		this.document = document;
	}

	public String getCollectionName() {
		return collectionName;
	}

	public WriteConcern getDefaultWriteConcern() {
		return defaultWriteConcern;
	}

	/**
	 * @deprecated use {@link #getEntityType()} instead.
	 */
	@Deprecated
	public Class<?> getEntityClass() {
		return entityType;
	}

	public Class<?> getEntityType() {
		return entityType;
	}

	public SequoiadbActionOperation getSequoiadbActionOperation() {
		return sequoiadbActionOperation;
	}

	public BSONObject getQuery() {
		return query;
	}

	public BSONObject getDocument() {
		return document;
	}

}
