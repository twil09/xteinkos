// Include miniz config FIRST so the ROM-safe renames apply before anything can
// pull in the ESP32 ROM's miniz.h.
#include "MinizConfig.h"

#include "Epub.h"

#include <LittleFS.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "BookSource.h"

#define EPUB_TEXT_PATH "/.vixbook.txt"
#define EPUB_META_PATH "/.vixbook.meta"

namespace {

// ---- little-endian readers -------------------------------------------------
inline uint16_t rd16(const uint8_t* p) { return p[0] | (p[1] << 8); }
inline uint32_t rd32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

bool readAt(BookSource& s, uint32_t off, void* buf, size_t n) {
  s.seek(off);
  return s.read((char*)buf, n) == n;
}

struct ZipEntry {
  uint16_t method = 0;
  uint32_t comp = 0, uncomp = 0, localOff = 0;
};

// ---- ZIP central directory (loaded once into a heap buffer) -----------------
struct Zip {
  BookSource& src;
  uint8_t* cd = nullptr;
  uint32_t cdSize = 0;
  explicit Zip(BookSource& s) : src(s) {}
  ~Zip() { free(cd); }

  bool open() {
    uint32_t sz = src.size();
    if (sz < 22) return false;
    // Find the End Of Central Directory by scanning a tail window.
    uint32_t win = sz < 4096 ? sz : 4096;
    uint8_t* tail = (uint8_t*)malloc(win);
    if (!tail) return false;
    if (!readAt(src, sz - win, tail, win)) { free(tail); return false; }
    long eocd = -1;
    for (long i = (long)win - 22; i >= 0; --i)
      if (rd32(tail + i) == 0x06054b50) { eocd = i; break; }
    if (eocd < 0) { free(tail); return false; }
    cdSize = rd32(tail + eocd + 12);
    uint32_t cdOff = rd32(tail + eocd + 16);
    free(tail);
    if (cdSize == 0 || cdSize > 1024 * 1024) return false;  // sanity cap
    cd = (uint8_t*)malloc(cdSize);
    if (!cd) return false;
    return readAt(src, cdOff, cd, cdSize);
  }

  bool find(const char* name, size_t nameLen, ZipEntry& e) {
    uint32_t p = 0;
    while (p + 46 <= cdSize) {
      if (rd32(cd + p) != 0x02014b50) break;
      uint16_t nlen = rd16(cd + p + 28), elen = rd16(cd + p + 30),
               clen = rd16(cd + p + 32);
      if (p + 46 + nlen > cdSize) break;
      if (nlen == nameLen && memcmp(cd + p + 46, name, nameLen) == 0) {
        e.method = rd16(cd + p + 10);
        e.comp = rd32(cd + p + 20);
        e.uncomp = rd32(cd + p + 24);
        e.localOff = rd32(cd + p + 42);
        return true;
      }
      p += 46 + nlen + elen + clen;
    }
    return false;
  }
  bool find(const String& name, ZipEntry& e) {
    return find(name.c_str(), name.length(), e);
  }

