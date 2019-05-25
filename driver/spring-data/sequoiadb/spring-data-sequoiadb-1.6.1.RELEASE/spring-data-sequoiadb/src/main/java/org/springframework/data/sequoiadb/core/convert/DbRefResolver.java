/*
 * Copyright 2013-2014 the original author or authors.
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
package org.springframework.data.sequoiadb.core.convert;

import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;

import org.springframework.data.sequoiadb.assist.DBRef;

/**
 * Used to resolve associations annotated with {@link org.springframework.data.sequoiadb.core.mapping.DBRef}.
 * 


 * @since 1.4
 */
public interface DbRefResolver {

	/**
	 * Resolves the given {@link DBRef} into an object of the given {@link SequoiadbPersistentProperty}'s type. The method
	 * might return a proxy object for the {@link DBRef} or resolve it immediately. In both cases the
	 * {@link DbRefResolverCallback} will be used to obtain the actual backing object.
	 * 
	 * @param property will never be {@literal null}.
	 * @param dbref the {@link DBRef} to resolve.
	 * @param callback will never be {@literal null}.
	 * @return
	 */
	Object resolveDbRef(SequoiadbPersistentProperty property, DBRef dbref, DbRefResolverCallback callback,
                        DbRefProxyHandler proxyHandler);

	/**
	 * Creates a {@link DBRef} instance for the given {@link org.springframework.data.sequoiadb.core.mapping.DBRef}
	 * annotation, {@link SequoiadbPersistentEntity} and id.
	 * 
	 * @param annotation will never be {@literal null}.
	 * @param entity will never be {@literal null}.
	 * @param id will never be {@literal null}.
	 * @return
	 */
	DBRef createDbRef(org.springframework.data.sequoiadb.core.mapping.DBRef annotation, SequoiadbPersistentEntity<?> entity,
			Object id);
}
