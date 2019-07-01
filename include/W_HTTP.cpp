#include "W_HTTP.h"
#include <iostream>
W_HTTP::W_HTTP()
	: m_timeout(30000)
	, m_http_resp_code(MG_EV_CONNECT)	//参考mongoose的定义
	, m_method("get")
{
	
}

W_HTTP::~W_HTTP()
{
	if (m_td_run.joinable())
	{
		m_td_run.join();
	}
	if (m_td_download.joinable())
	{
		m_td_download.join();
	}
}

W_HTTP &W_HTTP::method(const string &_method) {
	if (_method == "post" || _method == "get")
	{
		m_method = _method;
	}
	return *this;
}

W_HTTP &W_HTTP::url(const string& _url)
{
	//把url储存为mg_str结构
	mg_str _uri;
	_uri.p = _url.c_str();
	_uri.len = _url.size();

	if (mg_parse_uri(_uri, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == 0)
	{
		m_url = _url;
	}

	return *this;
}

W_HTTP &W_HTTP::urlParam(const string &_key, const std::vector<string> &_valueList)
{
	if (m_url.size() == 0 || _key.size() == 0 || _valueList.size() == 0)
	{
		return *this;
	}

	//将URL转储为mg_str结构
	mg_str _uri = { m_url.c_str(), m_url.length() };

	//要解析的结果
	mg_str scheme, user_info, host, path, query, fragment;
	unsigned int port;

	//进行解析
	if (0 != mg_parse_uri(_uri, &scheme, &user_info, &host, &port, &path, &query, &fragment))
	{
		//解析失败
		return *this;
	}

	//追加参数
	string sQuery(query.p, query.len);
	auto map = split_query_to_map(sQuery);
	for (auto v : _valueList)
	{
		if (v.size())
		{
			//参数的值需要进行URL编码
			mg_str v2 = mg_url_encode(mg_mk_str(v.c_str()));
			v.assign(v2.p, v2.len);
			free((void*)v2.p);
			map[_key].push_back(v);
		}
	}
	sQuery = splice_map_to_query(map);

	query.p = sQuery.c_str();
	query.len = sQuery.size();

	//重新聚合参数为URL
	if (0 != mg_assemble_uri(&scheme, &user_info, &host, port, &path, &query, &fragment, true, &_uri))
	{
		//聚合失败
		return *this;
	}

	m_url.assign(_uri.p, _uri.len);
	free((void*)_uri.p);

	return *this;
}
W_HTTP &W_HTTP::urlParam(const string &_key, const string &_value)
{
	vector<string> _valueList;
	_valueList.push_back(_value);
	return urlParam(_key, _valueList);
}
W_HTTP &W_HTTP::urlParam(const string &_key, const int &_value)
{
	char cBuffer[16] = { 0 };
	sprintf(cBuffer, "%d", _value);
	return urlParam(_key, cBuffer);
}
W_HTTP &W_HTTP::urlParam(const string &_key, const double &_value)
{
	char cBuffer[16] = { 0 };
	sprintf(cBuffer, "%f", _value);
	return urlParam(_key, cBuffer);
}

W_HTTP &W_HTTP::header(const string &_key, const string &_value, const bool &_replace)
{
	if (_key.size() > 0)
	{
		if (_replace)
		{
			m_request_header.set(_key, _value);
		}
		else
		{
			m_request_header.add(_key, _value);
		}
		
	}
	
	return *this;
}

W_HTTP &W_HTTP::filePath(const string &_path)
{
	m_file_path = _path;
	return *this;
}

W_HTTP &W_HTTP::contentType(const string &_value)
{
	m_request_header.set("Content-Type", _value);
	return *this;
}

W_HTTP &W_HTTP::data(const string &_data)
{
	//强制post
	m_method = "post";

	//修改内容类型
	m_request_header.set("Content-Type", "text/plain");

	m_postDataBuffer.assign(_data.begin(), _data.end());
	return *this;
}
#if SUPPORT_JSON
W_HTTP &W_HTTP::data(const Json::Value &_data)
{
	//强制post
	m_method = "post";

	//尝试修改内容类型
	m_request_header.set("Content-Type", "application/json");

	string jsonString = _data.toStyledString();
	m_postDataBuffer.assign(jsonString.begin(), jsonString.end());
	return *this;
}
#endif
W_HTTP &W_HTTP::data(const string &_key, const string &_value)
{
	vector<string> _valueList;
	_valueList.push_back(_value);
	return data(_key, _valueList);
}
W_HTTP &W_HTTP::data(const string &_key, const int &_value)
{
	char cBuffer[16] = { 0 };
	sprintf(cBuffer, "%d", _value);
	return data(_key, cBuffer);
}
W_HTTP &W_HTTP::data(const string &_key, const double _value)
{
	char cBuffer[16] = { 0 };
	sprintf(cBuffer, "%f", _value);
	return data(_key, cBuffer);
}
W_HTTP &W_HTTP::data(const string &_key, const std::vector<string> &_valueList)
{
	//强制post
	m_method = "post";

	//尝试修改内容类型
	m_request_header.set("Content-Type", "application/x-www-form-urlencoded"); 

	if (_key.size() == 0 || _valueList.size() == 0)
	{
		return *this;
	}

	//追加参数
	string sQuery(m_postDataBuffer.begin(), m_postDataBuffer.end());
	auto map = split_query_to_map(sQuery);
	for (auto v : _valueList)
	{
		if (v.size())
		{
			map[_key].push_back(v);
		}
	}
	sQuery = splice_map_to_query(map);

	m_postDataBuffer.assign(sQuery.begin(), sQuery.end());

	return *this;
}

W_HTTP &W_HTTP::timeout(const double &_millSec)
{
	m_timeout = _millSec;
	return *this;
}

W_HTTP &W_HTTP::success(const TYPE_CALLBACK_SUCCESS &_func)
{
	m_func_success = _func;
	return *this;
}

W_HTTP &W_HTTP::error(const TYPE_CALLBACK_ERROR &_func)
{
	m_func_error = _func;
	return *this;
}

W_HTTP &W_HTTP::completed(const TYPE_CALLBACK_COMPLETED &_func)
{
	m_func_completed = _func;
	return *this;
}

int W_HTTP::run()
{
	if (m_td_run.joinable() == false)
	{
		m_td_run = std::thread(std::bind(&W_HTTP::TFRun, this));
	}
	return 200;
}

int W_HTTP::run(W_HTTP_RESPONSE_HEADER &_resp_header, string &_recvData)
{
	//创建一个新的事件管理器
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, this);

	//制作头部
	string _header;
	{
		if (m_method == "post"
			&&
			(m_request_header.get("Content-Type") == nullptr
				||
				m_request_header.get("Content-Type")->size() == 0))
		{
			//如果是post方法，并且未设置Content-Type，则默认设置为
			m_request_header.set("Content-Type", "application/x-www-form-urlencoded");
		}

		_header = m_request_header.toString();
	}

	//创建请求
	mg_connection *mg_connect(nullptr);
	if (m_method == "get")
	{
		//设定好本次http请求的相关参数
		mg_connect = mg_connect_http(&mgr, W_HTTP::ev_handler, m_url.c_str(), _header.c_str(), NULL);
	}
	else {
		//设定好本次http请求的相关参数
		mg_connect = mg_connect_http(&mgr, W_HTTP::ev_handler, m_url.c_str(), _header.c_str(), m_postDataBuffer.c_str());
	}

	//创建失败时释放资源
	if (mg_connect == nullptr)
	{
		//释放资源
		mg_mgr_free(&mgr);
		return -1;
	}

	//设置超时时间
	if (m_timeout > 0)
	{
		mg_set_timer(mg_connect, mg_time() + m_timeout / 1000);
	}

	//驱动事件运行起来
	while (true)
	{
		int res = mg_mgr_poll(&mgr, 1000);
		
		if (/*res == 5 || */mgr.active_connections == nullptr)
		{
			break;
		}
	}

	//释放资源
	mg_mgr_free(&mgr);

	//解析结果
	_resp_header = m_resp_header;
	_recvData = m_recv_buffer;
	return m_http_resp_code;
}

