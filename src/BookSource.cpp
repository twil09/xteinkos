#include "BookSource.h"

bool BookSource::openLittleFS(const String& path) {
  close();
  lfs_ = LittleFS.open(path, "r");
  ok_ = (bool)lfs_;
  sd_ = false;
  size_ = ok_ ? lfs_.size() : 0;
  return ok_;
}

bool BookSource::openSD(const String& path) {
  close();
  sdf_ = SdMan.open(path.c_str(), O_RDONLY);
  ok_ = (bool)sdf_;
  sd_ = true;
  size_ = ok_ ? (uint32_t)sdf_.fileSize() : 0;
  return ok_;
}

void BookSource::seek(uint32_t pos) {
  if (!ok_) return;
  if (sd_) sdf_.seekSet(pos);
  else lfs_.seek(pos);
}

size_t BookSource::read(char* buf, size_t n) {
  if (!ok_) return 0;
  if (sd_) {
    int r = sdf_.read((uint8_t*)buf, n);
    return r < 0 ? 0 : (size_t)r;
  }
  return lfs_.readBytes(buf, n);
}

void BookSource::close() {
  if (lfs_) lfs_.close();
  if (sdf_) sdf_.close();
  ok_ = false;
  size_ = 0;
}
