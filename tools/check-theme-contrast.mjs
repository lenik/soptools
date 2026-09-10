#!/usr/bin/env node
/**
 * Verify semantic fg/bg contrast across theme palette modules.
 * Exit 1 if any pair is below the minimum ratio.
 */
import { pathToFileURL } from 'node:url';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import {
    contrastRatio,
    WEB_FG_BG_PAIRS,
    ERP_FG_BG_PAIRS,
    UI_FG_BG_PAIRS,
} from '../suite/worldman/themes/lib/color-utils.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const themesRoot = join(__dirname, '../suite/worldman/themes');
const MIN = Number(process.argv[2] || 4.5);

async function load(group, file, exportName) {
    const mod = await import(pathToFileURL(join(themesRoot, group, file)).href);
    return mod[exportName];
}

const fails = [];
for (const group of ['minor', 'vibe', 'country']) {
    const ui = await load(group, 'ui-palettes.mjs', 'uiPalettes');
    const web = await load(group, 'web-palettes.mjs', 'webPalettes');
    const erp = await load(group, 'erp-palettes.mjs', 'erpPalettes');
    for (const [name, pal] of Object.entries(ui)) {
        for (const [fg, bg] of UI_FG_BG_PAIRS) {
            if (!pal[fg] || !pal[bg]) continue;
            const r = contrastRatio(pal[fg], pal[bg]);
            if (r < MIN) fails.push(`${group}/ui ${name} ${fg}/${bg} ${r.toFixed(2)}`);
        }
    }
    for (const [name, pal] of Object.entries(web)) {
        for (const [fg, bg] of WEB_FG_BG_PAIRS) {
            if (!pal[fg] || !pal[bg]) continue;
            const r = contrastRatio(pal[fg], pal[bg]);
            if (r < MIN) fails.push(`${group}/web ${name} ${fg}/${bg} ${r.toFixed(2)}`);
        }
    }
    for (const [name, pal] of Object.entries(erp)) {
        for (const [fg, bg] of ERP_FG_BG_PAIRS) {
            if (!pal[fg] || !pal[bg]) continue;
            const r = contrastRatio(pal[fg], pal[bg]);
            if (r < MIN) fails.push(`${group}/erp ${name} ${fg}/${bg} ${r.toFixed(2)}`);
        }
    }
}

if (fails.length) {
    console.error(`FAIL ${fails.length} pairs < ${MIN}:1`);
    for (const f of fails.slice(0, 40)) console.error(' ', f);
    if (fails.length > 40) console.error(`  … +${fails.length - 40} more`);
    process.exit(1);
}
console.log(`OK all semantic fg/bg pairs ≥ ${MIN}:1`);
