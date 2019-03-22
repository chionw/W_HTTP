#pragma once
#include "mongoose/mongoose.h"
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <vector>
#include <memory>
#include <thread>
using namespace std;

//是否支持JSON，需要额外提供json-cpp的支持
#ifndef SUPPORT_JSON
#define SUPPORT_JSON 0
#endif


/*
	定义http的url参数结构
*/
typedef std::vector<std::string> W_HTTP_REQUEST_PARAM_VALUE;
typedef std::map<std::string, W_HTTP_REQUEST_PARAM_VALUE> W_HTTP_REQUEST_PARAM;

/*
	定义了http的头部
*/
typedef std::set<string> W_HTTP_HEADER_VALUE;
typedef std::shared_ptr<W_HTTP_HEADER_VALUE> W_SP_HTTP_HEADER_VALUE;

typedef struct{
	/*添加一个新的头部,不会修改现有头部的值*/
	void add(const string &key, const string &value)
	{
		m_header[key].insert(value);
	}

	/*设置新的头部，如果key已经存在，则先删除所有的key头部*/
	void set(const string &key, const string &value)
	{
		m_header[key].clear();
		m_header[key].insert(value);
	}

	/*访问指定的头部，得到它所有的值，若不存在，则创建新的头部，但值为空*/
	W_HTTP_HEADER_VALUE &operator[](const string &_key)
	{
		return m_header[_key];
	}

	/*
		获取一条头部数据
			如果没有，返回空指针。否则返回W_SP_HTTP_RESPONSE_HEADER_VALUE
	*/
	W_SP_HTTP_HEADER_VALUE get(const string &_key) {
		if (m_header.find(_key) == m_header.end())
		{
			return nullptr;
		}

		return W_SP_HTTP_HEADER_VALUE(new W_HTTP_HEADER_VALUE(m_header[_key].begin(), m_header[_key].end()));
	}

	/*把头部输出为字符串，如：
		a:1
		b:2
	*/
	std::string toString()
	{
		std::string retString;
		for (auto &head : m_header)
		{
			auto key = head.first;
			auto values = head.second;
			for (auto &value : values)
			{
				retString += (key + ":" + value + "\r\n");
			}
		}

		return retString;
	}
private:
	std::map<string, W_HTTP_HEADER_VALUE > m_header;
}W_HTTP_RESPONSE_HEADER, W_HTTP_REQUEST_HEADER;

typedef std::function<void(const W_HTTP_RESPONSE_HEADER& _resp_header, const string &_recv_data)> TYPE_CALLBACK_SUCCESS;

typedef std::function<void(const int &result, const string &_errMsg, const W_HTTP_RESPONSE_HEADER& _resp_header, const string &_recv_data)> TYPE_CALLBACK_ERROR;

typedef std::function<void()> TYPE_CALLBACK_COMPLETED;

/*
	本类为http通讯提供了友好的操作方式。
		用户可以通过类实例的设置接口来设置通讯参数
		在参数设置完毕之后，本类还提供了异步和同步的执行方式。

	编码问题：
		当传送的数据中包含中文时，一般都要注意编码问题。本类在处理中文时，并不会自动将其转为utf-8

	例子(同步执行，使用百度搜索关键字‘中国’)：
		string recvData;
		W_HTTP http;
		int iRet = http.method("get")
						.url("http://www.baidu.com/s")
						.urlParam("wd", "中国")
						.syncRun(recvData);
		print(recvData)

	注：如果希望支持访问https的话，需要把mongoose.h中的宏#define MG_ENABLE_SSL 0改为#define MG_ENABLE_SSL 1
		同时增加openssl头文件和lib文件
*/
class W_HTTP
{
public:
	W_HTTP();
	~W_HTTP();

public:
	/************************************************************************/
	/* 参数设置-请求参数                                                    */
	/************************************************************************/
	/*
		设置方法
			_method:http通讯方法，可选值为post、get
			默认值：get
			容错性：若参数不对，则设置无效，维持原来值不变
	*/
	W_HTTP &method(const string& _method);

	/*
		设置URL
			设置目标地址。例如http://www.baidu.com?a=1#region
			允许URL中包含中文等需要编码的字符，程序会自动处理
			容错性：如果_url不是合法的，设置会无效
	*/
	W_HTTP &url(const string& _url);

