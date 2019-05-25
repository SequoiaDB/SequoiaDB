/*
 * Copyright 2013 the original author or authors.
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
package org.springframework.data.mongodb.core.aggregation;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * Represents one single operation in an aggregation pipeline.
 * 
 * @author Sebastian Herold
 * @author Thomas Darimont
 * @author Oliver Gierke
 * @since 1.3
 */
public interface AggregationOperation {

	/**
	 * Turns the {@link AggregationOperation} into a {@link DBObject} by using the given
	 * {@link AggregationOperationContext}.
	 * 
	 * @return the DBObject
	 */
	DBObject toDBObject(AggregationOperationContext context);
}
