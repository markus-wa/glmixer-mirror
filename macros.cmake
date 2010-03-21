
MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
 SET(DEFAULT_ARGS)
 FOREACH(arg_name ${arg_names})   
   SET(${prefix}_${arg_name})
 ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})           
    SET(larg_names ${arg_names})   
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})   
      LIST(FIND loption_names "${arg}" is_option)           
      IF (is_option GREATER -1)
       SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
       SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)


## Calls the install_name_tool utility to change the dependent shared
## library or framework install name to the corresponding library or
## framework that was previously installed in the .app bundle using
## GLMIXER_INSTALL_QT4_FRAMEWORK or GLMIXER_INSTALL_DYLIB when the given
## build target is executed.
macro(GLMIXER_INSTALL_NAME_TOOL)
   parse_arguments(INSTALL_NAME_TOOL "TARGET;LIBRARIES;FRAMEWORKS" "" ${ARGN})
   foreach(bin ${INSTALL_NAME_TOOL_DEFAULT_ARGS})
     foreach(it ${INSTALL_NAME_TOOL_FRAMEWORKS})
       add_custom_command(TARGET ${INSTALL_NAME_TOOL_TARGET}
         COMMAND install_name_tool -change
           ${it} @executable_path/../Frameworks/${it} ${bin}
       )
      endforeach(it)
      foreach(it ${INSTALL_NAME_TOOL_LIBRARIES})
        get_filename_component(libname ${it} NAME)
        add_custom_command(TARGET ${INSTALL_NAME_TOOL_TARGET}
          COMMAND install_name_tool -change
          ${it} @executable_path/../lib/${libname} ${bin}
        )
     endforeach(it)
   endforeach(bin)
endmacro(GLMIXER_INSTALL_NAME_TOOL)
	
## Copies the specified Qt4 framework into the .app bundle, updates its
## shared library identification name, and changes any dependent Qt4
## framework or shared library names to reference a framework previously
## installed in the .app bundle using GLMIXER_INSTALL_QT4_FRAMEWORK.
macro(GLMIXER_INSTALL_QT4_FRAMEWORK)
  parse_arguments(INSTALL_QT4_FRAMEWORK
    "NAME;TARGET;LIBRARY;APP_BUNDLE;DEPENDS_FRAMEWORKS;DEPENDS_LIBRARIES" ""
    ${ARGN}
  )
  set(ditto_ARGS "--rsrc")
  foreach (it ${CMAKE_OSX_ARCHITECTURES})
    set(ditto_ARGS ${ditto_ARGS} --arch ${it})
  endforeach(it)
  set(framework "${INSTALL_QT4_FRAMEWORK_NAME}.framework/Versions/4")
  set(outdir "${INSTALL_QT4_FRAMEWORK_APP_BUNDLE}/Contents/Frameworks/${framework}")
  get_filename_component(libname "${INSTALL_QT4_FRAMEWORK_LIBRARY}" NAME)
  add_custom_command(TARGET ${INSTALL_QT4_FRAMEWORK_TARGET}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${outdir}
    COMMAND ditto ${ditto_ARGS}
    ${INSTALL_QT4_FRAMEWORK_LIBRARY} ${outdir}/
    COMMAND install_name_tool -id
      @executable_path/../Frameworks/${framework}/${libname} ${outdir}/${libname}
  )
  GLMIXER_INSTALL_NAME_TOOL(${outdir}/${libname}
    TARGET     ${INSTALL_QT4_FRAMEWORK_TARGET}
    LIBRARIES  ${INSTALL_QT4_FRAMEWORK_DEPENDS_LIBRARIES}
    FRAMEWORKS ${INSTALL_QT4_FRAMEWORK_DEPENDS_FRAMEWORKS}
  )
  set(${INSTALL_QT4_FRAMEWORK_DEFAULT_ARGS} ${framework}/${libname})
endmacro(GLMIXER_INSTALL_QT4_FRAMEWORK)

## Copies the specified .dylib into the .app bundle, updates its shared
## library identification name, and changes any dependent framework or
## shared library names to reference a framework or shared library
## previously installed in the .app bundle.
macro(GLMIXER_INSTALL_DYLIB)
  parse_arguments(INSTALL_DYLIB
    "NAME;TARGET;LIBRARY;APP_BUNDLE;DEPENDS_FRAMEWORKS;DEPENDS_LIBRARIES" ""
    ${ARGN}
  )
  set(outdir "${INSTALL_DYLIB_APP_BUNDLE}/Contents/lib/")
  get_filename_component(libname "${INSTALL_DYLIB_LIBRARY}" NAME)
  add_custom_command(TARGET ${INSTALL_DYLIB_TARGET}
   COMMAND ${CMAKE_COMMAND} -E make_directory ${outdir}
   COMMAND cp ${INSTALL_DYLIB_LIBRARY} ${outdir}/
   COMMAND install_name_tool -id @executable_path/../lib/${libname} ${outdir}/${libname}
  )
  glmixer_install_name_tool(${outdir}/${libname}
    TARGET     ${INSTALL_DYLIB_TARGET}
    LIBRARIES  ${INSTALL_DYLIB_DEPENDS_LIBRARIES}
    FRAMEWORKS ${INSTALL_DYLIB_DEPENDS_FRAMEWORKS}
  )
 set(${INSTALL_DYLIB_DEFAULT_ARGS} "${libname}")
endmacro(GLMIXER_INSTALL_DYLIB)

 
 