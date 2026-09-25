#pragma once

namespace AttunementSkillbar {

    /**
     * Constants which describe the slots an equipped item can be assigned to.
     */
    enum class EquippedItemHand {

        /**
         * Indicates that the item is equipped in the left hand.
         */
        LeftHand = 1,

        /**
         * Indicates that the item is equipped in the right hand.
         */
        RightHand = 2

    };

    /**
     * Constants which describe the kind of equipped item there is in a
     * stored equipment slot.
     */
    enum class EquippedItemKind {

        /**
         * Indicates that no item is stored.
         */
        Null = 1,

        /**
         * Indicates that the slot contains no item and unequips the current item.
         */
        Fist = 2,

        /**
         * Indicates that the item is a weapon or armor.
         */
        WeaponArmor = 3,

        /**
         * Indicates that the item is a scroll.
         */
        Scroll = 4,

        /**
         * Indicates that the item is a spell.
         */
        Spell = 5

    };
     

    class EquippedItem;

    /**
     * A class that is used to obtain and store the player character's currently
     * equipped left and right hand slot items and contains methods to re-equip
     * them if they are still available.
     * 
     * A stored equipment class automatically reads the player's currently equipped
     * items when created and can restore them via the Equip method.
     */
    class StoredEquipment {

        private:

            /**
             * The item equipped in the left hand slot.
             */
            EquippedItem *_leftHand { nullptr };

            /**
             * The item equipped in the right hand slot.
             */
            EquippedItem *_rightHand { nullptr };

        public:

            /**
             * Returns the spell or scroll item equipped in the specified hand for this
             * stored equipped item.
             * @param hand          The hand for which to obtain the spell.
             * @returns             The equipped spell, if any, nullptr otherwise.
             */
            RE::TESForm *SpellItemInHand(EquippedItemHand hand);

            /**
             * Creates and initializes a stored equipment instance with the
             * player's currently equipped items.
             */
            StoredEquipment();

            ~StoredEquipment();

            /**
             * Returns true if this stored equipped has a two-handed item.
             * @returns     True if a two-handed item is stored, false otherwise.
             */
            bool ContainsTwoHandedItem();

            /**
             * Returns true if this stored equipped has a staff in either hand.
             * @returns     True if a staff item is stored, false otherwise.
             */
            bool ContainsStaffItem();

            /**
             * Returns true if this stored equipped has a weapon item in the main hand.
             * @returns     True if a weapon item is stored for the main hand, false otherwise.
             */
            bool ContainsWeaponMainHand();

            /**
             * Re-equips the stored equipped items.
             * @param leftHandChanged   Set to true if the equipped object in the left hand changed,
             *                          or to false otherwise.
             * @param rightHandChanged  Set to true if the equipped object in the right hand changed,
             *                          or to false otherwise.
             * @returns                 True if a single handed martial item was equipped in the right
             *                          hand, false otherwise.
             */
            bool Equip(bool *leftHandChanged = nullptr, bool *rightHandChanged = nullptr);

    };

    /**
     * Represents an item that can be equipped in the hand slots. This class is always
     * instantiated as one of its concrete subclasses.
     */
    class EquippedItem {
        friend class StoredEquipment;
        protected:
            /**
             * The hand from which this equipped item was created.
             */
            EquippedItemHand _hand;

            /**
             * The kind of equipped item.
             */
            EquippedItemKind _kind;

            /**
             * Set to true if this equipped item is two handed.
             */
            bool _isTwoHanded { false };

            EquippedItem(EquippedItemKind kind, bool isTwoHanded):
                _kind(kind),
                _isTwoHanded(isTwoHanded) {};

        public:

            virtual ~EquippedItem() {

            }

            /**
             * Obtains the kind of this equipped item.
             * @returns     The kind of item.
             */
            EquippedItemKind GetKind() const {
                return _kind;
            }

            /**
             * Test whether this equipped item is two handed.
             * @returns     True if this item is two handed, false otherwise.
             */
            bool IsTwoHanded() {
                return _isTwoHanded;
            }

            /**
             * Creates and returns an appropriate equipped item from the item
             * the player character has equipped in the specified hand.
             * @param hand          The hand from which to obtain the currently
             *                      equipped item.
             */
            static EquippedItem *CurrentlyEquipped(EquippedItemHand hand);

            /**
             * Gets the equipment slot corresponding to the hand this equipped item
             * was initialized from.
             * @returns         The equipment slot.
             */
            RE::BGSEquipSlot *GetSlot();

            /**
             * Equips this item in the hand slot from which it was created.
             * @returns         True if the item was equipped, false it the item
             *                  was already equipped or could not be equipped.
             */
            virtual bool Equip() = 0;

            /**
             * Tests whether this equipped item is a staff.
             * @returns         true if this equipped item is a staff, false otherwise. 
             */
            virtual bool IsStaff() {
                return false;
            }

    };

    /**
     * An equipped item representing no equipment action to take.
     */
    class NullEquippedItem : public EquippedItem {
        public:

            NullEquippedItem(): EquippedItem(EquippedItemKind::Null, false) {}

            virtual bool Equip() override {
                // Intentionally blank
                return false;
            }
    };

    /**
     * An equipped item representing no equipped item (bare fist).
     */
    class FistEquippedItem : public EquippedItem {
        public:
            FistEquippedItem(): EquippedItem(EquippedItemKind::Fist, false) {};

            virtual bool Equip() override;
    };

    /**
     * An equipped item representing a weapon or armor.
     */
    class WeaponArmorEquippedItem : public EquippedItem {

        private:

            /**
             * The base form ID of the weapon or armor.
             */
            RE::FormID _formID { 0 };

            /**
             * The unique ID for the specific isntance the player had
             * equipped.
             */
            uint16_t _uniqueID { 0 };

            /**
             * Set to true for staff weapons.
             */
            bool _isStaff { false };

        public:

            WeaponArmorEquippedItem(RE::FormID formID, uint16_t uniqueID, bool isTwoHanded, bool isStaff):
                _formID(formID),
                _uniqueID(uniqueID),
                _isStaff(isStaff),
                EquippedItem(EquippedItemKind::WeaponArmor, isTwoHanded) {};

            virtual bool Equip() override;

            virtual bool IsStaff() override {
                return _isStaff;
            }
    };


    /**
     * An equipped item representing a magical scroll.
     */
    class ScrollEquippedItem : public EquippedItem {
        
        private:

            /**
             * The base form ID of the scroll.
             */
            RE::FormID _formID { 0 };

        public:

            ScrollEquippedItem(RE::FormID formID, bool isTwoHanded):
                _formID(formID),
                EquippedItem(EquippedItemKind::Scroll, isTwoHanded) {};

            RE::TESForm *GetScroll() {
                return RE::TESForm::LookupByID(_formID);
            }

            virtual bool Equip() override;
    };


    /**
     * An equipped item representing a spell.
     */
    class SpellEquippedItem : public EquippedItem {
        
        private:

            /**
             * The base form ID of the spell.
             */
            RE::FormID _formID { 0 };

        public:

            SpellEquippedItem(RE::FormID formID, bool isTwoHanded):
                _formID(formID),
                EquippedItem(EquippedItemKind::Spell, isTwoHanded) {};

            RE::TESForm *GetSpell() {
                return RE::TESForm::LookupByID(_formID);
            }

            virtual bool Equip() override;
    };

}