int W_HTTP::run(string &_recvData)
{
	W_HTTP_RESPONSE_HEADER _resp_header;
	return run(_resp_header, _recvData);
}

int W_HTTP::download(const bool &asynchronous) {
	if (asynchronous == false)
	{
		string recv_data;
		int res = run(recv_data);
		if (res == 200 && recv_data.size() > 0)
		{
			fstream file;
			file.open(m_file_path, fstream::ios_base::out | fstream::ios_base::trunc | fstream::ios_base::binary);

			if (file.fail())
			{
				m_http_resp_code_msg = "文件路径不存在或权限问题创建失败";
				return -2;
			}
			file.write(recv_data.c_str(), recv_data.size());
			file.close();
		}
		return res;
	}
	else
	{
		if (m_td_download.joinable() == false)
		{
			m_td_download = std::thread(std::bind(&W_HTTP::TFDownload, this));
		}
		return 200;
	}
}


void W_HTTP::ev_handler(struct mg_connection *mc, int ev, void *p)
{
	//if (ev != 0)
	//{
	//	std::cout <<"ev:"<< ev << endl;
	//}
	
	//拿到实例对象
	W_HTTP *pThis = (W_HTTP*)mc->mgr->user_data;
	
	switch (ev)
	{
	case MG_EV_CONNECT:
	{
		int iSuccess = *((int*)p);
		if (iSuccess == 0)
		{
			//连接成功,状态进一步变为发送失败
			pThis->m_http_resp_code = MG_EV_SEND;
			pThis->m_http_resp_code_msg = "发送失败";
		}
		else
		{
			string errMsg(strerror(iSuccess));
			pThis->m_http_resp_code_msg = "连接失败:" + errMsg;
		}
	}
		break;
	case MG_EV_SEND:
	{
		//发送成功，状态进一步变为接收失败
		pThis->m_http_resp_code = MG_EV_RECV;
		pThis->m_http_resp_code_msg = "接收失败";
	}
		break;
	case MG_EV_TIMER:
	{
		pThis->m_http_resp_code = MG_EV_TIMER;
		pThis->m_http_resp_code_msg = "超时";
		mc->flags |= MG_F_CLOSE_IMMEDIATELY;
	}
		break;
	case MG_EV_HTTP_REPLY://接收成功
	{
		struct http_message *hm = (struct http_message *)p;
		mc->flags |= MG_F_CLOSE_IMMEDIATELY;

		//解析结果-返回码
		pThis->m_http_resp_code = hm->resp_code;
		pThis->m_http_resp_code_msg.assign(hm->resp_status_msg.p, hm->resp_status_msg.len);

		//解析结果-头部
		for (int i(0); i < sizeof(hm->header_names) / sizeof(mg_str); i++)
		{
			if (hm->header_names[i].len > 0)
			{
				string key(hm->header_names[i].p, hm->header_names[i].len);
				string value(hm->header_values[i].p, hm->header_values[i].len);
				pThis->m_resp_header.add(key, value);
			}
			else
			{
				break;
			}
		}

		//解析结果-主体
		pThis->m_recv_buffer.assign(hm->body.p, hm->body.len);

	}
		break;
	case MG_EV_CLOSE:
	{
	}
		break;
	}
}

