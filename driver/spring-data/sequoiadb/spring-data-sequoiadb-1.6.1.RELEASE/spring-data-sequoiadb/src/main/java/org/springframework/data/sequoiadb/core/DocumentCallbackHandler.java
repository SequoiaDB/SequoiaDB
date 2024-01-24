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
package org.springframework.data.sequoiadb.core;

import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.springframework.dao.DataAccessException;




/**
 * An interface used by {@link SequoiadbTemplate} for processing documents returned from a SequoiaDB query on a per-document
 * basis. Implementations of this interface perform the actual work of prcoessing each document but don't need to worry
 * about exception handling. {@BaseException}s will be caught and translated by the calling
 * SequoiadbTemplate
 * 
 * An DocumentCallbackHandler is typically stateful: It keeps the result state within the object, to be available later
 * for later inspection.
 * 

 * 
 */
public interface DocumentCallbackHandler {

	void processDocument(BSONObject dbObject) throws BaseException, DataAccessException;
}