  // Stream a member's decompressed bytes to `sink` (returns false to stop).
  template <typename Sink>
  bool extract(const ZipEntry& e, Sink&& sink) {
    uint8_t lh[4];
    if (!readAt(src, e.localOff + 26, lh, 4)) return false;
    uint16_t nlen = rd16(lh), elen = rd16(lh + 2);
    uint32_t dataOff = e.localOff + 30 + nlen + elen;

    if (e.method == 0) {  // stored
      src.seek(dataOff);
      uint32_t rem = e.uncomp;
      char b[1024];
      while (rem) {
        size_t k = rem < sizeof(b) ? rem : sizeof(b);
        size_t r = src.read(b, k);
        if (!r) return false;
        if (!sink((const uint8_t*)b, r)) return true;
        rem -= r;
      }
      return true;
    }
    if (e.method != 8) return false;  // only deflate/stored supported

    tinfl_decompressor* d = tinfl_decompressor_alloc();
    uint8_t* dict = (uint8_t*)malloc(TINFL_LZ_DICT_SIZE);
    if (!d || !dict) { free(dict); if (d) tinfl_decompressor_free(d); return false; }
    tinfl_init(d);

    src.seek(dataOff);
    uint32_t inFileRem = e.comp;
    uint8_t inBuf[2048];
    size_t inAvail = 0, inPos = 0, dictOfs = 0;
    bool ok = true;
    for (;;) {
      if (inPos >= inAvail) {
        size_t want = inFileRem < sizeof(inBuf) ? inFileRem : sizeof(inBuf);
        inAvail = want ? src.read((char*)inBuf, want) : 0;
        inFileRem -= inAvail;
        inPos = 0;
      }
      size_t inBytes = inAvail - inPos;
      size_t outBytes = TINFL_LZ_DICT_SIZE - dictOfs;
      mz_uint32 flags = inFileRem > 0 ? TINFL_FLAG_HAS_MORE_INPUT : 0;
      tinfl_status st = tinfl_decompress(d, inBuf + inPos, &inBytes, dict,
                                         dict + dictOfs, &outBytes, flags);
      inPos += inBytes;
      if (outBytes) {
        if (!sink(dict + dictOfs, outBytes)) { ok = true; break; }
        dictOfs += outBytes;
        if (dictOfs == TINFL_LZ_DICT_SIZE) dictOfs = 0;
      }
      if (st == TINFL_STATUS_DONE) break;
      if (st < 0) { ok = false; break; }
      if (st == TINFL_STATUS_NEEDS_MORE_INPUT && inFileRem == 0 &&
          (inAvail - inPos) == 0) { ok = false; break; }
    }
    free(dict);
    tinfl_decompressor_free(d);
    return ok;
  }
};

// ---- XHTML -> plain text (streaming, whitespace-collapsing) -----------------
struct HtmlToText {
  File& out;
  uint8_t wbuf[512];
  size_t wlen = 0;
  bool inTag = false, skipping = false, inEntity = false;
  char tag[24]; size_t taglen = 0;
  char skipName[8]; size_t skipLen = 0;
  char ent[12]; size_t entlen = 0;
  char lastCh = 0;
  bool pendingSpace = false;
  int newlines = 0;

  explicit HtmlToText(File& f) : out(f) {}

  void raw(char c) {
    wbuf[wlen++] = (uint8_t)c;
    if (wlen == sizeof(wbuf)) { out.write(wbuf, wlen); wlen = 0; }
  }
  void flush() { if (wlen) { out.write(wbuf, wlen); wlen = 0; } }

  void writeChar(char c) {
    if (c == '\n') {
      pendingSpace = false;
      if (lastCh != 0 && newlines < 2) { raw('\n'); ++newlines; lastCh = '\n'; }
      return;
    }
    if (pendingSpace) { if (lastCh != 0 && lastCh != '\n') raw(' '); pendingSpace = false; }
    raw(c); lastCh = c; newlines = 0;
  }
  void textByte(char c) {
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { pendingSpace = true; return; }
    writeChar(c);
  }

  bool nameEq(const char* a, size_t al, const char* b) {
    size_t bl = strlen(b);
    if (al != bl) return false;
    for (size_t i = 0; i < al; ++i) {
      char x = a[i]; if (x >= 'A' && x <= 'Z') x += 32;
      if (x != b[i]) return false;
    }
    return true;
  }
  bool isBlock(const char* n, size_t l) {
    static const char* B[] = {"p","div","br","li","tr","h1","h2","h3","h4","h5",
      "h6","blockquote","hr","section","article","header","footer","figure",
      "figcaption","ul","ol","table","pre"};
    for (auto b : B) if (nameEq(n, l, b)) return true;
    return false;
  }

  void processTag() {
    // parse closing + name
    size_t i = 0; bool closing = false;
    if (taglen && tag[0] == '/') { closing = true; i = 1; }
    char nm[16]; size_t nl = 0;
    for (; i < taglen && nl < sizeof(nm) - 1; ++i) {
      char c = tag[i];
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
        nm[nl++] = c;
      else break;
    }
    nm[nl] = 0;
    if (skipping) {
      if (closing && nameEq(nm, nl, skipName)) skipping = false;
      return;
    }
    if (nameEq(nm, nl, "script") || nameEq(nm, nl, "style") || nameEq(nm, nl, "head")) {
      if (!closing) { skipping = true; skipLen = nl; memcpy(skipName, nm, nl + 1); }
      return;
    }
    if (isBlock(nm, nl)) writeChar('\n');
  }

