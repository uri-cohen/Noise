// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <Exception.h>

namespace noise {

char NoiseException::_buf[BUF_SIZE];
size_t NoiseException::_idx = 0;

}  // namespace noise
