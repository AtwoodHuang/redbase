# my-redbase
斯坦福大学课程设计redbase，大概有7000多行代码接口部分是严格按照技术文档要求来的。不太会yacc，试了很多次没成功，自己写了一个只能算能用的sql解析器。相比技术文档要求，暂时还不支持多表查询，b+树和内存池部分可能还有一些bug。暂时写出个能用的，剩下的不足只能以后有时间再弄了，毕竟已经花了2个月在上面了，不能再在上面无意义的浪费时间了。RBparse的两个文件是sql解析部分，很low，默认只能处理正确情况，很多错误处理都没弄。部分操作如下：
![image](https://github.com/AtwoodHuang/my-redbase/blob/master/sp.gif)   
