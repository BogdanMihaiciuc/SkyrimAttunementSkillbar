#pragma once
#include "AttunementSlot.h"

namespace AttunementSkillbar {

    /**
     * A subclass of attunement slot whose contained skill slots
     * are all potion slots.
     */
    class PotionAttunement : public AttunementSlot {
        friend class SkillHUD;
        protected:

            virtual ~PotionAttunement() {};

            virtual SkillSlot *CreateSkillSlot() override;

    };

}