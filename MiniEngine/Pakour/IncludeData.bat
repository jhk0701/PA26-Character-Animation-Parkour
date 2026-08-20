if not exist "$(OutDir)\Datas\" (
    mkdir "$(OutDir)\Datas\"
)

XCOPY $(SolutionDir)\Datas\*.* $(OutDir)\Datas\*.* /E /H /Y