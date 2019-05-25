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
package org.springframework.data.mongodb.gridfs;

import static org.springframework.data.mongodb.core.query.Query.*;
import static org.springframework.data.mongodb.gridfs.GridFsCriteria.*;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import org.springframework.core.io.support.ResourcePatternResolver;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.data.mongodb.core.convert.MongoConverter;
import org.springframework.data.mongodb.core.convert.QueryMapper;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DB;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.GridFS;
import org.springframework.data.mongodb.assist.GridFSDBFile;
import org.springframework.data.mongodb.assist.GridFSFile;
import org.springframework.data.mongodb.assist.GridFSInputFile;

/**
 * {@link GridFsOperations} implementation to store content into MongoDB GridFS.
 * 
 * @author Oliver Gierke
 * @author Philipp Schneider
 * @author Thomas Darimont
 * @author Martin Baumgartner
 */
public class GridFsTemplate implements GridFsOperations, ResourcePatternResolver {

	private final MongoDbFactory dbFactory;
	private final String bucket;
	private final MongoConverter converter;
	private final QueryMapper queryMapper;

	/**
	 * Creates a new {@link GridFsTemplate} using the given {@link MongoDbFactory} and {@link MongoConverter}.
	 * 
	 * @param dbFactory must not be {@literal null}.
	 * @param converter must not be {@literal null}.
	 */
	public GridFsTemplate(MongoDbFactory dbFactory, MongoConverter converter) {
		this(dbFactory, converter, null);
	}

	/**
	 * Creates a new {@link GridFsTemplate} using the given {@link MongoDbFactory} and {@link MongoConverter}.
	 * 
	 * @param dbFactory must not be {@literal null}.
	 * @param converter must not be {@literal null}.
	 * @param bucket
	 */
	public GridFsTemplate(MongoDbFactory dbFactory, MongoConverter converter, String bucket) {

		Assert.notNull(dbFactory);
		Assert.notNull(converter);

		this.dbFactory = dbFactory;
		this.converter = converter;
		this.bucket = bucket;

		this.queryMapper = new QueryMapper(converter);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String)
	 */
	public GridFSFile store(InputStream content, String filename) {
		return store(content, filename, (Object) null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.Object)
	 */

	@Override
	public GridFSFile store(InputStream content, Object metadata) {
		return store(content, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public GridFSFile store(InputStream content, DBObject metadata) {
		return store(content, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, java.lang.String)
	 */
	public GridFSFile store(InputStream content, String filename, String contentType) {
		return store(content, filename, contentType, (Object) null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, java.lang.Object)
	 */
	public GridFSFile store(InputStream content, String filename, Object metadata) {
		return store(content, filename, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, java.lang.String, java.lang.Object)
	 */
	public GridFSFile store(InputStream content, String filename, String contentType, Object metadata) {

		DBObject dbObject = null;

		if (metadata != null) {
			dbObject = new BasicDBObject();
			converter.write(metadata, dbObject);
		}

		return store(content, filename, contentType, dbObject);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, org.springframework.data.mongodb.assist.DBObject)
	 */
	public GridFSFile store(InputStream content, String filename, DBObject metadata) {
		return this.store(content, filename, null, metadata);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#store(java.io.InputStream, java.lang.String, org.springframework.data.mongodb.assist.DBObject)
	 */
	public GridFSFile store(InputStream content, String filename, String contentType, DBObject metadata) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#find(org.springframework.data.mongodb.assist.DBObject)
	 */
	public List<GridFSDBFile> find(Query query) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#findOne(org.springframework.data.mongodb.assist.DBObject)
	 */
	public GridFSDBFile findOne(Query query) {
		throw new UnsupportedOperationException("not supported!");
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.gridfs.GridFsOperations#delete(org.springframework.data.mongodb.core.query.Query)
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

	private DBObject getMappedQuery(Query query) {
		return query == null ? new Query().getQueryObject() : getMappedQuery(query.getQueryObject());
	}

	private DBObject getMappedQuery(DBObject query) {
		return query == null ? null : queryMapper.getMappedObject(query, null);
	}

	private GridFS getGridFs() {
		throw new UnsupportedOperationException("not supported!");
	}
}
