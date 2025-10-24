# CMake generated Testfile for 
# Source directory: /home/runner/work/Partiels/Partiels/VampPlugins
# Build directory: /home/runner/work/Partiels/Partiels/_codeql_build_dir/VampPlugins
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(VampPluginTester "/home/runner/work/Partiels/Partiels/_codeql_build_dir/VampPlugins/vamp-plugin-tester/vamp-plugin-tester" "-a")
set_tests_properties(VampPluginTester PROPERTIES  ENVIRONMENT "VAMP_PATH=/home/runner/work/Partiels/Partiels/_codeql_build_dir/VampPlugins/Release" _BACKTRACE_TRIPLES "/home/runner/work/Partiels/Partiels/VampPlugins/CMakeLists.txt;35;add_test;/home/runner/work/Partiels/Partiels/VampPlugins/CMakeLists.txt;0;")
