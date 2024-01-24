[^_^]:
    JSON 实例-注意事项

用户在使用 SequoiaDB 巨杉数据库的 JSON 实例时，需要注意以下几点：

+ JSON 实例严格区分大小写

    例如，在执行以下两条获取集合空间句柄操作时，“emp”和“EMP”表示的是两个不同的集合空间，有独立的集合和数据

    ```lang-bash
    > db["emp"]
    localhost:11810.emp
    > db["EMP"]
    localhost:11810.EMP
    ```

+ JSON 实例使用动态数据类型

    例如，一变量在进行赋值操作后，仍然可以将其赋值为其他数据类型的值

     ```lang-bash
    > var variable = 1;
    > typeof(variable)
    number
    > variable = "hello world";
    > typeof(variable)
    string
    ```

+ JSON 实例无函数重载概念

    例如，对于以下定义的两个同名函数 func() 和 func(parameter) ，只有后定义的 func(parameter) 会生效

    ```lang-bash
    > function func(){ println("function has no parameter"); }
    > function func(parameter){ println("function has parameter"); }
    > func();
    function has parameter
    > func("hello");
    function has parameter
    ```

+ JSON 实例使用 Float 存储浮点型数据时，需要注意精度问题

    例如，定义两个浮点型数据 v1 和 v2 ，在执行加法操作后赋值给 v3 ，根据数学加法定义可以轻易计算出 v3 的值为 0.3 ，但是由于 Float 存储小数的精度问题，实际上 v3 与 0.3 并不相等

    ```lang-bash
    > var v1 = 0.1
    > var v2 = 0.2
    > var v3 = v1 + v2
    > println(v3)
    0.30000000000000004
    > println(v3 == 0.3)
    false
    ```
