/*
 * Copyright 2014 the original author or authors.
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

import org.bson.BSONObject;
import org.springframework.data.mapping.model.SpELExpressionEvaluator;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;


/**
 * Default implementation of {@link DbRefResolverCallback}.
 * 

 */
class DefaultDbRefResolverCallback implements DbRefResolverCallback {

	private final BSONObject surroundingObject;
	private final ObjectPath path;
	private final ValueResolver resolver;
	private final SpELExpressionEvaluator evaluator;

	/**
	 * Creates a new {@link DefaultDbRefResolverCallback} using the given {@link BSONObject}, {@link ObjectPath},
	 * {@link ValueResolver} and {@link SpELExpressionEvaluator}.
	 * 
	 * @param surroundingObject must not be {@literal null}.
	 * @param path must not be {@literal null}.
	 * @param evaluator must not be {@literal null}.
	 * @param resolver must not be {@literal null}.
	 */
	public DefaultDbRefResolverCallback(BSONObject surroundingObject, ObjectPath path, SpELExpressionEvaluator evaluator,
			ValueResolver resolver) {

		this.surroundingObject = surroundingObject;
		this.path = path;
		this.resolver = resolver;
		this.evaluator = evaluator;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.DbRefResolverCallback#resolve(org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty)
	 */
	@Override
	public Object resolve(SequoiadbPersistentProperty property) {
		return resolver.getValueInternal(property, surroundingObject, evaluator, path);
	}
}
