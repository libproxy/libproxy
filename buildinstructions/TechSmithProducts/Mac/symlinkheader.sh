libNames=(libproxy libmodman)
SRCDIR=`pwd`
mkdir -p "${CONFIGURATION_BUILD_DIR}"
cd "${CONFIGURATION_BUILD_DIR}"
for libName in ${libNames[@]}; do
   INCLUDE_DIR_PATH=$SRCDIR/$libName
   rm -rf $libName
   ln -fs "${INCLUDE_DIR_PATH}" $libName
done
