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
package org.springframework.data.mongodb.core;

import static org.springframework.data.domain.Sort.Direction.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.dao.DataAccessException;
import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.core.index.IndexDefinition;
import org.springframework.data.mongodb.core.index.IndexField;
import org.springframework.data.mongodb.core.index.IndexInfo;
import org.springframework.util.Assert;

import org.springframework.data.mongodb.assist.DBCollection;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.MongoException;

/**
 * Default implementation of {@link IndexOperations}.
 * 
 * @author Mark Pollack
 * @author Oliver Gierke
 * @author Komi Innocent
 * @author Christoph Strobl
 */
public class DefaultIndexOperations implements IndexOperations {

	private static final Double ONE = Double.valueOf(1);
	private static final Double MINUS_ONE = Double.valueOf(-1);
	private static final Collection<String> TWO_D_IDENTIFIERS = Arrays.asList("2d", "2dsphere");

	private final MongoOperations mongoOperations;
	private final String collectionName;

	/**
	 * Creates a new {@link DefaultIndexOperations}.
	 * 
	 * @param mongoOperations must not be {@literal null}.
	 * @param collectionName must not be {@literal null}.
	 */
	public DefaultIndexOperations(MongoOperations mongoOperations, String collectionName) {

		Assert.notNull(mongoOperations, "MongoOperations must not be null!");
		Assert.notNull(collectionName, "Collection name can not be null!");

		this.mongoOperations = mongoOperations;
		this.collectionName = collectionName;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.IndexOperations#ensureIndex(org.springframework.data.mongodb.core.index.IndexDefinition)
	 */
	public void ensureIndex(final IndexDefinition indexDefinition) {
		mongoOperations.execute(collectionName, new CollectionCallback<Object>() {
			public Object doInCollection(DBCollection collection) throws MongoException, DataAccessException {
				DBObject indexOptions = indexDefinition.getIndexOptions();
				if (indexOptions != null) {
					collection.ensureIndex(indexDefinition.getIndexKeys(), indexOptions);
				} else {
					collection.ensureIndex(indexDefinition.getIndexKeys());
				}
				return null;
			}
		});
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.IndexOperations#dropIndex(java.lang.String)
	 */
	public void dropIndex(final String name) {
		mongoOperations.execute(collectionName, new CollectionCallback<Void>() {
			public Void doInCollection(DBCollection collection) throws MongoException, DataAccessException {
				collection.dropIndex(name);
				return null;
			}
		});

	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.IndexOperations#dropAllIndexes()
	 */
	public void dropAllIndexes() {
		List<IndexInfo> indexNameList = getIndexInfo();
		for (IndexInfo indexInfo : indexNameList) {
			String indexName = indexInfo.getName();
			if (!indexName.equals("$id") && !indexName.equals("$shard")) {
				dropIndex(indexName);
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.IndexOperations#resetIndexCache()
	 */
	public void resetIndexCache() {
		mongoOperations.execute(collectionName, new CollectionCallback<Void>() {
			public Void doInCollection(DBCollection collection) throws MongoException, DataAccessException {
				collection.resetIndexCache();
				return null;
			}
		});
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.IndexOperations#getIndexInfo()
	 */
	public List<IndexInfo> getIndexInfo() {

		return mongoOperations.execute(collectionName, new CollectionCallback<List<IndexInfo>>() {
			public List<IndexInfo> doInCollection(DBCollection collection) throws MongoException, DataAccessException {
				List<DBObject> dbObjectList = collection.getIndexInfo();
				return getIndexData(dbObjectList);
			}

			private List<IndexInfo> getIndexData(List<DBObject> dbObjectList) {

				List<IndexInfo> indexInfoList = new ArrayList<IndexInfo>();

				for (DBObject ix : dbObjectList) {
					BSONObject idxDefObj = (BasicBSONObject) ix.get("IndexDef");
					BSONObject keyDbObject = (BasicBSONObject)idxDefObj.get("key");
					int numberOfElements = keyDbObject.keySet().size();
					List<IndexField> indexFields = new ArrayList<IndexField>(numberOfElements);

					for (String key : keyDbObject.keySet()) {
						Object value = keyDbObject.get(key);
						Double keyValue = new Double(value.toString());

						if (ONE.equals(keyValue)) {
							indexFields.add(IndexField.create(key, ASC));
						} else if (MINUS_ONE.equals(keyValue)) {
							indexFields.add(IndexField.create(key, DESC));
						}
					}

					String name = idxDefObj.get("name").toString();
					int v = idxDefObj.containsField("v") ? new Integer(idxDefObj.get("v").toString()) : 0;
					boolean unique = idxDefObj.containsField("unique") ? (Boolean) idxDefObj.get("unique") : false;
					boolean dropDuplicates = idxDefObj.containsField("dropDups") ? (Boolean) idxDefObj.get("dropDups") : false;
					boolean sparse = idxDefObj.containsField("sparse") ? (Boolean) idxDefObj.get("sparse") : false;
					String language = idxDefObj.containsField("default_language") ? (String) idxDefObj.get("default_language") : "";
					String indexFlag = (String)ix.get("IndexFlag");
					boolean enforced = idxDefObj.containsField("enforced") ? (Boolean) idxDefObj.get("enforced") : false;
					indexInfoList.add(new IndexInfo(indexFields, name, unique, dropDuplicates, sparse, language, enforced, indexFlag));
				}

				return indexInfoList;
			}
		});
	}
}
