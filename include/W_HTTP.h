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

//�Ƿ�֧��JSON����Ҫ�����ṩjson-cpp��֧��
#ifndef SUPPORT_JSON
#define SUPPORT_JSON 0
#endif


/*
	����http��url�����ṹ
*/
typedef std::vector<std::string> W_HTTP_REQUEST_PARAM_VALUE;
typedef std::map<std::string, W_HTTP_REQUEST_PARAM_VALUE> W_HTTP_REQUEST_PARAM;

/*
	������http��ͷ��
*/
typedef std::set<string> W_HTTP_HEADER_VALUE;
typedef std::shared_ptr<W_HTTP_HEADER_VALUE> W_SP_HTTP_HEADER_VALUE;

typedef struct{
	/*���һ���µ�ͷ��,�����޸�����ͷ����ֵ*/
	void add(const string &key, const string &value)
	{
		m_header[key].insert(value);
	}

	/*�����µ�ͷ�������key�Ѿ����ڣ�����ɾ�����е�keyͷ��*/
	void set(const string &key, const string &value)
	{
		m_header[key].clear();
		m_header[key].insert(value);
	}

	/*����ָ����ͷ�����õ������е�ֵ���������ڣ��򴴽��µ�ͷ������ֵΪ��*/
	W_HTTP_HEADER_VALUE &operator[](const string &_key)
	{
		return m_header[_key];
	}

	/*
		��ȡһ��ͷ������
			���û�У����ؿ�ָ�롣���򷵻�W_SP_HTTP_RESPONSE_HEADER_VALUE
	*/
	W_SP_HTTP_HEADER_VALUE get(const string &_key) {
		if (m_header.find(_key) == m_header.end())
		{
			return nullptr;
		}

		return W_SP_HTTP_HEADER_VALUE(new W_HTTP_HEADER_VALUE(m_header[_key].begin(), m_header[_key].end()));
	}

	/*��ͷ�����Ϊ�ַ������磺
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
	����ΪhttpͨѶ�ṩ���ѺõĲ�����ʽ��
		�û�����ͨ����ʵ�������ýӿ�������ͨѶ����
		�ڲ����������֮�󣬱��໹�ṩ���첽��ͬ����ִ�з�ʽ��

	�������⣺
		�����͵������а�������ʱ��һ�㶼Ҫע��������⡣�����ڴ�������ʱ���������Զ�����תΪutf-8

	����(ͬ��ִ�У�ʹ�ðٶ������ؼ��֡��й���)��
		string recvData;
		W_HTTP http;
		int iRet = http.method("get")
						.url("http://www.baidu.com/s")
						.urlParam("wd", "�й�")
						.syncRun(recvData);
		print(recvData)

	ע�����ϣ��֧�ַ���https�Ļ�����Ҫ��mongoose.h�еĺ�#define MG_ENABLE_SSL 0��Ϊ#define MG_ENABLE_SSL 1
		ͬʱ����opensslͷ�ļ���lib�ļ�
*/
class W_HTTP
{
public:
	W_HTTP();
	~W_HTTP();

public:
	/************************************************************************/
	/* ��������-�������                                                    */
	/************************************************************************/
	/*
		���÷���
			_method:httpͨѶ��������ѡֵΪpost��get
			Ĭ��ֵ��get
			�ݴ��ԣ����������ԣ���������Ч��ά��ԭ��ֵ����
	*/
	W_HTTP &method(const string& _method);

	/*
		����URL
			����Ŀ���ַ������http://www.baidu.com?a=1#region
			����URL�а������ĵ���Ҫ������ַ���������Զ�����
			�ݴ��ԣ����_url���ǺϷ��ģ����û���Ч
	*/
	W_HTTP &url(const string& _url);

	/*
		����httpͷ��
			_key ͷ������
			_value ͷ����ֵ
			_replace �Ƿ񸲸�ǰ�����ù���ͷ����Ĭ��Ϊfalse��������һ��ͬ����ͷ����������Ϊtrue������ɾ����������Ϊ_key��ͷ��
			��ͬһ��ͷ���ظ����ã����߻Ḳ�ǵ�ǰ�ߡ���head("key", "1");head("key", "2");����Ϊkey:2
	*/
	W_HTTP &header(const string &_key, const string &_value,const bool &replace = false);

	/*
		��url����Ӳ�����
		���øýӿ�ǰ�ȵ���url����һ����Ч��URL.
		�ݴ��ԣ�
			_key��_value������Ϊ��
			�ظ���������ͬһ��_key��Ч�����൱��������һ������Ϊ_key�����顣
				�磺urlParam("a", "1").urlParam("a", "2")�൱��urlParam("a", ["1", "2"]),���Ϊ?a[]=1&a[]=2
	*/
	W_HTTP &urlParam(const string &_key, const string &_value);
	W_HTTP &urlParam(const string &_key, const std::vector<string> &_valueList);
	W_HTTP &urlParam(const string &_key, const int &_value);
	W_HTTP &urlParam(const string &_key, const double &_value);

	/*
		���÷��͵���������
			�൱��head("Content-Type", value)
			Ĭ��ֵΪ��Content-Type:application/x-www-form-urlencoded
	*/
	W_HTTP &contentType(const string &_value);
	
	/*
		����post����
		ʹ�ø÷�������ǿ�ƽ�method��Ϊpost,
		ʹ�ø÷������ὫContent-Type��Ϊtext/plain���������������ʽ����contentType("...")����header("Content-Type", "...");
		ʹ�ø÷�����_data�Ḳ�����ǰ���ù�����data()ʱ���������
	*/
	W_HTTP &data(const string &_data);
#if SUPPORT_JSON
	/*
		����post����
		ʹ�ø÷�������ǿ�ƽ�method��Ϊpost,��ǿ�ƽ�
		ʹ�ø÷������ὫContent-Type��Ϊapplication/json���������������ʽ����contentType("...")����header("Content-Type", "...");
		ʹ�ø÷�����_data�Ḳ�����ǰ���ù�����data()ʱ���������
	*/
	W_HTTP &data(const Json::Value &_data);
#endif
	/*
		����post���ݣ��ж�����أ�
		ʹ�ø�ϵ�з�������ǿ�ƽ�method��Ϊpost,
		ʹ�ø�ϵ�з������ὫContent-Type��Ϊapplication/x-www-form-urlencoded���������������ʽ����contentType("...")����header("Content-Type", "...");
		ʹ�ø�ϵ�з�����_data���Ḳ�����ǰ��������data������������ݣ�����׷��
	*/
	W_HTTP &data(const string &_key, const string &_value);
	W_HTTP &data(const string &_key, const int &_value);
	W_HTTP &data(const string &_key, const double _value);
	W_HTTP &data(const string &_key, const std::vector<string> &_valueList);

	/*
		����ͨѶ�ĳ�ʱʱ�䣬��λΪ����
			Ĭ��ֵΪ30000���룬��30��
	*/
	W_HTTP &timeout(const double &_millSec);

	/*
	  �����ļ�·��
		���ϣ������󷵻ص����ݱ���Ϊ�ļ�������Ҫ���øò���
		����ʹ�õ���C++��fstream�⣬���Դ����·�������Ǹÿ��ܽ��ܵĸ�ʽ
	*/
	W_HTTP &filePath(const string &_path);
public:
	/************************************************************************
	* ��������-�ص�
	*	�ص�ֻ�����첽��������²Ż��õ�
	*
	************************************************************************/
	/*����ִ�гɹ�ʱ�Ļص�����
		���ڵ����첽ִ��ʱ�������ò�������
	*/
	W_HTTP &success(const TYPE_CALLBACK_SUCCESS &_func);

	/*����ִ��ʧ��ʱ�Ļص�����
		���ڵ����첽ִ��ʱ�������ò�������
	*/
	W_HTTP &error(const TYPE_CALLBACK_ERROR &_func);

	/*����ִ�����֮��Ļص���������ִ�н���ǳɹ�����ʧ�ܡ�������á�
		���ڵ����첽ִ��ʱ�������ò�������
	*/
	W_HTTP &completed(const TYPE_CALLBACK_COMPLETED &_func);

public:
	/************************************************************************
	* ִ�з�����Ϊ�첽��ͬ������
	************************************************************************/

	/*
		�첽ִ��httpͨѶ
		�������������أ����ڻص�������֪ͨ���
	*/
	int run();

	/*
		ͬ��ִ��httpͨѶ
		������ִ����Ϻ󷵻أ�������200����_recvData��ʾ���յ�������
	*/
	int run(W_HTTP_RESPONSE_HEADER &resp_header, string &_recvData);
	int run(string &_recvData);

	/*�����ļ�
		asynchronous-�Ƿ��첽���أ�Ĭ��Ϊfalse����ʾ�����̣߳�ֱ��������ϲŷ��ء�
		�����Ϊtrue������������200������ڻص��л�ý��
	*/
	int download(const bool &asynchronous = false);

	/*��ȡ���Ĵ�����Ϣ
	*/
	string getLastErrorMsg() { return m_http_resp_code_msg; }
private:
	//mongoose����У��¼�������
	static void ev_handler(struct mg_connection *c, int ev, void *p);

	//�첽ִ��ʱ���ṩ�������߳�
	std::thread m_td_run;
	void TFRun();
	std::thread m_td_download;
	void TFDownload();
private:
	/************************************************************************
	 * ��Ա����-����ʱ�Ĳ���
	************************************************************************/
	//ͨѶ����
	string m_method;

	//ͨѶĿ��
	string m_url;

	//ͨѶ��ʱʱ��,��λ����
	double m_timeout;

	//httpͷ��
	W_HTTP_REQUEST_HEADER m_request_header;

	//����post����ʱ�Ĳ���
	string m_postDataBuffer;

	//�첽�����еĻص�
	TYPE_CALLBACK_SUCCESS m_func_success;
	TYPE_CALLBACK_ERROR m_func_error;
	TYPE_CALLBACK_COMPLETED m_func_completed;

private:
	/************************************************************************
	* ��Ա����-HTTP���صĲ���
	************************************************************************/
	
	//����״̬��-1-�����������2-����ʧ�� 4-����ʧ�� 3-����ʧ�� 200-�ɹ����� ����-HTTP״̬��
	int m_http_resp_code;
	string m_http_resp_code_msg;

	//���ص�HTTP����ͷ��
	W_HTTP_RESPONSE_HEADER m_resp_header;

	//�������ݵĻ�����
	string m_recv_buffer;

	//�����ļ���·��
	string m_file_path;
private:
	/*���ߺ���-��url��ʽ�Ĳ���תΪmap
		֧�ֳ����ʽ��"a=1&b=2&c=3"
		֧����ͨ�����ʽ��"a[]=1&a[]=2&a[]=3"
	*/
	W_HTTP_REQUEST_PARAM split_query_to_map(const string &param)
	{
		W_HTTP_REQUEST_PARAM ret;

		//���������Ĺؼ��ֺ�ֵ
		string key, value;
		//��־��ǰ���ڽ����ؼ��ֻ����ڽ���ֵ
		enum {
			enkey,
			envalue
		} flag = enkey;
		//��ʼ����
		for (int i(0); i < param.size(); i++)
		{
			switch (param.at(i))
			{
			case '='://����=��ʱ��˵�������һ���ؼ��ֵĽ���
				if (key.size() == 0)
				{
					//����������
					return ret;
				}
				//�ж�����ؼ����Ǹ��廹�����飬��������飬���[]ȥ��
				if (key.find("[]") != string::npos )
				{
					key.erase(key.find("[]"), 2);
				}
				//�½�һ���ؼ���
				ret[key];
				//�л�״̬Ϊ����ֵ
				flag = envalue;
				break;
			case '&'://�������һ��&����ʾǰ�������һ�Լ�ֵ�Ľ���
				if (key.size() == 0 || value.size() == 0)
				{
					return ret;
				}
				//�Ѽ�ֵ�洢���������ָ�״̬
				ret[key].push_back(value);
				key.clear();
				value.clear();
				flag = enkey;
				break;
			default://���ݱ�־λ��ѡ����ַ�׷�ӵ�key����value
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
	/*���ߺ���-��map�ṹתΪurl��ʽ�Ĳ����ַ���*/
	string splice_map_to_query(const W_HTTP_REQUEST_PARAM &map)
	{
		string ret;
		for (auto it = map.begin(); it != map.end(); it++)
		{
			if (it->second.size() <= 1)
			{
				//��ͨ����
				ret += (ret.size()? "&":"") + (it->first + "=" + it->second[0]);
			}
			else
			{
				//�������
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
	�����붨�壺
	-2 ������·��������
	-1 ����������ȷ
	200 �� �ɹ�
	����200 �� HTTP������
*/