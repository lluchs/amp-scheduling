#include "swp_util.h"

namespace swp {

std::string section_name(const char *id, const char *pos) {
	return pos ? std::move(std::string(id) + " [" + pos + "]") : id;
}

}
