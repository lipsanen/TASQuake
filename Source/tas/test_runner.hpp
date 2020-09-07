// desc: Usage: tas_test_generate <filename>. Generates a test from script.
void Cmd_TAS_Test_Generate(void);
// desc: Usage: tas_test_run <filename>. Runs a test from file.
void Cmd_TAS_Test_Run(void);
bool Test_IsGeneratingTest();
bool Test_IsRunningTest();
void Test_Runner_Frame_Hook();
void Test_Changelevel_Hook();
void Test_Script_Completed_Hook();
void ReportFailure(const std::string& error);