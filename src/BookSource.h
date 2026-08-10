// BookSource — read a book from either LittleFS or the SD card, uniformly.
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <SDCardManager.h>

class BookSource {
 public:
  bool openLittleFS(const String& path);
  bool openSD(const String& path);
  bool ok() const { return ok_; }
  uint32_t size() const { return size_; }
  void seek(uint32_t pos);
  size_t read(char* buf, size_t n);
  void close();

 private:
  bool ok_ = false;
  bool sd_ = false;
  uint32_t size_ = 0;
  File   lfs_;
  FsFile sdf_;
};
