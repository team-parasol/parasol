#ifndef PARASOL_STRINGS
#define PARASOL_STRINGS 1
#ifdef __cplusplus

#include <sstream>

namespace parasol {

template <class InType, class OutIt>
void split(InType Input, OutIt Output)
{
   auto begin = Input.begin();
   auto end = Input.end();
   auto current = begin;
   while (begin != end) {
      if (*begin == ',') {
         *Output++ = std::string(current, begin);
         current = ++begin;
      }
      else ++begin;
   }
   *Output++ = std::string(current, begin);
}

INLINE void ltrim(std::string &s, const std::string &match) {
   s.erase(s.begin(), std::find_if(s.begin(), s.end(), [match](unsigned char ch) {
      return match.find(ch) == std::string::npos;
   }));
}

INLINE void rtrim(std::string &s, const std::string &match) {
   s.erase(std::find_if(s.rbegin(), s.rend(), [match](unsigned char ch) {
      return match.find(ch) == std::string::npos;
   }).base(), s.end());
}

INLINE void camelcase(std::string &s) {
   bool raise = true;
   for (ULONG i=0; i < s.size(); i++) {
      if (raise) {
         s[i] = UCASE(s[i]);
         raise = false;
      }
      else if (s[i] <= 0x20) raise = true;
   }
}

} // namespace
#endif
#endif
