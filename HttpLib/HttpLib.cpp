#include "HttpLib.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#if defined(WIN32) || defined(WIN64)
#	define errno GetLastError()
	typedef unsigned long in_addr_t;
#else
#	include <iconv.h>
#	define stricmp strcasecmp
#endif

#define ClearStdString(str)str.clear()

namespace HTTPLib
{
char xtod(char c) {
	if (c>='0' && c<='9') return c-'0';
	if (c>='A' && c<='F') return c-'A'+10;
	if (c>='a' && c<='f') return c-'a'+10;
	return c=0;        // not Hex digit
}
int HextoDec(char *hex, int l)
{
	if (*hex==0) return(l);
	return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}
int xstrtoi(char *hex)      // hex string to integer
{
	return HextoDec(hex,0);
}

char* itoa(int _Val, char * _DstBuf, int _Radix)
{
	const char* fmt = "%d";
	switch(_Radix){
	case 2: fmt = "%b"; break;
	case 8: fmt = "%o"; break;
	case 10: break;
	case 16: fmt = "%x"; break;
	}
	sprintf(_DstBuf, fmt, _Val);
	return _DstBuf;
}
in_addr_t GetIpByString(const char* strAddr)
{
	if(strAddr == NULL)
		return INADDR_NONE;

	in_addr_t ip = inet_addr(strAddr);
	if(ip == INADDR_NONE)
	{
		hostent* hst = gethostbyname(strAddr);
		if(hst){
			in_addr** addrArr = (in_addr**)hst->h_addr_list;
			if(addrArr){
				if(addrArr[0]){
					ip = addrArr[0]->s_addr;
				}
			}
		}
	}

	return ip;
}
}

#define CODE_BLOCK_OF_EXCEPTION
namespace HTTPLib
{
	HttpException::HttpException(ExceptionCode ec, const char* format, ...)
	{
		errorCode = ec;
		description = "unknown";
		va_list args;
		va_start(args, format);
		int len = vsnprintf(NULL, 0, format, args);
		if(len > 0){
			char* buf = new char[len+1];
			if(buf){
				vsprintf(buf, format, args);
				description = buf;
				delete [] buf;
			}
		}
		va_end(args);
	}
	ExceptionCode HttpException::GetErrorCode()
	{
		return errorCode;
	}
	const char* HttpException::GetErrorDescription()
	{
		return description.c_str();
	}
}

#define CODE_BLOCK_OF_COMMON_FUNCTIONS
namespace HTTPLib
{
#define ICONV_TMP_SIZE 256
	string Ansi_2_Utf8(const char* asciiString)
	{
#ifdef WIN32
		int len=MultiByteToWideChar(CP_ACP, 0, asciiString, -1, NULL,0);
		wchar_t* uni = new wchar_t[len+1];
		memset(uni, 0, len * 2 + 2);
		MultiByteToWideChar(CP_ACP, 0, asciiString, -1, uni, len); 

		len = WideCharToMultiByte(CP_UTF8, 0, uni, -1, NULL, 0, NULL, NULL); 
		char *u8=new char[len + 1];
		memset(u8, 0, len + 1);
		WideCharToMultiByte (CP_UTF8, 0, uni, -1, u8, len, NULL,NULL); 

		std::string strU8 = u8;
		delete[] uni;
		delete[] u8;

		return strU8;
#else
		iconv_t cd = iconv_open("ascii", "utf8");
		if(cd == (iconv_t)-1)
			return asciiString;

		std::string strU8;
		char u8_tmp[ICONV_TMP_SIZE] = {0};
		char* in = (char*)asciiString;
		char* out = u8_tmp;
		size_t inLeft = strlen(asciiString);
		size_t outLeft = ICONV_TMP_SIZE;
		while(inLeft > 0)
		{
			out = u8_tmp;
			outLeft = ICONV_TMP_SIZE;
			iconv(cd, &in, &inLeft, &out, &outLeft);
			strU8.append(u8_tmp, ICONV_TMP_SIZE-outLeft);
		}
		iconv_close(cd);
		return strU8;
#endif
	}

	string Utf8_2_Ansi(const char* strUtf8)
	{
#ifdef WIN32
		int len=MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, NULL,0);
		wchar_t* wszGBK = new wchar_t[len+1];
		if(wszGBK == NULL)
			return "";
		memset(wszGBK, 0, len * 2 + 2);
		MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, wszGBK, len); 

		len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL); 
		char *szGBK=new char[len + 1];
		if(szGBK == NULL){
			delete [] wszGBK;
			return "";
		}
		memset(szGBK, 0, len + 1);
		WideCharToMultiByte (CP_ACP, 0, wszGBK, -1, szGBK, len, NULL,NULL); 

		std::string gbk = szGBK;
		delete[] szGBK;
		delete[] wszGBK;

		return gbk;
