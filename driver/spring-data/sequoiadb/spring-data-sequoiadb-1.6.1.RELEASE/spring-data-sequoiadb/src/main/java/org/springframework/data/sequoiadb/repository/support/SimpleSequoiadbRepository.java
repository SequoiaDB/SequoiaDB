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
package org.springframework.data.sequoiadb.repository.support;

import static org.springframework.data.sequoiadb.core.query.Criteria.*;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageImpl;
import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Sort;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.repository.SequoiadbRepository;
import org.springframework.data.sequoiadb.repository.query.SequoiadbEntityInformation;
import org.springframework.util.Assert;

/**
 * Repository base implementation for Sdb.
 * 


 */
public class SimpleSequoiadbRepository<T, ID extends Serializable> implements SequoiadbRepository<T, ID> {

	private final SequoiadbOperations sequoiadbOperations;
	private final SequoiadbEntityInformation<T, ID> entityInformation;

	/**
	 * Creates a ew {@link SimpleSequoiadbRepository} for the given {@link SequoiadbEntityInformation} and {@link SequoiadbTemplate}.
	 * 
	 * @param metadata must not be {@literal null}.
	 * @param template must not be {@literal null}.
	 */
	public SimpleSequoiadbRepository(SequoiadbEntityInformation<T, ID> metadata, SequoiadbOperations sequoiadbOperations) {

		Assert.notNull(sequoiadbOperations);
		Assert.notNull(metadata);

		this.entityInformation = metadata;
		this.sequoiadbOperations = sequoiadbOperations;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#save(java.lang.Object)
	 */
	public <S extends T> S save(S entity) {

		Assert.notNull(entity, "Entity must not be null!");

		sequoiadbOperations.save(entity, entityInformation.getCollectionName());
		return entity;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#save(java.lang.Iterable)
	 */
	public <S extends T> List<S> save(Iterable<S> entities) {

		Assert.notNull(entities, "The given Iterable of entities not be null!");

		List<S> result = new ArrayList<S>();

		for (S entity : entities) {
			save(entity);
			result.add(entity);
		}

		return result;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#findOne(java.io.Serializable)
	 */
	public T findOne(ID id) {
		Assert.notNull(id, "The given id must not be null!");
		return sequoiadbOperations.findById(id, entityInformation.getJavaType(), entityInformation.getCollectionName());
	}

	private Query getIdQuery(Object id) {
		return new Query(getIdCriteria(id));
	}

	private Criteria getIdCriteria(Object id) {
		return where(entityInformation.getIdAttribute()).is(id);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#exists(java.io.Serializable)
	 */
	public boolean exists(ID id) {

		Assert.notNull(id, "The given id must not be null!");
		return sequoiadbOperations.exists(getIdQuery(id), entityInformation.getJavaType(),
				entityInformation.getCollectionName());
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#count()
	 */
	public long count() {
		return sequoiadbOperations.getCollection(entityInformation.getCollectionName()).count();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#delete(java.io.Serializable)
	 */
	public void delete(ID id) {
		Assert.notNull(id, "The given id must not be null!");
		sequoiadbOperations.remove(getIdQuery(id), entityInformation.getJavaType(), entityInformation.getCollectionName());
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#delete(java.lang.Object)
	 */
	public void delete(T entity) {
		Assert.notNull(entity, "The given entity must not be null!");
		delete(entityInformation.getId(entity));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#delete(java.lang.Iterable)
	 */
	public void delete(Iterable<? extends T> entities) {

		Assert.notNull(entities, "The given Iterable of entities not be null!");

		for (T entity : entities) {
			delete(entity);
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#deleteAll()
	 */
	public void deleteAll() {
		sequoiadbOperations.remove(new Query(), entityInformation.getCollectionName());
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#findAll()
	 */
	public List<T> findAll() {
		return findAll(new Query());
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.CrudRepository#findAll(java.lang.Iterable)
	 */
	public Iterable<T> findAll(Iterable<ID> ids) {

		Set<ID> parameters = new HashSet<ID>();
		for (ID id : ids) {
			parameters.add(id);
		}

		return findAll(new Query(new Criteria(entityInformation.getIdAttribute()).in(parameters)));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.PagingAndSortingRepository#findAll(org.springframework.data.domain.Pageable)
	 */
	public Page<T> findAll(final Pageable pageable) {

		Long count = count();
		List<T> list = findAll(new Query().with(pageable));

		return new PageImpl<T>(list, pageable, count);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.PagingAndSortingRepository#findAll(org.springframework.data.domain.Sort)
	 */
	public List<T> findAll(Sort sort) {
		return findAll(new Query().with(sort));
	}

	private List<T> findAll(Query query) {

		if (query == null) {
			return Collections.emptyList();
		}

		return sequoiadbOperations.find(query, entityInformation.getJavaType(), entityInformation.getCollectionName());
	}

	/**
	 * Returns the underlying {@link SequoiadbOperations} instance.
	 * 
	 * @return
	 */
	protected SequoiadbOperations getSequoiadbOperations() {
		return this.sequoiadbOperations;
	}

	/**
	 * @return the entityInformation
	 */
	protected SequoiadbEntityInformation<T, ID> getEntityInformation() {
		return entityInformation;
	}

}
