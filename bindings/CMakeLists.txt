option(WITH_PYTHON "Enables Python bindings" ON)
if (WITH_PYTHON)
  find_package(PythonInterp)

  if(PYTHONINTERP_FOUND)
    execute_process(COMMAND
		    ${PYTHON_EXECUTABLE}
		    -c "import distutils.sysconfig ; print distutils.sysconfig.get_python_lib(plat_specific=0)"
		    OUTPUT_VARIABLE pysitepkgdir
		    OUTPUT_STRIP_TRAILING_WHITESPACE)
    install(FILES libproxy.py DESTINATION ${pysitepkgdir})
  endif()
endif()

option(WITH_DOTNET "Enables C# bindings" OFF)
if (WITH_DOTNET)
  add_subdirectory(csharp)
endif()

#perl bindings live in their own subdir.
option(WITH_PERL "Enables Perl bindings" ON)
if (WITH_PERL)
  add_subdirectory(perl)
endif()

# Install Vala bindings / Install only when -DWITH_VALA is passed to cmake.
option(WITH_VALA "Enables Vala bindings" OFF)
if(WITH_VALA)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND AND NOT VAPI_DIR)
    execute_process(COMMAND
		    ${PKG_CONFIG_EXECUTABLE} vala-1.0 --variable=vapidir
		      OUTPUT_VARIABLE VAPI_DIR
		      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  if(VAPI_DIR)
    install(FILES vala/libproxy-1.0.vapi DESTINATION ${VAPI_DIR})
  else()
    message(WARNING "Vala API directory not found, please set VAPI_DIR")
  endif()
  set(VAPI_DIR ${VAPI_DIR} CACHE PATH "Vala API directory")
endif()