#else
		iconv_t cd = iconv_open("utf8", "ascii");
		if(cd == (iconv_t)-1)
			return strUtf8;

		std::string strAnsi;
		char ansi_tmp[ICONV_TMP_SIZE];
		char* in = (char*)strUtf8;
		char* out = ansi_tmp;
		size_t inLeft = strlen(strUtf8);
		size_t outLeft = ICONV_TMP_SIZE;
		size_t srcLen = inLeft;
		while(inLeft > 0){
			out = ansi_tmp;
			outLeft = ICONV_TMP_SIZE;
			iconv(cd, &in, &inLeft, &out, &outLeft);
			strAnsi.append(ansi_tmp, ICONV_TMP_SIZE-outLeft);
		}
		iconv_close(cd);
		return strAnsi;
#endif
	}

	string EncodeUrl(IN const char* url, IN const char* unsafeCharactors/*=" \t"*/)
	{
		if(unsafeCharactors == NULL)
			unsafeCharactors = " \t";

		string ret;

		const char* p = url;
		const char* unsafe = NULL;
		char hex[8] = {0};
		for(; *p!='\0'; p++)
		{
			unsafe = strchr(unsafeCharactors, *p);
			if(unsafe == NULL)
				ret.append(p, 1);
			else{
				sprintf(hex, "%%%02x", (int)(*p));
 				ret.append(hex);
			}
		}
		return ret;
	}

	/* http://user:password@server:port/object
	   http
	   user:password
	   server:port
	   object
	*/
	void ParseUrl(
		IN  const char* url,
		OUT HttpProtocal& proto,
		OUT string& server,
		OUT int& port,
		OUT string& object,
		OUT string& userName,
		OUT string& password)
	{
		if(url == NULL)
			throw HttpException(err_null_parameter, "Parameter of url is null");
		if(url[0] == '\0')
			throw HttpException(err_empty_parameter, "Parameter of url is empty");

		const char* p1 = url;
		const char* p2 = NULL;

		//http or https
		p2 = strstr(p1, "://");
		if(p2 == NULL)
			throw HttpException(err_invalid_url, "Invalid URL");
		string sProto(p1, p2);
		if(stricmp("http", sProto.c_str()) == 0){
			proto = proto_http;
			port = 80;
		}
		else if(stricmp("https", sProto.c_str()) == 0){
			proto = proto_https;
			port = 443;
		}
		else if(stricmp("ftp", sProto.c_str()) == 0){
			proto = proto_ftp;
			port = 21;
		}
		else if(stricmp("sftp", sProto.c_str()) == 0){
			proto = proto_sftp;
			port = 22;
		}
		else
			throw HttpException(err_invalid_protocal, "Unsupported scheme: %s", sProto.c_str());
		p1 = p2 + 3;

		//user:password
		p2 = strchr(p1, '@');
		if(p2)
		{
			ClearStdString(userName);
			userName.insert(userName.begin(), p1, p2);
			int pos = userName.find(':');
			if(pos > -1)
			{
				password = userName.substr(pos+1);
				userName = userName.substr(0, pos);
			}
			p1 = p2 + 1;
		}

		//server:port/object
		if(p1[0] == 0)
			throw HttpException(err_invalid_url, "Invalid URL");
		ClearStdString(server);
		ClearStdString(object);
		p2 = strchr(p1, '/');
		if(p2)
		{
			object = p2;
			server.insert(server.begin(), p1, p2);
		}
		else
		{
			object = "/";

			p2 = strchr(p1, '?');
			if(p2){
				object.append(p2);
				server.insert(server.begin(), p1, p2);
			}
			else server = p1;
		}

		//server:port
		int pos = server.find(':');
		if(pos == 0)
			throw HttpException(err_invalid_url, "Invalid URL");
		if(pos > -1)
		{
			string sPort = server.substr(pos+1);
			server = server.substr(0, pos);
			port = atoi(sPort.c_str());
		}
		if(server.length() <= 0)
			throw HttpException(err_invalid_url, "Invalid URL");
	}
}

#define CODE_BLOCK_OF_HTTP_REQUEST
namespace HTTPLib
{
	unsigned long HttpRequest::HeaderNode::genSysOrder =  0x00000000;
	unsigned long HttpRequest::HeaderNode::genUserOrder = 0x08000000;

