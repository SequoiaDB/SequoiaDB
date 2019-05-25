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
package org.springframework.data.sequoiadb.repository.query;

import java.util.Collections;
import java.util.List;

import org.springframework.core.convert.ConversionService;
import org.springframework.core.convert.support.DefaultConversionService;
import org.springframework.data.domain.PageImpl;
import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Slice;
import org.springframework.data.domain.SliceImpl;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.GeoPage;
import org.springframework.data.geo.GeoResult;
import org.springframework.data.geo.GeoResults;
import org.springframework.data.geo.Point;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.query.NearQuery;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.repository.query.ParameterAccessor;
import org.springframework.data.repository.query.RepositoryQuery;
import org.springframework.data.util.TypeInformation;
import org.springframework.util.Assert;

import org.springframework.data.sequoiadb.assist.WriteResult;

/**
 * Base class for {@link RepositoryQuery} implementations for Sdb.
 * 



 */
public abstract class AbstractSequoiadbQuery implements RepositoryQuery {

	private static final ConversionService CONVERSION_SERVICE = new DefaultConversionService();

	private final SequoiadbQueryMethod method;
	private final SequoiadbOperations operations;

	/**
	 * Creates a new {@link AbstractSequoiadbQuery} from the given {@link SequoiadbQueryMethod} and {@link SequoiadbOperations}.
	 * 
	 * @param method must not be {@literal null}.
	 * @param operations must not be {@literal null}.
	 */
	public AbstractSequoiadbQuery(SequoiadbQueryMethod method, SequoiadbOperations operations) {

		Assert.notNull(operations);
		Assert.notNull(method);

		this.method = method;
		this.operations = operations;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.RepositoryQuery#getQueryMethod()
	 */
	public SequoiadbQueryMethod getQueryMethod() {
		return method;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.RepositoryQuery#execute(java.lang.Object[])
	 */
	public Object execute(Object[] parameters) {

		SequoiadbParameterAccessor accessor = new SequoiadbParametersParameterAccessor(method, parameters);
		Query query = createQuery(new ConvertingParameterAccessor(operations.getConverter(), accessor));

		applyQueryMetaAttributesWhenPresent(query);

		if (isDeleteQuery()) {
			return new DeleteExecution().execute(query);
		} else if (method.isGeoNearQuery() && method.isPageQuery()) {

			SequoiadbParameterAccessor countAccessor = new SequoiadbParametersParameterAccessor(method, parameters);
			Query countQuery = createCountQuery(new ConvertingParameterAccessor(operations.getConverter(), countAccessor));

			return new GeoNearExecution(accessor).execute(query, countQuery);
		} else if (method.isGeoNearQuery()) {
			return new GeoNearExecution(accessor).execute(query);
		} else if (method.isSliceQuery()) {
			return new SlicedExecution(accessor.getPageable()).execute(query);
		} else if (method.isCollectionQuery()) {
			return new CollectionExecution(accessor.getPageable()).execute(query);
		} else if (method.isPageQuery()) {
			return new PagedExecution(accessor.getPageable()).execute(query);
		} else {
			return new SingleEntityExecution(isCountQuery()).execute(query);
		}
	}

	private Query applyQueryMetaAttributesWhenPresent(Query query) {

		return query;
	}

	/**
	 * Creates a {@link Query} instance using the given {@link ConvertingParameterAccessor}. Will delegate to
	 * {@link #createQuery(ConvertingParameterAccessor)} by default but allows customization of the count query to be
	 * triggered.
	 * 
	 * @param accessor must not be {@literal null}.
	 * @return
	 */
	protected Query createCountQuery(ConvertingParameterAccessor accessor) {

		Query query = createQuery(accessor);

		applyQueryMetaAttributesWhenPresent(query);

		return query;
	}

	/**
	 * Creates a {@link Query} instance using the given {@link ParameterAccessor}
	 * 
	 * @param accessor must not be {@literal null}.
	 * @return
	 */
	protected abstract Query createQuery(ConvertingParameterAccessor accessor);

	/**
	 * Returns whether the query should get a count projection applied.
	 * 
	 * @return
	 */
	protected abstract boolean isCountQuery();

	/**
	 * Return weather the query should delete matching documents.
	 * 
	 * @return
	 * @since 1.5
	 */
	protected abstract boolean isDeleteQuery();

	private abstract class Execution {

		abstract Object execute(Query query);

		protected List<?> readCollection(Query query) {

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();

			String collectionName = metadata.getCollectionName();
			return operations.find(query, metadata.getJavaType(), collectionName);
		}
	}

	/**
	 * {@link Execution} for collection returning queries.
	 * 

	 */
	final class CollectionExecution extends Execution {

		private final Pageable pageable;

		CollectionExecution(Pageable pageable) {
			this.pageable = pageable;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery.Execution#execute(org.springframework.data.sequoiadb.core.query.Query)
		 */
		@Override
		public Object execute(Query query) {
			return readCollection(query.with(pageable));
		}
	}

	/**
	 * {@link Execution} for {@link Slice} query methods.
	 * 


	 * @since 1.5
	 */

	final class SlicedExecution extends Execution {

		private final Pageable pageable;

		SlicedExecution(Pageable pageable) {
			this.pageable = pageable;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery.Execution#execute(org.springframework.data.sequoiadb.core.query.Query)
		 */
		@Override
		@SuppressWarnings({ "unchecked", "rawtypes" })
		Object execute(Query query) {

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();
			int pageSize = pageable.getPageSize();

			Query modifiedQuery = query.with(pageable).limit(pageSize + 1);

			List result = operations.find(modifiedQuery, metadata.getJavaType(), metadata.getCollectionName());

			boolean hasNext = result.size() > pageSize;

			return new SliceImpl<Object>(hasNext ? result.subList(0, pageSize) : result, pageable, hasNext);
		}
	}

	/**
	 * {@link Execution} for pagination queries.
	 * 

	 */
	final class PagedExecution extends Execution {

		private final Pageable pageable;

		/**
		 * Creates a new {@link PagedExecution}.
		 * 
		 * @param pageable
		 */
		public PagedExecution(Pageable pageable) {

			Assert.notNull(pageable);
			this.pageable = pageable;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.AbstractSequoiadbQuery.Execution#execute(org.springframework.data.sequoiadb.core.query.Query)
		 */
		@Override
		@SuppressWarnings({ "rawtypes", "unchecked" })
		Object execute(Query query) {

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();

			int overallLimit = query.getLimit();
			long count = operations.count(query, metadata.getCollectionName());
			count = overallLimit != 0 ? Math.min(count, query.getLimit()) : count;

			boolean pageableOutOfScope = pageable.getOffset() > count;

			if (pageableOutOfScope) {
				return new PageImpl<Object>(Collections.emptyList(), pageable, count);
			}

			query = query.with(pageable);

			if (overallLimit != 0 && pageable.getOffset() + pageable.getPageSize() > overallLimit) {
				query.limit(overallLimit - pageable.getOffset());
			}

			List<?> result = operations.find(query, metadata.getJavaType(), metadata.getCollectionName());
			return new PageImpl(result, pageable, count);
		}
	}

	/**
	 * {@link Execution} to return a single entity.
	 * 

	 */
	final class SingleEntityExecution extends Execution {

		private final boolean countProjection;

		private SingleEntityExecution(boolean countProjection) {
			this.countProjection = countProjection;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.AbstractSequoiadbQuery.Execution#execute(org.springframework.data.sequoiadb.core.core.query.Query)
		 */
		@Override
		Object execute(Query query) {

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();
			return countProjection ? operations.count(query, metadata.getJavaType()) : operations.findOne(query,
					metadata.getJavaType(), metadata.getCollectionName());
		}
	}

	/**
	 * {@link Execution} to execute geo-near queries.
	 * 

	 */
	final class GeoNearExecution extends Execution {

		private final SequoiadbParameterAccessor accessor;

		public GeoNearExecution(SequoiadbParameterAccessor accessor) {
			this.accessor = accessor;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.AbstractSequoiadbQuery.Execution#execute(org.springframework.data.sequoiadb.core.query.Query)
		 */
		@Override
		Object execute(Query query) {

			GeoResults<?> results = doExecuteQuery(query);
			return isListOfGeoResult() ? results.getContent() : results;
		}

		/**
		 * Executes the given {@link Query} to return a page.
		 * 
		 * @param query must not be {@literal null}.
		 * @param countQuery must not be {@literal null}.
		 * @return
		 */
		Object execute(Query query, Query countQuery) {

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();
			long count = operations.count(countQuery, metadata.getCollectionName());

			return new GeoPage<Object>(doExecuteQuery(query), accessor.getPageable(), count);
		}

		@SuppressWarnings("unchecked")
		private GeoResults<Object> doExecuteQuery(Query query) {

			Point nearLocation = accessor.getGeoNearLocation();
			NearQuery nearQuery = NearQuery.near(nearLocation);

			if (query != null) {
				nearQuery.query(query);
			}

			Distance maxDistance = accessor.getMaxDistance();
			if (maxDistance != null) {
				nearQuery.maxDistance(maxDistance).in(maxDistance.getMetric());
			}

			Pageable pageable = accessor.getPageable();
			if (pageable != null) {
				nearQuery.with(pageable);
			}

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();
			return (GeoResults<Object>) operations.geoNear(nearQuery, metadata.getJavaType(), metadata.getCollectionName());
		}

		private boolean isListOfGeoResult() {

			TypeInformation<?> returnType = method.getReturnType();

			if (!returnType.getType().equals(List.class)) {
				return false;
			}

			TypeInformation<?> componentType = returnType.getComponentType();
			return componentType == null ? false : GeoResult.class.equals(componentType.getType());
		}
	}

	/**
	 * {@link Execution} removing documents matching the query.
	 * 
	 * @since 1.5
	 */
	final class DeleteExecution extends Execution {

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery.Execution#execute(org.springframework.data.sequoiadb.core.query.Query)
		 */
		@Override
		Object execute(Query query) {

			SequoiadbEntityMetadata<?> metadata = method.getEntityInformation();
			return deleteAndConvertResult(query, metadata);
		}

		private Object deleteAndConvertResult(Query query, SequoiadbEntityMetadata<?> metadata) {

			if (method.isCollectionQuery()) {
				return operations.findAllAndRemove(query, metadata.getJavaType(), metadata.getCollectionName());
			}

			WriteResult writeResult = operations.remove(query, metadata.getJavaType(), metadata.getCollectionName());
			return writeResult != null ? writeResult.getN() : 0L;
		}
	}
}
