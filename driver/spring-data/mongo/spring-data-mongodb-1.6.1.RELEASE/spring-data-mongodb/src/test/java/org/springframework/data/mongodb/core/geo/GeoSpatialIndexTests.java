/*
 * Copyright 2013-2014 the original author or authors.
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
package org.springframework.data.mongodb.core.geo;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import java.util.List;
import java.util.Map;

import org.junit.Before;
import org.junit.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessException;
import org.springframework.data.geo.Point;
import org.springframework.data.mongodb.config.AbstractIntegrationTests;
import org.springframework.data.mongodb.core.CollectionCallback;
import org.springframework.data.mongodb.core.IndexOperations;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.data.mongodb.core.WriteResultChecking;
import org.springframework.data.mongodb.core.index.GeoSpatialIndexType;
import org.springframework.data.mongodb.core.index.GeoSpatialIndexed;
import org.springframework.data.mongodb.core.index.IndexInfo;
import org.springframework.data.mongodb.core.mapping.Document;

import org.springframework.data.mongodb.assist.DBCollection;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.MongoException;
import org.springframework.data.mongodb.assist.WriteConcern;

/**
 * Integration tests for geo-spatial indexing.
 * 
 * @author Laurent Canet
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public class GeoSpatialIndexTests extends AbstractIntegrationTests {

	@Autowired private MongoTemplate template;

	@Before
	public void setUp() {

		template.setWriteConcern(WriteConcern.FSYNC_SAFE);
		template.setWriteResultChecking(WriteResultChecking.EXCEPTION);
	}

	/**
	 * @see DATAMONGO-778
	 */
	@Test
	public void test2dIndex() {

		try {
			template.save(new GeoSpatialEntity2D(45.2, 4.6));
			assertThat(hasIndexOfType(GeoSpatialEntity2D.class, "2d"), is(true));
		} finally {
			template.dropCollection(GeoSpatialEntity2D.class);
		}
	}

	/**
	 * @see DATAMONGO-778
	 */
	@Test
	public void test2dSphereIndex() {

		try {
			template.save(new GeoSpatialEntity2DSphere(45.2, 4.6));
			assertThat(hasIndexOfType(GeoSpatialEntity2DSphere.class, "2dsphere"), is(true));
		} finally {
			template.dropCollection(GeoSpatialEntity2DSphere.class);
		}
	}

	/**
	 * @see DATAMONGO-778
	 */
	@Test
	public void testHaystackIndex() {

		try {
			template.save(new GeoSpatialEntityHaystack(45.2, 4.6, "Paris"));
			assertThat(hasIndexOfType(GeoSpatialEntityHaystack.class, "geoHaystack"), is(true));
		} finally {
			template.dropCollection(GeoSpatialEntityHaystack.class);
		}
	}

	/**
	 * @see DATAMONGO-827
	 */
	@Test
	public void useGeneratedNameShouldGenerateAnIndexName() {

		try {

			GeoSpatialEntity2dWithGeneratedIndex geo = new GeoSpatialEntity2dWithGeneratedIndex(45.2, 4.6);
			template.save(geo);

			IndexOperations indexOps = template.indexOps(GeoSpatialEntity2dWithGeneratedIndex.class);
			List<IndexInfo> indexInfo = indexOps.getIndexInfo();

			assertThat(indexInfo, hasSize(2));
			assertThat(indexInfo.get(1), is(notNullValue()));
			assertThat(indexInfo.get(1).getName(), is("location_2d"));

		} finally {
			template.dropCollection(GeoSpatialEntity2D.class);
		}
	}

	/**
	 * Returns whether an index with the given name exists for the given entity type.
	 * 
	 * @param indexName
	 * @param entityType
	 * @return
	 */
	private boolean hasIndexOfType(Class<?> entityType, final String type) {

		return template.execute(entityType, new CollectionCallback<Boolean>() {

			@SuppressWarnings("unchecked")
			public Boolean doInCollection(DBCollection collection) throws MongoException, DataAccessException {

				for (DBObject indexInfo : collection.getIndexInfo()) {

					DBObject keys = (DBObject) indexInfo.get("key");
					Map<String, Object> keysMap = keys.toMap();

					for (String key : keysMap.keySet()) {
						Object indexType = keys.get(key);
						if (type.equals(indexType)) {
							return true;
						}
					}
				}

				return false;
			}
		});
	}

	@Document
	static class GeoSpatialEntity2D {
		public String id;
		@GeoSpatialIndexed(type = GeoSpatialIndexType.GEO_2D) public org.springframework.data.geo.Point location;

		public GeoSpatialEntity2D(double x, double y) {
			this.location = new org.springframework.data.geo.Point(x, y);
		}
	}

	@Document
	static class GeoSpatialEntityHaystack {
		public String id;
		public String name;
		@GeoSpatialIndexed(type = GeoSpatialIndexType.GEO_HAYSTACK, additionalField = "name") public Point location;

		public GeoSpatialEntityHaystack(double x, double y, String name) {
			this.location = new Point(x, y);
			this.name = name;
		}
	}

	static class GeoJsonPoint {
		String type = "Point";
		double coordinates[];
	}

	@Document
	static class GeoSpatialEntity2DSphere {
		public String id;
		@GeoSpatialIndexed(type = GeoSpatialIndexType.GEO_2DSPHERE) public GeoJsonPoint location;

		public GeoSpatialEntity2DSphere(double x, double y) {
			this.location = new GeoJsonPoint();
			this.location.coordinates = new double[] { x, y };
		}
	}

	@Document
	static class GeoSpatialEntity2dWithGeneratedIndex {

		public String id;
		@GeoSpatialIndexed(type = GeoSpatialIndexType.GEO_2D, useGeneratedName = true) public Point location;

		public GeoSpatialEntity2dWithGeneratedIndex(double x, double y) {
			this.location = new Point(x, y);
		}
	}
}
