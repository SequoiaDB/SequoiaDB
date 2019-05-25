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

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.junit.Test;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * Unit tests for {@link SkipOperation}.
 * 
 * @author Oliver Gierke
 */
public class SkipOperationUnitTests {

	static final String OP = "$skip";

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNegativeSkip() {
		new SkipOperation(-1L);
	}

	@Test
	public void rendersSkipOperation() {

		SkipOperation operation = new SkipOperation(10L);
		DBObject dbObject = operation.toDBObject(Aggregation.DEFAULT_CONTEXT);

		assertThat(dbObject.get(OP), is((Object) 10L));
	}
}
