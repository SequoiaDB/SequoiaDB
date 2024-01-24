
# 1. 标题

# 这是 h1

## 这是 h2

这也是 h2
---

### 这是 h3

###### 这是 h6

> **Note:**  
> 在官网模式，会根据 H2 生成本页导航

--

# 2. 区块引用

> This is a blockquote with two paragraphs. Lorem ipsum dolor sit amet,  
> consectetuer adipiscing elit. Aliquam hendrerit mi posuere lectus.  
> Vestibulum enim wisi, viverra nec, fringilla in, laoreet vitae, risus.
> 
> Donec sit amet nisl. Aliquam semper ipsum sit amet velit. Suspendisse  
> id sem consectetuer libero luctus adipiscing.

**区块引用内部同样支持双空格强制换行！**

--

# 3. 换行

**换行有两种形式，第一种是普通换行，第二种是隔行**

**换行：在后面加两个空格就可以，如：**

床前明月光  
疑是地上霜  
举头望明月  
低头思故乡

**隔行：两个回车，如：**

床前明月光

疑是地上霜

举头望明月

低头思故乡

--

# 4. 列表

**无序列表**

- Red
- Green
- Blue

**有序列表**

1. Bird
2. McHale
3. Parish

**嵌入列表**

- item 1
- item 2
  - item 2_a
  - item 2_b
- item 3
  - item 3_a
  - item 3_b
  - item 3_c

**分隔比较大的列表**

1. A

2. B

3. C


--


# 5. 注释

[^_^]: 单行注释

[^_^]: 多行注释
   你看不到我 1
   你看不到我 2
   你看不到我 3

--

# 6. 图片

**图片支持两种语法，两种语法前面都有方括号，里面写的是图片的标注，  
它规定在图像无法显示时的替代文本，同时也有助于搜索引擎的检索。**

第一种：

![这是logo](logo.png)

第二种（推荐）：

![这是logo][logo_img]
![这是logo][logo_img]


[logo_img]: logo.png

--

# 7. 链接

**链接支持两种语法，两种语法前面都有方括号，里面写的是链接的文本。**

第一种：

[百度](http://www.baidu.com)

第二种（推荐）：

[百度][baidu_link]

[baidu][baidu_link]


[baidu_link]: http://www.baidu.com


--

# 8. 突出显示

**加粗1**
__加粗2__

*倾斜1*
_倾斜2_

_你 **可以** 混合使用_


# 9. 代码

官网样式，需要设置【菜单】-【模式】-【官网模式】

```shell
cd /opt
mkdir sequoiadb
```

# 10. 表格

| First Header | Second Header |
| ------------ | ------------- |
| Content from cell 1 | Content from cell 2 |
| Content in the first column | Content in the second column |
| 我要<br/>换行 | 我不换行 |

# 11. 删除线

~~123456~~


# 12. 转义字符

Markdown 支持以下这些符号前面加上反斜杠来帮助插入普通的符号：

| 符号 | 注释 |
| ---- | ---- |
| \\   | 反斜线 |
| \`   | 反引号 |
| \*   | 星号 |
| \_   | 底线 |
| \{ \} | 花括号 |
| \[ \] | 方括号 |
| \( \) | 括弧 |
| \#   | 井字号 |
| \+   | 加号 |
| \-   | 减号 |
| \.   | 英文句点 |
| \!   | 惊叹号 |
| \^   | 托字符 |
| \|   | 竖线 |
| \<\> | 尖括号 |

# 13. 分页

**我们利用注释开发的语法，通过模式切换【普通模式】和【官网模式】查看效果  
【普通模式】是列表，【官网模式】是分页**

**列表支持有序列表和无序列表**

```
[^_^]:tab
1. [Title1]:

   Content1

2. [Title2]:

   Content2
```

--

[^_^]:tab
1. Linux:

   Linux是一套免费使用和自由传播的类Unix操作系统

   ```
   还能嵌入代码
   123
   456
   ```

2. Windows:

   Microsoft Windows操作系统是美国微软公司研发的一套操作系统

   ```
   还能嵌入代码
   789
   abc
   ```