  void decodeEntity() {
    // ent holds "&....." without trailing ';'
    char c = 0;
    if (entlen >= 2 && ent[1] == '#') {
      long v = 0;
      if (entlen >= 3 && (ent[2] == 'x' || ent[2] == 'X'))
        v = strtol(ent + 3, nullptr, 16);
      else v = strtol(ent + 2, nullptr, 10);
      if (v == 160) { pendingSpace = true; return; }         // nbsp
      if (v == 8217 || v == 8216) c = '\'';
      else if (v == 8220 || v == 8221) c = '"';
      else if (v == 8211 || v == 8212) c = '-';
      else if (v == 8230) { textByte('.'); textByte('.'); textByte('.'); return; }
      else if (v > 0 && v < 128) c = (char)v;
      else return;  // unsupported non-ASCII -> drop
      textByte(c);
      return;
    }
    struct { const char* n; char c; } named[] = {
      {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''},
      {"nbsp", ' '}, {"mdash", '-'}, {"ndash", '-'}, {"hellip", '.'},
      {"lsquo", '\''}, {"rsquo", '\''}, {"ldquo", '"'}, {"rdquo", '"'}};
    for (auto& e : named)
      if (nameEq(ent + 1, entlen - 1, e.n)) {
        if (e.c == ' ') pendingSpace = true; else textByte(e.c);
        return;
      }
    // unknown: drop silently
  }

  bool operator()(const uint8_t* data, size_t n) {
    for (size_t k = 0; k < n; ++k) {
      char c = (char)data[k];
      if (inTag) {
        if (c == '>') { inTag = false; processTag(); }
        else if (taglen < sizeof(tag) - 1) tag[taglen++] = c;
        continue;
      }
      if (inEntity) {
        if (c == ';') { inEntity = false; decodeEntity(); }
        else if (c == '&') { /* malformed */ inEntity = true; entlen = 0; ent[entlen++] = '&'; }
        else if (entlen < sizeof(ent) - 1) ent[entlen++] = c;
        else { inEntity = false; }  // too long, abandon
        continue;
      }
      if (c == '<') { inTag = true; taglen = 0; continue; }
      if (skipping) continue;
      if (c == '&') { inEntity = true; entlen = 0; ent[entlen++] = '&'; continue; }
      textByte(c);
    }
    return true;
  }
};

// ---- OPF / path helpers ----------------------------------------------------
String attrOf(const String& s, int from, int to, const char* attr) {
  int a = s.indexOf(attr, from);
  if (a < 0 || a > to) return "";
  a += strlen(attr);
  while (a < to && (s[a] == ' ' || s[a] == '=')) ++a;
  if (a >= to) return "";
  char q = s[a];
  if (q != '"' && q != '\'') return "";
  int b = s.indexOf(q, a + 1);
  if (b < 0 || b > to) return "";
  return s.substring(a + 1, b);
}

String urlDecode(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '%' && i + 2 < s.length()) {
      auto hx = [](char h) -> int {
        if (h >= '0' && h <= '9') return h - '0';
        if (h >= 'a' && h <= 'f') return h - 'a' + 10;
        if (h >= 'A' && h <= 'F') return h - 'A' + 10;
        return -1; };
      int hi = hx(s[i + 1]), lo = hx(s[i + 2]);
      if (hi >= 0 && lo >= 0) { o += (char)(hi * 16 + lo); i += 2; continue; }
    }
    o += c;
  }
  return o;
}

// join opfDir + href, resolve ./ and ../, strip #anchor and query.
String resolvePath(const String& opfDir, const String& hrefRaw) {
  String href = hrefRaw;
  int h = href.indexOf('#'); if (h >= 0) href = href.substring(0, h);
  href = urlDecode(href);
  String full = href.startsWith("/") ? href.substring(1)
                : (opfDir.length() ? opfDir + "/" + href : href);
  // normalize segments
  String out;
  int i = 0, n = full.length();
  std::vector<String> parts;
  while (i < n) {
    int j = full.indexOf('/', i);
    if (j < 0) j = n;
    String seg = full.substring(i, j);
    if (seg == "..") { if (!parts.empty()) parts.pop_back(); }
    else if (seg != "." && seg.length()) parts.push_back(seg);
    i = j + 1;
  }
  for (size_t k = 0; k < parts.size(); ++k) { if (k) out += "/"; out += parts[k]; }
  return out;
}

String dirOf(const String& p) {
  int s = p.lastIndexOf('/');
  return s >= 0 ? p.substring(0, s) : "";
}

// Read a whole zip member into a String (for small XML files).
bool extractToString(Zip& zip, const String& name, String& out) {
  ZipEntry e;
  if (!zip.find(name, e)) return false;
  out = "";
  out.reserve(e.uncomp + 1);
  bool ok = zip.extract(e, [&](const uint8_t* d, size_t n) {
    out.concat((const char*)d, n);
    return true;
  });
  return ok;
}

