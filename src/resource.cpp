#include "resource.hpp"

using std::string, std::string_view;

GA_NAMESPACE_BEGIN

string getResourcePath(string_view resource) {
    // FIXME: This is a hack, we should use a proper resource manager
    return string(resource);
}

GA_NAMESPACE_END