	HttpRequest::HttpRequest()
	{
		m_nPort = 80;
		itoa(m_nPort, m_strPort, 10);
		m_postData = NULL;
	}
	HttpRequest::~HttpRequest()
	{
		ClearHeaders();
	}
	void HttpRequest::Reset()
	{
		ClearStdString(m_version);
		m_method = method_get;
		ClearHeaders();
		m_postData = NULL;
		ClearStdString(m_rawRequestHeader);

		m_protocal = proto_http;
		ClearStdString(m_userName);
		ClearStdString(m_password);
		ClearStdString(m_server);
		m_nPort = 80;
		itoa(m_nPort, m_strPort, 10);
		ClearStdString(m_object);
	}
	void HttpRequest::ClearHeaders()
	{
		HeaderMap::iterator it = m_mapHeaders.begin();
		for(; it!=m_mapHeaders.end(); it++)
			delete it->second;
		m_mapHeaders.clear();
	}

	void HttpRequest::SetAgentName(const char* agentName)
	{
		m_agentName = agentName;
	}
	void HttpRequest::SetURL(const char* url)
	{
		if(url == NULL)
			throw HttpException(err_null_parameter, "You push a null parameter while SetURL");
		if(url[0] == 0)
			throw HttpException(err_empty_parameter, "You push a empty parameter while SetURL");
		ParseUrl(url, m_protocal, m_server, m_nPort,
			m_object, m_userName, m_password);
		itoa(m_nPort, m_strPort, 10);
	}
	void HttpRequest::SetHttpVersion(HttpVersion version)
	{
		switch(version)
		{
		case ver_http11:
			m_version = "HTTP/1.1";
			break;
		default:
			throw HttpException(err_unsupported_http_version, "Unsupported HTTP Version");
		}
	}
	void HttpRequest::SetMethod(HttpMethod method)
	{
		switch(method)
		{
		case method_get:
		case method_post:
		case method_head:
			break;
		default:
			throw HttpException(err_unsupported_http_version, "Unsupported HTTP method");
		}
		m_method = method;
	}
	void HttpRequest::SetKeepAlive(bool bKeep)
	{
		setRequestProperty("Connection", bKeep?"Keep-Alive":"close");
	}
	void HttpRequest::SetRange(uint64 bytesBegin, uint64 bytesEnd/* =-1 */)
	{
		char sEnd[32] = {0};
		if(bytesEnd != -1)
			sprintf(sEnd, "%llu", bytesEnd);

		char range[128] = {0};
		sprintf(range, "bytes=%llu-%s\r\n", bytesBegin, sEnd);

		setRequestProperty("Range", range);
	}
	void HttpRequest::setRequestProperty(const char* name, const char* value)
	{
		AddSysHeader(name, value, HeaderNode::genUserOrder);
	}
	void HttpRequest::AddSysHeader(const char* name, const char* value, unsigned long& genOrder)
	{
		HeaderNode* hn = NULL;

		HeaderMap::iterator it = m_mapHeaders.find(name);
		if(it != m_mapHeaders.end())
		{
			hn = it->second;
			hn->name = name;
			hn->value = value;
			if(&genOrder == &HeaderNode::genSysOrder) //keep SysHdr's order
				hn->order = HeaderNode::genSysOrder++;
			return;
		}

		hn = new HeaderNode;
		if(hn == NULL)
			throw HttpException(err_no_enough_memory, "Out of memory to setRequestProperty (%d bytes)", sizeof(HeaderNode));
		hn->order = genOrder++;
		hn->name = name;
		hn->value = value;
		m_mapHeaders.insert(std::make_pair(name, hn));
	}

	void HttpRequest::SetPostData(IPostData* data)
	{
		m_postData = data;
	}
	int HttpRequest::GetServerPort()
	{
		return m_nPort;
	}
	const char* HttpRequest::GetServerIp()
	{
		return m_server.c_str();
	}
	const char* HttpRequest::GetRawRequestHeader()
	{
		if(m_rawRequestHeader.length() > 0)
			return m_rawRequestHeader.c_str();

		if(m_agentName.length() <= 0)
			m_agentName = HTTPLIB_AGENT;
		AddSysHeader("Host", (m_server + string(":") + m_strPort).c_str(), HeaderNode::genSysOrder);
		AddSysHeader("User-Agent", m_agentName.c_str(), HeaderNode::genSysOrder);
		if(m_mapHeaders.find("Accept") == m_mapHeaders.end())
			AddSysHeader("Accept", "*/*", HeaderNode::genSysOrder);

		string strMethod;
		switch(m_method)
		{
		case method_post:
			strMethod = "POST ";
			{
				uint64 len = 0;
				if(m_postData){
					len = m_postData->GetPostDataSize();
					const char* ct = m_postData->GetContentType();
					if(ct && ct[0]!='\0')
						AddSysHeader("Content-Type", ct, HeaderNode::genSysOrder);
				}
				char tmp[64];
				sprintf(tmp, "%llu", len);
				AddSysHeader("Content-Length", tmp, HeaderNode::genSysOrder);
			}
			break;
		case method_get:
			strMethod = "GET ";
			m_postData = NULL;
			break;
		case method_head:
			strMethod = "HEAD ";
			m_postData = NULL;
			break;
		default:
			throw HttpException(err_unsupported_http_method, "Unsupported HTTP method");
		}

		ClearStdString(m_rawRequestHeader);
		MakeRawRequestHeader(strMethod.c_str());
//		m_rawRequestHeader = Ansi_2_Utf8(m_rawRequestHeader.c_str());

		return m_rawRequestHeader.c_str();
	}

