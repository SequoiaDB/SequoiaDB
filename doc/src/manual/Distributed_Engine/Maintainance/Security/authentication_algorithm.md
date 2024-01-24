
##SequoiaDB 鉴权机制##

鉴权能够保障数据库的安全性。SequoiaDB 3.4.1 之前的版本，支持 MD5 鉴权；SequoiaDB 3.4.1 及以上的版本，支持 SCRAM-SHA256 鉴权。

##MD5 鉴权##

MD5 鉴权是通过验证用户密码的 MD5 值是否合法来判断用户是否有权限访问数据库。

###创建用户###

在创建用户时，用户系统表会保存该用户的用户名以及密码的 MD5 值。

###鉴权认证###

当用户执行连接操作时，客户端会发送用户名以及密码的 MD5 值给服务端。服务端接收消息之后，会根据用户名去用户系统表中查看对应密码的 MD5 值。如果客户端发送的 MD5 值与用户系统表中的保存的 MD5 值相匹配，则鉴权通过。否则鉴权失败。

##SCRAM-SHA-256 鉴权##

SCRAM 是密码学中的一种认证机制，全称 Salted Challenge Response Authentication Mechanism。SCRAM 适用于使用基于『用户名：密码』这种简单认证模型的连接协议。SCRAM-SHA-256 指的是将 SHA-256 作为基础哈希函数的 SCRAM。

###创建用户###

在创建用户时，引擎会生成一个随机盐值，然后根据盐值、迭代数和原始密码计算出两个 Key，分别是 StoredKey 和 ServerKey。最后用户系统表会保存该用户的用户名、随机盐值、迭代数、 StoredKey 和 ServerKey。为了兼容以前的版本，用户系统表还会保存用户密码的 MD5 值。

> **Note：**

> 建议用户使用 SSL 连接创建用户。关于 SSL 连接详细可参考 [网络安全][network_security]。

StoredKey 和 ServerKey 的计算规则如下：

```
// 把明文密码的 MD5 值作为原始密码，然后利用 HmacSHA256 加密算法，以随机盐值作为密钥，原始密码作为待加密信息，计算出摘要信息 tmp。这里的 tmp 不仅是作为下一次加密的密钥，还是 saltPassword 的初始值。
tmp = HmacSHA256( MD5( password ), salt )
saltPassword = tmp

// iterationCount 指的是迭代数
for( int i = 0; i < iterationCount; i++ )
{
   // 之后每次计算出来的摘要会作为新的密钥，然后 saltPassword 会与摘要进行异或操作
   tmp = HmacSHA256( MD5( password ), tmp )
   saltPassword = saltPassword XOR tmp
}

ClientKey = HmacSHA256( saltPassword, "Client Key" )
ServerKey = HmacSHA256( saltPassword, "Server Key" )
StoredKey = SHA256( ClientKey )
```

###鉴权认证###

SCRAM-SHA-256 鉴权认证分为三个步骤。

- **步骤一**

 ![第一次步骤][authStep1]

- **步骤二**

 ![第二次步骤][authStep2]

   客户端从服务端获取盐值和计算的迭代数，按照已经协商好的算法分别计算出 ClientKey，StoredKey 和 ServerKey 。

   客户端的身份证明 ClientProof 计算规则如下：

   ```lang-ini
   // AuthMsg 是鉴权认证过程中客户端和服务端交互的信息组合成的一个消息串
   ClientSignature = HmacSHA256( AuthMsg, StoredKey )
   ClientProof = ClientSignature XOR ClientKey
   ```

   服务端验证客户端身份证明的合法性，从客户端的身份证明还原出 ClientKey 的规则如下：

   ```lang-ini
   ClientSignature = HmacSHA256( AuthMsg, StoredKey )
   ClientKey = ClientProof XOR ClientSignature
   ```

   在鉴权过程中客户端和服务端之间不会发送密码或者密码的 MD5 值，所以服务端无法获取到用户密码，只有客户端才获取得到用户密码，从而计算出 ClientKey。服务端通过客户端的身份证明得到 ClientKey，然后计算出 StoredKey，如果该 StoredKey 与系统用户表中保存的 StoredKey 相同，就说明 ClientKey 是合法的，从而说明客户端是合法的。

   服务端的身份证明 ServerProof 计算规则如下：

   ```lang-ini
   ServerProof = HmacSHA256( AuthMsg, ServerKey )
   ```

- **步骤三**

 ![第三次步骤][authStep3]

   客户端利用在第一次认证中的 ServerKey 计算出服务端的身份证明，然后同服务端发送过来的身份证明作比较，如果相同则说明服务器是合法的。

###优势###

- 防窃听攻击。攻击者利用各种方式能够窃取客户端和服务端之间交换的所有信息。但是在该鉴权过程中，客户端不会以明文形式发送密码，所以攻击者窃取不了用户的密码。

- 防重放攻击。攻击者可以将客户端的有效响应重新发送给服务端来达到欺骗服务端的目的。但是在该鉴权过程中，客户端和服务端发送的身份证明都是基于随机字符串计算的，所以每个身份证明都是唯一的，仅对单个会话有效。

- 防数据破解。攻击者可以获取服务器永久保存的内容。但是在该鉴权中，服务器在存储密码之前都会先对密码加盐和迭代地哈希处理，所以即使获取到表中数据也破解不出密码。

- 防服务器伪造。攻击者可将自己伪装为服务器。但是如果不了解客户端的身份证明，攻击者将无法冒充为服务器。


[^_^]:
     本文使用的所有引用和链接
[network_security]:manual/Distributed_Engine/Maintainance/Security/network_security.md
[authStep1]:images/Distributed_Engine/Maintainance/Security/authStep1.png
[authStep2]:images/Distributed_Engine/Maintainance/Security/authStep2.png
[authStep3]:images/Distributed_Engine/Maintainance/Security/authStep3.png
