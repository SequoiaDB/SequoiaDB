/*
 * Copyright 2010-2013 the original author or authors.
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
package org.springframework.data.sequoiadb.repository;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Set;

import org.springframework.data.geo.Point;
import org.springframework.data.sequoiadb.core.index.GeoSpatialIndexed;
import org.springframework.data.sequoiadb.core.index.Indexed;
import org.springframework.data.sequoiadb.core.mapping.DBRef;
import org.springframework.data.sequoiadb.core.mapping.Document;

/**
 * Sample domain class.
 * 


 */
@Document
public class Person extends Contact {

	public enum Sex {
		MALE, FEMALE;
	}

	private String firstname;
	private String lastname;
	private String email;
	private Integer age;
	@SuppressWarnings("unused") private Sex sex;
	Date createdAt;

	List<String> skills;

	private Point location;

	private Address address;
	private Set<Address> shippingAddresses;

	@DBRef User creator;

	@DBRef(lazy = true) User coworker;

	@DBRef(lazy = true) List<User> fans;

	@DBRef(lazy = true) ArrayList<User> realFans;

	Credentials credentials;

	public Person() {

		this(null, null);
	}

	public Person(String firstname, String lastname) {

		this(firstname, lastname, null);
	}

	public Person(String firstname, String lastname, Integer age) {

		this(firstname, lastname, age, Sex.MALE);
	}

	public Person(String firstname, String lastname, Integer age, Sex sex) {

		super();
		this.firstname = firstname;
		this.lastname = lastname;
		this.age = age;
		this.sex = sex;
		this.email = (firstname == null ? "noone" : firstname.toLowerCase()) + "@dmband.com";
		this.createdAt = new Date();
	}

	/**
	 * @return the firstname
	 */
	public String getFirstname() {

		return firstname;
	}

	/**
	 * @param firstname the firstname to set
	 */
	public void setFirstname(String firstname) {

		this.firstname = firstname;
	}

	/**
	 * @return the lastname
	 */
	public String getLastname() {

		return lastname;
	}

	/**
	 * @param lastname the lastname to set
	 */
	public void setLastname(String lastname) {

		this.lastname = lastname;
	}

	/**
	 * @return the email
	 */
	public String getEmail() {
		return email;
	}

	/**
	 * @param email the email to set
	 */
	public void setEmail(String email) {
		this.email = email;
	}

	/**
	 * @return the age
	 */
	public Integer getAge() {

		return age;
	}

	/**
	 * @param age the age to set
	 */
	public void setAge(Integer age) {

		this.age = age;
	}

	/**
	 * @return the location
	 */
	public Point getLocation() {
		return location;
	}

	/**
	 * @param location the location to set
	 */
	public void setLocation(Point location) {
		this.location = location;
	}

	/**
	 * @return the address
	 */
	public Address getAddress() {
		return address;
	}

	/**
	 * @param address the address to set
	 */
	public void setAddress(Address address) {
		this.address = address;
	}

	/**
	 * @return the addresses
	 */
	public Set<Address> getShippingAddresses() {
		return shippingAddresses;
	}

	/**
	 * @param addresses the addresses to set
	 */
	public void setShippingAddresses(Set<Address> addresses) {
		this.shippingAddresses = addresses;
	}

	/* (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.Contact#getName()
	 */
	public String getName() {
		return String.format("%s %s", firstname, lastname);
	}

	/**
	 * @return the fans
	 */
	public List<User> getFans() {
		return fans;
	}

	/**
	 * @param fans the fans to set
	 */
	public void setFans(List<User> fans) {
		this.fans = fans;
	}

	/**
	 * @return the realFans
	 */
	public ArrayList<User> getRealFans() {
		return realFans;
	}

	/**
	 * @param realFans the realFans to set
	 */
	public void setRealFans(ArrayList<User> realFans) {
		this.realFans = realFans;
	}

	/**
	 * @return the coworker
	 */
	public User getCoworker() {
		return coworker;
	}

	/**
	 * @param coworker the coworker to set
	 */
	public void setCoworker(User coworker) {
		this.coworker = coworker;
	}

	/*
	* (non-Javadoc)
	*
	* @see java.lang.Object#equals(java.lang.Object)
	*/
	@Override
	public boolean equals(Object obj) {

		if (this == obj) {
			return true;
		}

		if (obj == null || !getClass().equals(obj.getClass())) {
			return false;
		}

		Person that = (Person) obj;

		return this.getId().equals(that.getId());
	}

	public Person withAddress(Address address) {

		this.address = address;
		return this;
	}

	public void setCreator(User creator) {
		this.creator = creator;
	}

	public void setSkills(List<String> skills) {
		this.skills = skills;
	}

	public List<String> getSkills() {
		return skills;
	}

	/*
	* (non-Javadoc)
	*
	* @see java.lang.Object#hashCode()
	*/
	@Override
	public int hashCode() {

		return getId().hashCode();
	}

	/* (non-Javadoc)
	* @see java.lang.Object#toString()
	*/
	@Override
	public String toString() {
		return String.format("%s %s", firstname, lastname);
	}
}
