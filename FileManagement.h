#ifndef SPIFFS_H
  #define SPIFFS_H
#include <FS.h>

class FileManagement
{
  private:
    Stream &DBG_PORT;
    
  public:
    FileManagement(Stream &dbgPort);
    void saveOptionToFlash(String fileName, String textOpt);
    int readFileLine(String fileName, int iLine, char* buf, int len);
    int readFileLine(String fileName, int iLine, bool buf);
    bool fileGotoLine(File file, int iLine);
    File getFileR(String path);
    File getFileW(String path);
    File getFileWCreate(String path);
    bool fileExists(String path);
    bool fileRemove(String path);
    Dir openDir(String path);
    void ListAllFiles();
    String formatBytes(size_t bytes);
};


#endif
