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
package org.springframework.data.sequoiadb.gridfs;

import static org.springframework.data.sequoiadb.core.query.Query.*;
import static org.springframework.data.sequoiadb.gridfs.GridFsCriteria.*;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.core.io.support.ResourcePatternResolver;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.QueryMapper;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;


import org.springframework.data.sequoiadb.assist.GridFS;
import org.springframework.data.sequoiadb.assist.GridFSDBFile;
import org.springframework.data.sequoiadb.assist.GridFSFile;

/**
 * {@link GridFsOperations} implementation to store content into SequoiaDB GridFS.
 * 




 */
public class GridFsTemplate implements GridFsOperations, ResourcePatternResolver {

	private final SequoiadbFactory dbFactory;
	private final String bucket;
	private final SequoiadbConverter converter;
	private final QueryMapper queryMapper;

	/**
	 * Creates a new {@link GridFsTemplate} using the given {@link SequoiadbFactory} and {@link SequoiadbConverter}.
	 * 
	 * @param dbFactory must not be {@literal null}.
	 * @param converter must not be {@literal null}.
	 */
	public GridFsTemplate(SequoiadbFactory dbFactory, SequoiadbConverter converter) {
		this(dbFactory, converter, null);
	}

	/**
	 * Creates a new {@link GridFsTemplate} using the given {@link SequoiadbFactory} and {@link SequoiadbConverter}.
	 * 
	 * @param dbFactory must not be {@literal null}.
	 * @param converter must not be {@literal null}.
	 * @param bucket
	 */
	public GridFsTemplate(SequoiadbFactory dbFactory, SequoiadbConverter converter, String bucket) {

		Assert.notNull(dbFactory);
		Assert.notNull(converter);

		this.dbFactory = dbFactory;
		this.converter = converter;
		this.bucket = bucket;

		this.queryMapper = new QueryMapper(converter);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String)
	 */
	public GridFSFile store(InputStream content, String filename) {
		return store(content, filename, (Object) null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.Object)
	 */

	@Override
	public GridFSFile store(InputStream content, Object metadata) {
		return store(content, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, BSONObject)
	 */
	@Override
	public GridFSFile store(InputStream content, BSONObject metadata) {
		return store(content, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, java.lang.String)
	 */
	public GridFSFile store(InputStream content, String filename, String contentType) {
		return store(content, filename, contentType, (Object) null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, java.lang.Object)
	 */
	public GridFSFile store(InputStream content, String filename, Object metadata) {
		return store(content, filename, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, java.lang.String, java.lang.Object)
	 */
	public GridFSFile store(InputStream content, String filename, String contentType, Object metadata) {

		BSONObject dbObject = null;

		if (metadata != null) {
			dbObject = new BasicBSONObject();
			converter.write(metadata, dbObject);
		}

		return store(content, filename, contentType, dbObject);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, BSONObject)
	 */
	public GridFSFile store(InputStream content, String filename, BSONObject metadata) {
		return this.store(content, filename, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, BSONObject)
	 */
	public GridFSFile store(InputStream content, String filename, String contentType, BSONObject metadata) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#find(BSONObject)
	 */
	public List<GridFSDBFile> find(Query query) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#findOne(BSONObject)
	 */
	public GridFSDBFile findOne(Query query) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.gridfs.GridFsOperations#delete(org.springframework.data.sequoiadb.core.query.Query)
	 */
	public void delete(Query query) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.core.io.ResourceLoader#getClassLoader()
	 */
	public ClassLoader getClassLoader() {
		return dbFactory.getClass().getClassLoader();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.core.io.ResourceLoader#getResource(java.lang.String)
	 */
	public GridFsResource getResource(String location) {

		GridFSDBFile file = findOne(query(whereFilename().is(location)));
		return file != null ? new GridFsResource(file) : null;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.core.io.support.ResourcePatternResolver#getResources(java.lang.String)
	 */
	public GridFsResource[] getResources(String locationPattern) {

		if (!StringUtils.hasText(locationPattern)) {
			return new GridFsResource[0];
		}

		AntPath path = new AntPath(locationPattern);

		if (path.isPattern()) {

			List<GridFSDBFile> files = find(query(whereFilename().regex(path.toRegex())));
			List<GridFsResource> resources = new ArrayList<GridFsResource>(files.size());

			for (GridFSDBFile file : files) {
				resources.add(new GridFsResource(file));
			}

			return resources.toArray(new GridFsResource[resources.size()]);
		}

		return new GridFsResource[] { getResource(locationPattern) };
	}

	private BSONObject getMappedQuery(Query query) {
		return query == null ? new Query().getQueryObject() : getMappedQuery(query.getQueryObject());
	}

	private BSONObject getMappedQuery(BSONObject query) {
		return query == null ? null : queryMapper.getMappedObject(query, null);
	}

	private GridFS getGridFs() {
		throw new UnsupportedOperationException("not supported!");
	}
}
