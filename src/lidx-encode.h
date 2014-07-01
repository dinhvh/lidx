#ifndef LIDX_ENCODE_H

#define LIDX_ENCODE_H

#include <string>

void lidx_encode_uint64(std::string & buffer, uint64_t value);
size_t lidx_decode_uint64(std::string & buffer, size_t position, uint64_t * p_value);

#endif

