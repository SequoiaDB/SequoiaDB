/*
 * Copyright 2010-2011 the original author or authors.
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
package org.springframework.data.mongodb.core.mapreduce;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;

/**
 * Collects the parameters required to perform a group operation on a collection. The query condition and the input
 * collection are specified on the group method as method arguments to be consistent with other operations, e.g.
 * map-reduce.
 * 
 * @author Mark Pollack
 */
public class GroupBy {

	private DBObject dboKeys;
	private String keyFunction;
	private String initial;
	private DBObject initialDbObject;
	private String reduce;
	private String finalize;

	public GroupBy(String... keys) {
		DBObject dbo = new BasicDBObject();
		for (String key : keys) {
			dbo.put(key, 1);
		}
		dboKeys = dbo;
	}


	public GroupBy(String key, boolean isKeyFunction) {
		DBObject dbo = new BasicDBObject();
		if (isKeyFunction) {
			keyFunction = key;
		} else {
			dbo.put(key, 1);
			dboKeys = dbo;
		}
	}

	public static GroupBy keyFunction(String key) {
		return new GroupBy(key, true);
	}

	public static GroupBy key(String... keys) {
		return new GroupBy(keys);
	}

	public GroupBy initialDocument(String initialDocument) {
		initial = initialDocument;
		return this;
	}

	public GroupBy initialDocument(DBObject initialDocument) {
		initialDbObject = initialDocument;
		return this;
	}

	public GroupBy reduceFunction(String reduceFunction) {
		reduce = reduceFunction;
		return this;
	}

	public GroupBy finalizeFunction(String finalizeFunction) {
		finalize = finalizeFunction;
		return this;
	}

	public DBObject getGroupByObject() {
		BasicDBObject dbo = new BasicDBObject();
		if (dboKeys != null) {
			dbo.put("key", dboKeys);
		}
		if (keyFunction != null) {
			dbo.put("$keyf", keyFunction);
		}

		dbo.put("$reduce", reduce);

		dbo.put("initial", initialDbObject);
		if (initial != null) {
			dbo.put("initial", initial);
		}
		if (finalize != null) {
			dbo.put("finalize", finalize);
		}
		return dbo;
	}

}
