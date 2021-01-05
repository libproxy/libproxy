LIB_PATH=`pwd`/lib

dylibPaths=`ls $LIB_PATH/*.dylib`

outputDirs=( "${CONFIGURATION_BUILD_DIR}" "${TARGET_BUILD_DIR}" )

for outputDir in ${outputDirs[@]}; do
   cd $outputDir
   echo "outputDir = $outputDir"
   for dylibPath in ${dylibPaths[@]}; do
      dylibName=`basename $dylibPath`
#      echo "ln -fs $dylibPath $dylibName"
      ln -fs "${LIB_PATH}/$dylibName" $dylibName
   done
done
