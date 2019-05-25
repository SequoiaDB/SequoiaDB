/*
 * Copyright 2011 the original author or authors.
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
package org.springframework.data.sequoiadb.repository;

import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.List;

import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessException;
import org.springframework.data.sequoiadb.core.CollectionCallback;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.index.IndexInfo;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

import org.springframework.data.sequoiadb.assist.DBCollection;



/**
 * Integration test for index creation for query methods.
 * 

 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public class RepositoryIndexCreationIntegrationTests {

	@Autowired
    SequoiadbOperations operations;

	@Autowired
	PersonRepository repository;

	@After
	public void tearDown() {
		operations.execute(Person.class, new CollectionCallback<Void>() {

			public Void doInCollection(DBCollection collection) throws BaseException, DataAccessException {

				for (BSONObject index : collection.getIndexInfo()) {
					String indexName = index.get("name").toString();
					if (indexName.startsWith("find")) {
						collection.dropIndex(indexName);
					}
				}

				return null;
			}
		});
	}

	@Test
	public void testname() {

		List<IndexInfo> indexInfo = operations.indexOps(Person.class).getIndexInfo();

		assertHasIndexForField(indexInfo, "lastname");
		assertHasIndexForField(indexInfo, "firstname");
	}

	private static void assertHasIndexForField(List<IndexInfo> indexInfo, String... fields) {

		for (IndexInfo info : indexInfo) {
			if (info.isIndexForFields(Arrays.asList(fields))) {
				return;
			}
		}

		fail(String.format("Did not find index for field(s) %s in %s!", fields, indexInfo));
	}
}
