find_package(PythonInterp)

if(PYTHONINTERP_FOUND)
  execute_process(COMMAND
		  ${PYTHON_EXECUTABLE}
		  -c "import sys; print sys.version[0:3]"
		  OUTPUT_VARIABLE PYTHON_VERSION
		  OUTPUT_STRIP_TRAILING_WHITESPACE)

  message("-- Found python version ${PYTHON_VERSION}")

  if (IS_ABSOLUTE "${LIB_INSTALL_DIR}")
    set(libdir "${LIB_INSTALL_DIR}")
  else()
    set(libdir "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")
  endif()

  if (IS_DIRECTORY "${libdir}/python${PYTHON_VERSION}/site-packages")
    set(PYTHON_SITEPKG_DIR "${libdir}/python${PYTHON_VERSION}/site-packages")
  endif()

  if(NOT PYTHON_SITEPKG_DIR)
    execute_process(COMMAND
                    ${PYTHON_EXECUTABLE}
                    -c "import distutils.sysconfig ; print distutils.sysconfig.get_python_lib(plat_specific=0)"
                    OUTPUT_VARIABLE PYTHON_SITEPKG_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  
  message("-- Using PYTHON_SITEPKG_DIR=${PYTHON_SITEPKG_DIR}")
  set(PYTHON_SITEPKG_DIR "${PYTHON_SITEPKG_DIR}" CACHE PATH "Python site-package directory.")

  install(FILES libproxy.py DESTINATION ${PYTHON_SITEPKG_DIR})
endif()
