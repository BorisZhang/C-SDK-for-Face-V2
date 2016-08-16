#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#if defined(WIN32) || defined(WIN64)
#	include <WinSock2.h>
#	pragma comment(lib, "ws2_32.lib")
#else
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <fcntl.h>
#	include <time.h>
#	include <sys/time.h>
#	include <string.h>
#	include <stddef.h>
#	include <signal.h>
#endif

#include <list>
#include <map>
#include <set>
#include <string>
using namespace std;

#if defined(WIN32) || defined(WIN64)
#else
#	define INVALID_SOCKET	-1
#	define SOCKET_ERROR	-1
typedef int SOCKET;
#endif

typedef unsigned long long int uint64;

#define IN
#define OUT
#define USER_INTERFACE
#define RESERVED_INTERFACE

//////////////////////////////////////////////////////////////////////////
// Overview of HTTPLib
namespace HTTPLib
{
#define HTTPLIB_AGENT "Goddy/1.0"

	enum HttpVersion{
		ver_http11,
	};
	enum HttpMethod{
		method_get,
		method_post,
		method_head,
	};
	enum HttpProtocal{
		proto_http,
		proto_https,
		proto_ftp,
		proto_sftp,
	};
	enum ResponseStatusCode{
		resp_unknown = 0,

		resp_continue = 100,
		resp_switching_protocols = 101,

		resp_ok = 200,
		resp_created = 201,
		resp_accepted = 202,
		resp_non_authoritative_information = 203,
		resp_no_content = 204,
		resp_reset_content = 205,
		resp_partial_content = 206,

		resp_multiple_choices = 300,
		resp_moved_permanently = 301,
		resp_found = 302,
		resp_see_other = 303,
		resp_not_modified = 304,
		resp_use_proxy = 305,
		resp_unused = 306,
		resp_temporary_redirect = 307,

		resp_bad_request = 400,
		resp_unauthorized = 401,
		resp_payment_required = 402,
		resp_forbidden = 403,
		resp_not_found = 404,
		resp_method_not_allowed = 405,
		resp_not_acceptable = 406,
		resp_proxy_authentication_required = 407,
		resp_request_timeout = 408,
		resp_conflict = 409,
		resp_gone = 410,
		resp_length_required = 411,
		resp_precondition_failed = 412,
		resp_request_entity_too_large = 413,
		resp_request_URI_too_long = 414,
		resp_unsupported_media_type = 415,
		resp_request_range_not_satisfiable = 416,
		resp_expectation_failed = 417,

		resp_internal_server_error = 500,
		resp_not_implemented = 501,
		resp_bad_gateway = 502,
		resp_service_unavailable = 503,
		resp_gateway_timeout = 504,
		resp_http_version_not_supported = 505,
	};

	enum ExceptionCode
	{
		err_undefined,
		err_null_parameter,
		err_empty_parameter,
		err_no_enough_memory,
		err_unsupported_http_version,
		err_unsupported_http_method,
		err_invalid_url,
		err_invalid_protocal,
		err_invalid_http_response,
		err_create_socket_failed,
		err_connection_failed,
		err_send_request_failed,
		err_receive_content_failed,
		err_open_file_failed,
		err_connect_timeout,
		err_read_timeout,
	};

	class HttpException
	{
	public:
		HttpException(ExceptionCode ec, const char* format, ...);

		ExceptionCode GetErrorCode();
		const char* GetErrorDescription();
	protected:
		ExceptionCode errorCode;
		string description;
	};

	string Ansi_2_Utf8(const char* asciiString);
	string Utf8_2_Ansi(const char* strUtf8);
	string EncodeUrl(IN const char* url, IN const char* unsafeCharactors=" \t");

	void ParseUrl(
		IN  const char* url,
		OUT HttpProtocal& proto,
		OUT string& server,
		OUT int& port,
		OUT string& object,
		OUT string& userName,
		OUT string& password);

	struct IPostData
	{
		virtual USER_INTERFACE const char* GetContentType() = 0;
		virtual USER_INTERFACE uint64 GetPostDataSize() = 0;
		virtual USER_INTERFACE bool PrepareToSendPostData() = 0;
		virtual USER_INTERFACE bool GetPartOfPostData(OUT const char** dataPtr, OUT int* dataSizeInBytes) = 0;
	};

