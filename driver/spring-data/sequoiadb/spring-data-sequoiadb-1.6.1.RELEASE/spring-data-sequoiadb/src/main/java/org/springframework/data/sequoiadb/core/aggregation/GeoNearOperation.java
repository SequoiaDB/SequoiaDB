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
package org.springframework.data.sequoiadb.core.aggregation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.data.sequoiadb.core.query.NearQuery;
import org.springframework.util.Assert;




/**

 * @since 1.3
 */
public class GeoNearOperation implements AggregationOperation {

	private final NearQuery nearQuery;

	public GeoNearOperation(NearQuery nearQuery) {

		Assert.notNull(nearQuery);
		this.nearQuery = nearQuery;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.aggregation.AggregationOperation#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
	 */
	@Override
	public BSONObject toDBObject(AggregationOperationContext context) {
		return new BasicBSONObject("$geoNear", context.getMappedObject(nearQuery.toDBObject()));
	}
}
