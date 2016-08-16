#ifndef __FACE_PLUS_PLUS_H__
#define __FACE_PLUS_PLUS_H__

#include <string>
#include <list>
#include <vector>
#include "../HttpLib/HttpLib.h"

#ifndef OUT
#define OUT
#endif

#ifndef IN
#define IN
#endif

/**
document: http://www.faceplusplus.com.cn/api-overview/
*/
class FacePP
{
public:
	enum ParamType{
		ParamType_as_id,
		ParamType_as_name,
	};

	struct Point{
		float x, y;
	};
	struct Position{
		float width;
		float height;
		Point center;
		Point eye_left;
		Point eye_right;
		Point mouse_left;
		Point mouse_right;
		Point nose;
	};
	struct Face{
		std::string face_id;
		std::string tag;
	};
	struct FaceDetail : public Face{
		Position position;
	};

	struct DetectedFace : public FaceDetail{
		struct{
			struct{ float range; float value; } age;
			struct{ float confidence; std::string value; } gender;
			struct{ float confidence; std::string value; } glass;
			struct{
				struct{float value;} pitch_angle;
				struct{float value;} roll_angle;
				struct{float value;} yaw_angle;
			}pose;
			struct{ float confidence; std::string value; } race;
			struct{ float value; } smiling;
		}attribute;
	};
	typedef std::list<DetectedFace> DetectedFaceList;
	struct DetectFaceResult{
		float img_width;
		float img_height;
		std::string img_id;
		std::string session_id;
		std::string url;
		DetectedFaceList faces;
	};

	struct ImageInfo{
		std::string img_id;
		std::string url;
		std::vector<FaceDetail> face;
	};

	struct FaceSet {
		std::string faceset_id;
		std::string faceset_name;
		std::string tag;
	};
	struct Person {
		std::string person_id;
		std::string person_name;
		std::string tag;
	};
	struct FaceInfo {
		std::string face_id;
		std::string img_id;
		std::string tag;
		std::string url;
		struct {
			struct{ float range, value; } age;
			struct{ float confidence; std::string value; } gender;
			struct{ float confidence; std::string value; } race;
		} attribute;
		std::vector<FaceSet> faceset;
		std::vector<Person> person;
		Position position;
	};

	typedef std::vector<Person> PersonList;
	typedef std::vector<FaceSet> FaceSetList;

	struct Group {
		std::string group_id;
		std::string group_name;
		std::string tag;
	};
	typedef std::vector<Group> GroupList;

	struct AppInfo {
		std::string name;
		std::string description;
	};

	struct CreateGroupResult{
		int added_person;
		std::string group_id;
		std::string group_name;
		std::string tag;
	};
	struct DeleteResult{
		int deleted;
		bool success;
	};
	struct AddResult{
		int added;
		bool success;
	};
	struct RemoveResult{
		int removed;
		bool success;
	};

	struct GroupDetail : public Group{
		std::vector<Person> person;
	};

	struct CreateFaceSetResult{
		int added_face;
		std::string faceset_id;
		std::string faceset_name;
		std::string tag;
	};

	struct FaceSetDetail : public FaceSet{
		std::vector<Face> face;
	};

	struct AddPersonResult{
		int added_face;
		int added_group;
		Person person;
	};

	struct PersonDetail : public Person{
		std::vector<Face> face;
		std::vector<Group> group;
	};

	struct Session {
		std::string session_id;
	};
	struct ResultSession : public Session{
		long long create_time;
		long long finish_time;
		std::string session_id;
		std::string status;
		std::string resultJsonString;
	};

	struct IdentifyCandidate {
		double confidence;
		std::string person_id;
		std::string person_name;
		std::string tag;
	};
	typedef std::list<IdentifyCandidate> IdentifyCandidateList;

public:
	FacePP(const char* baseUrl, const char* apiKey, const char* apiSecret); //baseUrl likes "apicn.faceplusplus.com"
	virtual ~FacePP();

public:
	bool detection_detect(DetectFaceResult& OUT result, const char* imgFileName);
	bool detection_detect(DetectFaceResult& OUT result, const char* imgData, int imgDataSize);

