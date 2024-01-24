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
package org.springframework.data.sequoiadb.config;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessResourceFailureException;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

/**
 * Integration tests for {@link SequoiadbFactory}.
 * 


 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public class SequoiadbFactoryNoDatabaseRunningTests {

	@Autowired
    SequoiadbTemplate sequoiadbTemplate;

	/**
	 * @see DATA_JIRA-139
	 */
	@Test
	public void startsUpWithoutADatabaseRunning() {
		assertThat(sequoiadbTemplate.getClass().getName(), is("org.springframework.data.sequoiadb.core.SequoiadbTemplate"));
	}

	@Test(expected = DataAccessResourceFailureException.class)
	public void failsDataAccessWithoutADatabaseRunning() {
		sequoiadbTemplate.getCollectionNames();
	}
}