	/*
		设置http头部
			_key 头部名称
			_value 头部的值
			_replace 是否覆盖前面设置过的头部，默认为false，会新增一个同名的头部，若设置为true，则先删除所有名称为_key的头部
			若同一个头部重复设置，后者会覆盖掉前者。如head("key", "1");head("key", "2");则结果为key:2
	*/
	W_HTTP &header(const string &_key, const string &_value,const bool &replace = false);

	/*
		向url中添加参数。
		调用该接口前先调用url设置一个有效的URL.
		容错性：
			_key和_value都不能为空
			重复调用设置同一个_key的效果，相当于设置了一个名称为_key的数组。
				如：urlParam("a", "1").urlParam("a", "2")相当于urlParam("a", ["1", "2"]),结果为?a[]=1&a[]=2
	*/
	W_HTTP &urlParam(const string &_key, const string &_value);
	W_HTTP &urlParam(const string &_key, const std::vector<string> &_valueList);
	W_HTTP &urlParam(const string &_key, const int &_value);
	W_HTTP &urlParam(const string &_key, const double &_value);

	/*
		设置发送的数据类型
			相当于head("Content-Type", value)
			默认值为：Content-Type:application/x-www-form-urlencoded
	*/
	W_HTTP &contentType(const string &_value);
	
	/*
		设置post数据
		使用该方法，会强制将method改为post,
		使用该方法，会将Content-Type改为text/plain，除非你在随后显式调用contentType("...")或者header("Content-Type", "...");
		使用该方法，_data会覆盖你此前调用过其它data()时传入的数据
	*/
	W_HTTP &data(const string &_data);
#if SUPPORT_JSON
	/*
		设置post数据
		使用该方法，会强制将method改为post,会强制将
		使用该方法，会将Content-Type改为application/json，除非你在随后显式调用contentType("...")或者header("Content-Type", "...");
		使用该方法，_data会覆盖你此前调用过其它data()时传入的数据
	*/
	W_HTTP &data(const Json::Value &_data);
#endif
	/*
		设置post数据（有多个重载）
		使用该系列方法，会强制将method改为post,
		使用该系列方法，会将Content-Type改为application/x-www-form-urlencoded，除非你在随后显式调用contentType("...")或者header("Content-Type", "...");
		使用该系列方法，_data不会覆盖你此前调用其它data（）传入的数据，而是追加
	*/
	W_HTTP &data(const string &_key, const string &_value);
	W_HTTP &data(const string &_key, const int &_value);
	W_HTTP &data(const string &_key, const double _value);
	W_HTTP &data(const string &_key, const std::vector<string> &_valueList);

	/*
		设置通讯的超时时间，单位为毫秒
			默认值为30000毫秒，即30秒
	*/
	W_HTTP &timeout(const double &_millSec);

	/*
	  保存文件路径
		如果希望请求后返回的内容保存为文件，则需要设置该参数
		由于使用的是C++的fstream库，所以传入的路径必须是该库能接受的格式
	*/
	W_HTTP &filePath(const string &_path);
public:
	/************************************************************************
	* 参数设置-回调
	*	回调只有在异步操作情况下才会用到
	*
	************************************************************************/
	/*设置执行成功时的回调函数
		仅在调用异步执行时，该设置才有意义
	*/
	W_HTTP &success(const TYPE_CALLBACK_SUCCESS &_func);

	/*设置执行失败时的回调函数
		仅在调用异步执行时，该设置才有意义
	*/
	W_HTTP &error(const TYPE_CALLBACK_ERROR &_func);

	/*设置执行完毕之后的回调，它不管执行结果是成功还是失败。都会调用。
		仅在调用异步执行时，该设置才有意义
	*/
	W_HTTP &completed(const TYPE_CALLBACK_COMPLETED &_func);

public:
	/************************************************************************
	* 执行方案分为异步与同步两种
	************************************************************************/

	/*
		异步执行http通讯
		函数会立即返回，并在回调函数中通知结果
	*/
	int run();

	/*
		同步执行http通讯
		函数在执行完毕后返回，若返回200，则_recvData表示接收到的数据
	*/
	int run(W_HTTP_RESPONSE_HEADER &resp_header, string &_recvData);
	int run(string &_recvData);

