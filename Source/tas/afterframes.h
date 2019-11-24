#ifdef __cplusplus
extern "C"
{
#endif // !__cplusplus

void PauseAfterframes();
void UnpauseAfterframes();
void ClearAfterframes();
void AddAfterframes(int frames, char* cmd);
char* GetQueuedCommands();

#ifdef __cplusplus
}

#endif // !__cplusplus
