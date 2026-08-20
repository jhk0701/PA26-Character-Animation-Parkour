if not exist "$(OutDir)\Datas\" ( mkdir "$(OutDir)\Datas\" )
if not exist "$(OutDir)\Assets\" ( mkdir "$(OutDir)\Assets\" )

XCOPY $(SolutionDir)\Datas\*.* $(OutDir)\Datas\*.* /E /H /Y
XCOPY $(SolutionDir)\Assets\*.* $(OutDir)\Assets\*.* /E /H /Y