# Сервер приложений для системы Quantum

TARGET = quantumserver
TEMPLATE = app
QT = core network sql
CONFIG += console

HEADERS += \
            src/startup.h \            
            src/requestmapper.h \
    src/fileactions.h \
    src/databaseopenhelper.h \
    src/controller/baseresttablecontroller.h \
    src/httpsessionmanager.h \    
    src/controller/selectqueryjsoncontroller.h \
    src/controller/abstractsqlcontroller.h \                                 
    src/controller/basejsonsqlcontroller.h \
    src/controller/authorizationcontroller.h \
    src/core/slogger.h \
    src/core/appconst.h \
    src/core/appsettings.h \
    src/core/qdatabaseupdater.h \
    src/core/api.h \
    src/utils/qfileutils.h \
    src/utils/qsqlqueryhelper.h \
    src/utils/strutils.h \
    src/controller/adminonlyresttablecontroller.h \
    src/core/studiesdictionary.h \
    src/controller/studentinfocontoller.h \
    src/controller/studentactioncontroller.h \
    src/controller/usertablecontroller.h \
    src/controller/teacheractioncontroller.h \
    src/controller/teacherinfocontroller.h \
    src/controller/adminactioncontroller.h \
    src/controller/basejsonactioncontroller.h \
    src/controller/cityrestrictedtablecontroller.h \
    src/controller/hierarchytreecontroller.h \
    src/controller/coursetablecontroller.h \
    src/controller/courseadmininfocontroller.h \
    src/controller/schedulehourscontroller.h \
    src/controller/adminschedulecontroller.h \
    src/controller/recreatesubmitresttablecontroller.h \
    src/controller/batchsubmitresttablecontroller.h \
    src/dicts/abstractdict.h \
    src/dicts/timinggriddict.h \
    src/controller/testcontroller.h \
    src/controller/basejsonreportcontroller.h \
    src/core/rtftemplater.h \
    src/models/autosizejsontablemodel.h \
    src/controller/fileattachmentcontroller.h \
    src/core/taskscheduler.h \
    src/task/checklearninggroupstask.h \
    src/core/usernotifier.h \
    src/core/notifications.h \
    src/controller/notificationcontroller.h \
    src/controller/usermessagecontroller.h \
    src/task/tomorrowlessonnotifytask.h \
    src/controller/citytablecontroller.h \
    src/controller/casetablecontroller.h \
    src/task/removeinvalidachievementstask.h \
    src/controller/actionlogtablecontroller.h \
    src/controller/superuseractioncontroller.h \
    src/controller/logfilecontroller.h

SOURCES += \
            src/main.cpp \
            src/startup.cpp \            
            src/requestmapper.cpp \
    src/fileactions.cpp \
    src/databaseopenhelper.cpp \
    src/controller/baseresttablecontroller.cpp \
    src/httpsessionmanager.cpp \    
    src/controller/selectqueryjsoncontroller.cpp \
    src/controller/abstractsqlcontroller.cpp \             
    src/controller/basejsonsqlcontroller.cpp \
    src/controller/authorizationcontroller.cpp \
    src/core/slogger.cpp \
    src/core/appsettings.cpp \
    src/core/qdatabaseupdater.cpp \
    src/utils/qfileutils.cpp \
    src/utils/qsqlqueryhelper.cpp \
    src/utils/strutils.cpp \
    src/controller/adminonlyresttablecontroller.cpp \
    src/core/studiesdictionary.cpp \
    src/controller/studentinfocontoller.cpp \
    src/controller/studentactioncontroller.cpp \
    src/controller/usertablecontroller.cpp \
    src/controller/teacheractioncontroller.cpp \
    src/controller/teacherinfocontroller.cpp \
    src/controller/adminactioncontroller.cpp \
    src/controller/basejsonactioncontroller.cpp \
    src/controller/cityrestrictedtablecontroller.cpp \
    src/controller/hierarchytreecontroller.cpp \
    src/controller/coursetablecontroller.cpp \
    src/controller/courseadmininfocontroller.cpp \
    src/controller/schedulehourscontroller.cpp \
    src/controller/adminschedulecontroller.cpp \
    src/controller/recreatesubmitresttablecontroller.cpp \
    src/controller/batchsubmitresttablecontroller.cpp \
    src/dicts/abstractdict.cpp \
    src/dicts/timinggriddict.cpp \
    src/controller/testcontroller.cpp \
    src/controller/basejsonreportcontroller.cpp \
    src/core/rtftemplater.cpp \
    src/models/autosizejsontablemodel.cpp \
    src/controller/fileattachmentcontroller.cpp \
    src/core/taskscheduler.cpp \
    src/task/checklearninggroupstask.cpp \
    src/core/usernotifier.cpp \
    src/controller/notificationcontroller.cpp \
    src/controller/usermessagecontroller.cpp \
    src/task/tomorrowlessonnotifytask.cpp \
    src/controller/citytablecontroller.cpp \
    src/controller/casetablecontroller.cpp \
    src/task/removeinvalidachievementstask.cpp \
    src/controller/actionlogtablecontroller.cpp \
    src/controller/superuseractioncontroller.cpp \
    src/controller/logfilecontroller.cpp

OTHER_FILES += etc/* logs/* etc/docroot/*

#---------------------------------------------------------------------------------------
# The following lines include the sources of the QtWebAppLib library
#---------------------------------------------------------------------------------------

include(QtWebApp/qtservice/qtservice.pri)
include(QtWebApp/httpserver/httpserver.pri)
include(QtWebApp/logging/logging.pri)
#include(QtWebApp/templateengine/templateengine.pri)

# Not used: include(QtWebApp/templateengine/templateengine.pri)

RESOURCES +=
