/*
 * Copyright 2002-2010 the original author or authors.
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

import org.springframework.data.sequoiadb.assist.DBCursor;
import org.springframework.data.sequoiadb.core.query.Query;

/**
 * Simple callback interface to allow customization of a {@link DBCursor}.
 * 

 */
interface CursorPreparer {

	/**
	 * Prepare the given cursor (apply limits, skips and so on). Returns th eprepared cursor.
	 * 
	 * @param cursor
	 */
	DBCursor prepare(DBCursor cursor);

	Query getQuery();

	Class<?> getType();
}
