# W_HTTP
基于mongoose开发的http客户端库。 
mongoose是一款强大的通讯库，http只是它其中一部分。
它能搭建嵌入式的http服务器，也可以做为http客户端。
但它的使用并不友好，例如想要发起一次http请求，你需要准备的工作很多。
有鉴于此，参照ajax的使用风格，对它进行了封装。
使用者通过引用W_HTTP的源文件，即可使用。只需要一行代码，便可完成一次HTTP请求。  
注：如果需要访问https的网站，额外安装openssl后配置到你的工程中即可。

使用时将include文件添加至你的工程，然后包含进
#include "W_HTTP.h"
随后即可使用。

使用示例请参考demo
