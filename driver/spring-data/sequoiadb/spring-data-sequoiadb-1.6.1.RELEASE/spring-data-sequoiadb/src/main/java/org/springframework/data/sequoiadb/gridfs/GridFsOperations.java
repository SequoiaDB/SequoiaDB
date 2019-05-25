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

import java.io.InputStream;
import java.util.List;

import org.bson.BSONObject;
import org.springframework.core.io.support.ResourcePatternResolver;
import org.springframework.data.domain.Sort;
import org.springframework.data.sequoiadb.core.query.Query;


import org.springframework.data.sequoiadb.assist.GridFSDBFile;
import org.springframework.data.sequoiadb.assist.GridFSFile;

/**
 * Collection of operations to store and read files from SequoiaDB GridFS.
 * 




 */
public interface GridFsOperations extends ResourcePatternResolver {

	/**
	 * Stores the given content into a file with the given name.
	 * 
	 * @param content must not be {@literal null}.
	 * @param filename must not be {@literal null} or empty.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, String filename);

	/**
	 * Stores the given content into a file with the given name.
	 * 
	 * @param content must not be {@literal null}.
	 * @param metadata can be {@literal null}.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, Object metadata);

	/**
	 * Stores the given content into a file with the given name.
	 * 
	 * @param content must not be {@literal null}.
	 * @param metadata can be {@literal null}.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, BSONObject metadata);

	/**
	 * Stores the given content into a file with the given name and content type.
	 * 
	 * @param content must not be {@literal null}.
	 * @param filename must not be {@literal null} or empty.
	 * @param contentType can be {@literal null}.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, String filename, String contentType);

	/**
	 * Stores the given content into a file with the given name using the given metadata. The metadata object will be
	 * marshalled before writing.
	 * 
	 * @param content must not be {@literal null}.
	 * @param filename must not be {@literal null} or empty.
	 * @param metadata can be {@literal null}.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, String filename, Object metadata);

	/**
	 * Stores the given content into a file with the given name and content type using the given metadata. The metadata
	 * object will be marshalled before writing.
	 * 
	 * @param content must not be {@literal null}.
	 * @param filename must not be {@literal null} or empty.
	 * @param contentType can be {@literal null}.
	 * @param metadata can be {@literal null}
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, String filename, String contentType, Object metadata);

	/**
	 * Stores the given content into a file with the given name using the given metadata.
	 * 
	 * @param content must not be {@literal null}.
	 * @param filename must not be {@literal null} or empty.
	 * @param metadata can be {@literal null}.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, String filename, BSONObject metadata);

	/**
	 * Stores the given content into a file with the given name and content type using the given metadata.
	 * 
	 * @param content must not be {@literal null}.
	 * @param filename must not be {@literal null} or empty.
	 * @param contentType can be {@literal null}.
	 * @param metadata can be {@literal null}.
	 * @return the {@link GridFSFile} just created
	 */
	GridFSFile store(InputStream content, String filename, String contentType, BSONObject metadata);

	/**
	 * Returns all files matching the given query. Note, that currently {@link Sort} criterias defined at the
	 * {@link Query} will not be regarded as SequoiaDB does not support ordering for GridFS file access.
	 * 
	 * @see https://jira.sequoiadb.org/browse/JAVA-431
	 * @param query
	 * @return
	 */
	List<GridFSDBFile> find(Query query);

	/**
	 * Returns a single file matching the given query or {@literal null} in case no file matches.
	 * 
	 * @param query
	 * @return
	 */
	GridFSDBFile findOne(Query query);

	/**
	 * Deletes all files matching the given {@link Query}.
	 * 
	 * @param query
	 */
	void delete(Query query);

	/**
	 * Returns all {@link GridFsResource} with the given file name.
	 * 
	 * @param filename
	 * @return the resource if it exists or {@literal null}.
	 * @see ResourcePatternResolver#getResource(String)
	 */
	GridFsResource getResource(String filename);

	/**
	 * Returns all {@link GridFsResource}s matching the given file name pattern.
	 * 
	 * @param filenamePattern
	 * @return
	 * @see ResourcePatternResolver#getResources(String)
	 */
	GridFsResource[] getResources(String filenamePattern);
}
