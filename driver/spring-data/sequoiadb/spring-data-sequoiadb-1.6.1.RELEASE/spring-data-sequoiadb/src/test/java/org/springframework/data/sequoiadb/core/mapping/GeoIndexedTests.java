/*
 * Copyright 2011-2013 the original author or authors.
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
package org.springframework.data.sequoiadb.core.mapping;

import static org.junit.Assert.*;

import java.net.UnknownHostException;
import java.util.List;

import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.dao.DataAccessException;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.CollectionCallback;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;



/**


 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration(classes = GeoIndexedAppConfig.class)
public class GeoIndexedTests {

	private final String[] collectionsToDrop = new String[] { GeoIndexedAppConfig.GEO_COLLECTION, "Person" };

	@Autowired ApplicationContext applicationContext;
	@Autowired
    SequoiadbTemplate template;
	@Autowired
    SequoiadbMappingContext mappingContext;

	@Before
	public void setUp() throws Exception {
		cleanDb();
	}

	@After
	public void cleanUp() throws Exception {
		cleanDb();
	}

	private void cleanDb() throws UnknownHostException {

		Sdb sdb = new SdbClient();
		DB db = sdb.getDB(GeoIndexedAppConfig.GEO_DB);

		for (String coll : collectionsToDrop) {
			db.getCollection(coll).drop();
		}
	}

	@Test
	public void testGeoLocation() {
		GeoLocation geo = new GeoLocation(new double[] { 40.714346, -74.005966 });
		template.insert(geo);

		boolean hasIndex = template.execute("geolocation", new CollectionCallback<Boolean>() {
			public Boolean doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				List<BSONObject> indexes = collection.getIndexInfo();
				for (BSONObject dbo : indexes) {
					if ("location".equals(dbo.get("name"))) {
						return true;
					}
				}
				return false;
			}
		});

		assertTrue(hasIndex);
	}
}
