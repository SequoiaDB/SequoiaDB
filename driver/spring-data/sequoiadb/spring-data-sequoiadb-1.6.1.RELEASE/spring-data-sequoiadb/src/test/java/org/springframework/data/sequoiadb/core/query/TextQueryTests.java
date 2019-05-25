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

import static org.hamcrest.collection.IsCollectionWithSize.*;
import static org.hamcrest.collection.IsEmptyCollection.*;
import static org.hamcrest.collection.IsIterableContainingInOrder.*;
import static org.hamcrest.core.AnyOf.*;
import static org.hamcrest.core.IsCollectionContaining.*;
import static org.hamcrest.core.IsEqual.*;
import static org.junit.Assert.*;
import static org.springframework.data.sequoiadb.core.query.Criteria.*;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Before;
import org.junit.ClassRule;
import org.junit.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.annotation.Id;
import org.springframework.data.domain.PageRequest;


import org.springframework.data.sequoiadb.config.AbstractIntegrationTests;
import org.springframework.data.sequoiadb.core.IndexOperations;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.index.IndexDefinition;
import org.springframework.data.sequoiadb.core.mapping.Document;
import org.springframework.data.sequoiadb.core.mapping.Field;
import org.springframework.data.sequoiadb.core.mapping.Language;
import org.springframework.data.sequoiadb.core.mapping.TextScore;
import org.springframework.data.sequoiadb.core.query.TextQueryTests.FullTextDoc.FullTextDocBuilder;
import org.springframework.data.sequoiadb.test.util.SequoiadbVersionRule;
import org.springframework.data.util.Version;

/**

 */
public class TextQueryTests extends AbstractIntegrationTests {

	public static @ClassRule
    SequoiadbVersionRule version = SequoiadbVersionRule.atLeast(new Version(2, 6));

	private static final FullTextDoc BAKE = new FullTextDocBuilder().headline("bake").build();
	private static final FullTextDoc COFFEE = new FullTextDocBuilder().subHeadline("coffee").build();
	private static final FullTextDoc CAKE = new FullTextDocBuilder().body("cake").build();
	private static final FullTextDoc NOT_TO_BE_FOUND = new FullTextDocBuilder().headline("o_O").build();
	private static final FullTextDoc SPANISH_MILK = new FullTextDocBuilder().headline("leche").lanugage("spanish")
			.build();
	private static final FullTextDoc FRENCH_MILK = new FullTextDocBuilder().headline("leche").lanugage("french").build();
	private static final FullTextDoc MILK_AND_SUGAR = new FullTextDocBuilder().headline("milk and sugar").build();

	private @Autowired
    SequoiadbOperations template;

