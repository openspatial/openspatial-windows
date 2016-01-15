cmake_minimum_required(VERSION 2.8.8)

include_directories("C:/Program Files (x86)/Nod Labs/Nod Configuration/Dynamic SDK")
add_executable(OpenSpatialExample OpenSpatialExample.cpp)

 if(CMAKE_GENERATOR MATCHES "Win64")
    target_link_libraries(OpenSpatialExample "C:/Program Files (x86)/Nod Labs/Nod Configuration/Dynamic SDK/x86_64/OpenSpatialDLL.lib")
    add_custom_command(TARGET OpenSpatialExample POST_BUILD COMMAND ${CMAKE_COMMAND} -E 
        copy_if_different "C:/Program Files (x86)/Nod Labs/Nod Configuration/Dynamic SDK/x86_64/OpenSpatialDLL.dll"
        $<TARGET_FILE_DIR:OpenSpatialExample>)
 else()
    target_link_libraries(OpenSpatialExample "C:/Program Files (x86)/Nod Labs/Nod Configuration/Dynamic SDK/x86/OpenSpatialDLL.lib")
    add_custom_command(TARGET OpenSpatialExample POST_BUILD COMMAND ${CMAKE_COMMAND} -E 
        copy_if_different "C:/Program Files (x86)/Nod Labs/Nod Configuration/Dynamic SDK/x86/OpenSpatialDLL.dll"
         $<TARGET_FILE_DIR:OpenSpatialExample>)
 endif()
 