	IPostData* HttpRequest::GetPostData()
	{
		return m_postData;
	}

	//
	void HttpRequest::CopyHeadersToSet(HeaderMap& hdrMap, HeaderSet& hdrSet)
	{
		HeaderMap::iterator it = hdrMap.begin();
		for(; it!=hdrMap.end(); it++)
		{
			hdrSet.insert(it->second);
		}
	}
	void HttpRequest::MakeRawRequestHeader(const char* method)
	{
		m_rawRequestHeader.append(method).append(m_object).append(" ").append(m_version).append("\r\n");

		HeaderSet hdrSet;
		HeaderSet::iterator it;

		CopyHeadersToSet(m_mapHeaders, hdrSet);
		it = hdrSet.begin();
		for(; it!=hdrSet.end(); it++)
			m_rawRequestHeader.append((*it)->name).append(": ").append((*it)->value).append("\r\n");
		m_rawRequestHeader.append("\r\n");
	}
}

#define CODE_BLOCK_OF_HTTP_RESPONSE
namespace HTTPLib
{
	HttpResponseHeader::HttpResponseHeader()
	{
		m_respCode = resp_unknown;
	}
	HttpResponseHeader::~HttpResponseHeader()
	{

	}
	void HttpResponseHeader::Reset()
	{
		ClearStdString(m_rawHeader);
		m_respCode = resp_unknown;
		m_headers.clear();
	}

	void HttpResponseHeader::SetRawHeader(const char* rawHdr)
	{
		if(rawHdr == NULL)
			throw HttpException(err_null_parameter, "A null parameter while SetRawHeader");
		if(rawHdr[0] == 0)
			throw HttpException(err_empty_parameter, "An empty parameter while SetRawHeader");

		m_rawHeader = rawHdr;
		ParseHeaders(m_rawHeader.c_str());
	}
	const char* HttpResponseHeader::GetRawHeaderContent()
	{
		return m_rawHeader.c_str();
	}
	ResponseStatusCode HttpResponseHeader::GetStatus()
	{
		return m_respCode;
	}
	bool HttpResponseHeader::IsHeaderExists(const char* header)
	{
		return m_headers.find(header) != m_headers.end();
	}
	const char* HttpResponseHeader::GetHeaderValue(const char* header)
	{
		HeaderMap::iterator it = m_headers.find(header);
		if(it != m_headers.end())
		{
			return it->second.c_str();
		}
		return NULL;
	}

	void HttpResponseHeader::ParseHeaders(const char* headers)
	{
		//parse STATUS
		m_headers.clear();
		const char* pos = strchr(headers, ' ');
		if(pos == NULL)
			throw HttpException(err_invalid_http_response, "Invalid HTTP response");
		m_respCode = (ResponseStatusCode)atoi(++pos);

		//parse HEADERs
		pos = strstr(headers, "\r\n");
		if(pos == NULL)
			throw HttpException(err_invalid_http_response, "Invalid HTTP response");
		pos += 2;

		string line;
		while(true)
		{
			const char* p = strstr(pos, "\r\n");
			if(p == NULL)
				break;
			ClearStdString(line);
			line.insert(line.begin(), pos, p);
			pos = p + 2;

			ParseOneHeader(line.c_str());
		}
		ParseOneHeader(pos);
	}
	void HttpResponseHeader::ParseOneHeader(const char* rawResp)
	{
		const char* p = strchr(rawResp, ':');
		if(p)
		{
			string name(rawResp, p);
			string value(++p);
			int pos = value.find_first_not_of(" \t");
			if(pos > 0)
				value.erase(0, pos);
			else if(pos < 0)
				ClearStdString(value);
			m_headers.insert(std::make_pair(name, value));
		}
	}
}

