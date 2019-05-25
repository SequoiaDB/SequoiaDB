package com.sequoiadb.hive;

import javax.xml.bind.SchemaOutputResolver;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Created by gaoxing on 2014-11-03 .
 */
public class Demo {
    public static String say(String str){
        System.out.printf(str);
        return str;
    }

    public static void main(String[] args) throws NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        Method method=Demo.class.getDeclaredMethod("say",String.class);
        String str= (String) method.invoke(null,"gaoxing");
        System.out.println(str);
    }
}
