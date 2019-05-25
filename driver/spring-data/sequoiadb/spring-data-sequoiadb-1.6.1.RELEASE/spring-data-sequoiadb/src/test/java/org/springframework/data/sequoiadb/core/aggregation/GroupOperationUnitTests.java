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
package org.springframework.data.sequoiadb.core.aggregation;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;
import static org.springframework.data.sequoiadb.core.aggregation.Fields.*;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;


import org.springframework.data.sequoiadb.core.DBObjectTestUtils;

/**
 * Unit tests for {@link GroupOperation}.
 * 

 */
public class GroupOperationUnitTests {

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullFields() {
		new GroupOperation((Fields) null);
	}

	/**
	 * @see DATA_JIRA-759
	 */
	@Test
	public void groupOperationWithNoGroupIdFieldsShouldGenerateNullAsGroupId() {

		GroupOperation operation = new GroupOperation(Fields.from());
		ExposedFields fields = operation.getFields();
		BSONObject groupClause = extractDbObjectFromGroupOperation(operation);

		assertThat(fields.exposesSingleFieldOnly(), is(true));
		assertThat(fields.exposesNoFields(), is(false));
		assertThat(groupClause.get(UNDERSCORE_ID), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-759
	 */
	@Test
	public void groupOperationWithNoGroupIdFieldsButAdditionalFieldsShouldGenerateNullAsGroupId() {

		GroupOperation operation = new GroupOperation(Fields.from()).count().as("cnt").last("foo").as("foo");
		ExposedFields fields = operation.getFields();
		BSONObject groupClause = extractDbObjectFromGroupOperation(operation);

		assertThat(fields.exposesSingleFieldOnly(), is(false));
		assertThat(fields.exposesNoFields(), is(false));
		assertThat(groupClause.get(UNDERSCORE_ID), is(nullValue()));
		assertThat((BasicBSONObject) groupClause.get("cnt"), is(new BasicBSONObject("$sum", 1)));
		assertThat((BasicBSONObject) groupClause.get("foo"), is(new BasicBSONObject("$last", "$foo")));
	}

	@Test
	public void createsGroupOperationWithSingleField() {

		GroupOperation operation = new GroupOperation(fields("a"));

		BSONObject groupClause = extractDbObjectFromGroupOperation(operation);

		assertThat(groupClause.get(UNDERSCORE_ID), is((Object) "$a"));
	}

	@Test
	public void createsGroupOperationWithMultipleFields() {

		GroupOperation operation = new GroupOperation(fields("a").and("b", "c"));

		BSONObject groupClause = extractDbObjectFromGroupOperation(operation);
		BSONObject idClause = DBObjectTestUtils.getAsDBObject(groupClause, UNDERSCORE_ID);

		assertThat(idClause.get("a"), is((Object) "$a"));
		assertThat(idClause.get("b"), is((Object) "$c"));
	}

	@Test
	public void groupFactoryMethodWithMultipleFieldsAndSumOperation() {

		GroupOperation groupOperation = Aggregation.group(fields("a", "b").and("c")) //
				.sum("e").as("e");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject eOp = DBObjectTestUtils.getAsDBObject(groupClause, "e");
		assertThat(eOp, is((BSONObject) new BasicBSONObject("$sum", "$e")));
	}

	@Test
	public void groupFactoryMethodWithMultipleFieldsAndSumOperationWithAlias() {

		GroupOperation groupOperation = Aggregation.group(fields("a", "b").and("c")) //
				.sum("e").as("ee");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject eOp = DBObjectTestUtils.getAsDBObject(groupClause, "ee");
		assertThat(eOp, is((BSONObject) new BasicBSONObject("$sum", "$e")));
	}

	@Test
	public void groupFactoryMethodWithMultipleFieldsAndCountOperationWithout() {

		GroupOperation groupOperation = Aggregation.group(fields("a", "b").and("c")) //
				.count().as("count");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject eOp = DBObjectTestUtils.getAsDBObject(groupClause, "count");
		assertThat(eOp, is((BSONObject) new BasicBSONObject("$sum", 1)));
	}

	@Test
	public void groupFactoryMethodWithMultipleFieldsAndMultipleAggregateOperationsWithAlias() {

		GroupOperation groupOperation = Aggregation.group(fields("a", "b").and("c")) //
				.sum("e").as("sum") //
				.min("e").as("min"); //

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject sum = DBObjectTestUtils.getAsDBObject(groupClause, "sum");
		assertThat(sum, is((BSONObject) new BasicBSONObject("$sum", "$e")));

		BSONObject min = DBObjectTestUtils.getAsDBObject(groupClause, "min");
		assertThat(min, is((BSONObject) new BasicBSONObject("$min", "$e")));
	}

	@Test
	public void groupOperationPushWithValue() {

		GroupOperation groupOperation = Aggregation.group("a", "b").push(1).as("x");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject push = DBObjectTestUtils.getAsDBObject(groupClause, "x");

		assertThat(push, is((BSONObject) new BasicBSONObject("$push", 1)));
	}

	@Test
	public void groupOperationPushWithReference() {

		GroupOperation groupOperation = Aggregation.group("a", "b").push("ref").as("x");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject push = DBObjectTestUtils.getAsDBObject(groupClause, "x");

		assertThat(push, is((BSONObject) new BasicBSONObject("$push", "$ref")));
	}

	@Test
	public void groupOperationAddToSetWithReference() {

		GroupOperation groupOperation = Aggregation.group("a", "b").addToSet("ref").as("x");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject push = DBObjectTestUtils.getAsDBObject(groupClause, "x");

		assertThat(push, is((BSONObject) new BasicBSONObject("$addToSet", "$ref")));
	}

	@Test
	public void groupOperationAddToSetWithValue() {

		GroupOperation groupOperation = Aggregation.group("a", "b").addToSet(42).as("x");

		BSONObject groupClause = extractDbObjectFromGroupOperation(groupOperation);
		BSONObject push = DBObjectTestUtils.getAsDBObject(groupClause, "x");

		assertThat(push, is((BSONObject) new BasicBSONObject("$addToSet", 42)));
	}

	private BSONObject extractDbObjectFromGroupOperation(GroupOperation groupOperation) {
		BSONObject dbObject = groupOperation.toDBObject(Aggregation.DEFAULT_CONTEXT);
		BSONObject groupClause = DBObjectTestUtils.getAsDBObject(dbObject, "$group");
		return groupClause;
	}
}
