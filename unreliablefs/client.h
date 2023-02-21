#ifdef __cplusplus
class FileServiceClient;
extern FileServiceClient client;
#endif

#ifdef __cplusplus
extern "C"
#endif
int downloadFileFromServer(char filePath[]);

#ifdef __cplusplus
extern "C"
#endif
int getAttributeFromServer(char filePath[], struct stat *stbuf);

#ifdef __cplusplus
extern "C"
#endif
int makeDir(char filePath[], int mode);

#ifdef __cplusplus
extern "C"
#endif
int uploadFileToServer(char filePath[]);

#ifdef __cplusplus
extern "C"
#endif
int readDir(char filePath[]);

#ifdef __cplusplus
extern "C"
#endif
void initClient();



