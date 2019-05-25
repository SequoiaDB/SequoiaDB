/*
 * Copyright 2011-2012 the original author or authors.
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
package org.springframework.data.sequoiadb.core.query;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;
import org.springframework.data.domain.Sort.Direction;



import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;
import static org.springframework.data.sequoiadb.core.query.Criteria.where;

/**
 * Unit tests for {@link BasicQuery}.
 * 

 */
public class SdbQueryUnitTests {

	@Test
	public void createsQueryFromPlainJson() {
		Query q = new BasicQuery("{ \"name\" : \"Thomas\" }");
		BSONObject reference = new BasicBSONObject("name", "Thomas");
		assertThat(q.getQueryObject(), is(reference));
	}

	@Test
	public void addsCriteriaCorrectly() {
		Query q = new BasicQuery("{ \"name\" : \"Thomas\" }").addCriteria(where("age").lt(80));
		BSONObject reference = new BasicBSONObject("name", "Thomas");
		reference.put("age", new BasicBSONObject("$lt", 80));
		assertThat(q.getQueryObject(), is(reference));
	}

	@Test
	public void overridesSortCorrectly() {

		BasicQuery query = new BasicQuery("{}");
		query.setSortObject(new BasicBSONObject("name", -1));
		query.with(new org.springframework.data.domain.Sort(Direction.ASC, "lastname"));

		BSONObject sortReference = new BasicBSONObject("name", -1);
		sortReference.put("lastname", 1);
		assertThat(query.getSortObject(), is(sortReference));
	}
}
