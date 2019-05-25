/*
 * Copyright 2012 the original author or authors.
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
package org.springframework.data.sequoiadb.repository.query;

import org.springframework.util.Assert;

/**
 * Bean based implementation of {@link SequoiadbEntityMetadata}.
 * 

 */
class SimpleSequoiadbEntityMetadata<T> implements SequoiadbEntityMetadata<T> {

	private final Class<T> type;
	private final String collectionName;

	/**
	 * Creates a new {@link SimpleSequoiadbEntityMetadata} using the given type and collection name.
	 * 
	 * @param type must not be {@literal null}.
	 * @param collectionName must not be {@literal null} or empty.
	 */
	public SimpleSequoiadbEntityMetadata(Class<T> type, String collectionName) {

		Assert.notNull(type, "Type must not be null!");
		Assert.hasText(collectionName, "Collection name must not be null or empty!");

		this.type = type;
		this.collectionName = collectionName;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.core.EntityMetadata#getJavaType()
	 */
	public Class<T> getJavaType() {
		return type;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.query.SequoiadbEntityMetadata#getCollectionName()
	 */
	public String getCollectionName() {
		return collectionName;
	}
}