	@Before
	public void setUp() {

		IndexOperations indexOps = template.indexOps(FullTextDoc.class);
		indexOps.dropAllIndexes();

		indexOps.ensureIndex(new IndexDefinition() {

			@Override
			public BSONObject getIndexOptions() {
				BSONObject options = new BasicBSONObject();
				options.put("weights", weights());
				options.put("name", "TextQueryTests_TextIndex");
				options.put("language_override", "lang");
				options.put("default_language", "english");
				return options;
			}

			@Override
			public BSONObject getIndexKeys() {
				BSONObject keys = new BasicBSONObject();
				keys.put("headline", "text");
				keys.put("subheadline", "text");
				keys.put("body", "text");
				return keys;
			}

			private BSONObject weights() {
				BSONObject weights = new BasicBSONObject();
				weights.put("headline", 10);
				weights.put("subheadline", 5);
				weights.put("body", 1);
				return weights;
			}
		});
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldOnlyFindDocumentsMatchingAnyWordOfGivenQuery() {

		initWithDefaultDocuments();

		List<FullTextDoc> result = template.find(new TextQuery("bake coffee cake"), FullTextDoc.class);
		assertThat(result, hasSize(3));
		assertThat(result, hasItems(BAKE, COFFEE, CAKE));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldNotFindDocumentsWhenQueryDoesNotMatchAnyDocumentInIndex() {

		initWithDefaultDocuments();

		List<FullTextDoc> result = template.find(new TextQuery("tasmanian devil"), FullTextDoc.class);
		assertThat(result, hasSize(0));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldApplySortByScoreCorrectly() {

		initWithDefaultDocuments();
		FullTextDoc coffee2 = new FullTextDocBuilder().headline("coffee").build();
		template.insert(coffee2);

		List<FullTextDoc> result = template.find(new TextQuery("bake coffee cake").sortByScore(), FullTextDoc.class);
		assertThat(result, hasSize(4));
		assertThat(result.get(0), anyOf(equalTo(BAKE), equalTo(coffee2)));
		assertThat(result.get(1), anyOf(equalTo(BAKE), equalTo(coffee2)));
		assertThat(result.get(2), equalTo(COFFEE));
		assertThat(result.get(3), equalTo(CAKE));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldFindTextInAnyLanguage() {

		initWithDefaultDocuments();
		List<FullTextDoc> result = template.find(new TextQuery("leche"), FullTextDoc.class);
		assertThat(result, hasSize(2));
		assertThat(result, hasItems(SPANISH_MILK, FRENCH_MILK));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldOnlyFindTextInSpecificLanguage() {

		initWithDefaultDocuments();
		List<FullTextDoc> result = template.find(new TextQuery("leche").addCriteria(where("language").is("spanish")),
				FullTextDoc.class);
		assertThat(result, hasSize(1));
		assertThat(result.get(0), equalTo(SPANISH_MILK));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldNotFindDocumentsWithNegatedTerms() {

		initWithDefaultDocuments();

		List<FullTextDoc> result = template.find(new TextQuery("bake coffee -cake"), FullTextDoc.class);
		assertThat(result, hasSize(2));
		assertThat(result, hasItems(BAKE, COFFEE));
	}

	/**
	 * @see DATA_JIRA-976
	 */
	@Test
	public void shouldInlcudeScoreCorreclty() {

		initWithDefaultDocuments();

		List<FullTextDoc> result = template.find(new TextQuery("bake coffee -cake").includeScore().sortByScore(),
				FullTextDoc.class);

		assertThat(result, hasSize(2));
		for (FullTextDoc scoredDoc : result) {
			assertTrue(scoredDoc.score > 0F);
		}
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldApplyPhraseCorrectly() {

		initWithDefaultDocuments();

		TextQuery query = TextQuery.queryText(TextCriteria.forDefaultLanguage().matchingPhrase("milk and sugar"));
		List<FullTextDoc> result = template.find(query, FullTextDoc.class);

		assertThat(result, hasSize(1));
		assertThat(result, contains(MILK_AND_SUGAR));
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldReturnEmptyListWhenNoDocumentsMatchGivenPhrase() {

		initWithDefaultDocuments();

		TextQuery query = TextQuery.queryText(TextCriteria.forDefaultLanguage().matchingPhrase("milk no sugar"));
		List<FullTextDoc> result = template.find(query, FullTextDoc.class);

		assertThat(result, empty());
	}

	/**
	 * @see DATA_JIRA-850
	 */
	@Test
	public void shouldApplyPaginationCorrectly() {

		initWithDefaultDocuments();

		List<FullTextDoc> result = template.find(new TextQuery("bake coffee cake").sortByScore()
				.with(new PageRequest(0, 2)), FullTextDoc.class);
		assertThat(result, hasSize(2));
		assertThat(result, contains(BAKE, COFFEE));

		result = template.find(new TextQuery("bake coffee cake").sortByScore().with(new PageRequest(1, 2)),
				FullTextDoc.class);
		assertThat(result, hasSize(1));
		assertThat(result, contains(CAKE));
	}

	private void initWithDefaultDocuments() {
		this.template.save(BAKE);
		this.template.save(COFFEE);
		this.template.save(CAKE);
		this.template.save(NOT_TO_BE_FOUND);
		this.template.save(SPANISH_MILK);
		this.template.save(FRENCH_MILK);
		this.template.save(MILK_AND_SUGAR);
	}

	@Document(collection = "fullTextDoc")
	static class FullTextDoc {

		@Id String id;

		private @Language @Field("lang") String language;

		private String headline;
		private String subheadline;
		private String body;

		private @TextScore Float score;

		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((body == null) ? 0 : body.hashCode());
			result = prime * result + ((headline == null) ? 0 : headline.hashCode());
			result = prime * result + ((id == null) ? 0 : id.hashCode());
			result = prime * result + ((language == null) ? 0 : language.hashCode());
			result = prime * result + ((subheadline == null) ? 0 : subheadline.hashCode());
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj) {
				return true;
			}
			if (obj == null) {
				return false;
			}
			if (!(obj instanceof FullTextDoc)) {
				return false;
			}
			FullTextDoc other = (FullTextDoc) obj;
			if (body == null) {
				if (other.body != null) {
					return false;
				}
			} else if (!body.equals(other.body)) {
				return false;
			}
			if (headline == null) {
				if (other.headline != null) {
					return false;
				}
			} else if (!headline.equals(other.headline)) {
				return false;
			}
			if (id == null) {
				if (other.id != null) {
					return false;
				}
			} else if (!id.equals(other.id)) {
				return false;
			}
			if (language == null) {
				if (other.language != null) {
					return false;
				}
			} else if (!language.equals(other.language)) {
				return false;
			}
			if (subheadline == null) {
				if (other.subheadline != null) {
					return false;
				}
			} else if (!subheadline.equals(other.subheadline)) {
				return false;
			}
			return true;
		}

		static class FullTextDocBuilder {

			private FullTextDoc instance;

			public FullTextDocBuilder() {
				this.instance = new FullTextDoc();
			}

			public FullTextDocBuilder headline(String headline) {
				this.instance.headline = headline;
				return this;
			}

			public FullTextDocBuilder subHeadline(String subHeadline) {
				this.instance.subheadline = subHeadline;
				return this;
			}

			public FullTextDocBuilder body(String body) {
				this.instance.body = body;
				return this;
			}

			public FullTextDocBuilder lanugage(String language) {
				this.instance.language = language;
				return this;
			}

			public FullTextDoc build() {
				return this.instance;
			}
		}

	}
}