	bool info_getImage(const char* img_id, ImageInfo& OUT imageInfo);
	bool info_getFace(const char* face_id, FaceInfo& OUT faceInfo);
	bool info_getPersonList(PersonList& OUT personList);
	bool info_getFaceSetList(FaceSetList& OUT faceSetList);
	bool info_getGroupList(GroupList& OUT grpList);
	bool info_getSession(ResultSession& OUT result, const char* session_id);
	bool info_getApp(AppInfo& OUT appInfo);

	bool group_create(CreateGroupResult& OUT result, const char* group_name, const char* tag = NULL);
	bool group_delete(DeleteResult& OUT result, const char* group, ParamType groupParamType);
	bool group_addPerson(AddResult& OUT result, const char* group, ParamType groupParamType, const char* person, ParamType personParamType);
	bool group_removePerson(RemoveResult& OUT result, const char* group, ParamType groupParamType, const char* person, ParamType personParamType);
	bool group_getInfo(GroupDetail& OUT result, const char* group, ParamType groupParamType);

	bool faceset_create(CreateFaceSetResult& OUT result, const char* faceset_name, const char* tag = NULL);
	bool faceset_delete(DeleteResult& OUT result, const char* faceset, ParamType facesetParamType);
	bool faceset_addFace(AddResult& OUT result, const char* faceset, ParamType facesetParamType, const char* face_id);
	bool faceset_removeFace(RemoveResult& OUT result, const char* faceset, ParamType facesetParamType, const char* face_id);
	bool faceset_setInfo(FaceSet& OUT result, const char* faceset, ParamType facesetParamType, const char* newName=NULL, const char* newTag=NULL);
	bool faceset_getInfo(FaceSetDetail& OUT result, const char* faceset, ParamType facesetParamType);

	bool person_create(AddPersonResult& OUT result, const char* person_name=NULL, const char* face_id=NULL, const char* tag=NULL, const char* group=NULL, ParamType groupParamType=ParamType_as_id);
	bool person_delete(DeleteResult& OUT result, const char* person, ParamType personParamType);
	bool person_addFace(AddResult& OUT result, const char* person, ParamType personParamType, const char* face_id);
	bool person_removeFace(RemoveResult& OUT result, const char* person, ParamType personParamType, const char* face_id);
	bool person_setInfo(Person& OUT result, const char* person, ParamType personParamType, const char* newName=NULL, const char* newTag=NULL);
	bool person_getInfo(PersonDetail& OUT result, const char* person, ParamType personParamType);

	bool train_verify(Session& OUT result, const char* person, ParamType personParamType);
	bool train_search(Session& OUT result, const char* faceset, ParamType facesetParamType);
	bool train_identify(Session& OUT result, const char* group, ParamType groupParamType);

	bool recognition_identify(
		const char* IN imgFileName,
		const char* IN group_name,
		IdentifyCandidateList& OUT personList,
		std::string& OUT face_id);
	bool recognition_identify(
		const char* IN pngData,
		int IN pngDataSize,
		const char* IN group_name,
		IdentifyCandidateList& OUT personList,
		std::string& OUT face_id);

private:
	bool doHttpGet(const char* url, HTTPLib::IHttpResponseContentCallback* callback, HTTPLib::ResponseStatusCode* outHttpCode=NULL);
	bool doHttpPost(const char* url, HTTPLib::IPostData* postData, HTTPLib::IHttpResponseContentCallback* callback, HTTPLib::ResponseStatusCode* outHttpCode=NULL);

	bool detection_detect_internal(
		DetectFaceResult& OUT result,
		const char* url,
		HTTPLib::IPostData* postData);

	bool recognition_identify_internal(
		const char* url,
		HTTPLib::IPostData* postData,
		IdentifyCandidateList& OUT personList,
		std::string& OUT face_id);

private:
	std::string m_apiUrl;
	std::string m_apiKey;
	std::string m_apiSecret;
};

#endif //__FACE_PLUS_PLUS_H__
