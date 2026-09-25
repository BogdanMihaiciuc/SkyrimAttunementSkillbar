#include "PotionAttunement.h"
#include "PotionSlot.h"

namespace AttunementSkillbar {

    SkillSlot *PotionAttunement::CreateSkillSlot() {
        return new PotionSlot();
    }

}