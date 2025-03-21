/**
 * @brief
 *
 * @copyright Copyright (c) 2025 Pandema AB, Electrolux Professional
 *
 * @file file_manager.h
 * @author Claes Ivarsson (ci@pandema.com)
 * @date 2025-03-17
 *
 */

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <cstdio>
#include <string>

#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30


typedef enum {
	FR_OK = 0,				/* (0) Succeeded */
	FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,				/* (2) Assertion failed */
	FR_NOT_READY,			/* (3) The physical drive cannot work */
	FR_NO_FILE,				/* (4) Could not find the file */
	FR_NO_PATH,				/* (5) Could not find the path */
	FR_INVALID_NAME,		/* (6) The path name format is invalid */
	FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	FR_EXIST,				/* (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/* (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any problem */
	FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > FF_FS_LOCK */
	FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} FRESULT;
typedef int FSIZE_t;
typedef uint8_t BYTE;
typedef uint8_t char8_t;

namespace els::cpro2::common::platform::file_utils::file_monitor {
class IMonitoredFile {
  protected:
   IMonitoredFile() = default;  // Prevent instantiation of interface

  public:
   typedef uint32_t FileHandle;
   static constexpr uint32_t kDefaultDuration = 60000;
   virtual ~IMonitoredFile() = default;

   virtual FRESULT Open(const char *path, BYTE mode) = 0;
   virtual FRESULT Open(const char8_t *path, BYTE mode) = 0;
   virtual FRESULT Close() = 0;
   virtual FRESULT Write(const void *buffer, size_t bytes_to_write, size_t &bytes_written) = 0;
   virtual FRESULT Write(const void *buffer, size_t file_offset, size_t bytes_to_write, size_t &bytes_written) = 0;
   virtual FRESULT Read(void *buffer, size_t bytes_to_read, size_t &bytes_read) = 0;
   virtual FRESULT Read(void *buffer, size_t file_offset, size_t bytes_to_read, size_t &bytes_read) = 0;
   virtual FSIZE_t Tell() const = 0;
   virtual FRESULT Seek(FSIZE_t pos) = 0;
   virtual bool IsOpen() const = 0;
   virtual bool IsInactive() const = 0;
   virtual FSIZE_t GetSize() const = 0;
   virtual FileHandle GetFileHandle() const = 0;
};

class MonitoredFile : public IMonitoredFile {
  public:
   explicit MonitoredFile(bool remove_on_close = false, uint32_t duration = kDefaultDuration) {}
   virtual ~MonitoredFile() override {}

   FRESULT Open(const char *path, BYTE mode) override {
      const char* fmode = (mode & FA_READ) ? "r" : "w";
      file_ = fopen(path, fmode);
      if (!file_) {
         return FR_NO_FILE;
      }
      is_open_ = true;
      path_ = path;
      return FR_OK;
   }
   FRESULT Open(const char8_t *path, BYTE mode) override {
      file_ = fopen(reinterpret_cast<const char *>(path), "r");
      if (!file_) {
         return FR_NO_FILE;
      }
      is_open_ = true;
      path_ = reinterpret_cast<const char *>(path);
      return FR_OK;
   }
   FRESULT Close() override {
      if (file_) {
         fclose(file_);
         file_ = nullptr;
         is_open_ = false;
      }
      return FR_OK;
   }
   FRESULT Write(const void *buffer, size_t bytes_to_write, size_t &bytes_written) override {
      for (size_t i = 0; i < bytes_to_write; i++) {
         fputc(static_cast<int>(reinterpret_cast<const char *>(buffer)[i]), file_);
      }
      bytes_written = bytes_to_write;
      return FR_OK; 
   }
   FRESULT Write(const void *buffer, size_t file_offset, size_t bytes_to_write, size_t &bytes_written) override { return FR_OK; }
   FRESULT Read(void *buffer, size_t bytes_to_read, size_t &bytes_read) override {
      return fread(buffer, bytes_to_read, 1, file_) == 1 ? FR_OK : FR_DISK_ERR;
   }
   FRESULT Read(void *buffer, size_t file_offset, size_t bytes_to_read, size_t &bytes_read) override { return FR_OK; };
   FSIZE_t Tell() const override {
        return ftell(file_);
   };
   FRESULT Seek(FSIZE_t pos) override {
        return fseek(file_, pos, SEEK_SET) == 0 ? FR_OK : FR_DISK_ERR;
   }
   bool IsOpen() const override { return is_open_; };
   bool IsInactive() const override { return false; };
   FSIZE_t GetSize() const override {
      fseek(file_, 0, SEEK_END);
      FSIZE_t size = ftell(file_);
      fseek(file_, 0, SEEK_SET);
      return size;
   };
   IMonitoredFile::FileHandle GetFileHandle() const override { return 0; };

  protected:
   FILE *file_;  // file handle
   bool is_open_;
   std::string path_;
};

}  // namespace els::cpro2::common::platform::file_utils::file_monitor

#endif
