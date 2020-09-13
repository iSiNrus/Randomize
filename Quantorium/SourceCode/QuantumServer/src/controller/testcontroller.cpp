#include "testcontroller.h"
#include "../dicts/timinggriddict.h"
#include "../core/studiesdictionary.h"

extern StudyStructureCache* studyStructureCache;

TestController::TestController(Context &con) : BaseJsonSqlController(con)
{
}

void TestController::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;
  qlonglong cityId = paramLong("city_id");
//  qlonglong courseId = paramLong("course_id");
  qlonglong groupId = paramLong("skill_group_id");
  qlonglong skillId = paramLong("skill_id");
  StudiesDictionary* dict = studyStructureCache->dictByCity(cityId);
//  Course* course = dict->courseById(courseId);
//  Quantum* quantum = dict->quantumById(4);
  Skill* skill = dict->skillById(skillId);
  if (skill)
    writeResultOk(response, "Ok", skill->toJsonObject(groupId));
  else
    writeError(response, "Не найдено", 404);
}
