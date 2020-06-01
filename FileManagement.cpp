#include "FileManagement.h"

FileManagement::FileManagement(Stream &dbgPort): DBG_PORT(dbgPort)
{
  SPIFFS.begin();
}


void FileManagement::saveOptionToFlash(String fileName, String textOpt)
{
  SPIFFS.remove(fileName);
  File file = SPIFFS.open(fileName, "w+");
  file.println(textOpt);
  file.close();
}

int FileManagement::readFileLine(String fileName, int iLine, char* buf, int len)
{
  File file = SPIFFS.open(fileName, "r");
  bool hasLine = fileGotoLine(file, iLine);
  int lenRead = 0;
  if (hasLine)
    lenRead = file.readBytesUntil('\r', buf, len);
  else {
    buf[0] = '\0';
    //DBG_PORT.print("no line");
  }
  file.close();
  DBG_PORT.write("Read ");
  DBG_PORT.print(fileName);
  DBG_PORT.write(" line: ");
  String s = String(*buf);
  DBG_PORT.write((uint8_t*)buf,lenRead);
  DBG_PORT.write("\r\n");
  return lenRead;
}

int FileManagement::readFileLine(String fileName, int iLine, bool buf)
{
  char tmp[1];
  int lenRead = readFileLine(fileName,iLine,tmp,1);
  buf = (bool)tmp;
  return lenRead;
}

bool FileManagement::fileGotoLine(File file, int iLine)
{
  int len = 50;
  char buf[50];
  file.seek(0,SeekSet);
  if (iLine == 0) return true;
  for (int i = 0; i < iLine; i++)
  {
    //DBG_PORT.write("GOTOLine=");
    //DBG_PORT.print(i, DEC);
    int l = file.readBytesUntil('\n',buf,len);
    //DBG_PORT.write((uint8_t*)buf,l);
    //DBG_PORT.println("");
    if (file.position() == file.size()) {
      //DBG_PORT.println("Fail. End of file!");
      return false;
    }
  }
  return true;
}

File FileManagement::getFileR(String path)
{
    return SPIFFS.open(path, "r");
}

File FileManagement::getFileW(String path)
{
    return SPIFFS.open(path, "w");
}

File FileManagement::getFileWCreate(String path)
{
    return SPIFFS.open(path, "w+");
}

bool FileManagement::fileExists(String path)
{
  return SPIFFS.exists(path);
}

bool FileManagement::fileRemove(String path)
{
  return SPIFFS.remove(path);
}

Dir FileManagement::openDir(String path)
{
  return SPIFFS.openDir(path);
}

void FileManagement::ListAllFiles()
{
  Dir dir = openDir("/");
  while (dir.next()) {    
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    DBG_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  DBG_PORT.printf("\n");
}

String FileManagement::formatBytes(size_t bytes)
{
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}
