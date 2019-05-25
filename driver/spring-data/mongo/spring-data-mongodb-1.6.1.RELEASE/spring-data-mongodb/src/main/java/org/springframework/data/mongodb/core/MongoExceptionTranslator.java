/*
 * Copyright 2010-2013 the original author or authors.
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

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.springframework.dao.*;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.data.mongodb.UncategorizedMongoDbException;

/**
 * Simple {@link PersistenceExceptionTranslator} for Mongo. Convert the given runtime exception to an appropriate
 * exception from the {@code org.springframework.dao} hierarchy. Return {@literal null} if no translation is
 * appropriate: any other exception may have resulted from user code, and should not be translated.
 * 
 * @author Oliver Gierke
 * @author Michal Vich
 */
public class MongoExceptionTranslator implements PersistenceExceptionTranslator {

	/*
	 * (non-Javadoc)
	 * @see org.springframework.dao.support.PersistenceExceptionTranslator#translateExceptionIfPossible(java.lang.RuntimeException)
	 */
	public DataAccessException translateExceptionIfPossible(RuntimeException ex) {

		if (ex instanceof DataAccessException) {
			return (DataAccessException)ex;
		}

		if (ex instanceof  BaseException) {
			switch (SDBError.getSDBError(((BaseException) ex).getErrorCode())) {
				case SDB_INVALIDARG: // -6
					return new InvalidDataAccessResourceUsageException(ex.getMessage(), ex);
				case SDB_NETWORK: // -15
					return new DataAccessResourceFailureException(ex.getMessage(), ex);
				case SDB_IXM_DUP_KEY: // -38
					return new DuplicateKeyException(ex.getMessage(), ex);
				case SDB_AUTH_AUTHORITY_FORBIDDEN: // -179
					return new PermissionDeniedDataAccessException(ex.getMessage(), ex);
				default:
					return new UncategorizedMongoDbException(ex.getMessage(), ex);
			}
		}

		return null;
	}
}
