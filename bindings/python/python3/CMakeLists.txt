find_package(Python3Interp)

if(PYTHON3INTERP_FOUND)
  set(PYTHON3_SITEPKG_DIR "${PYTHON3_SITEPKG_DIR}" CACHE PATH "Python 3 site-package directory.")

  if(NOT PYTHON3_SITEPKG_DIR)
    execute_process(COMMAND
		    ${PYTHON3_EXECUTABLE}
		    -c "import sys; print (sys.version[0:3])"
		    OUTPUT_VARIABLE PYTHON3_VERSION
		    OUTPUT_STRIP_TRAILING_WHITESPACE)

    message(STATUS "Found python 3 version ${PYTHON3_VERSION}")

    execute_process(COMMAND
		    ${PYTHON3_EXECUTABLE}
		    -c "import distutils.sysconfig ; print (distutils.sysconfig.get_python_lib(plat_specific=0))"
		    OUTPUT_VARIABLE PYTHON3_SITEPKG_DIR
		    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  
  message(STATUS "Using PYTHON3_SITEPKG_DIR=${PYTHON3_SITEPKG_DIR}")

  install(FILES ../libproxy.py DESTINATION ${PYTHON3_SITEPKG_DIR})
endif()
