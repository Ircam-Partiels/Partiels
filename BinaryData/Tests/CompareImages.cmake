execute_process(
    COMMAND ${MAGICKEXE} ${MAGICKARGS} -metric RMSE ${IMAGE1} ${IMAGE2} "null:"
    OUTPUT_VARIABLE COMPARE_OUT_VAR
    ERROR_VARIABLE COMPARE_ERR_VAR
    RESULT_VARIABLE COMPARE_RES_VAR
)

string(REGEX MATCH "\\([0-9.eE+-]+\\)" RMSE_MATCH "${COMPARE_ERR_VAR}")
if(NOT RMSE_MATCH)
  message(FATAL_ERROR "❌ Unable to extract RMSE from output: ${COMPARE_ERR_VAR}")
endif()

string(REGEX REPLACE "[()]" "" RMSE "${RMSE_MATCH}")

if(RMSE GREATER THRESHOLD)
  message(FATAL_ERROR "❌ The images are too different: RMSE=${RMSE} > threshold=${THRESHOLD}")
else()
  message(STATUS "✅ The images are similar: RMSE=${RMSE} ≤ threshold=${THRESHOLD}")
endif()
