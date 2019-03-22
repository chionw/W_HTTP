// W_HTTP.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../../../include/W_HTTP.h"
#include <iostream>

#if MG_ENABLE_SSL
#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")
#endif

#include <boost/locale/encoding.hpp>

/*
	下面两个转码函数用到了boost库，如果你没有安装boost库，可以自行实现下面两个转码函数.转码函数网上很多
	These two functions refer to the boost library.if you don't install boost library. you can finish them yourself;
*/
static string GBKToUTF8(const  string& strGBK)
{
	return boost::locale::conv::to_utf<char>(strGBK, "gbk");
}

static string UTF8ToGBK(const  string& strUTF8)
{
	return boost::locale::conv::from_utf<char>(strUTF8, "gbk");
}

/*
	this is the callback functions.
*/
void callbackSuccess(const W_HTTP_RESPONSE_HEADER& _resp_header, const string &_recv_data)
{
	std::cout << "异步操作成功\r\n" << UTF8ToGBK(_recv_data) << endl;
}

void callbackError(const int &result, const string &errmsg, const W_HTTP_RESPONSE_HEADER& _resp_header, const string &_recv_data)
{
	std::cout << "异步操作失败\r\nresult:" << result << "\r\nmsg:" << errmsg<<endl;
}

void callbackComplete()
{
	std::cout << "异步操作完成" << endl;
}

int main()
{
	{
		int result;
		string recv_data;

		std::cout << "示例1：获取广州市天气预报(query weather of guangzhou.China)" << endl;
		W_HTTP http1;
		result = http1.url("http://www.weather.com.cn/data/sk/101280101.html")
			.run(recv_data);
		std::cout << "result:" << result << "\r\nerrmsg:" << http1.getLastErrorMsg() << "\r\n" << UTF8ToGBK(recv_data) << endl;


		std::cout << "\r\n示例2：把示例1的结果保存为文件(save the result of example1 as a file)" << endl;
		W_HTTP http2;
		result = http2.url("http://www.weather.com.cn/data/sk/101280101.html")
			.filePath("guangzhou.json")
			.download();
		std::cout << "result:" << result << "\r\nerrmsg:" << http2.getLastErrorMsg() <<"\r\nfile:guangzhou.json"<< endl;

		std::cout << "\r\n示例3：下载图片(download jpg)" << endl;
		W_HTTP http3;
		result = http3.url("http://www.gov.cn/govweb/xhtml/2016gov/images/public/logo.jpg")
			.filePath("logo.jpg")
			.download();
		std::cout << "result:" << result << "\r\nerrmsg:" << http3.getLastErrorMsg() <<"\r\nfile:logo.jpg"<< endl;

#if MG_ENABLE_SSL
		std::cout << "\r\n示例4：https域名的访问(visit a https)" << endl;
		W_HTTP http6;
		result = http6.url("https://www.baidu.com")
			.run(recv_data);
		std::cout << "result:" << result << "\r\nerrmsg:" << http6.getLastErrorMsg() << endl;
#endif

		std::cout << "\r\n示例5：异步下载图片(download jpg Asynchronous)" << endl;
		W_HTTP http4;
		result = http4.url("http://www.gov.cn/govweb/xhtml/2016gov/images/public/logo.jpg")
			.filePath("logo.jpg")
			.download(true);
		//'download()' function return immediately

		std::cout << "\r\n示例6：异步查询天气(query weather Asynchronous)" << endl;
		W_HTTP http5;
		http5.url("http://www.weather.com.cn/data/sk/101280101.html")
			.error(std::bind(&callbackError, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
			.success(std::bind(&callbackSuccess, std::placeholders::_1, std::placeholders::_2))
			.completed(std::bind(&callbackComplete))
			.run();
		//'run()' function return immediately
	}
	

	system("pause");
    return 0;
}

