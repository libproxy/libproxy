find_package(Python2Interp)

if(PYTHON2INTERP_FOUND)
  set(PYTHON2_SITEPKG_DIR "${PYTHON2_SITEPKG_DIR}" CACHE PATH "Python 2 site-package directory.")

  if(NOT PYTHON2_SITEPKG_DIR)
    execute_process(COMMAND
		    ${PYTHON2_EXECUTABLE}
		    -c "import sys; print (sys.version[0:3])"
		    OUTPUT_VARIABLE PYTHON2_VERSION
		    OUTPUT_STRIP_TRAILING_WHITESPACE)

    message(STATUS "Found python 2 version ${PYTHON2_VERSION}")

    execute_process(COMMAND
		    ${PYTHON2_EXECUTABLE}
		    -c "import distutils.sysconfig ; print (distutils.sysconfig.get_python_lib(plat_specific=0))"
		    OUTPUT_VARIABLE PYTHON2_SITEPKG_DIR
		    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  
  message(STATUS "Using PYTHON2_SITEPKG_DIR=${PYTHON2_SITEPKG_DIR}")

  install(FILES ../libproxy.py DESTINATION ${PYTHON2_SITEPKG_DIR})
endif()
