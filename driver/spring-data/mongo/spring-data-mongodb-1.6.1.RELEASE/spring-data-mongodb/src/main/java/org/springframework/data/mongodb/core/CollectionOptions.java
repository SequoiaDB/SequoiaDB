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
package org.springframework.data.mongodb.core;

import java.util.HashMap;
import java.util.Map;

/**
 * Provides a simple wrapper to encapsulate the variety of settings you can use when creating a collection.
 * 
 * @author Thomas Risberg
 */
public class CollectionOptions {

	private Map<String, Integer> shardingKey;
	private String shardingType;
	private int partition;
	private int replSize;
	private boolean compressed;
	private String compressionType;
	private boolean isMainCL;
	private boolean autoSplit;
	private String group;
	private boolean autoIndexId;
	private boolean ensureShardingIndex;

	public Map<String, Integer> getShardingKey() {
		return shardingKey;
	}

	public String getShardingType() {
		return shardingType;
	}

	public int getPartition() {
		return partition;
	}

	public int getReplSize() {
		return replSize;
	}

	public boolean isCompressed() {
		return compressed;
	}

	public String getCompressionType() {
		return compressionType;
	}

	public boolean isMainCL() {
		return isMainCL;
	}

	public boolean isAutoSplit() {
		return autoSplit;
	}

	public String getGroup() {
		return group;
	}

	public boolean isAutoIndexId() {
		return autoIndexId;
	}

	public boolean isEnsureShardingIndex() {
		return ensureShardingIndex;
	}

	public static class Builder {
		private Map<String, Integer> shardingKey = new HashMap<String, Integer>();
		private String shardingType = "hash";
		private int partition = 1024;
		private int replSize = 1;
		private boolean compressed = false;
		private String compressionType = "snappy";
		private boolean isMainCL = false;
		private boolean autoSplit = true;
		private String group;
		private boolean autoIndexId = true;
		private boolean ensureShardingIndex = true;

		public Builder shardingKey(Map<String, Integer> val) {
			shardingKey = val;
			return this;
		}

		public Builder shardingType(String val) {
			shardingType = val;
			return this;
		}

		public Builder partition(int val) {
			partition = val;
			return this;
		}

		public Builder replSize(int val) {
			replSize = val;
			return this;
		}

		public Builder compressed(boolean val) {
			compressed = val;
			return this;
		}

		public Builder compressionType(String val) {
			compressionType = val;
			return this;
		}

		public Builder isMainCL(boolean val) {
			isMainCL = val;
			return this;
		}

		public Builder autoSplit(boolean val) {
			autoSplit = val;
			return this;
		}

		public Builder group(String val) {
			group = val;
			return this;
		}

		public Builder autoIndexId(boolean val) {
			autoIndexId = val;
			return this;
		}

		public Builder ensureShardingIndex(boolean val) {
			ensureShardingIndex = val;
			return this;
		}

		public CollectionOptions build() {
			return new CollectionOptions(this);
		}
	}

	private CollectionOptions(Builder builder) {
		shardingKey = builder.shardingKey;
		shardingType = builder.shardingType;
		partition = builder.partition;
		replSize = builder.replSize;
		compressed = builder.compressed;
		compressionType = builder.compressionType;
		isMainCL = builder.isMainCL;
		autoSplit = builder.autoSplit;
		group = builder.group;
		autoIndexId = builder.autoIndexId;
		ensureShardingIndex = builder.ensureShardingIndex;
	}

}
