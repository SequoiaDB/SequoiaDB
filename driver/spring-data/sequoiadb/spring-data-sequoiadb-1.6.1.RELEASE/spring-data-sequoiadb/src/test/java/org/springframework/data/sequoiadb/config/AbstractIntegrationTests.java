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
package org.springframework.data.sequoiadb.config;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import com.sequoiadb.exception.BaseException;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.dao.DataAccessException;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.CollectionCallback;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

/**

 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public abstract class AbstractIntegrationTests {

	@Configuration
	static class TestConfig extends AbstractSequoiadbConfiguration {

		@Override
		protected String getDatabaseName() {
			return "database";
		}

		@Override
		public Sdb sdb() throws Exception {
			return new SdbClient();
		}
	}

	@Autowired
    SequoiadbOperations operations;

	@Before
	@After
	public void cleanUp() {

		for (String collectionName : operations.getCollectionNames()) {
			if (!collectionName.startsWith("system")) {
				operations.execute(collectionName, new CollectionCallback<Void>() {
					@Override
					public Void doInCollection(DBCollection collection) throws BaseException, DataAccessException {
						collection.remove(new BasicBSONObject());
						assertThat(collection.find().hasNext(), is(false));
						return null;
					}
				});
			}
		}
	}
}
