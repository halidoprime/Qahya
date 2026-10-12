include("/home/halidoprime/Documents/QControl/QControl/build/Desktop_Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/QControl-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/home/halidoprime/Documents/QControl/QControl/build/Desktop_Debug/QControl"
    GENERATE_QT_CONF
)
