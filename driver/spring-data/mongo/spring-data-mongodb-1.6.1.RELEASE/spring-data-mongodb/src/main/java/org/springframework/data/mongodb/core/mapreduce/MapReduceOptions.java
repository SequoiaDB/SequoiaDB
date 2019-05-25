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

import java.util.HashMap;
import java.util.Map;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.MapReduceCommand;

public class MapReduceOptions {

	private String outputCollection;

	private String outputDatabase;

	private Boolean outputSharded;

	private MapReduceCommand.OutputType outputType = MapReduceCommand.OutputType.REPLACE;

	private String finalizeFunction;

	private Map<String, Object> scopeVariables = new HashMap<String, Object>();

	private Boolean jsMode;

	private Boolean verbose = true;

	private Map<String, Object> extraOptions = new HashMap<String, Object>();

	/**
	 * Static factory method to create a MapReduceOptions instance
	 * 
	 * @return a new instance
	 */
	public static MapReduceOptions options() {
		return new MapReduceOptions();
	}

	/**
	 * Limit the number of objects to return from the collection that is fed into the map reduce operation Often used in
	 * conjunction with a query and sort option so as to reduce the portion of the data that will be processed.
	 * 
	 * @param limit Limit the number of objects to process
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions limit(int limit) {
		return this;
	}

	/**
	 * The collection where the results from the map-reduce operation will be stored. Note, you can set the database name
	 * as well with the outputDatabase option.
	 * 
	 * @param collectionName The name of the collection where the results of the map-reduce operation will be stored.
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions outputCollection(String collectionName) {
		this.outputCollection = collectionName;
		return this;
	}

	/**
	 * The database where the results from the map-reduce operation will be stored. Note, you ca set the collection name
	 * as well with the outputCollection option.
	 * 
	 * @param outputDatabase The name of the database where the results of the map-reduce operation will be stored.
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions outputDatabase(String outputDatabase) {
		this.outputDatabase = outputDatabase;
		return this;
	}

	/**
	 * With this option, no collection will be created, and the whole map-reduce operation will happen in RAM. Also, the
	 * results of the map-reduce will be returned within the result object. Note that this option is possible only when
	 * the result set fits within the 16MB limit of a single document.
	 * 
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions outputTypeInline() {
		this.outputType = MapReduceCommand.OutputType.INLINE;
		return this;
	}

	/**
	 * This option will merge new data into the old output collection. In other words, if the same key exists in both the
	 * result set and the old collection, the new key will overwrite the old one.
	 * 
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions outputTypeMerge() {
		this.outputType = MapReduceCommand.OutputType.MERGE;
		return this;
	}

	/**
	 * If documents exists for a given key in the result set and in the old collection, then a reduce operation (using the
	 * specified reduce function) will be performed on the two values and the result will be written to the output
	 * collection. If a finalize function was provided, this will be run after the reduce as well.
	 * 
	 * @return
	 */
	public MapReduceOptions outputTypeReduce() {
		this.outputType = MapReduceCommand.OutputType.REDUCE;
		return this;
	}

	/**
	 * The output will be inserted into a collection which will atomically replace any existing collection with the same
	 * name. Note, the default is MapReduceCommand.OutputType.REPLACE
	 * 
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions outputTypeReplace() {
		this.outputType = MapReduceCommand.OutputType.REPLACE;
		return this;
	}

	/**
	 * If true and combined with an output mode that writes to a collection, the output collection will be sharded using
	 * the _id field. For MongoDB 1.9+
	 * 
	 * @param outputShared if true, output will be sharded based on _id key.
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions outputSharded(boolean outputShared) {
		this.outputSharded = outputShared;
		return this;
	}

	/**
	 * Sets the finalize function
	 * 
	 * @param finalizeFunction The finalize function. Can be a JSON string or a Spring Resource URL
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions finalizeFunction(String finalizeFunction) {
		this.finalizeFunction = finalizeFunction;
		return this;
	}

	/**
	 * Key-value pairs that are placed into JavaScript global scope and can be accessed from map, reduce, and finalize
	 * scripts.
	 * 
	 * @param scopeVariables variables that can be accessed from map, reduce, and finalize scripts
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions scopeVariables(Map<String, Object> scopeVariables) {
		this.scopeVariables = scopeVariables;
		return this;
	}

	/**
	 * Flag that toggles behavior in the map-reduce operation so as to avoid intermediate conversion to BSON between the
	 * map and reduce steps. For MongoDB 1.9+
	 * 
	 * @param javaScriptMode if true, have the execution of map-reduce stay in JavaScript
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions javaScriptMode(boolean javaScriptMode) {
		this.jsMode = javaScriptMode;
		return this;
	}

	/**
	 * Flag to set that will provide statistics on job execution time.
	 * 
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions verbose(boolean verbose) {
		this.verbose = verbose;
		return this;
	}

	/**
	 * Add additional extra options that may not have a method on this class. This method will help if you use a version
	 * of this client library with a server version that has added additional map-reduce options that do not yet have an
	 * method for use in setting them. options
	 * 
	 * @param key The key option
	 * @param value The value of the option
	 * @return MapReduceOptions so that methods can be chained in a fluent API style
	 */
	public MapReduceOptions extraOption(String key, Object value) {
		extraOptions.put(key, value);
		return this;
	}

	public Map<String, Object> getExtraOptions() {
		return extraOptions;
	}

	public String getFinalizeFunction() {
		return this.finalizeFunction;
	}

	public Boolean getJavaScriptMode() {
		return this.jsMode;
	}

	public String getOutputCollection() {
		return this.outputCollection;
	}

	public String getOutputDatabase() {
		return this.outputDatabase;
	}

	public Boolean getOutputSharded() {
		return this.outputSharded;
	}

	public MapReduceCommand.OutputType getOutputType() {
		return this.outputType;
	}

	public Map<String, Object> getScopeVariables() {
		return this.scopeVariables;
	}

	public DBObject getOptionsObject() {
		BasicDBObject cmd = new BasicDBObject();

		if (verbose != null) {
			cmd.put("verbose", verbose);
		}

		cmd.put("out", createOutObject());

		if (finalizeFunction != null) {
			cmd.put("finalize", finalizeFunction);
		}

		if (scopeVariables != null) {
			cmd.put("scope", scopeVariables);
		}

		if (!extraOptions.keySet().isEmpty()) {
			cmd.putAll(extraOptions);
		}

		return cmd;
	}

	protected BasicDBObject createOutObject() {
		BasicDBObject out = new BasicDBObject();

		switch (outputType) {
		case INLINE:
			out.put("inline", 1);
			break;
		case REPLACE:
			out.put("replace", outputCollection);
			break;
		case MERGE:
			out.put("merge", outputCollection);
			break;
		case REDUCE:
			out.put("reduce", outputCollection);
			break;
		}

		if (outputDatabase != null) {
			out.put("db", outputDatabase);
		}

		if (outputSharded != null) {
			out.put("sharded", outputSharded);
		}

		return out;
	}
}