	/* System will set following headers:  Host, User-Agent, Accept
	   If POST, also:  Content-Length
	   You can call setRequestProperty() to add more HTTP Headers
	*/
	struct IHttpRequest
	{
		virtual USER_INTERFACE void SetAgentName(const char* agentName) = 0;
		virtual USER_INTERFACE void SetURL(const char* url) = 0;
		virtual USER_INTERFACE void SetHttpVersion(HttpVersion version) = 0;
		virtual USER_INTERFACE void SetMethod(HttpMethod method) = 0;
		virtual USER_INTERFACE void SetKeepAlive(bool bKeep) = 0;
		virtual USER_INTERFACE void SetRange(uint64 bytesBegin, uint64 bytesEnd=-1) = 0;
		virtual USER_INTERFACE void setRequestProperty(const char* name, const char* value) = 0;
		virtual USER_INTERFACE void SetPostData(IPostData* data) = 0;
		virtual USER_INTERFACE void Reset() = 0;
		virtual RESERVED_INTERFACE int  GetServerPort() = 0;
		virtual RESERVED_INTERFACE const char* GetServerIp() = 0;
		virtual RESERVED_INTERFACE const char* GetRawRequestHeader() = 0;
		virtual RESERVED_INTERFACE IPostData* GetPostData() = 0;
	};

	struct IHttpResponseHeader
	{
		virtual USER_INTERFACE ResponseStatusCode GetStatus() = 0;
		virtual USER_INTERFACE bool IsHeaderExists(const char* header) = 0;
		virtual USER_INTERFACE const char* GetHeaderValue(const char* header) = 0;
		virtual USER_INTERFACE const char* GetRawHeaderContent() = 0;
		virtual USER_INTERFACE void Reset() = 0;
	};

	struct IHttpResponseContentCallback
	{
		virtual void OnHeaderCompleted() = 0;
		virtual void OnContent(void* content, int len) = 0;
	};

	struct IHttpClient
	{
		virtual USER_INTERFACE void SetResponseContentCallback(IHttpResponseContentCallback* callback) = 0;
		virtual USER_INTERFACE void StartRequest(IHttpRequest* request, int connectTimeoutMilliSeconds=5000) = 0;
		virtual USER_INTERFACE void WaitForResponse(int recvBufferSize=1500-40, int recvTimeoutMilliSeconds=7000) = 0;
		virtual USER_INTERFACE IHttpResponseHeader* GetResponseHeader() = 0;
		virtual USER_INTERFACE void Cancel() = 0;
		virtual USER_INTERFACE void Reset() = 0;
	};
}

//////////////////////////////////////////////////////////////////////////
// Declare of HTTPLib classes
namespace HTTPLib
{
	class HttpRequest;
	class HttpResponseHeader;
	class HttpClient;

	class HttpRequest : public IHttpRequest
	{
		struct HeaderNode{
			static unsigned long genSysOrder;
			static unsigned long genUserOrder;
			unsigned long order;
			string name;
			string value;
		};
		struct HeaderNodeLess{
			bool operator()(const HeaderNode* l, const HeaderNode* r) const{
				return l->order < r->order;
			}
		};
		typedef set<HeaderNode*, HeaderNodeLess> HeaderSet;
		typedef map<string, HeaderNode*> HeaderMap;

	public:
		HttpRequest();
		virtual ~HttpRequest();

	public:
		virtual USER_INTERFACE void SetAgentName(const char* agentName);
		virtual USER_INTERFACE void SetURL(const char* url);
		virtual USER_INTERFACE void SetHttpVersion(HttpVersion version);
		virtual USER_INTERFACE void SetMethod(HttpMethod method);
		virtual USER_INTERFACE void SetKeepAlive(bool bKeep);
		virtual USER_INTERFACE void SetRange(uint64 bytesBegin, uint64 bytesEnd=-1);
		virtual USER_INTERFACE void setRequestProperty(const char* name, const char* value);
		virtual USER_INTERFACE void SetPostData(IPostData* data);
		virtual USER_INTERFACE void Reset();
		virtual RESERVED_INTERFACE int  GetServerPort();
		virtual RESERVED_INTERFACE const char* GetServerIp();
		virtual RESERVED_INTERFACE const char* GetRawRequestHeader();
		virtual RESERVED_INTERFACE IPostData* GetPostData();

	protected:
		void ClearHeaders();
		void AddSysHeader(const char* name, const char* value, unsigned long& genOrder);
		void MakeRawRequestHeader(const char* method);
		void CopyHeadersToSet(HeaderMap& hdrMap, HeaderSet& hdrSet);

	protected:
		string m_agentName;
		string m_version;
		HttpMethod m_method;
		HeaderMap m_mapHeaders;
		IPostData* m_postData;
		string m_rawRequestHeader;

