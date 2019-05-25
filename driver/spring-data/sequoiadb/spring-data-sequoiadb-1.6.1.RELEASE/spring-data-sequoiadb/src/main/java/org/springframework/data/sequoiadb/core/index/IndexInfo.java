/*
 * Copyright 2002-2014 the original author or authors.
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
package org.springframework.data.sequoiadb.core.index;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

import org.springframework.util.Assert;
import org.springframework.util.ObjectUtils;

/**



 */
public class IndexInfo {

	private final List<IndexField> indexFields;

	private final String name;
	private final boolean unique;
	private final boolean dropDuplicates;
	private final boolean sparse;
	private final String language;
	private final boolean enforced;
	private final String indexFlag;

	/**
	 * @deprecated Will be removed in 1.7. Please use {@link #IndexInfo(List, String, boolean, boolean, boolean, String)}
	 * @param indexFields
	 * @param name
	 * @param unique
	 * @param dropDuplicates
	 * @param sparse
	 */
	@Deprecated
	public IndexInfo(List<IndexField> indexFields, String name, boolean unique, boolean dropDuplicates, boolean sparse) {
		this(indexFields, name, unique, dropDuplicates, sparse, "");
	}

	public IndexInfo(List<IndexField> indexFields, String name, boolean unique, boolean dropDuplicates, boolean sparse,
					 String language) {
		this(indexFields, name, unique, dropDuplicates, sparse, "", false,"");
	}

	public IndexInfo(List<IndexField> indexFields, String name, boolean unique, boolean dropDuplicates, boolean sparse,
			String language, boolean enforced, String indexFlag) {

		this.indexFields = Collections.unmodifiableList(indexFields);
		this.name = name;
		this.unique = unique;
		this.dropDuplicates = dropDuplicates;
		this.sparse = sparse;
		this.language = language;
		this.enforced = enforced;
		this.indexFlag = indexFlag;
	}

	/**
	 * Returns the individual index fields of the index.
	 * 
	 * @return
	 */
	public List<IndexField> getIndexFields() {
		return this.indexFields;
	}

	/**
	 * Returns whether the index is covering exactly the fields given independently of the order.
	 * 
	 * @param keys must not be {@literal null}.
	 * @return
	 */
	public boolean isIndexForFields(Collection<String> keys) {

		Assert.notNull(keys);
		List<String> indexKeys = new ArrayList<String>(indexFields.size());

		for (IndexField field : indexFields) {
			indexKeys.add(field.getKey());
		}

		return indexKeys.containsAll(keys);
	}

	public String getName() {
		return name;
	}

	public boolean isUnique() {
		return unique;
	}

	public boolean isDropDuplicates() {
		return dropDuplicates;
	}

	public boolean isSparse() {
		return sparse;
	}

	/**
	 * @return
	 * @since 1.6
	 */
	public String getLanguage() {
		return language;
	}

	public String getIndexFlag() {
		return indexFlag;
	}

	@Override
	public String toString() {
		return "IndexInfo [indexFields=" + indexFields + ", name=" + name + ", unique=" + unique + ", dropDuplicates="
				+ dropDuplicates + ", sparse=" + sparse + ", language=" + language
				+ ", enforced=" + enforced + ", indexFlag=" + indexFlag  + "]";
	}

	@Override
	public int hashCode() {

		final int prime = 31;
		int result = 1;
		result = prime * result + (dropDuplicates ? 1231 : 1237);
		result = prime * result + ObjectUtils.nullSafeHashCode(indexFields);
		result = prime * result + ((name == null) ? 0 : name.hashCode());
		result = prime * result + (sparse ? 1231 : 1237);
		result = prime * result + (unique ? 1231 : 1237);
		result = prime * result + ObjectUtils.nullSafeHashCode(language);
		result = prime * result + (enforced ? 1231 : 1237);
		result = prime * result + ObjectUtils.nullSafeHashCode(indexFlag);
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj) {
			return true;
		}
		if (obj == null) {
			return false;
		}
		if (getClass() != obj.getClass()) {
			return false;
		}
		IndexInfo other = (IndexInfo) obj;
		if (dropDuplicates != other.dropDuplicates) {
			return false;
		}
		if (indexFields == null) {
			if (other.indexFields != null) {
				return false;
			}
		} else if (!indexFields.equals(other.indexFields)) {
			return false;
		}
		if (name == null) {
			if (other.name != null) {
				return false;
			}
		} else if (!name.equals(other.name)) {
			return false;
		}
		if (sparse != other.sparse) {
			return false;
		}
		if (unique != other.unique) {
			return false;
		}
		if (!ObjectUtils.nullSafeEquals(language, other.language)) {
			return false;
		}
		if (enforced != other.enforced) {
			return false;
		}
		if (!ObjectUtils.nullSafeEquals(indexFlag, other.indexFlag)) {
			return false;
		}
		return true;
	}
}
