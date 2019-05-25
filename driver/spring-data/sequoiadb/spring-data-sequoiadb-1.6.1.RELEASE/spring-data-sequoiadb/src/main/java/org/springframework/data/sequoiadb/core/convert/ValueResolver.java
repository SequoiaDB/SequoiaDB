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
 * Internal API to trigger the resolution of properties.
 * 

 */
interface ValueResolver {

	/**
	 * Resolves the value for the given {@link SequoiadbPersistentProperty} within the given {@link BSONObject} using the given
	 * {@link SpELExpressionEvaluator} and {@link ObjectPath}.
	 * 
	 * @param prop
	 * @param dbo
	 * @param evaluator
	 * @param parent
	 * @return
	 */
	Object getValueInternal(SequoiadbPersistentProperty prop, BSONObject dbo, SpELExpressionEvaluator evaluator,
                            ObjectPath parent);
}
