#include "make_effect.h"

namespace banggame {

    template std::vector<equip_holder> make_effects_from_json<equip_holder>(const Json::Value &value);

}