void W_HTTP::TFRun()
{
	W_HTTP_RESPONSE_HEADER _resp_header;
	string _recvData;
	int result = run(_resp_header, _recvData);
	if (result == 200)
	{
		if (m_func_success)
		{
			m_func_success(_resp_header, _recvData);
		}
	}
	else
	{
		if (m_func_error)
		{
			m_func_error(result, getLastErrorMsg(), _resp_header, _recvData);
		}
	}

	if (m_func_completed)
	{
		m_func_completed();
	}
}

void W_HTTP::TFDownload()
{
	W_HTTP_RESPONSE_HEADER _resp_header;
	string _recv_data;
	int result = run(_recv_data);
	if (result == 200 && _recv_data.size() > 0)
	{
		fstream file;
		file.open(m_file_path, fstream::ios_base::out | fstream::ios_base::trunc | fstream::ios_base::binary);

		if (file.fail())
		{
			if (m_func_error)
			{
				m_func_error(-2, "文件路径不存在或权限问题创建失败", _resp_header, _recv_data);
			}
		}
		else
		{
			file.write(_recv_data.c_str(), _recv_data.size());
			file.close();
			if (m_func_success)
			{
				m_func_success(_resp_header, _recv_data);
			}
		}
	}
	else
	{
		if (m_func_error)
		{
			m_func_error(result, getLastErrorMsg(), _resp_header, _recv_data);
		}
	}

	if (m_func_completed)
	{
		m_func_completed();
	}
}