	protected:
		HttpProtocal m_protocal;
		string m_userName;
		string m_password;
		string m_server;
		char   m_strPort[32];
		int    m_nPort;
		string m_object;
	};

	class HttpResponseHeader : public IHttpResponseHeader
	{
		typedef map<string, string> HeaderMap;
	public:
		HttpResponseHeader();
		virtual ~HttpResponseHeader();

	public:
		virtual USER_INTERFACE ResponseStatusCode GetStatus();
		virtual USER_INTERFACE bool IsHeaderExists(const char* header);
		virtual USER_INTERFACE const char* GetHeaderValue(const char* header);
		virtual USER_INTERFACE const char* GetRawHeaderContent();
		virtual USER_INTERFACE void Reset();

	protected:
		void SetRawHeader(const char* rawHdr);
		void ParseHeaders(const char* headers);
		void ParseOneHeader(const char* header);

	protected:
		string m_rawHeader;
		ResponseStatusCode m_respCode;
		HeaderMap m_headers;

	public:
		friend class HttpClient;
	};

	class HttpClient : public IHttpClient
	{
	public:
		HttpClient();
		virtual ~HttpClient();

	public:
		virtual USER_INTERFACE void SetResponseContentCallback(IHttpResponseContentCallback* callback);
		virtual USER_INTERFACE void StartRequest(IHttpRequest* request, int connectTimeoutMilliSeconds=5000);
		virtual USER_INTERFACE void WaitForResponse(int recvBufferSize=1500-40, int recvTimeoutMilliSeconds=7000);
		virtual USER_INTERFACE IHttpResponseHeader* GetResponseHeader();
		virtual USER_INTERFACE void Cancel();
		virtual void Reset();

	protected:
		void CloseSocket();
		bool SetNonBlocking(bool bNonBlocking = true);
		int memstr(const char* pSource, int srcLen, const char* pStr, int nStart);
		int SendBuffer(const char* buffer, int sizeInBytes);
		void OnRawHeaderCompleted();
		void OnRawContent(void* content, int sizeInBytes);

	protected:
		IHttpResponseContentCallback* m_callback;
		HttpResponseHeader m_respHeader;

		bool   m_bContinue;
		bool   m_bHdrCompleted;
		SOCKET m_socket;

		bool m_bChunked;
		char* m_chunkedBuff;
		int m_chunkedBuffSize;
		int m_chunkedBuffUsedSize;
	};

	class DefaultHttpResponseContentCallback : public IHttpResponseContentCallback
	{
	public:
		virtual void OnHeaderCompleted(){
			m_content.clear();
		}
		virtual void OnContent(void* content, int len){
			m_content.append((char*)content, len);
		}
		string m_content;
	};

	class Form : public IPostData
	{
	public:
		Form();
		virtual ~Form();

	public:
		virtual USER_INTERFACE const char* GetContentType();
		virtual USER_INTERFACE uint64 GetPostDataSize();
		virtual USER_INTERFACE bool PrepareToSendPostData();
		virtual USER_INTERFACE bool GetPartOfPostData(OUT const char** dataPtr, OUT int* dataSizeInBytes);

	public:
		void AddField(const char* name, const char* value);

	protected:
		std::string m_data;
	};


	class MultipartForm : public IPostData
	{
	public:
		MultipartForm();
		virtual ~MultipartForm();

	public:
		virtual USER_INTERFACE const char* GetContentType();
		virtual USER_INTERFACE uint64 GetPostDataSize();
		virtual USER_INTERFACE bool PrepareToSendPostData();
		virtual USER_INTERFACE bool GetPartOfPostData(OUT const char** dataPtr, OUT int* dataSizeInBytes);

	public:
		void AddField(const char* name, const char* value);
		void AddFile(const char* name, const char* localFileName, const char* remoteFileName);
		void AddFile(const char* name, const char* fileContent, int fileContentSize, const char* remoteFileName);

	protected:
		struct FileDesc{
			char* fileContentPtr;
			int fileContentSize;
			int fileContentStep;
			string localFileName;
			string field;
			FileDesc(){
				fileContentPtr = NULL;
				fileContentSize = 0;
				fileContentStep = 0;
			}
		};
		typedef list<string> FieldList;
		typedef list<FileDesc> FileList;
		uint64 m_dataSize;
		string m_boundary;
		string m_separator;
		string m_endSeparator;
		string m_contentType;
		FieldList m_fields;
		FileList m_files;

		FILE* m_file;
		FieldList::iterator m_itField;
		FileList::iterator m_itFile;
		char* m_buffer;
		bool m_bSendingFileContent;
	};
}//HTTPLib

#endif //__HTTP_CLIENT_H__
