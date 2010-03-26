find_package(PythonInterp)

if(PYTHONINTERP_FOUND)
  execute_process(COMMAND
                  ${PYTHON_EXECUTABLE}
                  -c "import distutils.sysconfig ; print distutils.sysconfig.get_python_lib(plat_specific=0)"
                  OUTPUT_VARIABLE pysitepkgdir
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  install(FILES libproxy.py DESTINATION ${pysitepkgdir})
endif()

#TODO c-sharp bindings

#TODO perl bindings

# Install Vala bindings / Install only when -DWITH_VALA is passed to cmake.
if(WITH_VALA)
  install(FILES vala/libproxy-1.0.vapi DESTINATION ${datadir}/vala/vapi)
endif()
