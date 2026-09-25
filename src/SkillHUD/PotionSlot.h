#pragma once
#include "SkillSlot.h"

namespace AttunementSkillbar {

    /**
     * Constants which describe the kind of item an alchemy item is.
     */
    enum PotionSlotItemKind {

        None = 0,

        /**
         * Indicates that this item is a beneficial potion.
         */
        Potion = 1,

        /**
         * Indicates that this item is a weapon coating poison.
         */
        Poison = 2,

        /**
         * Indicates that this item is a raw food.
         */
        RawFood = 3,

        /**
         * Indicates that this item is a cooked food.
         */
        CookedFood = 4,

        /**
         * Indicates that this item is a non-alcoholic drink.
         */
        Drink = 5,

        /**
         * Indicates that this item is an alcoholic drink.
         */
        AlcoholDrink = 6,

        /**
         * Indicates that this item is a scroll.
         */
        Scroll = 7,
        
    };

    /**
     * Contains additional configuration for potions describing a
     * generic potion kind when the form ID is empty.
     */
    struct PotionSlotConfiguration {

        /**
         * If specified and the skill form configuration is 0, this represents
         * a generic potion affecting the specified actor value. Certain specific
         * actor values are automatically expanded to related actor values.
         * Otherwise this represents the primary effect of the skill form potion.
         * Not used for food items.
         */
        RE::ActorValue effect { RE::ActorValue::kNone };

        /**
         * If specified and the skill form configuration is 0, this represents
         * a generic magic effect..
         * Otherwise this represents the primary effect of the skill form potion.
         * Not used for food items.
         */
        RE::FormID effectID { 0 };

        /**
         * The kind of alchemy item this configuration represents.
         */
        PotionSlotItemKind kind { PotionSlotItemKind::None };

        bool operator==(const PotionSlotConfiguration &other) const {
            return effectID == other.effectID && kind == other.kind;
        }

    };

    /**
     * A subclass of skill slot that optionally includes a potion configuration
     * specifying a generic kind of potion instead of a specific instance.
     */
    class PotionSlot : public SkillSlot {
        friend class SkillHUD;
        friend class PersistenceController;
        private:

            /**
             * A configuration that describes the generic potion kind assigned to this
             * potion slot.
             */
            PotionSlotConfiguration _genericConfiguration;

            /**
             * The number of potions of this slot's kind the player has in their inventory.
             */
            int32_t _count { 0 };

        protected:

            virtual bool CanPress() override;

            virtual void UpdateSkillTexture() override;

            virtual RenderMetrics RenderSlotAtLocation(ImVec2 location, float delta, float scaleY = 1.0f, float translateY = 0.0f, float opacity = 1.0f) override;

            virtual ~PotionSlot() {};
        
        public:
        
            /**
             * Sets this potion slot's configuration.
             * @param config        The skill configuration to use.
             * @param genericConfig The configuration describing the effect.
             */
            void SetConfiguration(SkillSlotConfiguration config, PotionSlotConfiguration genericConfig);

            /**
             * Obtains the local form configuration for the current generic configuration.
             * @returns             The local form configuration.
             */
            SkillSlotLocalFormConfiguration GetGenericLocalFormConfiguration();

            /**
             * Updates the potion slot's generic configuration using the effect specified
             * in the local form configuration.
             * @param config        The local form configuration.
             * @param genericConfig The configuration describing the effect.
             */
            void SetGenericLocalFormConfiguration(SkillSlotLocalFormConfiguration config, SkillSlotLocalFormConfiguration genericConfig);

            /**
             * Updates the count displayed by this potion slot.
             * @param count         The new count to display.
             */
            void SetCount(uint32_t count) {
                _count = count;
            };

            /**
             * Updates the count displayed by this potion slot.
             * @param count         The amount by which to change the displayed count.
             */
            void AdjustCount(int32_t count) {
                _count += count;
            };

    };

    /**
     * A subclass of potion slot that can always play the pressing animation.
     */
    class PotionSlotIndicator : public PotionSlot {
        friend class SkillHUD;
        protected:

            virtual bool CanPress() override;

            virtual ~PotionSlotIndicator() {};

    };

}