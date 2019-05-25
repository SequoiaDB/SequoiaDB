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
package org.springframework.data.sequoiadb.core.mapreduce;


import org.bson.BSONObject;
import org.bson.BasicBSONObject;

/**
 * Collects the parameters required to perform a group operation on a collection. The query condition and the input
 * collection are specified on the group method as method arguments to be consistent with other operations, e.g.
 * map-reduce.
 * 

 */
public class GroupBy {

	private BSONObject dboKeys;
	private String keyFunction;
	private String initial;
	private BSONObject initialDbObject;
	private String reduce;
	private String finalize;

	public GroupBy(String... keys) {
		BSONObject dbo = new BasicBSONObject();
		for (String key : keys) {
			dbo.put(key, 1);
		}
		dboKeys = dbo;
	}


	public GroupBy(String key, boolean isKeyFunction) {
		BSONObject dbo = new BasicBSONObject();
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

	public GroupBy initialDocument(BSONObject initialDocument) {
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

	public BSONObject getGroupByObject() {
		BasicBSONObject dbo = new BasicBSONObject();
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