String cacheKey(const String& srcPath, bool fromSd, uint32_t size) {
  return (fromSd ? String("sd:") : String("")) + srcPath + "|" + String(size);
}

}  // namespace

namespace Epub {

Result prepare(const String& srcPath, bool fromSd) {
  Result r;
  BookSource arc;
  bool opened = fromSd ? arc.openSD(srcPath) : arc.openLittleFS(srcPath);
  if (!opened) { r.error = "Cannot open file"; return r; }
  String key = cacheKey(srcPath, fromSd, arc.size());

  // Cache hit?
  if (LittleFS.exists(EPUB_TEXT_PATH) && LittleFS.exists(EPUB_META_PATH)) {
    File m = LittleFS.open(EPUB_META_PATH, "r");
    if (m) {
      String storedKey = m.readStringUntil('\n');
      String storedTitle = m.readStringUntil('\n');
      m.close();
      if (storedKey == key) {
        r.ok = true; r.textPath = EPUB_TEXT_PATH; r.title = storedTitle;
        return r;
      }
    }
  }

  Zip zip(arc);
  if (!zip.open()) { r.error = "Not a valid EPUB (zip)"; return r; }

  String container;
  if (!extractToString(zip, "META-INF/container.xml", container)) {
    r.error = "No container.xml"; return r;
  }
  String opfPath = attrOf(container, 0, container.length(), "full-path");
  if (opfPath.length() == 0) { r.error = "No OPF path"; return r; }

  String opf;
  if (!extractToString(zip, opfPath, opf)) { r.error = "Cannot read OPF"; return r; }
  String opfDir = dirOf(opfPath);

  // title
  int t0 = opf.indexOf("<dc:title");
  if (t0 < 0) t0 = opf.indexOf("<title");
  if (t0 >= 0) {
    int g = opf.indexOf('>', t0);
    int e = (g >= 0) ? opf.indexOf('<', g + 1) : -1;
    if (g >= 0 && e > g) r.title = opf.substring(g + 1, e);
    r.title.trim();
  }

  // manifest: id -> href
  std::vector<String> ids, hrefs;
  int mStart = opf.indexOf("<manifest");
  int mEnd = opf.indexOf("</manifest");
  if (mStart >= 0 && mEnd > mStart) {
    int p = mStart;
    while (true) {
      int it = opf.indexOf("<item", p);
      if (it < 0 || it > mEnd) break;
      int close = opf.indexOf('>', it);
      if (close < 0) break;
      // avoid matching <itemref>
      if (opf.substring(it, it + 6) != "<itemref") {
        String id = attrOf(opf, it, close, "id=");
        String hr = attrOf(opf, it, close, "href=");
        if (id.length() && hr.length()) { ids.push_back(id); hrefs.push_back(hr); }
      }
      p = close + 1;
    }
  }

  // spine: ordered idrefs
  std::vector<String> spine;
  int sStart = opf.indexOf("<spine");
  int sEnd = opf.indexOf("</spine");
  if (sStart >= 0 && sEnd > sStart) {
    int p = sStart;
    while (true) {
      int it = opf.indexOf("<itemref", p);
      if (it < 0 || it > sEnd) break;
      int close = opf.indexOf('>', it);
      if (close < 0) break;
      String idref = attrOf(opf, it, close, "idref=");
      if (idref.length()) {
        for (size_t k = 0; k < ids.size(); ++k)
          if (ids[k] == idref) { spine.push_back(hrefs[k]); break; }
      }
      p = close + 1;
    }
  }
  if (spine.empty()) { r.error = "Empty spine"; return r; }

  // Extract each spine document to the text file.
  File out = LittleFS.open(EPUB_TEXT_PATH, "w");
  if (!out) { r.error = "No space to extract"; return r; }
  int wrote = 0;
  for (auto& href : spine) {
    String path = resolvePath(opfDir, href);
    ZipEntry e;
    if (!zip.find(path, e)) continue;
    HtmlToText sink(out);
    if (zip.extract(e, [&](const uint8_t* d, size_t n) { return sink(d, n); })) {
      sink.flush();
      out.write((const uint8_t*)"\n\n", 2);
      ++wrote;
    }
  }
  out.close();
  if (wrote == 0) { LittleFS.remove(EPUB_TEXT_PATH); r.error = "No readable chapters"; return r; }

  File m = LittleFS.open(EPUB_META_PATH, "w");
  if (m) { m.println(key); m.println(r.title); m.close(); }

  r.ok = true;
  r.textPath = EPUB_TEXT_PATH;
  return r;
}

}  // namespace Epub
