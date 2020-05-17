void Cmd_Test_Generate(void);
void Cmd_Test_Run(void);
bool Test_IsGeneratingTest();
bool Test_IsRunningTest();
void Test_Runner_Frame_Hook();
void Test_Changelevel_Hook();
void Test_Script_Completed_Hook();
void ReportFailure(const std::string& error);