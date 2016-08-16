#include "FacePP.h"

#if defined(WIN32) || defined(WIN64)
void LOGE(const char* fmt, ...){
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
};
#else
#	include <android/log.h>
#	define LOGE(x...) __android_log_print(ANDROID_LOG_ERROR, "JNI", x)
#endif
#include "../../../third/json/json.h"


#define RECV_BUF_SIZE 512

inline bool isEmptyString(const char* str){
	if(str == NULL) return true;
	return str[0] == '\0';
}
FacePP::FacePP(const char* baseUrl, const char* apiKey, const char* apiSecret)
{
	if(isEmptyString(baseUrl)) baseUrl = "apicn.faceplusplus.com";
	if(isEmptyString(apiKey)) apiKey = "2ad338f1a01d7191c49c1b58b88cadac";
	if(isEmptyString(apiSecret)) apiSecret = "ZInPsqf81vB8ud2flNxvNqC35-FXid3D";

	m_apiUrl.append("http://").append(baseUrl).append("/v2");
	m_apiKey = apiKey;
	m_apiSecret = apiSecret;
}

FacePP::~FacePP()
{
}

bool FacePP::doHttpGet(const char* url, HTTPLib::IHttpResponseContentCallback* callback, HTTPLib::ResponseStatusCode* outHttpCode/*=NULL*/)
{
	try
	{
		HTTPLib::HttpRequest httpReq;
		httpReq.SetAgentName("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36");
		httpReq.SetHttpVersion(HTTPLib::ver_http11);
		httpReq.SetURL(url);
		httpReq.SetMethod(HTTPLib::method_get);
		httpReq.setRequestProperty("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
		httpReq.SetKeepAlive(false);

		HTTPLib::HttpClient httpClient;
		httpClient.SetResponseContentCallback(callback);
		httpClient.StartRequest(&httpReq);
		httpClient.WaitForResponse(RECV_BUF_SIZE);
		HTTPLib::ResponseStatusCode code = httpClient.GetResponseHeader()->GetStatus();
		if(outHttpCode) *outHttpCode = code;
		if(code == HTTPLib::resp_ok)
			return true;
		LOGE("HTTP_ERROR: %d", httpClient.GetResponseHeader()->GetStatus());
	}catch(HTTPLib::HttpException he){
		LOGE("HTTP_ERROR: %s", he.GetErrorDescription());
	}
	return false;
}

bool FacePP::doHttpPost(const char* url, HTTPLib::IPostData* postData, HTTPLib::IHttpResponseContentCallback* callback, HTTPLib::ResponseStatusCode* outHttpCode/*=NULL*/)
{
	try
	{
		HTTPLib::HttpRequest httpReq;
		httpReq.SetAgentName("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36");
		httpReq.SetHttpVersion(HTTPLib::ver_http11);
		httpReq.SetURL(url);
		httpReq.SetMethod(HTTPLib::method_post);
		httpReq.SetPostData(postData);
		httpReq.setRequestProperty("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
		httpReq.SetKeepAlive(false);

		HTTPLib::HttpClient httpClient;
		httpClient.SetResponseContentCallback(callback);
		httpClient.StartRequest(&httpReq);
		httpClient.WaitForResponse(RECV_BUF_SIZE);
		HTTPLib::ResponseStatusCode code = httpClient.GetResponseHeader()->GetStatus();
		if(outHttpCode) *outHttpCode = code;
		if(code == HTTPLib::resp_ok)
			return true;
		LOGE("HTTP_ERROR: %d", httpClient.GetResponseHeader()->GetStatus());
	}catch(HTTPLib::HttpException he){
		LOGE("HTTP_ERROR: %s", he.GetErrorDescription());
	}
	return false;
}

#define _hh_getFacePoint(_jposition, _position, _point) \
	jpoint = _jposition.get(#_point, Json::Value::null);\
	if(jpoint.isObject()){\
		_position._point.x = (float)jpoint.get("x", Json::Value(0.0f)).asDouble();\
		_position._point.y = (float)jpoint.get("y", Json::Value(0.0f)).asDouble();\
	}

bool FacePP::detection_detect(DetectFaceResult& OUT result, const char* imgFileName)
{
	const char* remoteFile = strrchr(imgFileName, '/');
	if(remoteFile) remoteFile++;
	else remoteFile = imgFileName;

	std::string url = m_apiUrl + "/detection/detect";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::MultipartForm postData;
	try{
		postData.AddFile("img", imgFileName, remoteFile);
	}catch(HTTPLib::HttpException he){
		LOGE("HTTP_ERROR: %s: %s", he.GetErrorDescription(), imgFileName);
		return false;
	}

	return detection_detect_internal(result, url.c_str(), &postData);
}
bool FacePP::detection_detect(DetectFaceResult& OUT result, const char* imgData, int imgDataSize)
{
	const char* remoteFile = "face.png";

	std::string url = m_apiUrl + "/detection/detect";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::MultipartForm postData;
	try{
		postData.AddFile("img", imgData, imgDataSize, remoteFile);
	}catch(HTTPLib::HttpException he){
		LOGE("HTTP_ERROR: %s", he.GetErrorDescription());
		return false;
	}

	return detection_detect_internal(result, url.c_str(), &postData);
}

bool FacePP::info_getImage(const char* img_id, ImageInfo& OUT imageInfo)
{
	std::string url = m_apiUrl + "/info/get_image";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	url.append("&img_id=").append(img_id);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				imageInfo.img_id = jval.get("img_id", Json::Value("")).asString();
				imageInfo.url = jval.get("url", Json::Value("")).asString();
				Json::Value jarr = jval.get("face", Json::Value::null);
				if(jarr.isArray()) {
					Json::UInt n = jarr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jface = jarr.get(i, Json::Value::null);
						if(jface.isObject()){
							FaceDetail face;
							face.face_id = jface.get("face_id", Json::Value("")).asString();
							face.tag = jface.get("tag", Json::Value("")).asString();
							Json::Value jposition = jface.get("position", Json::Value::null);
							if(jposition.isObject()){
								face.position.width = (float)jposition.get("width", Json::Value(0.0f)).asDouble();
								face.position.height = (float)jposition.get("height", Json::Value(0.0f)).asDouble();
								Json::Value jpoint;
								_hh_getFacePoint(jposition, face.position, center);
								_hh_getFacePoint(jposition, face.position, eye_left);
								_hh_getFacePoint(jposition, face.position, eye_right);
								_hh_getFacePoint(jposition, face.position, mouse_left);
								_hh_getFacePoint(jposition, face.position, mouse_right);
								_hh_getFacePoint(jposition, face.position, nose);
							}
							imageInfo.face.push_back(face);
						} //if(jgrp.isObject())
					} //for each group
					return true;
				} //if(jarr.isArray())
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::info_getFace(const char* face_id, FaceInfo& OUT faceInfo)
{
	std::string url = m_apiUrl + "/info/get_face";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	url.append("&face_id=").append(face_id);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jvalRoot;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jvalRoot, false))
		{
			if(jvalRoot.isObject()) {
				Json::Value jfaceinfoArr = jvalRoot.get("face_info", Json::Value::null);
				if(jfaceinfoArr.isArray()){
					Json::UInt idx = 0;
					Json::Value jfaceinfo = jfaceinfoArr.get(idx, Json::Value::null);
					if(jfaceinfo.isObject()){
						faceInfo.face_id = jfaceinfo.get("face_id", Json::Value("")).asString();
						faceInfo.img_id = jfaceinfo.get("img_id", Json::Value("")).asString();
						faceInfo.tag = jfaceinfo.get("tag", Json::Value("")).asString();
						faceInfo.url = jfaceinfo.get("url", Json::Value("")).asString();

						Json::Value jattribute = jfaceinfo.get("attribute", Json::Value::null);
						if(jattribute.isObject()){
							Json::Value jage = jattribute.get("age", Json::Value::null);
							if(jage.isObject()){
								faceInfo.attribute.age.range = (float)jage.get("range", Json::Value(0.0f)).asDouble();
								faceInfo.attribute.age.value = (float)jage.get("value", Json::Value(0.0f)).asDouble();
							}
							Json::Value jgender = jattribute.get("gender", Json::Value::null);
							if(jgender.isObject()){
								faceInfo.attribute.gender.confidence = (float)jgender.get("confidence", Json::Value(0.0f)).asDouble();
								faceInfo.attribute.gender.value = jgender.get("value", Json::Value("")).asString();
							}
							Json::Value jrace = jattribute.get("race", Json::Value::null);
							if(jrace.isObject()){
								faceInfo.attribute.race.confidence = (float)jrace.get("confidence", Json::Value(0.0f)).asDouble();
								faceInfo.attribute.race.value = jrace.get("value", Json::Value("")).asString();
							}
						}

						Json::Value jfacesetArr = jfaceinfo.get("faceset", Json::Value::null);
						if(jfacesetArr.isArray()){
							Json::UInt n = jfacesetArr.size();
							for(Json::UInt i=0; i<n; i++){
								Json::Value jfaceset = jfacesetArr.get(i, Json::Value::null);
								if(jfaceset.isObject()){
									FaceSet faceset;
									faceset.faceset_id = jfaceset.get("faceset_id", Json::Value("")).asString();
									faceset.faceset_name = jfaceset.get("faceset_name", Json::Value("")).asString();
									faceset.tag = jfaceset.get("tag", Json::Value("")).asString();
									faceInfo.faceset.push_back(faceset);
								}
							}
						}

						Json::Value jpersonArr = jfaceinfo.get("person", Json::Value::null);
						if(jpersonArr.isArray()){
							Json::UInt n = jpersonArr.size();
							for(Json::UInt i=0; i<n; i++){
								Json::Value jperson = jpersonArr.get(i, Json::Value::null);
								if(jperson.isObject()){
									Person person;
									person.person_id = jperson.get("person_id", Json::Value("")).asString();
									person.person_name = jperson.get("person_name", Json::Value("")).asString();
									person.tag = jperson.get("tag", Json::Value("")).asString();
									faceInfo.person.push_back(person);
								}
							}
						}

						Json::Value jposition = jfaceinfo.get("position", Json::Value::null);
						if(jposition.isObject()){
							faceInfo.position.width = (float)jposition.get("width", Json::Value(0.0f)).asDouble();
							faceInfo.position.height = (float)jposition.get("height", Json::Value(0.0f)).asDouble();
							Json::Value jpoint;
							_hh_getFacePoint(jposition, faceInfo.position, center);
							_hh_getFacePoint(jposition, faceInfo.position, eye_left);
							_hh_getFacePoint(jposition, faceInfo.position, eye_right);
							_hh_getFacePoint(jposition, faceInfo.position, mouse_left);
							_hh_getFacePoint(jposition, faceInfo.position, mouse_right);
							_hh_getFacePoint(jposition, faceInfo.position, nose);
						}
						return true;
					}//if(jfaceinfo.isObject())
				} //if(jfaceinfoArr.isArray())
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::info_getPersonList(PersonList& OUT personList)
{
	std::string url = m_apiUrl + "/info/get_person_list";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				Json::Value jarr = jval.get("person", Json::Value::null);
				if(jarr.isArray()) {
					Json::UInt n = jarr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jperson = jarr.get(i, Json::Value::null);
						if(jperson.isObject()){
							Person person;
							person.person_id = jperson.get("person_id", Json::Value("")).asString();
							person.person_name = jperson.get("person_name", Json::Value("")).asString();
							person.tag = jperson.get("tag", Json::Value("")).asString();
							personList.push_back(person);
						}
					} //for each person
					return true;
				} //is array
			} //is object
		} //parse JSON
	} //
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::info_getFaceSetList(FaceSetList& OUT faceSetList)
{
	std::string url = m_apiUrl + "/info/get_faceset_list";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				Json::Value jarr = jval.get("faceset", Json::Value::null);
				if(jarr.isArray()) {
					Json::UInt n = jarr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jfaceset = jarr.get(i, Json::Value::null);
						if(jfaceset.isObject()){
							FaceSet faceset;
							faceset.faceset_id = jfaceset.get("faceset_id", Json::Value("")).asString();
							faceset.faceset_name = jfaceset.get("faceset_name", Json::Value("")).asString();
							faceset.tag = jfaceset.get("tag", Json::Value("")).asString();
							faceSetList.push_back(faceset);
						}
					} //for each person
					return true;
				} //is array
			} //is object
		} //parse JSON
	} //
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::info_getGroupList(GroupList& OUT grpList)
{
	std::string url = m_apiUrl + "/info/get_group_list";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				Json::Value jarr = jval.get("group", Json::Value::null);
				if(jarr.isArray()) {
					Json::UInt n = jarr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jgrp = jarr.get(i, Json::Value::null);
						if(jgrp.isObject()){
							Group grp;
							grp.group_id = jgrp.get("group_id", Json::Value("")).asCString();
							grp.group_name = jgrp.get("group_name", Json::Value("")).asCString();
							grp.tag = jgrp.get("tag", Json::Value("")).asCString();
							grpList.push_back(grp);
						} //if(jgrp.isObject())
					} //for each group
					return true;
				} //if(jarr.isArray())
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::info_getSession(ResultSession& OUT result, const char* session_id)
{
	std::string url = m_apiUrl + "/info/get_session";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.create_time = (long long)jval.get("create_time", Json::Value(0.0f)).asDouble();
				result.finish_time = (long long)jval.get("finish_time", Json::Value(0.0f)).asDouble();
				result.session_id = jval.get("session_id", Json::Value("")).asString();
				result.status = jval.get("status", Json::Value("")).asString();
				Json::Value jresult = jval.get("result", Json::Value::null);
				if(jresult.isObject()){
					Json::FastWriter writer;
					result.resultJsonString = writer.write(jresult);
				}
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::info_getApp(AppInfo& OUT appInfo)
{
	std::string url = m_apiUrl + "/info/get_app";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				appInfo.name = jval.get("name", Json::Value("")).asCString();
				appInfo.description = jval.get("description", Json::Value("")).asCString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::group_create(CreateGroupResult& OUT result, const char* group_name, const char* tag/* = NULL*/)
{
	std::string url = m_apiUrl + "/group/create";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	url.append("&group_name=").append(group_name);
	if(tag)
		url.append("&tag=").append(tag);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	HTTPLib::ResponseStatusCode httpCode = HTTPLib::resp_unknown;
	if(doHttpGet(url.c_str(), &callback, &httpCode))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.added_person = jval.get("added_person", Json::Value(0)).asInt();
				result.group_id = jval.get("group_id", Json::Value("")).asCString();
				result.group_name = jval.get("group_name", Json::Value("")).asCString();
				result.tag = jval.get("tag", Json::Value("")).asCString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	else if(httpCode == 453) return true; //NAME_EXIST
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::group_delete(DeleteResult& OUT result, const char* group, ParamType groupParamType)
{
	std::string url = m_apiUrl + "/group/delete";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(groupParamType){
	case ParamType_as_id:
		url.append("&group_id=").append(group);
		break;
	case ParamType_as_name:
		url.append("&group_name=").append(group);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.deleted = jval.get("deleted", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::group_addPerson(AddResult& OUT result,
	const char* group, ParamType groupParamType,
	const char* person, ParamType personParamType)
{
	std::string url = m_apiUrl + "/group/add_person";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(groupParamType){
	case ParamType_as_id:
		url.append("&group_id=").append(group);
		break;
	case ParamType_as_name:
		url.append("&group_name=").append(group);
		break;
	}
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.added = jval.get("added", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::group_removePerson(RemoveResult& OUT result,
	const char* group, ParamType groupParamType,
	const char* person, ParamType personParamType)
{
	std::string url = m_apiUrl + "/group/remove_person";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(groupParamType){
	case ParamType_as_id:
		url.append("&group_id=").append(group);
		break;
	case ParamType_as_name:
		url.append("&group_name=").append(group);
		break;
	}
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.removed = jval.get("removed", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::group_getInfo(GroupDetail& OUT result, const char* group, ParamType groupParamType)
{
	std::string url = m_apiUrl + "/group/get_info";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(groupParamType){
	case ParamType_as_id:
		url.append("&group_id=").append(group);
		break;
	case ParamType_as_name:
		url.append("&group_name=").append(group);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.group_id = jval.get("group_id", Json::Value("")).asString();
				result.group_name = jval.get("group_name", Json::Value("")).asString();
				result.tag = jval.get("tag", Json::Value("")).asString();
				Json::Value jpersonArr = jval.get("person", Json::Value::null);
				if(jpersonArr.isArray()){
					Json::UInt n = jpersonArr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jperson = jpersonArr.get(i, Json::Value::null);
						if(jperson.isObject()){
							Person person;
							person.person_id = jperson.get("person_id", Json::Value("")).asString();
							person.person_name = jperson.get("person_name", Json::Value("")).asString();
							person.tag = jperson.get("tag", Json::Value("")).asString();
							result.person.push_back(person);
						}
					}
				}
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::faceset_create(CreateFaceSetResult& OUT result, const char* faceset_name, const char* tag/* = NULL*/)
{
	std::string url = m_apiUrl + "/faceset/create";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	url.append("&faceset_name=").append(faceset_name);
	if(tag)
		url.append("&tag=").append(tag);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.added_face = jval.get("added_face", Json::Value(0)).asInt();
				result.faceset_id = jval.get("faceset_id", Json::Value("")).asString();
				result.faceset_name = jval.get("faceset_name", Json::Value("")).asString();
				result.tag = jval.get("tag", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::faceset_delete(DeleteResult& OUT result, const char* faceset, ParamType facesetParamType)
{
	std::string url = m_apiUrl + "/faceset/delete";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(facesetParamType){
	case ParamType_as_id:
		url.append("&faceset_id=").append(faceset);
		break;
	case ParamType_as_name:
		url.append("&faceset_name=").append(faceset);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.deleted = jval.get("deleted", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::faceset_addFace(AddResult& OUT result, const char* faceset, ParamType facesetParamType, const char* face_id)
{
	std::string url = m_apiUrl + "/faceset/add_face";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(facesetParamType){
	case ParamType_as_id:
		url.append("&faceset_id=").append(faceset);
		break;
	case ParamType_as_name:
		url.append("&faceset_name=").append(faceset);
		break;
	}
	url.append("&face_id=").append(face_id);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.added = jval.get("added", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::faceset_removeFace(RemoveResult& OUT result, const char* faceset, ParamType facesetParamType, const char* face_id)
{
	std::string url = m_apiUrl + "/faceset/remove_face";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(facesetParamType){
	case ParamType_as_id:
		url.append("&faceset_id=").append(faceset);
		break;
	case ParamType_as_name:
		url.append("&faceset_name=").append(faceset);
		break;
	}
	url.append("&face_id=").append(face_id);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.removed = jval.get("removed", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::faceset_setInfo(FaceSet& OUT result,
	const char* faceset, ParamType facesetParamType,
	const char* newName/*=NULL*/, const char* newTag/*=NULL*/)
{
	std::string url = m_apiUrl + "/faceset/set_info";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(facesetParamType){
	case ParamType_as_id:
		url.append("&faceset_id=").append(faceset);
		break;
	case ParamType_as_name:
		url.append("&faceset_name=").append(faceset);
		break;
	}
	if(newName) url.append("&name=").append(newName);
	if(newTag) url.append("&tag=").append(newTag);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.faceset_id = jval.get("faceset_id", Json::Value("")).asString();
				result.faceset_name = jval.get("faceset_name", Json::Value("")).asString();
				result.tag = jval.get("tag", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::faceset_getInfo(FaceSetDetail& OUT result, const char* faceset, ParamType facesetParamType)
{
	std::string url = m_apiUrl + "/faceset/get_info";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(facesetParamType){
	case ParamType_as_id:
		url.append("&faceset_id=").append(faceset);
		break;
	case ParamType_as_name:
		url.append("&faceset_name=").append(faceset);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.faceset_id = jval.get("faceset_id", Json::Value("")).asString();
				result.faceset_name = jval.get("faceset_name", Json::Value("")).asString();
				result.tag = jval.get("tag", Json::Value("")).asString();
				Json::Value jfaceArr = jval.get("face", Json::Value::null);
				Json::UInt n = jfaceArr.size();
				for(Json::UInt i=0; i<n; i++){
					Json::Value jface = jfaceArr.get(i, Json::Value::null);
					if(jface.isObject()){
						Face face;
						face.face_id = jface.get("face_id", Json::Value("")).asString();
						face.tag = jface.get("tag", Json::Value("")).asString();
						result.face.push_back(face);
					}
				}
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::person_create(AddPersonResult& OUT result,
	const char* person_name/*=NULL*/, const char* tag/*=NULL*/,
	const char* face_id/*=NULL*/,
	const char* group/*=NULL*/, ParamType groupParamType/*=ParamType_as_id*/)
{
	std::string url = m_apiUrl + "/person/create";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	if(person_name) url.append("&person_name=").append(person_name);
	if(tag) url.append("&tag=").append(tag);
	if(face_id) url.append("&face_id=").append(face_id);
	if(group){
		switch(groupParamType){
		case ParamType_as_id:
			url.append("&group_id=").append(group);
			break;
		case ParamType_as_name:
			url.append("&group_name=").append(group);
			break;
		}
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	HTTPLib::ResponseStatusCode httpCode = HTTPLib::resp_unknown;
	if(doHttpGet(url.c_str(), &callback, &httpCode))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.added_face = jval.get("added_face", Json::Value(0)).asInt();
				result.added_group = jval.get("added_group", Json::Value(0)).asInt();
				result.person.person_id = jval.get("person_id", Json::Value("")).asString();
				result.person.person_name = jval.get("person_name", Json::Value("")).asString();
				result.person.tag = jval.get("tag", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	else if(httpCode == 453) return true; //NAME_EXIST
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::person_delete(DeleteResult& OUT result, const char* person, ParamType personParamType)
{
	std::string url = m_apiUrl + "/person/delete";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.deleted = jval.get("deleted", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::person_addFace(AddResult& OUT result, const char* person, ParamType personParamType, const char* face_id)
{
	std::string url = m_apiUrl + "/person/add_face";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}
	url.append("&face_id=").append(face_id);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.added = jval.get("added", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::person_removeFace(RemoveResult& OUT result, const char* person, ParamType personParamType, const char* face_id)
{
	std::string url = m_apiUrl + "/person/remove_face";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}
	url.append("&face_id=").append(face_id);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.removed = jval.get("removed", Json::Value(0)).asInt();
				result.success = jval.get("success", Json::Value(false)).asBool();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::person_setInfo(Person& OUT result, const char* person, ParamType personParamType,
	const char* newName/*=NULL*/, const char* newTag/*=NULL*/)
{
	std::string url = m_apiUrl + "/person/set_info";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}
	if(newName) url.append("&name=").append(newName);
	if(newTag) url.append("&tag=").append(newTag);

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.person_id = jval.get("person_id", Json::Value("")).asString();
				result.person_name = jval.get("person_name", Json::Value("")).asString();
				result.tag = jval.get("tag", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::person_getInfo(PersonDetail& OUT result, const char* person, ParamType personParamType)
{
	std::string url = m_apiUrl + "/person/get_info";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.person_id = jval.get("person_id", Json::Value("")).asString();
				result.person_name = jval.get("person_name", Json::Value("")).asString();
				result.tag = jval.get("tag", Json::Value("")).asString();

				Json::Value jfaceArr = jval.get("face", Json::Value::null);
				if(jfaceArr.isArray()){
					Json::UInt n = jfaceArr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jface = jfaceArr.get(i, Json::Value::null);
						if(jface.isObject()){
							Face face;
							face.face_id = jface.get("face_id", Json::Value("")).asString();
							face.tag = jface.get("tag", Json::Value("")).asString();
							result.face.push_back(face);
						}
					}
				}

				Json::Value jgroupArr = jval.get("group", Json::Value::null);
				if(jgroupArr.isArray()){
					Json::UInt n = jgroupArr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jgroup = jgroupArr.get(i, Json::Value::null);
						if(jgroup.isObject()){
							Group group;
							group.group_id = jgroup.get("group_id", Json::Value("")).asString();
							group.group_name = jgroup.get("group_name", Json::Value("")).asString();
							group.tag = jgroup.get("tag", Json::Value("")).asString();
							result.group.push_back(group);
						}
					}
				}
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::train_verify(Session& OUT result, const char* person, ParamType personParamType)
{
	std::string url = m_apiUrl + "/train/verify";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(personParamType){
	case ParamType_as_id:
		url.append("&person_id=").append(person);
		break;
	case ParamType_as_name:
		url.append("&person_name=").append(person);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.session_id = jval.get("session_id", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::train_search(Session& OUT result, const char* faceset, ParamType facesetParamType)
{
	std::string url = m_apiUrl + "/train/search";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(facesetParamType){
	case ParamType_as_id:
		url.append("&faceset_id=").append(faceset);
		break;
	case ParamType_as_name:
		url.append("&faceset_name=").append(faceset);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.session_id = jval.get("session_id", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::train_identify(Session& OUT result, const char* group, ParamType groupParamType)
{
	std::string url = m_apiUrl + "/train/identify";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	switch(groupParamType){
	case ParamType_as_id:
		url.append("&group_id=").append(group);
		break;
	case ParamType_as_name:
		url.append("&group_name=").append(group);
		break;
	}

	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpGet(url.c_str(), &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject()) {
				result.session_id = jval.get("session_id", Json::Value("")).asString();
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpGet
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::recognition_identify(
	const char* IN imgFileName,
	const char* IN group_name,
	IdentifyCandidateList& OUT personList,
	std::string& OUT face_id)
{
	const char* remoteFile = strrchr(imgFileName, '/');
	if(remoteFile) remoteFile++;
	else remoteFile = imgFileName;
	
	std::string url = m_apiUrl + "/recognition/identify";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	url.append("&group_name=").append(group_name);
//	url.append("&mode=").append("oneface");

	HTTPLib::MultipartForm postData;
	try{
		postData.AddFile("img", imgFileName, remoteFile);
	}catch(HTTPLib::HttpException he){
		LOGE("HTTP_ERROR: %s: %s", he.GetErrorDescription(), imgFileName);
		return false;
	}

	return recognition_identify_internal(url.c_str(), &postData, personList, face_id);
}

bool FacePP::recognition_identify(
	const char* IN pngData,
	int IN pngDataSize,
	const char* IN group_name,
	IdentifyCandidateList& OUT personList,
	std::string& OUT face_id)
{
	const char* remoteFile = "face.png";

	std::string url = m_apiUrl + "/recognition/identify";
	url.append("?api_key=").append(m_apiKey);
	url.append("&api_secret=").append(m_apiSecret);
	url.append("&group_name=").append(group_name);
//	url.append("&mode=").append("oneface");

	HTTPLib::MultipartForm postData;
	try{
		postData.AddFile("img", pngData, pngDataSize, remoteFile);
	}catch(HTTPLib::HttpException he){
		LOGE("HTTP_ERROR: %s", he.GetErrorDescription());
		return false;
	}

	return recognition_identify_internal(url.c_str(), &postData, personList, face_id);
}

bool FacePP::recognition_identify_internal(
	const char* url,
	HTTPLib::IPostData* postData,
	IdentifyCandidateList& OUT personList,
	std::string& OUT face_id)
{
	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpPost(url, postData, &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject())
			{
				Json::Value jfaceArr = jval.get("face", Json::Value::null);
				if(jfaceArr.isArray()){
					Json::Value jfirstFace = jfaceArr.get(Json::UInt(0), Json::Value::null);
					if(jfirstFace.isObject()){
						face_id = jfirstFace.get("face_id", Json::Value("")).asString();
						Json::Value jcandidateArr = jfirstFace.get("candidate", Json::Value::null);
						if(jcandidateArr.isArray()){
							Json::UInt n = jcandidateArr.size();
							for(Json::UInt i=0; i<n; i++){
								Json::Value jcandidate = jcandidateArr.get(i, Json::Value::null);
								if(jcandidate.isObject()){
									IdentifyCandidate candidate;
									candidate.confidence = jcandidate.get("confidence", Json::Value(0.0f)).asDouble();
									candidate.person_id= jcandidate.get("person_id", Json::Value("")).asCString();
									candidate.person_name = jcandidate.get("person_name", Json::Value("")).asCString();
									candidate.tag = jcandidate.get("tag", Json::Value("")).asCString();
									personList.push_back(candidate);
								} //if(jcandidate.isObject())
							} //for each candidate
						} //jcandidateArr.isArray()
						return true;
					} //jfirstFace.isObject()
				} //jfaceArr.isArray()
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpPost
	LOGE("%s", callback.m_content.c_str());
	return false;
}

bool FacePP::detection_detect_internal(
	DetectFaceResult& OUT result,
	const char* url,
	HTTPLib::IPostData* postData)
{
	HTTPLib::DefaultHttpResponseContentCallback callback;
	if(doHttpPost(url, postData, &callback))
	{
		Json::Value jval;
		Json::Reader jsonReader;
		if(jsonReader.parse(callback.m_content, jval, false))
		{
			if(jval.isObject())
			{
				result.img_width = (float)jval.get("img_width", Json::Value(0.0f)).asDouble();
				result.img_height = (float)jval.get("img_height", Json::Value(0.0f)).asDouble();
				result.img_id = jval.get("img_id", Json::Value("")).asString();
				result.session_id = jval.get("session_id", Json::Value("")).asString();
				result.url = jval.get("url", Json::Value("")).asString();

				//
				Json::Value jfaceArr = jval.get("face", Json::Value::null);
				if(jfaceArr.isArray()){
					Json::UInt n = jfaceArr.size();
					for(Json::UInt i=0; i<n; i++){
						Json::Value jface = jfaceArr.get(i, Json::Value::null);
						if(jface.isObject()){
							DetectedFace face;
							face.face_id = jface.get("face_id", Json::Value("")).asString();
							face.tag = jface.get("tag", Json::Value("")).asString();
							Json::Value jattribute = jface.get("attribute", Json::Value::null);
							if(jattribute.isObject()){
								Json::Value jage = jattribute.get("age", Json::Value::null);
								if(jage.isObject()){
									face.attribute.age.range = (float)jage.get("range", Json::Value(0.0f)).asDouble();
									face.attribute.age.value = (float)jage.get("value", Json::Value(0.0f)).asDouble();
								}
								Json::Value jgender = jattribute.get("gender", Json::Value::null);
								if(jgender.isObject()){
									face.attribute.gender.confidence = (float)jgender.get("confidence", Json::Value(0.0f)).asDouble();
									face.attribute.gender.value = jgender.get("value", Json::Value("")).asString();
								}
								Json::Value jglass = jattribute.get("glass", Json::Value::null);
								if(jglass.isObject()){
									face.attribute.glass.confidence = (float)jglass.get("confidence", Json::Value(0.0f)).asDouble();
									face.attribute.glass.value = jglass.get("value", Json::Value("")).asString();
								}
								Json::Value jpose = jattribute.get("pose", Json::Value::null);
								if(jpose.isObject()){
									Json::Value jpitch_angle = jpose.get("pitch_angle", Json::Value::null);
									if(jpitch_angle.isObject())
										face.attribute.pose.pitch_angle.value = (float)jpitch_angle.get("value", Json::Value(0.0f)).asDouble();
									Json::Value jroll_angle = jpose.get("roll_angle", Json::Value::null);
									if(jroll_angle.isObject())
										face.attribute.pose.roll_angle.value = (float)jroll_angle.get("value", Json::Value(0.0f)).asDouble();
									Json::Value jyaw_angle = jpose.get("yaw_angle", Json::Value::null);
									if(jyaw_angle.isObject())
										face.attribute.pose.yaw_angle.value = (float)jyaw_angle.get("value", Json::Value(0.0f)).asDouble();
								}
								Json::Value jrace = jattribute.get("race", Json::Value::null);
								if(jrace.isObject()){
									face.attribute.race.confidence = (float)jrace.get("confidence", Json::Value(0.0f)).asDouble();
									face.attribute.race.value = jrace.get("value", Json::Value("")).asString();
								}
								Json::Value jsmiling = jattribute.get("smiling", Json::Value::null);
								if(jsmiling.isObject()){
									face.attribute.smiling.value = (float)jsmiling.get("value", Json::Value(0.0f)).asDouble();
								}
							} //if(jattribute.isObject())
							Json::Value jposition = jface.get("position", Json::Value::null);
							if(jposition.isObject()){
								face.position.width = (float)jposition.get("width", Json::Value(0.0f)).asDouble();
								face.position.height = (float)jposition.get("height", Json::Value(0.0f)).asDouble();
								Json::Value jpoint;
								_hh_getFacePoint(jposition, face.position, center);
								_hh_getFacePoint(jposition, face.position, eye_left);
								_hh_getFacePoint(jposition, face.position, eye_right);
								_hh_getFacePoint(jposition, face.position, mouse_left);
								_hh_getFacePoint(jposition, face.position, mouse_right);
								_hh_getFacePoint(jposition, face.position, nose);
							} //if(jposition.isObject())
							result.faces.push_back(face);
						} //if(jface.isObject())
					} //for each face
				} //jfaceArr.isArray()
				return true;
			} //if(jval.isObject())
		} //jsonReader.parse
	} //doHttpPost
	LOGE("%s", callback.m_content.c_str());
	return false;
}
