/*
 * Copyright 2011-2014 the original author or authors.
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
package org.springframework.data.mongodb.config;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessResourceFailureException;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

/**
 * Integration tests for {@link MongoDbFactory}.
 * 
 * @author Thomas Risberg
 * @author Oliver Gierke
 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public class MongoDbFactoryNoDatabaseRunningTests {

	@Autowired MongoTemplate mongoTemplate;

	/**
	 * @see DATAMONGO-139
	 */
	@Test
	public void startsUpWithoutADatabaseRunning() {
		assertThat(mongoTemplate.getClass().getName(), is("org.springframework.data.mongodb.core.MongoTemplate"));
	}

	@Test(expected = DataAccessResourceFailureException.class)
	public void failsDataAccessWithoutADatabaseRunning() {
		mongoTemplate.getCollectionNames();
	}
}