#define CODE_BLOCK_OF_HTTP_CLIENT
namespace HTTPLib
{
	HttpClient::HttpClient()
	{
		m_socket = INVALID_SOCKET;
		m_bHdrCompleted = false;
		m_bContinue = true;
		m_bChunked = false;
		m_chunkedBuff = NULL;
		m_chunkedBuffSize = 0;
		m_chunkedBuffUsedSize = 0;
	}
	HttpClient::~HttpClient()
	{
		Reset();
	}
	void HttpClient::Reset()
	{
		m_respHeader.Reset();
		CloseSocket();
		m_bHdrCompleted = false;
		m_bContinue = true;
		m_bChunked = false;
		if(m_chunkedBuff)
			free(m_chunkedBuff);
		m_chunkedBuff = NULL;
		m_chunkedBuffSize = 0;
		m_chunkedBuffUsedSize = 0;
	}
	void HttpClient::SetResponseContentCallback(IHttpResponseContentCallback* callback)
	{
		m_callback = callback;
	}
	bool HttpClient::SetNonBlocking(bool bNonBlocking /*= true*/)
	{
		int ret = 0;
#	if defined(WIN32) || defined(WIN64)
		unsigned long ul = bNonBlocking ? 1 : 0;
		ret = ioctlsocket(m_socket, FIONBIO, (unsigned long*)&ul);
#	else
		int option = bNonBlocking ? O_ASYNC|O_NONBLOCK : 0;
		ret = fcntl(m_socket, F_SETFL, option);
#	endif
		return ret == 0;
	}
	void HttpClient::StartRequest(IHttpRequest* request, int connectTimeoutMilliSeconds/*=5000*/)
	{
		m_bContinue = true;
		try
		{
			int ret = 0;

			m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if(m_socket == INVALID_SOCKET)
				throw HttpException(err_create_socket_failed, "Create socket failed: errno=%d", errno);

			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(request->GetServerPort());
			addr.sin_addr.s_addr = GetIpByString(request->GetServerIp());

			bool bNonBlocking = SetNonBlocking(true);

			ret = connect(m_socket, (sockaddr*)&addr, sizeof(addr));
 			if(!bNonBlocking && SOCKET_ERROR == ret)
 				throw HttpException(err_connection_failed, "Connect to web server failed A: errno=%d", errno);

			if(bNonBlocking)
			{
				fd_set set;
				timeval tv;
				tv.tv_sec = connectTimeoutMilliSeconds/1000;
				tv.tv_usec = (connectTimeoutMilliSeconds%1000)*1000;
				FD_ZERO(&set);
				FD_SET(m_socket, &set);
#if defined(WIN32) || defined(WIN64)
				ret = select(0, NULL, &set, NULL, &tv);
#else
				ret = select(m_socket+1, NULL, &set, NULL, &tv);
#endif
				if(ret == 0){ //timed out
					throw HttpException(err_connect_timeout, "Connect to web server timed out B: errno=%d", errno);
				}else if(ret < 0){ //error
					throw HttpException(err_connection_failed, "Connect to web server failed C: errno=%d", errno);
				}else{
					if(!FD_ISSET(m_socket, &set))
						throw HttpException(err_connection_failed, "Connect to web server failed D: errno=%d", errno);
				}

				SetNonBlocking(false);
			}

			//send request header
			const char* reqHdr = request->GetRawRequestHeader();
			int reqLen = strlen(reqHdr);
			int sendLen = SendBuffer(reqHdr, reqLen);

			//send post data
			IPostData* postData = request->GetPostData();
			if(postData){
				if(postData->PrepareToSendPostData()){
					const char* data = NULL;
					int sizeInBytes = 0;
					bool bMoreData = false;
					do{
						bMoreData = postData->GetPartOfPostData(&data, &sizeInBytes);
						if(data && sizeInBytes>0){
							SendBuffer(data, sizeInBytes);
						}
					}while(bMoreData);
				}
			}
		}
		catch(HttpException& he)
		{
			CloseSocket();
			throw he;
		}
	}
	int HttpClient::SendBuffer(const char* buffer, int sizeInBytes)
	{
		int sendLen = 0;
		while (m_bContinue && sendLen<sizeInBytes)
		{
			int n = send(m_socket, buffer+sendLen, sizeInBytes-sendLen, 0);
			if(n == SOCKET_ERROR)
				throw HttpException(err_send_request_failed, "Send HTTP Request failed: errno=%d", errno);
			sendLen += n;
		}
		return sendLen;
	}
	void HttpClient::OnRawHeaderCompleted()
	{
		const char* transEnc = m_respHeader.GetHeaderValue("Transfer-Encoding");
		if(transEnc){
			if(strstr(transEnc, "chunked")){
				m_bChunked = true;
			}
		}
		if(m_callback)m_callback->OnHeaderCompleted();
	}
	void HttpClient::OnRawContent(void* content, int sizeInBytes)
	{
		if(m_callback==NULL || content==NULL || sizeInBytes==0)
			return;

		//normal content
		if(!m_bChunked){
			m_callback->OnContent(content, sizeInBytes);
			return;
		}

		//trunked content: prepare buffer
		int leftSize = m_chunkedBuffSize - m_chunkedBuffUsedSize;
		if(leftSize < sizeInBytes){
			char* tmpBuf = (char*)realloc(m_chunkedBuff, m_chunkedBuffUsedSize+sizeInBytes);
			if(tmpBuf == NULL)
				throw HttpException(err_no_enough_memory, "Out of memory to receive HTTP Content (%d bytes)", m_chunkedBuffUsedSize+sizeInBytes);
			m_chunkedBuff = tmpBuf;
			m_chunkedBuffSize = m_chunkedBuffUsedSize+sizeInBytes;
		}

		//trunked content: cache content
		memcpy(m_chunkedBuff+m_chunkedBuffUsedSize, content, sizeInBytes);
		m_chunkedBuffUsedSize += sizeInBytes;

NEXT_CHUNK_BLOCK:

		//chunk size
#define CHUNK_BOUNDARY "\r\n"
#define CHUNK_BOUNDARY_LEN 2
		int pos = memstr(m_chunkedBuff, m_chunkedBuffUsedSize, CHUNK_BOUNDARY, 0);
		if(pos < 0)
			return;
		m_chunkedBuff[pos] = '\0';
		int chunk_size = xstrtoi(m_chunkedBuff);
		int handleSize = pos+CHUNK_BOUNDARY_LEN+chunk_size+CHUNK_BOUNDARY_LEN;
		if(m_chunkedBuffUsedSize < handleSize){
			m_chunkedBuff[pos] = '\r';
			return;
		}

		//
		char* chunk = m_chunkedBuff+pos+CHUNK_BOUNDARY_LEN;
		m_callback->OnContent(chunk, chunk_size);
		memmove(m_chunkedBuff, m_chunkedBuff+handleSize, m_chunkedBuffUsedSize-handleSize);
		m_chunkedBuffUsedSize -= handleSize;

		goto NEXT_CHUNK_BLOCK;
	}
	void HttpClient::WaitForResponse(int recvBufferSize/* = 1500-40*/, int recvTimeoutMilliSeconds/*=7000*/)
	{
#define __ReleaseLocalBuffers \
	if(recvBuffer){delete [] recvBuffer; recvBuffer = NULL;}\
	if(headerBuf){free(headerBuf); headerBuf = NULL;}

		char* recvBuffer = NULL;
		char* headerBuf = NULL;
		int headerBufSize = 0;
		try
		{
			recvBuffer = new char[recvBufferSize];
			if(recvBuffer == NULL)
				throw HttpException(err_no_enough_memory, "Out of memory while receive HTTP response (%d bytes)", recvBufferSize);

			bool bNonBlocking = SetNonBlocking(true);
			fd_set set;
			timeval tvValue;
			timeval tvWork;
			tvValue.tv_sec = recvTimeoutMilliSeconds / 1000;
			tvValue.tv_usec = (recvTimeoutMilliSeconds%1000)*1000;

			int i=1;
			while(m_bContinue)
			{
				if(bNonBlocking)
				{
					tvWork = tvValue;
					FD_ZERO(&set);
					FD_SET(m_socket, &set);
#if defined(WIN32) || defined(WIN64)
					int nfds = select(0, &set, NULL, NULL, &tvWork);
#else
					int nfds = select(m_socket+1, &set, NULL, NULL, &tvWork);
#endif
					if(nfds == 0){
						throw HttpException(err_read_timeout, "Receive HTTP Content timed out: errno=%d", errno);
					}else if(nfds < 0){
						throw HttpException(err_receive_content_failed, "Receive HTTP Content failed A: errno=%d", errno);
					}else{
						if(!FD_ISSET(m_socket, &set)){
							throw HttpException(err_receive_content_failed, "Receive HTTP Content failed B: errno=%d", errno);
						}
					}
				}

				int recvLen = recv(m_socket, recvBuffer, recvBufferSize, 0);
				if(recvLen == SOCKET_ERROR)
					throw HttpException(err_receive_content_failed, "Receive HTTP Content failed C: errno=%d", errno);
				if(recvLen == 0)
					break;

				//header has completed
				if(m_bHdrCompleted && m_bContinue){
					OnRawContent(recvBuffer, recvLen);
					continue;
				}

				//append to header buffer before header completed
				char* tmpBuf = (char*)realloc(headerBuf, headerBufSize+recvLen);
				if(tmpBuf == NULL)
					throw HttpException(err_no_enough_memory, "Out of memory to receive HTTP Content (%d bytes)", headerBufSize+recvLen);
				headerBuf = tmpBuf;
				memcpy(headerBuf+headerBufSize, recvBuffer, recvLen);
				headerBufSize += recvLen;

				//check whether the header is completed
				int pos = memstr(headerBuf, headerBufSize, "\r\n\r\n", 0);

				//not completed yet
				if(pos < 0)
					continue;

				//yeah, its completed. maybe: "<tail_of_header><br><br><begin_of_content>"
				m_bHdrCompleted = true;
				headerBuf[pos] = '\0';
				m_respHeader.SetRawHeader(headerBuf);

				OnRawHeaderCompleted();

				pos += 4;
				if(m_bContinue)
					OnRawContent(headerBuf+pos, headerBufSize-pos);
			}
		}
		catch(HttpException& he)
		{
			__ReleaseLocalBuffers;
			CloseSocket();
			throw he;
		}
		__ReleaseLocalBuffers;
	}
	void HttpClient::Cancel()
	{
		m_bContinue = false;
		CloseSocket();
	}
	IHttpResponseHeader* HttpClient::GetResponseHeader()
	{
		return &m_respHeader;
	}

