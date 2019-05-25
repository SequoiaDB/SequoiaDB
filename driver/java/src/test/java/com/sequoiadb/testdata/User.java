package com.sequoiadb.testdata;

import java.util.Arrays;
import java.util.Map;

public class User {
    public int age;
    public String name;
    public Map<String, Student> student;

    public void setAge(int age) {
        this.age = age;
    }

    public int getAge() {
        return this.age;
    }

    public void setName(String name) {
        this.name = name;
    }

    public User(int age, String name, Map<String, Student> student) {
        super();
        this.age = age;
        this.name = name;
        this.student = student;
    }

    public String getName() {
        return this.name;
    }


    public Map<String, Student> getStudent() {
        return student;
    }

    public void setStudent(Map<String, Student> student) {
        this.student = student;
    }


    public User() {
    }

    @Override
    public String toString() {
        return "User [age=" + age + ", name=" + name + ", student=" + student
            + "]";
    }

    @Override
    public int hashCode() {
        Object[] objects = new Object[]{age, name, student};
        return Arrays.hashCode(objects);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof User) {
            if (this == obj) {
                return true;
            }

            User other = (User) obj;
            return this.age == other.age && this.name.equals(other.name) && this.student.equals(other.student);
        } else {
            return false;
        }
    }
}