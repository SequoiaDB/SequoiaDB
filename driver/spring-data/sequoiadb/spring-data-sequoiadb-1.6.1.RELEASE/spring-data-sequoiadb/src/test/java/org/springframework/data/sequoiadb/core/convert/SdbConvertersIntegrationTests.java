/*
 * Copyright 2012 the original author or authors.
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

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.util.UUID;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

/**
 * Integration tests for {@link SequoiadbConverters}.
 * 

 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration("classpath:infrastructure.xml")
public class SdbConvertersIntegrationTests {

	static final String COLLECTION = "_sample";

	@Autowired
    SequoiadbOperations template;

	@Before
	public void setUp() {
		template.dropCollection(COLLECTION);
	}

	@Test
	public void writesUUIDBinaryCorrectly() {

		Wrapper wrapper = new Wrapper();
		wrapper.uuid = UUID.randomUUID();
		template.save(wrapper);

		assertThat(wrapper.id, is(notNullValue()));

		Wrapper result = template.findOne(Query.query(Criteria.where("id").is(wrapper.id)), Wrapper.class);
		assertThat(result.uuid, is(wrapper.uuid));
	}

	static class Wrapper {

		String id;
		UUID uuid;
	}
}