	void HttpClient::CloseSocket()
	{
		if(m_socket != INVALID_SOCKET){
#if defined(WIN32) || defined(WIN64)
			closesocket(m_socket);
#else
			shutdown(m_socket, SHUT_RDWR);
			close(m_socket);
#endif
			m_socket = INVALID_SOCKET;
		}
	}

	int HttpClient::memstr(const char* pSource, int srcLen, const char* pStr, int nStart)
	{
		int nStrLen = strlen(pStr);

		const char* pTmp = pSource + nStart;	//pTmp不是字符串，它不涉及结束符'\0'
		int leftSize = srcLen - nStart;

		for(; nStrLen<=leftSize; pTmp++, leftSize--) {
			bool bEqual = true;
			for(int nStrIndex=0; nStrIndex<nStrLen; nStrIndex++){
				if (pTmp[nStrIndex] != pStr[nStrIndex]) {
					bEqual = false;
					break;
				}
			}
			if (bEqual) {
				return pTmp - pSource;
			}
		}

		return -1;
	}
}

#define CODE_BLOCK_OF_Form
namespace HTTPLib
{
	Form::Form()
	{
	}
	Form::~Form()
	{
	}
	uint64 Form::GetPostDataSize()
	{
		return m_data.length();
	}
	bool Form::PrepareToSendPostData()
	{
		return true;
	}
	bool Form::GetPartOfPostData(OUT const char** dataPtr, OUT int* dataSizeInBytes)
	{
		*dataPtr = m_data.c_str();
		*dataSizeInBytes = m_data.length();
		return false;
	}
	const char* Form::GetContentType()
	{
		return "application/x-www-form-urlencoded";
	}
	void Form::AddField(const char* name, const char* value)
	{
		if(m_data.length() > 0)
			m_data.append("&");
		m_data.append(name).append("=").append(value);
	}

} //namespace HTTPLib

