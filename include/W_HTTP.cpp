#include "W_HTTP.h"
#include <iostream>
W_HTTP::W_HTTP()
	: m_timeout(30000)
	, m_http_resp_code(MG_EV_CONNECT)	//�ο�mongoose�Ķ���
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
	//��url����Ϊmg_str�ṹ
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

	//��URLת��Ϊmg_str�ṹ
	mg_str _uri = { m_url.c_str(), m_url.length() };

	//Ҫ�����Ľ��
	mg_str scheme, user_info, host, path, query, fragment;
	unsigned int port;

	//���н���
	if (0 != mg_parse_uri(_uri, &scheme, &user_info, &host, &port, &path, &query, &fragment))
	{
		//����ʧ��
		return *this;
	}

	//׷�Ӳ���
	string sQuery(query.p, query.len);
	auto map = split_query_to_map(sQuery);
	for (auto v : _valueList)
	{
		if (v.size())
		{
			//������ֵ��Ҫ����URL����
			mg_str v2 = mg_url_encode(mg_mk_str(v.c_str()));
			v.assign(v2.p, v2.len);
			free((void*)v2.p);
			map[_key].push_back(v);
		}
	}
	sQuery = splice_map_to_query(map);

	query.p = sQuery.c_str();
	query.len = sQuery.size();

	//���¾ۺϲ���ΪURL
	if (0 != mg_assemble_uri(&scheme, &user_info, &host, port, &path, &query, &fragment, true, &_uri))
	{
		//�ۺ�ʧ��
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
	//ǿ��post
	m_method = "post";

	//�޸���������
	m_request_header.set("Content-Type", "text/plain");

	m_postDataBuffer.assign(_data.begin(), _data.end());
	return *this;
}
#if SUPPORT_JSON
W_HTTP &W_HTTP::data(const Json::Value &_data)
{
	//ǿ��post
	m_method = "post";

	//�����޸���������
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
	//ǿ��post
	m_method = "post";

	//�����޸���������
	m_request_header.set("Content-Type", "application/x-www-form-urlencoded"); 

	if (_key.size() == 0 || _valueList.size() == 0)
	{
		return *this;
	}

	//׷�Ӳ���
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
	//����һ���µ��¼�������
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, this);

	//����ͷ��
	string _header;
	{
		if (m_method == "post"
			&&
			(m_request_header.get("Content-Type") == nullptr
				||
				m_request_header.get("Content-Type")->size() == 0))
		{
			//�����post����������δ����Content-Type����Ĭ������Ϊ
			m_request_header.set("Content-Type", "application/x-www-form-urlencoded");
		}

		_header = m_request_header.toString();
	}

	//��������
	mg_connection *mg_connect(nullptr);
	if (m_method == "get")
	{
		//�趨�ñ���http�������ز���
		mg_connect = mg_connect_http(&mgr, W_HTTP::ev_handler, m_url.c_str(), _header.c_str(), NULL);
	}
	else {
		//�趨�ñ���http�������ز���
		mg_connect = mg_connect_http(&mgr, W_HTTP::ev_handler, m_url.c_str(), _header.c_str(), m_postDataBuffer.c_str());
	}

	//����ʧ��ʱ�ͷ���Դ
	if (mg_connect == nullptr)
	{
		//�ͷ���Դ
		mg_mgr_free(&mgr);
		return -1;
	}

	//���ó�ʱʱ��
	if (m_timeout > 0)
	{
		mg_set_timer(mg_connect, mg_time() + m_timeout / 1000);
	}

	//�����¼���������
	while (true)
	{
		int res = mg_mgr_poll(&mgr, 1000);
		
		if (/*res == 5 || */mgr.active_connections == nullptr)
		{
			break;
		}
	}

	//�ͷ���Դ
	mg_mgr_free(&mgr);

	//�������
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
				m_http_resp_code_msg = "�ļ�·�������ڻ�Ȩ�����ⴴ��ʧ��";
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
	
	//�õ�ʵ������
	W_HTTP *pThis = (W_HTTP*)mc->mgr->user_data;
	
	switch (ev)
	{
	case MG_EV_CONNECT:
	{
		int iSuccess = *((int*)p);
		if (iSuccess == 0)
		{
			//���ӳɹ�,״̬��һ����Ϊ����ʧ��
			pThis->m_http_resp_code = MG_EV_SEND;
			pThis->m_http_resp_code_msg = "����ʧ��";
		}
		else
		{
			string errMsg(strerror(iSuccess));
			pThis->m_http_resp_code_msg = "����ʧ��:" + errMsg;
		}
	}
		break;
	case MG_EV_SEND:
	{
		//���ͳɹ���״̬��һ����Ϊ����ʧ��
		pThis->m_http_resp_code = MG_EV_RECV;
		pThis->m_http_resp_code_msg = "����ʧ��";
	}
		break;
	case MG_EV_TIMER:
	{
		pThis->m_http_resp_code = MG_EV_TIMER;
		pThis->m_http_resp_code_msg = "��ʱ";
		mc->flags |= MG_F_CLOSE_IMMEDIATELY;
	}
		break;
	case MG_EV_HTTP_REPLY://���ճɹ�
	{
		struct http_message *hm = (struct http_message *)p;
		mc->flags |= MG_F_CLOSE_IMMEDIATELY;

		//�������-������
		pThis->m_http_resp_code = hm->resp_code;
		pThis->m_http_resp_code_msg.assign(hm->resp_status_msg.p, hm->resp_status_msg.len);

		//�������-ͷ��
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

		//�������-����
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
				m_func_error(-2, "�ļ�·�������ڻ�Ȩ�����ⴴ��ʧ��", _resp_header, _recv_data);
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