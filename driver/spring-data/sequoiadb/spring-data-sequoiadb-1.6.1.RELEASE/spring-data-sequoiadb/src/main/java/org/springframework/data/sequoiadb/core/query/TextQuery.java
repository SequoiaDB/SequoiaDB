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

import java.util.Locale;




/**
 * {@link Query} implementation to be used to for performing full text searches.
 * 

 * @since 1.6
 */
public class TextQuery extends Query {

	private final String DEFAULT_SCORE_FIELD_FIELDNAME = "score";
	private final BSONObject META_TEXT_SCORE = new BasicBSONObject("$meta", "textScore");

	private String scoreFieldName = DEFAULT_SCORE_FIELD_FIELDNAME;
	private boolean includeScore = false;
	private boolean sortByScore = false;

	/**
	 * Creates new {@link TextQuery} using the the given {@code wordsAndPhrases} with {@link TextCriteria}
	 * 
	 * @param wordsAndPhrases
	 * @see TextCriteria#matching(String)
	 */
	public TextQuery(String wordsAndPhrases) {
		super(TextCriteria.forDefaultLanguage().matching(wordsAndPhrases));
	}

	/**
	 * Creates new {@link TextQuery} in {@code language}. <br />
	 * 
	 * @param wordsAndPhrases
	 * @param language
	 * @see TextCriteria#forLanguage(String)
	 * @see TextCriteria#matching(String)
	 */
	public TextQuery(String wordsAndPhrases, String language) {
		super(TextCriteria.forLanguage(language).matching(wordsAndPhrases));
	}

	/**
	 * Creates new {@link TextQuery} using the {@code locale}s language.<br />
	 * For a full list of supported languages see the mongdodb reference manual for <a
	 * href="http://docs.sequoiadb.org/manual/reference/text-search-languages/">Text Search Languages</a>.
	 * 
	 * @param wordsAndPhrases
	 * @param locale
	 */
	public TextQuery(String wordsAndPhrases, Locale locale) {
		this(wordsAndPhrases, locale != null ? locale.getLanguage() : (String) null);
	}

	/**
	 * Creates new {@link TextQuery} for given {@link TextCriteria}.
	 * 
	 * @param criteria.
	 */
	public TextQuery(TextCriteria criteria) {
		super(criteria);
	}

	/**
	 * Creates new {@link TextQuery} searching for given {@link TextCriteria}.
	 * 
	 * @param criteria
	 * @return
	 */
	public static TextQuery queryText(TextCriteria criteria) {
		return new TextQuery(criteria);
	}

	/**
	 * Add sorting by text score. Will also add text score to returned fields.
	 * 
	 * @see TextQuery#includeScore()
	 * @return
	 */
	public TextQuery sortByScore() {

		this.includeScore();
		this.sortByScore = true;
		return this;
	}

	/**
	 * Add field {@literal score} holding the documents textScore to the returned fields.
	 * 
	 * @return
	 */
	public TextQuery includeScore() {

		this.includeScore = true;
		return this;
	}

	/**
	 * Include text search document score in returned fields using the given fieldname.
	 * 
	 * @param fieldname
	 * @return
	 */
	public TextQuery includeScore(String fieldname) {

		setScoreFieldName(fieldname);
		includeScore();
		return this;
	}

	/**
	 * Set the fieldname used for scoring.
	 * 
	 * @param fieldName
	 */
	public void setScoreFieldName(String fieldName) {
		this.scoreFieldName = fieldName;
	}

	/**
	 * Get the fieldname used for scoring
	 * 
	 * @return
	 */
	public String getScoreFieldName() {
		return scoreFieldName;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.query.Query#getFieldsObject()
	 */
	@Override
	public BSONObject getFieldsObject() {

		if (!this.includeScore) {
			return super.getFieldsObject();
		}

		BSONObject fields = super.getFieldsObject();

		if (fields == null) {
			fields = new BasicBSONObject();
		}

		fields.put(getScoreFieldName(), META_TEXT_SCORE);
		return fields;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.query.Query#getSortObject()
	 */
	@Override
	public BSONObject getSortObject() {

		BSONObject sort = new BasicBSONObject();

		if (this.sortByScore) {
			sort.put(getScoreFieldName(), META_TEXT_SCORE);
		}

		if (super.getSortObject() != null) {
			sort.putAll(super.getSortObject());
		}

		return sort;
	}
}