#define CODE_BLOCK_OF_MultipartForm
namespace HTTPLib
{
	MultipartForm::MultipartForm()
	{
		char sz[64];
		time_t t = time(NULL);
		sprintf(sz, "%ld", t);
		m_boundary = "----HeyHaUploaderBoundary";
		m_boundary.append(sz);

		m_separator = "--" +m_boundary+ "\r\n";
		m_endSeparator = "--" + m_boundary + "--\r\n";

		m_contentType = "multipart/form-data; boundary="+m_boundary;

		m_dataSize = 0;

		m_file = NULL;
		m_buffer = NULL;
		m_bSendingFileContent = false;
	}
	MultipartForm::~MultipartForm()
	{
		if(m_file){
			fclose(m_file);
			m_file = NULL;
		}
		if(m_buffer){
			delete [] m_buffer;
			m_buffer = NULL;
		}
	}

	const char* MultipartForm::GetContentType()
	{
		return m_contentType.c_str();
	}

	void MultipartForm::AddField(const char* name, const char* value)
	{
		string field;
		field.append(m_separator);
		field.append("Content-Disposition: form-data; name=\"").append(name).append("\"\r\n\r\n");
		field.append(value).append("\r\n");
		m_fields.push_back(field);
		m_dataSize += field.length();
	}
	void MultipartForm::AddFile(const char* name, const char* localFileName, const char* remoteFileName)
	{
		FileDesc fd;
		fd.localFileName = localFileName;
		fd.field.append(m_separator);
		fd.field.append("Content-Disposition: form-data; name=\"").append(name).append("\"; filename=\"").append(remoteFileName).append("\"\r\n")
			.append("Content-Type: application/octet-stream\r\n\r\n");
		m_files.push_back(fd);

		struct stat buf;
		memset(&buf, 0, sizeof(buf));
		if(stat(localFileName, &buf))
			throw HTTPLib::HttpException(HTTPLib::err_undefined, "AddFile error: errno=%d", errno);

		m_dataSize += fd.field.length();
		m_dataSize += buf.st_size;
		m_dataSize += 2; //2 = strlen("\r\n");
	}
	void MultipartForm::AddFile(const char* name, const char* fileContent, int fileContentSize, const char* remoteFileName)
	{
		if(fileContent==NULL || fileContentSize<=0)
			return;
		FileDesc fd;
		fd.fileContentPtr = new char[fileContentSize];
		if(fd.fileContentPtr == NULL)
			throw HTTPLib::HttpException(HTTPLib::err_no_enough_memory, "AddFileContent error: Out of memory");
		fd.fileContentSize = fileContentSize;
		fd.field.append(m_separator);
		fd.field.append("Content-Disposition: form-data; name=\"").append(name).append("\"; filename=\"").append(remoteFileName).append("\"\r\n")
			.append("Content-Type: application/octet-stream\r\n\r\n");
		m_files.push_back(fd);

		memcpy(fd.fileContentPtr, fileContent, fileContentSize);

		m_dataSize += fd.field.length();
		m_dataSize += fileContentSize;
		m_dataSize += 2; //2 = strlen("\r\n");
	}

