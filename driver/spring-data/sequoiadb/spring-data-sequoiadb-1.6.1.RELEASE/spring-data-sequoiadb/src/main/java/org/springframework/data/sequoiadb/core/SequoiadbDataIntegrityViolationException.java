/*
 * Copyright 2013 the original author or authors.
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

import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.util.Assert;

import org.springframework.data.sequoiadb.assist.WriteResult;

/**
 * Sdb-specific {@link DataIntegrityViolationException}.
 * 

 */
public class SequoiadbDataIntegrityViolationException extends DataIntegrityViolationException {

	private static final long serialVersionUID = -186980521176764046L;

	private final WriteResult writeResult;
	private final SequoiadbActionOperation actionOperation;

	/**
	 * Creates a new {@link SequoiadbDataIntegrityViolationException} using the given message and {@link WriteResult}.
	 * 
	 * @param message the exception message
	 * @param writeResult the {@link WriteResult} that causes the exception, must not be {@literal null}.
	 * @param actionOperation the {@link SequoiadbActionOperation} that caused the exception, must not be {@literal null}.
	 */
	public SequoiadbDataIntegrityViolationException(String message, WriteResult writeResult,
													SequoiadbActionOperation actionOperation) {

		super(message);

		Assert.notNull(writeResult, "WriteResult must not be null!");
		Assert.notNull(actionOperation, "SequoiadbActionOperation must not be null!");

		this.writeResult = writeResult;
		this.actionOperation = actionOperation;
	}

	/**
	 * Returns the {@link WriteResult} that caused the exception.
	 * 
	 * @return the writeResult
	 */
	public WriteResult getWriteResult() {
		return writeResult;
	}

	/**
	 * Returns the {@link SequoiadbActionOperation} in which the current exception occured.
	 * 
	 * @return the actionOperation
	 */
	public SequoiadbActionOperation getActionOperation() {
		return actionOperation;
	}
}
