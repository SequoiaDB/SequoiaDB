/*
 * Copyright 2010-2014 the original author or authors.
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
package org.springframework.data.mongodb.repository;

import java.util.Collection;
import java.util.Date;
import java.util.List;

import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Slice;
import org.springframework.data.domain.Sort;
import org.springframework.data.geo.Box;
import org.springframework.data.geo.Circle;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.GeoPage;
import org.springframework.data.geo.GeoResults;
import org.springframework.data.geo.Point;
import org.springframework.data.geo.Polygon;
import org.springframework.data.mongodb.repository.Person.Sex;
import org.springframework.data.querydsl.QueryDslPredicateExecutor;

/**
 * Sample repository managing {@link Person} entities.
 * 
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public interface PersonRepository extends MongoRepository<Person, String>, QueryDslPredicateExecutor<Person> {

	/**
	 * Returns all {@link Person}s with the given lastname.
	 * 
	 * @param lastname
	 * @return
	 */
	List<Person> findByLastname(String lastname);

	List<Person> findByLastnameStartsWith(String prefix);

	List<Person> findByLastnameEndsWith(String postfix);

	/**
	 * Returns all {@link Person}s with the given lastname ordered by their firstname.
	 * 
	 * @param lastname
	 * @return
	 */
	List<Person> findByLastnameOrderByFirstnameAsc(String lastname);

	/**
	 * Returns the {@link Person}s with the given firstname. Uses {@link Query} annotation to define the query to be
	 * executed.
	 * 
	 * @param firstname
	 * @return
	 */
	@Query(value = "{ 'firstname' : ?0 }", fields = "{ 'firstname': 1, 'lastname': 1}")
	List<Person> findByThePersonsFirstname(String firstname);

	/**
	 * @see DATAMONGO-871
	 */
	@Query(value = "{ 'firstname' : ?0 }")
	Person[] findByThePersonsFirstnameAsArray(String firstname);

	/**
	 * Returns all {@link Person}s with a firstname matching the given one (*-wildcard supported).
	 * 
	 * @param firstname
	 * @return
	 */
	List<Person> findByFirstnameLike(String firstname);

	List<Person> findByFirstnameLikeOrderByLastnameAsc(String firstname, Sort sort);

	@Query("{'age' : { '$lt' : ?0 } }")
	List<Person> findByAgeLessThan(int age, Sort sort);

	/**
	 * Returns a page of {@link Person}s with a lastname mathing the given one (*-wildcards supported).
	 * 
	 * @param lastname
	 * @param pageable
	 * @return
	 */
	Page<Person> findByLastnameLike(String lastname, Pageable pageable);

	@Query("{ 'lastname' : { '$regex' : ?0, '$options' : ''}}")
	Page<Person> findByLastnameLikeWithPageable(String lastname, Pageable pageable);

	/**
	 * Returns all {@link Person}s with a firstname contained in the given varargs.
	 * 
	 * @param firstnames
	 * @return
	 */
	List<Person> findByFirstnameIn(String... firstnames);

	/**
	 * Returns all {@link Person}s with a firstname not contained in the given collection.
	 * 
	 * @param firstnames
	 * @return
	 */
	List<Person> findByFirstnameNotIn(Collection<String> firstnames);

	List<Person> findByFirstnameAndLastname(String firstname, String lastname);

	/**
	 * Returns all {@link Person}s with an age between the two given values.
	 * 
	 * @param from
	 * @param to
	 * @return
	 */
	List<Person> findByAgeBetween(int from, int to);

	/**
	 * Returns the {@link Person} with the given {@link Address} as shipping address.
	 * 
	 * @param address
	 * @return
	 */
	Person findByShippingAddresses(Address address);

	/**
	 * Returns all {@link Person}s with the given {@link Address}.
	 * 
	 * @param address
	 * @return
	 */
	List<Person> findByAddress(Address address);

	List<Person> findByAddressZipCode(String zipCode);

	List<Person> findByLastnameLikeAndAgeBetween(String lastname, int from, int to);

	List<Person> findByAgeOrLastnameLikeAndFirstnameLike(int age, String lastname, String firstname);

	List<Person> findByLocationNear(Point point);

	List<Person> findByLocationWithin(Circle circle);

	List<Person> findByLocationWithin(Box box);

	List<Person> findByLocationWithin(Polygon polygon);

	List<Person> findBySex(Sex sex);

	List<Person> findBySex(Sex sex, Pageable pageable);

	List<Person> findByNamedQuery(String firstname);

	GeoResults<Person> findByLocationNear(Point point, Distance maxDistance);

	GeoPage<Person> findByLocationNear(Point point, Distance maxDistance, Pageable pageable);

	List<Person> findByCreator(User user);

	/**
	 * @see DATAMONGO-425
	 */
	List<Person> findByCreatedAtLessThan(Date date);

	/**
	 * @see DATAMONGO-425
	 */
	List<Person> findByCreatedAtGreaterThan(Date date);

	/**
	 * @see DATAMONGO-425
	 */
	@Query("{ 'createdAt' : { '$lt' : ?0 }}")
	List<Person> findByCreatedAtLessThanManually(Date date);

	/**
	 * @see DATAMONGO-427
	 */
	List<Person> findByCreatedAtBefore(Date date);

	/**
	 * @see DATAMONGO-427
	 */
	List<Person> findByCreatedAtAfter(Date date);

	/**
	 * @see DATAMONGO-472
	 * @param lastname
	 * @return
	 */
	List<Person> findByLastnameNot(String lastname);

	/**
	 * @see DATAMONGO-600
	 * @param credentials
	 * @return
	 */
	List<Person> findByCredentials(Credentials credentials);

	/**
	 * @see DATAMONGO-636
	 */
	long countByLastname(String lastname);

	/**
	 * @see DATAMONGO-636
	 */
	int countByFirstname(String firstname);

	/**
	 * @see DATAMONGO-636
	 */
	@Query(value = "{ 'lastname' : ?0 }", count = true)
	long someCountQuery(String lastname);

	/**
	 * @see DATAMONGO-770
	 */
	List<Person> findByFirstnameIgnoreCase(String firstName);

	/**
	 * @see DATAMONGO-770
	 */
	List<Person> findByFirstnameNotIgnoreCase(String firstName);

	/**
	 * @see DATAMONGO-770
	 */
	List<Person> findByFirstnameStartingWithIgnoreCase(String firstName);

	/**
	 * @see DATAMONGO-770
	 */
	List<Person> findByFirstnameEndingWithIgnoreCase(String firstName);

	/**
	 * @see DATAMONGO-770
	 */
	List<Person> findByFirstnameContainingIgnoreCase(String firstName);

	/**
	 * @see DATAMONGO-870
	 */
	Slice<Person> findByAgeGreaterThan(int age, Pageable pageable);

	/**
	 * @see DATAMONGO-821
	 */
	@Query("{ creator : { $exists : true } }")
	Page<Person> findByHavingCreator(Pageable page);

	/**
	 * @see DATAMONGO-566
	 */
	List<Person> deleteByLastname(String lastname);

	/**
	 * @see DATAMONGO-566
	 */
	Long deletePersonByLastname(String lastname);

	/**
	 * @see DATAMONGO-566
	 */
	@Query(value = "{ 'lastname' : ?0 }", delete = true)
	List<Person> removeByLastnameUsingAnnotatedQuery(String lastname);

	/**
	 * @see DATAMONGO-566
	 */
	@Query(value = "{ 'lastname' : ?0 }", delete = true)
	Long removePersonByLastnameUsingAnnotatedQuery(String lastname);

	/**
	 * @see DATAMONGO-893
	 */
	Page<Person> findByAddressIn(List<Address> address, Pageable page);

	/**
	 * @see DATAMONGO-745
	 */
	@Query("{firstname:{$in:?0}, lastname:?1}")
	Page<Person> findByCustomQueryFirstnamesAndLastname(List<String> firstnames, String lastname, Pageable page);

	/**
	 * @see DATAMONGO-745
	 */
	@Query("{lastname:?0, address.street:{$in:?1}}")
	Page<Person> findByCustomQueryLastnameAndAddressStreetInList(String lastname, List<String> streetNames, Pageable page);

	/**
	 * @see DATAMONGO-950
	 */
	List<Person> findTop3ByLastnameStartingWith(String lastname);

	/**
	 * @see DATAMONGO-950
	 */
	Page<Person> findTop3ByLastnameStartingWith(String lastname, Pageable pageRequest);

	/**
	 * @see DATAMONGO-1030
	 */
	PersonSummary findSummaryByLastname(String lastname);

	@Query("{ ?0 : ?1 }")
	List<Person> findByKeyValue(String key, String value);
}
