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
//package org.springframework.data.mongodb.repository.support;
//
//import java.io.Serializable;
//import java.util.List;
//
//import org.springframework.data.domain.Page;
//import org.springframework.data.domain.PageImpl;
//import org.springframework.data.domain.Pageable;
//import org.springframework.data.domain.Sort;
//import org.springframework.data.domain.Sort.Order;
//import org.springframework.data.mongodb.core.MongoOperations;
//import org.springframework.data.mongodb.core.MongoTemplate;
//import org.springframework.data.mongodb.repository.query.MongoEntityInformation;
//import org.springframework.data.querydsl.EntityPathResolver;
//import org.springframework.data.querydsl.QueryDslPredicateExecutor;
//import org.springframework.data.querydsl.SimpleEntityPathResolver;
//import org.springframework.data.repository.core.EntityMetadata;
//import org.springframework.util.Assert;
//
//import com.mysema.query.mongodb.MongodbQuery;
//import com.mysema.query.types.EntityPath;
//import com.mysema.query.types.Expression;
//import com.mysema.query.types.OrderSpecifier;
//import com.mysema.query.types.Predicate;
//import com.mysema.query.types.path.PathBuilder;
//
///**
// * Special QueryDsl based repository implementation that allows execution {@link Predicate}s in various forms.
// *
// * @author Oliver Gierke
// */
//public class QueryDslMongoRepository<T, ID extends Serializable> extends SimpleMongoRepository<T, ID> implements
//		QueryDslPredicateExecutor<T> {
//
//	private final PathBuilder<T> builder;
//
//	/**
//	 * Creates a new {@link QueryDslMongoRepository} for the given {@link EntityMetadata} and {@link MongoTemplate}. Uses
//	 * the {@link SimpleEntityPathResolver} to create an {@link EntityPath} for the given domain class.
//	 *
//	 * @param entityInformation
//	 * @param template
//	 */
//	public QueryDslMongoRepository(MongoEntityInformation<T, ID> entityInformation, MongoOperations mongoOperations) {
//		this(entityInformation, mongoOperations, SimpleEntityPathResolver.INSTANCE);
//	}
//
//	/**
//	 * Creates a new {@link QueryDslMongoRepository} for the given {@link MongoEntityInformation}, {@link MongoTemplate}
//	 * and {@link EntityPathResolver}.
//	 *
//	 * @param entityInformation
//	 * @param mongoOperations
//	 * @param resolver
//	 */
//	public QueryDslMongoRepository(MongoEntityInformation<T, ID> entityInformation, MongoOperations mongoOperations,
//			EntityPathResolver resolver) {
//
//		super(entityInformation, mongoOperations);
//		Assert.notNull(resolver);
//		EntityPath<T> path = resolver.createPath(entityInformation.getJavaType());
//		this.builder = new PathBuilder<T>(path.getType(), path.getMetadata());
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see org.springframework.data.querydsl.QueryDslPredicateExecutor#findOne(com.mysema.query.types.Predicate)
//	 */
//	public T findOne(Predicate predicate) {
//		return createQueryFor(predicate).uniqueResult();
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see org.springframework.data.querydsl.QueryDslPredicateExecutor#findAll(com.mysema.query.types.Predicate)
//	 */
//	public List<T> findAll(Predicate predicate) {
//		return createQueryFor(predicate).list();
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see org.springframework.data.querydsl.QueryDslPredicateExecutor#findAll(com.mysema.query.types.Predicate, com.mysema.query.types.OrderSpecifier<?>[])
//	 */
//	public List<T> findAll(Predicate predicate, OrderSpecifier<?>... orders) {
//
//		return createQueryFor(predicate).orderBy(orders).list();
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see org.springframework.data.querydsl.QueryDslPredicateExecutor#findAll(com.mysema.query.types.Predicate, org.springframework.data.domain.Pageable)
//	 */
//	public Page<T> findAll(Predicate predicate, Pageable pageable) {
//
//		MongodbQuery<T> countQuery = createQueryFor(predicate);
//		MongodbQuery<T> query = createQueryFor(predicate);
//
//		return new PageImpl<T>(applyPagination(query, pageable).list(), pageable, countQuery.count());
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see org.springframework.data.querydsl.QueryDslPredicateExecutor#count(com.mysema.query.types.Predicate)
//	 */
//	public long count(Predicate predicate) {
//		return createQueryFor(predicate).count();
//	}
//
//	/**
//	 * Creates a {@link MongodbQuery} for the given {@link Predicate}.
//	 *
//	 * @param predicate
//	 * @return
//	 */
//	private MongodbQuery<T> createQueryFor(Predicate predicate) {
//
//		Class<T> domainType = getEntityInformation().getJavaType();
//
//		MongodbQuery<T> query = new SpringDataMongodbQuery<T>(getMongoOperations(), domainType);
//		return query.where(predicate);
//	}
//
//	/**
//	 * Applies the given {@link Pageable} to the given {@link MongodbQuery}.
//	 *
//	 * @param query
//	 * @param pageable
//	 * @return
//	 */
//	private MongodbQuery<T> applyPagination(MongodbQuery<T> query, Pageable pageable) {
//
//		if (pageable == null) {
//			return query;
//		}
//
//		query = query.offset(pageable.getOffset()).limit(pageable.getPageSize());
//		return applySorting(query, pageable.getSort());
//	}
//
//	/**
//	 * Applies the given {@link Sort} to the given {@link MongodbQuery}.
//	 *
//	 * @param query
//	 * @param sort
//	 * @return
//	 */
//	private MongodbQuery<T> applySorting(MongodbQuery<T> query, Sort sort) {
//
//		if (sort == null) {
//			return query;
//		}
//
//		for (Order order : sort) {
//			query.orderBy(toOrder(order));
//		}
//
//		return query;
//	}
//
//	/**
//	 * Transforms a plain {@link Order} into a QueryDsl specific {@link OrderSpecifier}.
//	 *
//	 * @param order
//	 * @return
//	 */
//	@SuppressWarnings({ "rawtypes", "unchecked" })
//	private OrderSpecifier<?> toOrder(Order order) {
//
//		Expression<Object> property = builder.get(order.getProperty());
//
//		return new OrderSpecifier(order.isAscending() ? com.mysema.query.types.Order.ASC
//				: com.mysema.query.types.Order.DESC, property);
//	}
//}
