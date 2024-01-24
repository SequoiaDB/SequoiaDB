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
package org.springframework.data.sequoiadb.repository.support;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.domain.Sort;
import org.springframework.data.domain.Sort.Direction;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.index.Index;
import org.springframework.data.sequoiadb.repository.query.SequoiadbEntityMetadata;
import org.springframework.data.sequoiadb.repository.query.PartTreeSequoiadbQuery;
import org.springframework.data.repository.core.support.QueryCreationListener;
import org.springframework.data.repository.query.parser.Part;
import org.springframework.data.repository.query.parser.Part.Type;
import org.springframework.data.repository.query.parser.PartTree;
import org.springframework.util.Assert;

/**
 * {@link QueryCreationListener} inspecting {@link PartTreeSequoiadbQuery}s and creating an index for the properties it
 * refers to.
 * 

 */
class IndexEnsuringQueryCreationListener implements QueryCreationListener<PartTreeSequoiadbQuery> {

	private static final Set<Type> GEOSPATIAL_TYPES = new HashSet<Type>(Arrays.asList(Type.NEAR, Type.WITHIN));
	private static final Logger LOG = LoggerFactory.getLogger(IndexEnsuringQueryCreationListener.class);

	private final SequoiadbOperations operations;

	/**
	 * Creates a new {@link IndexEnsuringQueryCreationListener} using the given {@link SequoiadbOperations}.
	 * 
	 * @param operations must not be {@literal null}.
	 */
	public IndexEnsuringQueryCreationListener(SequoiadbOperations operations) {

		Assert.notNull(operations);
		this.operations = operations;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.core.support.QueryCreationListener#onCreation(org.springframework.data.repository.query.RepositoryQuery)
	 */
	public void onCreation(PartTreeSequoiadbQuery query) {

		PartTree tree = query.getTree();
		Index index = new Index();
		index.named(query.getQueryMethod().getName());
		Sort sort = tree.getSort();

		for (Part part : tree.getParts()) {
			if (GEOSPATIAL_TYPES.contains(part.getType())) {
				return;
			}
			String property = part.getProperty().toDotPath();
			Direction order = toDirection(sort, property);
			index.on(property, order);
		}

		// Add fixed sorting criteria to index
		if (sort != null) {
			for (Sort.Order order : sort) {
				index.on(order.getProperty(), order.getDirection());
			}
		}

		SequoiadbEntityMetadata<?> metadata = query.getQueryMethod().getEntityInformation();
		operations.indexOps(metadata.getCollectionName()).ensureIndex(index);
		LOG.debug(String.format("Created %s!", index));
	}

	private static Direction toDirection(Sort sort, String property) {

		if (sort == null) {
			return Direction.DESC;
		}

		org.springframework.data.domain.Sort.Order order = sort.getOrderFor(property);
		return order == null ? Direction.DESC : order.isAscending() ? Direction.ASC : Direction.DESC;
	}
}
