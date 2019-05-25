package com.sequoiadb.testdata;

import org.bson.types.ObjectId;

import java.sql.Date;

public class ClassWithID {
    private ObjectId _id = ObjectId.get();
    private long id = 1;
    private String name = "shanks";
    private int age = 25;
    private double scode = 99.999;

    private Integer ageInteger = 25;
    private Long idLong = 1L;
    private Double scodeDouble = 9999.001;
    private Float floatvalue = 012121.212f;
    private Date date = Date.valueOf("2012-12-24");


    public ObjectId get_id() {
        return _id;
    }

    public void set_id(ObjectId value) {
        _id = value;
    }

    public long getId() {
        return id;
    }

    public void setId(long value) {
        id = value;
    }

    public String getName() {
        return name;
    }

    public void setName(String value) {
        name = value;
    }

    public int getAge() {
        return age;
    }

    public void setAge(int value) {
        age = value;
    }

    public double getScode() {
        return scode;
    }

    public void setScode(double value) {
        scode = value;
    }

    public Integer getAgeInteger() {
        return ageInteger;
    }

    public void setAgeInteger(Integer value) {
        ageInteger = value;
    }

    public Long getIdLong() {
        return idLong;
    }

    public void setIdLong(Long value) {
        idLong = value;
    }

    public Double getScodeDouble() {
        return scodeDouble;
    }

    public void setScodeDouble(Double value) {
        scodeDouble = value;
    }

    @Override
    public boolean equals(Object other) {

        if (!(other instanceof ClassWithID)) {
            return false;
        }
        ClassWithID otherObj = (ClassWithID) other;

        if (!this._id.equals(otherObj._id)) {
            return false;
        }
        if (this.age != otherObj.age) {
            return false;
        }
        if (!this.ageInteger.equals(otherObj.ageInteger)) {
            return false;
        }
        if (!this.floatvalue.equals(otherObj.floatvalue)) {
            return false;
        }
        if (this.id != otherObj.id) {
            return false;
        }
        if (!this.idLong.equals(otherObj.idLong)) {
            return false;
        }
        if (!this.name.equals(otherObj.name)) {
            return false;
        }
        if (this.scode - otherObj.scode > 0.1) {
            return false;
        }
        if (!this.scodeDouble.equals(otherObj.scodeDouble)) {
            return false;
        }

        return true;
    }
}