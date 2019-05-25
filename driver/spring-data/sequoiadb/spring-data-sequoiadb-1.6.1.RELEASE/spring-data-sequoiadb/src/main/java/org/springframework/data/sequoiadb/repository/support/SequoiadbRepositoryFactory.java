/*
 * Copyright 2010-2012 the original author or authors.
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

import static org.springframework.data.querydsl.QueryDslUtils.*;

import java.io.Serializable;
import java.lang.reflect.Method;

import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.repository.SequoiadbRepository;
import org.springframework.data.sequoiadb.repository.query.PartTreeSequoiadbQuery;
import org.springframework.data.sequoiadb.repository.query.SequoiadbEntityInformation;
import org.springframework.data.sequoiadb.repository.query.SequoiadbQueryMethod;
import org.springframework.data.sequoiadb.repository.query.StringBasedSequoiadbQuery;
import org.springframework.data.querydsl.QueryDslPredicateExecutor;
import org.springframework.data.repository.core.NamedQueries;
import org.springframework.data.repository.core.RepositoryMetadata;
import org.springframework.data.repository.core.support.RepositoryFactorySupport;
import org.springframework.data.repository.query.QueryLookupStrategy;
import org.springframework.data.repository.query.QueryLookupStrategy.Key;
import org.springframework.data.repository.query.RepositoryQuery;
import org.springframework.util.Assert;

/**
 * Factory to create {@link SequoiadbRepository} instances.
 *
 */
public class SequoiadbRepositoryFactory extends RepositoryFactorySupport {

	private final SequoiadbOperations sequoiadbOperations;
	private final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;

	/**
	 * Creates a new {@link SequoiadbRepositoryFactory} with the given {@link SequoiadbOperations}.
	 * 
	 * @param sequoiadbOperations must not be {@literal null}
	 */
	public SequoiadbRepositoryFactory(SequoiadbOperations sequoiadbOperations) {

		Assert.notNull(sequoiadbOperations);
		this.sequoiadbOperations = sequoiadbOperations;
		this.mappingContext = sequoiadbOperations.getConverter().getMappingContext();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.core.support.RepositoryFactorySupport#getRepositoryBaseClass(org.springframework.data.repository.core.RepositoryMetadata)
	 */
	@Override
	protected Class<?> getRepositoryBaseClass(RepositoryMetadata metadata) {
		return SimpleSequoiadbRepository.class;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.core.support.RepositoryFactorySupport#getTargetRepository(org.springframework.data.repository.core.RepositoryMetadata)
	 */
	@Override
	@SuppressWarnings({ "rawtypes", "unchecked" })
	protected Object getTargetRepository(RepositoryMetadata metadata) {

		Class<?> repositoryInterface = metadata.getRepositoryInterface();
		SequoiadbEntityInformation<?, Serializable> entityInformation = getEntityInformation(metadata.getDomainType());

		return new SimpleSequoiadbRepository(entityInformation, sequoiadbOperations);
	}

	private static boolean isQueryDslRepository(Class<?> repositoryInterface) {

		return QUERY_DSL_PRESENT && QueryDslPredicateExecutor.class.isAssignableFrom(repositoryInterface);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.core.support.RepositoryFactorySupport#getQueryLookupStrategy(org.springframework.data.repository.query.QueryLookupStrategy.Key)
	 */
	@Override
	protected QueryLookupStrategy getQueryLookupStrategy(Key key) {
		return new SequoiadbQueryLookupStrategy();
	}

	/**
	 * {@link QueryLookupStrategy} to create {@link PartTreeSequoiadbQuery} instances.
	 * 

	 */
	private class SequoiadbQueryLookupStrategy implements QueryLookupStrategy {

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.repository.query.QueryLookupStrategy#resolveQuery(java.lang.reflect.Method, org.springframework.data.repository.core.RepositoryMetadata, org.springframework.data.repository.core.NamedQueries)
		 */
		public RepositoryQuery resolveQuery(Method method, RepositoryMetadata metadata, NamedQueries namedQueries) {

			SequoiadbQueryMethod queryMethod = new SequoiadbQueryMethod(method, metadata, mappingContext);
			String namedQueryName = queryMethod.getNamedQueryName();

			if (namedQueries.hasQuery(namedQueryName)) {
				String namedQuery = namedQueries.getQuery(namedQueryName);
				return new StringBasedSequoiadbQuery(namedQuery, queryMethod, sequoiadbOperations);
			} else if (queryMethod.hasAnnotatedQuery()) {
				return new StringBasedSequoiadbQuery(queryMethod, sequoiadbOperations);
			} else {
				return new PartTreeSequoiadbQuery(queryMethod, sequoiadbOperations);
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.core.support.RepositoryFactorySupport#getEntityInformation(java.lang.Class)
	 */
	@Override
	@SuppressWarnings("unchecked")
	public <T, ID extends Serializable> SequoiadbEntityInformation<T, ID> getEntityInformation(Class<T> domainClass) {

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(domainClass);

		if (entity == null) {
			throw new MappingException(String.format("Could not lookup mapping metadata for domain class %s!",
					domainClass.getName()));
		}

		return new MappingSequoiadbEntityInformation<T, ID>((SequoiadbPersistentEntity<T>) entity);
	}
}
