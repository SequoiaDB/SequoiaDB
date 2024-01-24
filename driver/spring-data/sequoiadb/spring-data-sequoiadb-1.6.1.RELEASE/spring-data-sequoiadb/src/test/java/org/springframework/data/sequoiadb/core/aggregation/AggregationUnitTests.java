/*
 * Copyright 2013-2014 the original author or authors.
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
import static org.springframework.data.sequoiadb.core.DBObjectTestUtils.*;
import static org.springframework.data.sequoiadb.core.aggregation.Aggregation.*;
import static org.springframework.data.sequoiadb.core.query.Criteria.*;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.springframework.data.domain.Sort.Direction;




/**
 * Unit tests for {@link Aggregation}.
 * 


 */
public class AggregationUnitTests {

	public @Rule ExpectedException exception = ExpectedException.none();

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullAggregationOperation() {
		newAggregation((AggregationOperation[]) null);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullTypedAggregationOperation() {
		newAggregation(String.class, (AggregationOperation[]) null);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNoAggregationOperation() {
		newAggregation(new AggregationOperation[0]);
	}

	@Test(expected = IllegalArgumentException.class)
	public void rejectsNoTypedAggregationOperation() {
		newAggregation(String.class, new AggregationOperation[0]);
	}

	/**
	 * @see DATA_JIRA-753
	 */
	@Test
	public void checkForCorrectFieldScopeTransfer() {

		exception.expect(IllegalArgumentException.class);
		exception.expectMessage("Invalid reference");
		exception.expectMessage("'b'");

		newAggregation( //
				project("a", "b"), //
				group("a").count().as("cnt"), // a was introduced to the context by the project operation
				project("cnt", "b") // b was removed from the context by the group operation
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT); // -> triggers IllegalArgumentException
	}

	/**
	 * @see DATA_JIRA-753
	 */
	@Test
	public void unwindOperationShouldNotChangeAvailableFields() {

		newAggregation( //
				project("a", "b"), //
				unwind("a"), //
				project("a", "b") // b should still be available
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);
	}

	/**
	 * @see DATA_JIRA-753
	 */
	@Test
	public void matchOperationShouldNotChangeAvailableFields() {

		newAggregation( //
				project("a", "b"), //
				match(where("a").gte(1)), //
				project("a", "b") // b should still be available
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);
	}

	/**
	 * @see DATA_JIRA-788
	 */
	@Test
	public void referencesToGroupIdsShouldBeRenderedAsReferences() {

		BSONObject agg = newAggregation( //
				project("a"), //
				group("a").count().as("aCnt"), //
				project("aCnt", "a") //
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		@SuppressWarnings("unchecked")
        BSONObject secondProjection = ((List<BSONObject>) agg.get("pipeline")).get(2);
		BSONObject fields = getAsDBObject(secondProjection, "$project");
		assertThat(fields.get("aCnt"), is((Object) 1));
		assertThat(fields.get("a"), is((Object) "$_id.a"));
	}

	/**
	 * @see DATA_JIRA-791
	 */
	@Test
	public void allowAggregationOperationsToBePassedAsIterable() {

		List<AggregationOperation> ops = new ArrayList<AggregationOperation>();
		ops.add(project("a"));
		ops.add(group("a").count().as("aCnt"));
		ops.add(project("aCnt", "a"));

		BSONObject agg = newAggregation(ops).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		@SuppressWarnings("unchecked")
        BSONObject secondProjection = ((List<BSONObject>) agg.get("pipeline")).get(2);
		BSONObject fields = getAsDBObject(secondProjection, "$project");
		assertThat(fields.get("aCnt"), is((Object) 1));
		assertThat(fields.get("a"), is((Object) "$_id.a"));
	}

	/**
	 * @see DATA_JIRA-791
	 */
	@Test
	public void allowTypedAggregationOperationsToBePassedAsIterable() {

		List<AggregationOperation> ops = new ArrayList<AggregationOperation>();
		ops.add(project("a"));
		ops.add(group("a").count().as("aCnt"));
		ops.add(project("aCnt", "a"));

		BSONObject agg = newAggregation(BSONObject.class, ops).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		@SuppressWarnings("unchecked")
        BSONObject secondProjection = ((List<BSONObject>) agg.get("pipeline")).get(2);
		BSONObject fields = getAsDBObject(secondProjection, "$project");
		assertThat(fields.get("aCnt"), is((Object) 1));
		assertThat(fields.get("a"), is((Object) "$_id.a"));
	}

	/**
	 * @see DATA_JIRA-838
	 */
	@Test
	public void expressionBasedFieldsShouldBeReferencableInFollowingOperations() {

		BSONObject agg = newAggregation( //
				project("a").andExpression("b+c").as("foo"), //
				group("a").sum("foo").as("foosum") //
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		@SuppressWarnings("unchecked")
        BSONObject secondProjection = ((List<BSONObject>) agg.get("pipeline")).get(1);
		BSONObject fields = getAsDBObject(secondProjection, "$group");
		assertThat(fields.get("foosum"), is((Object) new BasicBSONObject("$sum", "$foo")));
	}

	/**
	 * @see DATA_JIRA-908
	 */
	@Test
	public void shouldSupportReferingToNestedPropertiesInGroupOperation() {

		BSONObject agg = newAggregation( //
				project("cmsParameterId", "rules"), //
				unwind("rules"), //
				group("cmsParameterId", "rules.ruleType").count().as("totol") //
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		assertThat(agg, is(notNullValue()));

		BSONObject group = ((List<BSONObject>) agg.get("pipeline")).get(2);
		BSONObject fields = getAsDBObject(group, "$group");
		BSONObject id = getAsDBObject(fields, "_id");

		assertThat(id.get("ruleType"), is((Object) "$rules.ruleType"));
	}

	/**
	 * @see DATA_JIRA-924
	 */
	@Test
	public void referencingProjectionAliasesFromPreviousStepShouldReferToTheSameFieldTarget() {

		BSONObject agg = newAggregation( //
				project().and("foo.bar").as("ba") //
				, project().and("ba").as("b") //
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		BSONObject projection0 = extractPipelineElement(agg, 0, "$project");
		assertThat(projection0, is((BSONObject) new BasicBSONObject("ba", "$foo.bar")));

		BSONObject projection1 = extractPipelineElement(agg, 1, "$project");
		assertThat(projection1, is((BSONObject) new BasicBSONObject("b", "$ba")));
	}

	/**
	 * @see DATA_JIRA-960
	 */
	@Test
	public void shouldRenderAggregationWithDefaultOptionsCorrectly() {

		BSONObject agg = newAggregation( //
				project().and("a").as("aa") //
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		assertThat(agg.toString(),
				is("{ \"aggregate\" : \"foo\" , \"pipeline\" : [ { \"$project\" : { \"aa\" : \"$a\"}}]}"));
	}

	/**
	 * @see DATA_JIRA-960
	 */
	@Test
	public void shouldRenderAggregationWithCustomOptionsCorrectly() {

		AggregationOptions aggregationOptions = newAggregationOptions().explain(true).cursor(new BasicBSONObject("foo", 1))
				.allowDiskUse(true).build();

		BSONObject agg = newAggregation( //
				project().and("a").as("aa") //
		) //
		.withOptions(aggregationOptions) //
				.toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		assertThat(agg.toString(), is("{ \"aggregate\" : \"foo\" , " //
				+ "\"pipeline\" : [ { \"$project\" : { \"aa\" : \"$a\"}}] , " //
				+ "\"allowDiskUse\" : true , " //
				+ "\"explain\" : true , " //
				+ "\"cursor\" : { \"foo\" : 1}}" //
		));
	}

	/**
	 * @see DATA_JIRA-954
	 */
	@Test
	public void shouldSupportReferencingSystemVariables() {

		BSONObject agg = newAggregation( //
				project("someKey") //
						.and("a").as("a1") //
						.and(Aggregation.CURRENT + ".a").as("a2") //
				, sort(Direction.DESC, "a") //
				, group("someKey").first(Aggregation.ROOT).as("doc") //
		).toDbObject("foo", Aggregation.DEFAULT_CONTEXT);

		BSONObject projection0 = extractPipelineElement(agg, 0, "$project");
		assertThat(projection0, is((BSONObject) new BasicBSONObject("someKey", 1).append("a1", "$a")
				.append("a2", "$$CURRENT.a")));

		BSONObject sort = extractPipelineElement(agg, 1, "$sort");
		assertThat(sort, is((BSONObject) new BasicBSONObject("a", -1)));

		BSONObject group = extractPipelineElement(agg, 2, "$group");
		assertThat(group,
				is((BSONObject) new BasicBSONObject("_id", "$someKey").append("doc", new BasicBSONObject("$first", "$$ROOT"))));
	}

	private BSONObject extractPipelineElement(BSONObject agg, int index, String operation) {

		List<BSONObject> pipeline = (List<BSONObject>) agg.get("pipeline");
		return (BSONObject) pipeline.get(index).get(operation);
	}
}
