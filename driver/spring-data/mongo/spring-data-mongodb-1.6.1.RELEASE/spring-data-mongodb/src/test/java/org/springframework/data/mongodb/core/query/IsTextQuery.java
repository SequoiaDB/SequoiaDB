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
package org.springframework.data.mongodb.core.query;

import org.hamcrest.TypeSafeMatcher;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.TextQuery;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;

/**
 * A {@link TypeSafeMatcher} that tests whether a given {@link TextQuery} matches a query specification.
 * 
 * @author Christoph Strobl
 * @param <T>
 */
public class IsTextQuery<T extends Query> extends IsQuery<T> {

	private final String SCORE_DEFAULT_FIELDNAME = "score";
	private final DBObject META_TEXT_SCORE = new BasicDBObject("$meta", "textScore");

	private String scoreFieldName = SCORE_DEFAULT_FIELDNAME;

	private IsTextQuery() {
		super();
	}

	public static <T extends Query> IsTextQuery<T> isTextQuery() {
		return new IsTextQuery<T>();
	}

	public IsTextQuery<T> searchingFor(String term) {
		appendTerm(term);
		return this;
	}

	public IsTextQuery<T> inLanguage(String language) {
		appendLanguage(language);
		return this;
	}

	public IsTextQuery<T> returningScore() {

		if (fields == null) {
			fields = new BasicDBObject();
		}
		fields.put(scoreFieldName, META_TEXT_SCORE);

		return this;
	}

	public IsTextQuery<T> returningScoreAs(String fieldname) {

		this.scoreFieldName = fieldname != null ? fieldname : SCORE_DEFAULT_FIELDNAME;

		return this.returningScore();
	}

	public IsTextQuery<T> sortingByScore() {

		sort.put(scoreFieldName, META_TEXT_SCORE);

		return this;
	}

	@Override
	public IsTextQuery<T> where(Criteria criteria) {

		super.where(criteria);
		return this;
	}

	@Override
	public IsTextQuery<T> excludingField(String fieldname) {

		super.excludingField(fieldname);
		return this;
	}

	@Override
	public IsTextQuery<T> includingField(String fieldname) {

		super.includingField(fieldname);
		return this;
	}

	@Override
	public IsTextQuery<T> limitingTo(int limit) {

		super.limitingTo(limit);
		return this;
	}

	@Override
	public IsQuery<T> skippig(int skip) {

		super.skippig(skip);
		return this;
	}

	private void appendLanguage(String language) {

		DBObject dbo = getOrCreateTextDbo();
		dbo.put("$language", language);
	}

	private DBObject getOrCreateTextDbo() {

		DBObject dbo = (DBObject) query.get("$text");
		if (dbo == null) {
			dbo = new BasicDBObject();
		}

		return dbo;
	}

	private void appendTerm(String term) {

		DBObject dbo = getOrCreateTextDbo();
		String searchString = (String) dbo.get("$search");
		if (StringUtils.hasText(searchString)) {
			searchString += (" " + term);
		} else {
			searchString = term;
		}
		dbo.put("$search", searchString);
		query.put("$text", dbo);
	}

}
