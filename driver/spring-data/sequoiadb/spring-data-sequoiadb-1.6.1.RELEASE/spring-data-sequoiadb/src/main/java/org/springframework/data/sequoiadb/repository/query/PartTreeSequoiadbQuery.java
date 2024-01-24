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
package org.springframework.data.sequoiadb.repository.query;

import org.bson.util.JSONParseException;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.query.BasicQuery;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.core.query.TextCriteria;
import org.springframework.data.repository.query.QueryMethod;
import org.springframework.data.repository.query.RepositoryQuery;
import org.springframework.data.repository.query.parser.PartTree;
import org.springframework.util.StringUtils;

/**
 * {@link RepositoryQuery} implementation for Sdb.
 * 


 */
public class PartTreeSequoiadbQuery extends AbstractSequoiadbQuery {

	private final PartTree tree;
	private final boolean isGeoNearQuery;
	private final MappingContext<?, SequoiadbPersistentProperty> context;

	/**
	 * Creates a new {@link PartTreeSequoiadbQuery} from the given {@link QueryMethod} and {@link SequoiadbTemplate}.
	 * 
	 * @param method must not be {@literal null}.
	 * @param template must not be {@literal null}.
	 */
	public PartTreeSequoiadbQuery(SequoiadbQueryMethod method, SequoiadbOperations sequoiadbOperations) {

		super(method, sequoiadbOperations);
		this.tree = new PartTree(method.getName(), method.getEntityInformation().getJavaType());
		this.isGeoNearQuery = method.isGeoNearQuery();
		this.context = sequoiadbOperations.getConverter().getMappingContext();
	}

	/**
	 * Return the {@link PartTree} backing the query.
	 * 
	 * @return the tree
	 */
	public PartTree getTree() {
		return tree;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery#createQuery(org.springframework.data.sequoiadb.repository.query.ConvertingParameterAccessor, boolean)
	 */
	@Override
	protected Query createQuery(ConvertingParameterAccessor accessor) {

		sequoiadbQueryCreator creator = new sequoiadbQueryCreator(tree, accessor, context, isGeoNearQuery);
		Query query = creator.createQuery();

		if (tree.isLimiting()) {
			query.limit(tree.getMaxResults());
		}

		TextCriteria textCriteria = accessor.getFullText();
		if (textCriteria != null) {
			query.addCriteria(textCriteria);
		}

		String fieldSpec = this.getQueryMethod().getFieldSpecification();

		if (!StringUtils.hasText(fieldSpec)) {
			return query;
		}

		try {

			BasicQuery result = new BasicQuery(query.getQueryObject().toString(), fieldSpec);
			result.setSortObject(query.getSortObject());

			return result;

		} catch (JSONParseException o_O) {
			throw new IllegalStateException(String.format("Invalid query or field specification in %s!", getQueryMethod(),
					o_O));
		}
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery#createCountQuery(org.springframework.data.sequoiadb.repository.query.ConvertingParameterAccessor)
	 */
	@Override
	protected Query createCountQuery(ConvertingParameterAccessor accessor) {
		return new sequoiadbQueryCreator(tree, accessor, context, false).createQuery();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery#isCountQuery()
	 */
	@Override
	protected boolean isCountQuery() {
		return tree.isCountProjection();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.query.AbstractSequoiadbQuery#isDeleteQuery()
	 */
	@Override
	protected boolean isDeleteQuery() {
		return tree.isDelete();
	}
}