	uint64 MultipartForm::GetPostDataSize()
	{
		return m_dataSize + m_endSeparator.length();
	}
	bool MultipartForm::PrepareToSendPostData()
	{
		if(m_file){
			fclose(m_file);
			m_file = NULL;
		}
		m_itField = m_fields.begin();
		m_itFile = m_files.begin();
		return true;
	}
	bool MultipartForm::GetPartOfPostData(OUT const char** dataPtr, OUT int* dataSizeInBytes)
	{
#define READ_FILE_BUF_SIZE (1024*1024)

		*dataPtr = NULL;
		*dataSizeInBytes = 0;
		
		//normal field
		if(m_itField != m_fields.end())
		{
			*dataPtr = m_itField->c_str();
			*dataSizeInBytes = m_itField->length();
			m_itField++;
			return true;
		}

BeginOfNextFile:
		/* file: binary content */
		if(m_itFile != m_files.end())
		{
			if(m_itFile->fileContentPtr && m_itFile->fileContentSize>0)
			{
				switch(m_itFile->fileContentStep){
				case 0:
					*dataPtr = m_itFile->field.c_str();
					*dataSizeInBytes = m_itFile->field.length();
					break;
				case 1:
					*dataPtr = m_itFile->fileContentPtr;
					*dataSizeInBytes = m_itFile->fileContentSize;
					break;
				case 2:
					*dataPtr = "\r\n";
					*dataSizeInBytes = 2;
					break;
				case 3:
					delete [] m_itFile->fileContentPtr;
					m_itFile++;
					goto BeginOfNextFile;
					break;
				default:
					break;
				}
				m_itFile->fileContentStep++;
				return true;
			}

			/* file: in file storage system */

			//file description
			if(m_file == NULL){
				m_file = fopen(m_itFile->localFileName.c_str(), "rb");
				if(m_file == NULL){
					throw HttpException(err_open_file_failed, "Error to open file of MultipartForm");
				}
				*dataPtr = m_itFile->field.c_str();
				*dataSizeInBytes = m_itFile->field.length();
				m_bSendingFileContent = true;
				return true;
			}

			//allocate buffer for file
			if(m_buffer == NULL){
				m_buffer = new char[READ_FILE_BUF_SIZE];
				if(m_buffer == NULL)
					throw HttpException(err_no_enough_memory, "Out of memory while sending file of MultipartForm");
			}

			//file content
			int n = fread(m_buffer, 1, READ_FILE_BUF_SIZE, m_file);
			if(n <= 0){
				if(m_bSendingFileContent){
					*dataPtr = "\r\n";
					*dataSizeInBytes = 2;
					m_bSendingFileContent = false;
					return true;
				}
				fclose(m_file);
				m_file = NULL;
				m_itFile++;
				goto BeginOfNextFile;
			}

			*dataPtr = m_buffer;
			*dataSizeInBytes = n;
			return true;
		}

		//last separator
		*dataPtr = m_endSeparator.c_str();
		*dataSizeInBytes = m_endSeparator.length();
		return false;
	}
}//namespace HTTPLib
