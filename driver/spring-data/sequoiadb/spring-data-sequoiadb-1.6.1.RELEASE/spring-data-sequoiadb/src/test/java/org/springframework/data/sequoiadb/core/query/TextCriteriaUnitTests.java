/*
 * Copyright 2014 the original author or authors.
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
package org.springframework.data.sequoiadb.core.query;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.hamcrest.core.IsEqual;
import org.junit.Assert;
import org.junit.Test;


import org.springframework.data.sequoiadb.core.DBObjectTestUtils;

/**
 * Unit tests for {@link TextCriteria}.
 * 

 */
public class TextCriteriaUnitTests {

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldNotHaveLanguageField() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage();
		Assert.assertThat(criteria.getCriteriaObject(), IsEqual.equalTo(searchObject("{ }")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldNotHaveLanguageForNonDefaultLanguageField() {

		TextCriteria criteria = TextCriteria.forLanguage("spanish");
		Assert.assertThat(criteria.getCriteriaObject(), IsEqual.equalTo(searchObject("{ \"$language\" : \"spanish\" }")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldCreateSearchFieldForSingleTermCorrectly() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage().matching("cake");
		Assert.assertThat(criteria.getCriteriaObject(), IsEqual.equalTo(searchObject("{ \"$search\" : \"cake\" }")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldCreateSearchFieldCorrectlyForMultipleTermsCorrectly() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage().matchingAny("bake", "coffee", "cake");
		Assert.assertThat(criteria.getCriteriaObject(),
				IsEqual.equalTo(searchObject("{ \"$search\" : \"bake coffee cake\" }")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldCreateSearchFieldForPhraseCorrectly() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage().matchingPhrase("coffee cake");
		Assert.assertThat(DBObjectTestUtils.getAsDBObject(criteria.getCriteriaObject(), "$text"),
				IsEqual.<BSONObject> equalTo(new BasicBSONObject("$search", "\"coffee cake\"")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldCreateNotFieldCorrectly() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage().notMatching("cake");
		Assert.assertThat(criteria.getCriteriaObject(), IsEqual.equalTo(searchObject("{ \"$search\" : \"-cake\" }")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldCreateSearchFieldCorrectlyForNotMultipleTermsCorrectly() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage().notMatchingAny("bake", "coffee", "cake");
		Assert.assertThat(criteria.getCriteriaObject(),
				IsEqual.equalTo(searchObject("{ \"$search\" : \"-bake -coffee -cake\" }")));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldCreateSearchFieldForNotPhraseCorrectly() {

		TextCriteria criteria = TextCriteria.forDefaultLanguage().notMatchingPhrase("coffee cake");
		Assert.assertThat(DBObjectTestUtils.getAsDBObject(criteria.getCriteriaObject(), "$text"),
				IsEqual.<BSONObject> equalTo(new BasicBSONObject("$search", "-\"coffee cake\"")));
	}

	private BSONObject searchObject(String json) {
		return new BasicBSONObject("$text", JSON.parse(json));
	}

}
