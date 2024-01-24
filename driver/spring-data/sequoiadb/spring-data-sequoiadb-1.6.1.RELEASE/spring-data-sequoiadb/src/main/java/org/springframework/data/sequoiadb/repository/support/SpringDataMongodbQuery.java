///*
// * Copyright 2012 the original author or authors.
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
//import org.springframework.data.sequoiadb.core.SequoiadbOperations;
//
//import com.google.common.base.Function;
//import org.springframework.data.sequoiadb.assist.DBCollection;
//
//import com.mysema.query.sequoiadb.SequoiadbQuery;
//
///**
// * Spring Data specfic {@link SequoiadbQuery} implementation.
// *
//
// */
//class SpringDataSequoiadbQuery<T> extends SequoiadbQuery<T> {
//
//	private final SequoiadbOperations operations;
//
//	/**
//	 * Creates a new {@link SpringDataSequoiadbQuery}.
//	 *
//	 * @param operations must not be {@literal null}.
//	 * @param type must not be {@literal null}.
//	 */
//	public SpringDataSequoiadbQuery(final SequoiadbOperations operations, final Class<? extends T> type) {
//		this(operations, type, operations.getCollectionName(type));
//	}
//
//	/**
//	 * Creates a new {@link SpringDataSequoiadbQuery} to query the given collection.
//	 *
//	 * @param operations must not be {@literal null}.
//	 * @param type must not be {@literal null}.
//	 * @param collectionName must not be {@literal null} or empty.
//	 */
//	public SpringDataSequoiadbQuery(final SequoiadbOperations operations, final Class<? extends T> type, String collectionName) {
//
//		super(operations.getCollection(collectionName), new Function<BSONObject, T>() {
//			public T apply(BSONObject input) {
//				return operations.getConverter().read(type, input);
//			}
//		}, new SpringDataSequoiadbSerializer(operations.getConverter()));
//
//		this.operations = operations;
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see com.mysema.query.sequoiadb.SequoiadbQuery#getCollection(java.lang.Class)
//	 */
//	@Override
//	protected DBCollection getCollection(Class<?> type) {
//		return operations.getCollection(operations.getCollectionName(type));
//	}
//}
