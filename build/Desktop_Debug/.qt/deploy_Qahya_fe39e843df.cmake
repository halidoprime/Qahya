include("/home/halidoprime/Documents/QControl/QControl/build/Desktop_Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Qahya-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/home/halidoprime/Documents/QControl/QControl/build/Desktop_Debug/Qahya"
    GENERATE_QT_CONF
)
