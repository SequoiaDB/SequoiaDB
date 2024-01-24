/*
 * Copyright 2012 the original author or authors.
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
package org.springframework.data.sequoiadb.core.mapreduce;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.util.Collections;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;




/**
 * Unit tests for {@link MapReduceResults}.
 * 

 */
public class MapReduceResultsUnitTests {

	/**
	 * @see DATA_JIRA-428
	 */
	@Test
	public void resolvesOutputCollectionForPlainResult() {

		BSONObject rawResult = new BasicBSONObject("result", "FOO");
		MapReduceResults<Object> results = new MapReduceResults<Object>(Collections.emptyList(), rawResult);

		assertThat(results.getOutputCollection(), is("FOO"));
	}

	/**
	 * @see DATA_JIRA-428
	 */
	@Test
	public void resolvesOutputCollectionForDBObjectResult() {

		BSONObject rawResult = new BasicBSONObject("result", new BasicBSONObject("collection", "FOO"));
		MapReduceResults<Object> results = new MapReduceResults<Object>(Collections.emptyList(), rawResult);

		assertThat(results.getOutputCollection(), is("FOO"));
	}

	/**
	 * @see DATA_JIRA-378
	 */
	@Test
	public void handlesLongTotalInResult() {

		BSONObject inner = new BasicBSONObject("total", 1L);
		inner.put("mapTime", 1L);
		inner.put("emitLoop", 1);

		BSONObject source = new BasicBSONObject("timing", inner);
		new MapReduceResults<Object>(Collections.emptyList(), source);
	}

	/**
	 * @see DATA_JIRA-378
	 */
	@Test
	public void handlesLongResultsForCounts() {

		BSONObject inner = new BasicBSONObject("input", 1L);
		inner.put("emit", 1L);
		inner.put("output", 1);

		BSONObject source = new BasicBSONObject("counts", inner);
		new MapReduceResults<Object>(Collections.emptyList(), source);
	}
}
