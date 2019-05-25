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
package org.springframework.data.sequoiadb.core;

import static org.mockito.Mockito.*;
import static org.springframework.data.sequoiadb.core.query.Criteria.*;
import static org.springframework.data.sequoiadb.core.query.Query.*;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate.QueryCursorPreparer;
import org.springframework.data.sequoiadb.core.query.Query;

import org.springframework.data.sequoiadb.assist.DBCursor;

/**
 * Unit tests for {@link QueryCursorPreparer}.
 * 


 */
@RunWith(MockitoJUnitRunner.class)
public class QueryCursorPreparerUnitTests {

	@Mock
    SequoiadbFactory factory;
	@Mock DBCursor cursor;

	@Mock DBCursor cursorToUse;

	@Before
	public void setUp() {
		when(cursor.copy()).thenReturn(cursorToUse);
	}

	/**
	 * @see DATA_JIRA-185
	 */
	@Test
	public void appliesHintsCorrectly() {

		Query query = query(where("foo").is("bar")).withHint("hint");

		pepare(query);

		verify(cursorToUse).hint("hint");
	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void doesNotApplyMetaWhenEmpty() {

	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void appliesMaxScanCorrectly() {
	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void appliesMaxTimeCorrectly() {

	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void appliesCommentCorrectly() {

	}

	/**
	 * @see DATA_JIRA-957
	 */
	@Test
	public void appliesSnapshotCorrectly() {

	}

	private DBCursor pepare(Query query) {

		CursorPreparer preparer = new SequoiadbTemplate(factory).new QueryCursorPreparer(query, null);
		return preparer.prepare(cursor);
	}
}
