// Used via the browser console to transform Texture Packer's output json array to the format
// used by attunement skillbar for icons and requires the json to be preloaded as global
// variable _t
window._skillbarJSON = function() {

    const result = {
        iconSize: 128,
        skills: [],
    };
    for (const f of _t.frames) {
        /** @type {string} */
        const filename = f.filename;

        // A # indicates a decimal ID, while a $ indicates a hex ID
        const hasHexID = filename.indexOf('#') == -1;
        let [package, IDStrings] = filename.split(hasHexID ? '$' : '#');
        let kind = 0;

        // The [[Potion]] package name should actually be Skyrim.esm
        if (package == '[[Potion]]') {
            package = 'Skyrim.esm';
            kind = 1;
        }
        // [[packageName.esm]] style packages refer to potion icons, but should be
        // the package name as normal
        else if (package.startsWith('[[') && package != '[[AttunementSkillbar]]') {
            package = package.substring(2, package.length - 2);
            kind = 1;
        }

        const IDs = IDStrings.split(',').map(i => hasHexID ? parseInt(i, 16) : parseInt(i, 10));
        const x = f.frame.x / result.iconSize | 0;
        const y = f.frame.y / result.iconSize | 0;

        for (const ID of IDs) {
            // Potions having IDs larger than the allowable local ID represent generic potion kinds
            if (kind == 1 && ID > 0x10000000) {
                kind = 2;
            }
            result.skills.push({x, y, package, ID, kind});
        }
    }

    return result;

};