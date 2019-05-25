/*
 * Copyright (c) 2011 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.repository.support;

import java.io.Serializable;

import org.springframework.data.mapping.model.BeanWrapper;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.repository.query.SequoiadbEntityInformation;
import org.springframework.data.repository.core.support.AbstractEntityInformation;

/**
 * {@link SequoiadbEntityInformation} implementation using a {@link SequoiadbPersistentEntity} instance to lookup the necessary
 * information. Can be configured with a custom collection to be returned which will trump the one returned by the
 * {@link SequoiadbPersistentEntity} if given.
 * 

 */
public class MappingSequoiadbEntityInformation<T, ID extends Serializable> extends AbstractEntityInformation<T, ID>
		implements SequoiadbEntityInformation<T, ID> {

	private final SequoiadbPersistentEntity<T> entityMetadata;
	private final String customCollectionName;

	/**
	 * Creates a new {@link MappingSequoiadbEntityInformation} for the given {@link SequoiadbPersistentEntity}.
	 * 
	 * @param entity must not be {@literal null}.
	 */
	public MappingSequoiadbEntityInformation(SequoiadbPersistentEntity<T> entity) {
		this(entity, null);
	}

	/**
	 * Creates a new {@link MappingSequoiadbEntityInformation} for the given {@link SequoiadbPersistentEntity} and custom
	 * collection name.
	 * 
	 * @param entity must not be {@literal null}.
	 * @param customCollectionName
	 */
	public MappingSequoiadbEntityInformation(SequoiadbPersistentEntity<T> entity, String customCollectionName) {
		super(entity.getType());
		this.entityMetadata = entity;
		this.customCollectionName = customCollectionName;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.support.EntityInformation#getId(java.lang.Object)
	 */
	@SuppressWarnings("unchecked")
	public ID getId(T entity) {

		SequoiadbPersistentProperty idProperty = entityMetadata.getIdProperty();

		if (idProperty == null) {
			return null;
		}

		try {
			return (ID) BeanWrapper.create(entity, null).getProperty(idProperty);
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	/* (non-Javadoc)
	 * @see org.springframework.data.repository.support.EntityInformation#getIdType()
	 */
	@SuppressWarnings("unchecked")
	public Class<ID> getIdType() {
		return (Class<ID>) entityMetadata.getIdProperty().getType();
	}

	/* (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.SequoiadbEntityInformation#getCollectionName()
	 */
	public String getCollectionName() {
		return customCollectionName == null ? entityMetadata.getCollection() : customCollectionName;
	}

	/* (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.SequoiadbEntityInformation#getIdAttribute()
	 */
	public String getIdAttribute() {
		return entityMetadata.getIdProperty().getName();
	}
}