	/*下载文件
		asynchronous-是否异步下载，默认为false，表示阻塞线程，直到下载完毕才返回。
		如果改为true，则立即返回200，随后在回调中获得结果
	*/
	int download(const bool &asynchronous = false);

	/*获取最后的错误信息
	*/
	string getLastErrorMsg() { return m_http_resp_code_msg; }
private:
	//mongoose框架中，事件处理函数
	static void ev_handler(struct mg_connection *c, int ev, void *p);

	//异步执行时，提供驱动的线程
	std::thread m_td_run;
	void TFRun();
	std::thread m_td_download;
	void TFDownload();
private:
	/************************************************************************
	 * 成员变量-请求时的参数
	************************************************************************/
	//通讯方法
	string m_method;

	//通讯目标
	string m_url;

	//通讯超时时间,单位毫秒
	double m_timeout;

	//http头部
	W_HTTP_REQUEST_HEADER m_request_header;

	//保存post请求时的参数
	string m_postDataBuffer;

	//异步操作中的回调
	TYPE_CALLBACK_SUCCESS m_func_success;
	TYPE_CALLBACK_ERROR m_func_error;
	TYPE_CALLBACK_COMPLETED m_func_completed;

private:
	/************************************************************************
	* 成员变量-HTTP返回的参数
	************************************************************************/
	
	//返回状态码-1-请求参数错误，2-连接失败 4-发送失败 3-接收失败 200-成功接收 其它-HTTP状态码
	int m_http_resp_code;
	string m_http_resp_code_msg;

	//返回的HTTP报文头部
	W_HTTP_RESPONSE_HEADER m_resp_header;

	//接收数据的缓冲区
	string m_recv_buffer;

	//保存文件的路径
	string m_file_path;
private:
	/*工具函数-将url格式的参数转为map
		支持常规格式："a=1&b=2&c=3"
		支持普通数组格式："a[]=1&a[]=2&a[]=3"
	*/
	W_HTTP_REQUEST_PARAM split_query_to_map(const string &param)
	{
		W_HTTP_REQUEST_PARAM ret;

		//解析出来的关键字和值
		string key, value;
		//标志当前是在解析关键字还是在解析值
		enum {
			enkey,
			envalue
		} flag = enkey;
		//开始解析
		for (int i(0); i < param.size(); i++)
		{
			switch (param.at(i))
			{
			case '='://遇到=号时，说明完成了一个关键字的解析
				if (key.size() == 0)
				{
					//解析出错了
					return ret;
				}
				//判断这个关键字是个体还是数组，如果是数组，则把[]去掉
				if (key.find("[]") != string::npos )
				{
					key.erase(key.find("[]"), 2);
				}
				//新建一个关键字
				ret[key];
				//切换状态为解析值
				flag = envalue;
				break;
			case '&'://如果遇到一个&，表示前面完成了一对键值的解析
				if (key.size() == 0 || value.size() == 0)
				{
					return ret;
				}
				//把键值存储起来，并恢复状态
				ret[key].push_back(value);
				key.clear();
				value.clear();
				flag = enkey;
				break;
			default://根据标志位，选择把字符追加到key或是value
				if (flag == enkey)
				{
					key+= param.at(i);
				}
				else
				{
					value += param.at(i);
				}
			}
		}
		if (key.size() && value.size())
		{
			if (key.find("[]") != string::npos)
			{
				key.erase(key.find("[]"), 2);
			}
			ret[key].push_back(value);
		}
		return ret;
	}
	/*工具函数-将map结构转为url格式的参数字符串*/
	string splice_map_to_query(const W_HTTP_REQUEST_PARAM &map)
	{
		string ret;
		for (auto it = map.begin(); it != map.end(); it++)
		{
			if (it->second.size() <= 1)
			{
				//普通参数
				ret += (ret.size()? "&":"") + (it->first + "=" + it->second[0]);
			}
			else
			{
				//数组参数
				for (auto s : it->second)
				{
					ret += (ret.size() ? "&" : "") + (it->first + "[]=" + s);
				}
			}
		}
		return ret;
	}
};

/*
	错误码定义：
	-2 ：本地路径不存在
	-1 ：参数不正确
	200 ： 成功
	大于200 ： HTTP返回码
*/