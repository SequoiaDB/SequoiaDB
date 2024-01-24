///*
// * Copyright 2011-2012 the original author or authors.
// *
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// *
// *      http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// */
//package org.springframework.data.sequoiadb.repository.support;
//
//import org.springframework.data.mapping.context.MappingContext;
//import org.springframework.data.sequoiadb.core.SequoiadbOperations;
//import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
//import org.springframework.util.Assert;
//
//import com.mysema.query.sequoiadb.SequoiadbQuery;
//import com.mysema.query.types.EntityPath;
//
///**
// * Base class to create repository implementations based on Querydsl.
// *
//
// */
//public abstract class QuerydslRepositorySupport {
//
//	private final SequoiadbOperations template;
//	private final MappingContext<? extends SequoiadbPersistentEntity<?>, ?> context;
//
//	/**
//	 * Creates a new {@link QuerydslRepositorySupport} for the given {@link SequoiadbOperations}.
//	 *
//	 * @param operations must not be {@literal null}
//	 */
//	public QuerydslRepositorySupport(SequoiadbOperations operations) {
//
//		Assert.notNull(operations);
//
//		this.template = operations;
//		this.context = operations.getConverter().getMappingContext();
//	}
//
//	/**
//	 * Returns a {@link SequoiadbQuery} for the given {@link EntityPath}. The collection being queried is derived from the
//	 * entity metadata.
//	 *
//	 * @param path
//	 * @return
//	 */
//	protected <T> SequoiadbQuery<T> from(final EntityPath<T> path) {
//		Assert.notNull(path);
//		SequoiadbPersistentEntity<?> entity = context.getPersistentEntity(path.getType());
//		return from(path, entity.getCollection());
//	}
//
//	/**
//	 * Returns a {@link SequoiadbQuery} for the given {@link EntityPath} querying the given collection.
//	 *
//	 * @param path must not be {@literal null}
//	 * @param collection must not be blank or {@literal null}
//	 * @return
//	 */
//	protected <T> SequoiadbQuery<T> from(final EntityPath<T> path, String collection) {
//
//		Assert.notNull(path);
//		Assert.hasText(collection);
//
//		return new SpringDataSequoiadbQuery<T>(template, path.getType(), collection);
//	}
//}
