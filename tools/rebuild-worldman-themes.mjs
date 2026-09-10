#!/usr/bin/env node
/**
 * Full rebuild of suite/worldman/themes:
 *  1) extract from minor-themes (ui/token + derived web/erp)
 *  2) merge zfr L2 + L3 extra countries
 *  3) enforce fg/bg contrast (minor-themes ensureContrast method)
 *  4) write bilingual catalog/*.md + catalog.tsv
 *  5) refresh demo-data.js for index.html
 *
 * Usage:
 *   node tools/rebuild-worldman-themes.mjs [/path/to/minor-themes]
 */
import { spawnSync } from 'node:child_process';
import {
    readFileSync,
    writeFileSync,
    readdirSync,
    mkdirSync,
} from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import {
    enforcePairs,
    WEB_FG_BG_PAIRS,
    ERP_FG_BG_PAIRS,
    UI_FG_BG_PAIRS,
} from '../suite/worldman/themes/lib/color-utils.mjs';
import { l2ExtraUiPalettes, L2_LOCALE_TO_COUNTRY, TIER_I, TIER_II } from './data/l2-country-ui.mjs';
import { l3ExtraUiPalettes, L3_LOCALE_TO_COUNTRY, TIER_III } from './data/l3-country-ui.mjs';
import { CATALOG, COUNTRY_CATALOG } from './data/theme-catalog-prose.mjs';
const __dirname = dirname(fileURLToPath(import.meta.url));
const root = join(__dirname, '..');
const themesRoot = join(root, 'suite/worldman/themes');
const MT = process.argv[2] || '/home/cursor/archive/minor-themes';
const MIN_RATIO = 4.5;

function keyLit(k) {
    return /^[A-Za-z_][A-Za-z0-9_]*$/.test(k) ? k : JSON.stringify(k);
}

function serializePaletteObject(title, obj, exportName) {
    const body = Object.keys(obj)
        .map((k) => {
            const entry = obj[k];
            const fields = Object.entries(entry)
                .map(([fk, fv]) => `    ${keyLit(fk)}: ${JSON.stringify(fv)},`)
                .join('\n');
            return `  ${keyLit(k)}: {\n${fields}\n  },`;
        })
        .join('\n');
    return `/**
 * ${title} — grouped color definitions (HSL strings).
 * Hand-adapted from minor-themes; no VS Code generator output.
 * Fg/bg pairs contrast-checked (min ${MIN_RATIO}:1).
 */
export const ${exportName} = {\n${body}\n};\n`;
}

function mapWeb(ui) {
    const fg = ui.windowFg;
    const bg = ui.windowBg;
    const surface = ui.surfaceBg || bg;
    const panel = ui.panelBg || bg;
    const primary = ui.actionFg;
    return {
        background: bg,
        foreground: fg,
        primary,
        'primary-foreground': 'hsl(0, 0%, 100%)',
        card: ui.cardBg || surface,
        'card-foreground': fg,
        accent: ui.selectedBg || ui.intervalBg || panel,
        'accent-foreground': ui.intervalFg || fg,
        'muted-foreground': ui.mutedFg,
        border: ui.border,
        ring: primary,
        destructive: ui.gitDeleted,
        success: ui.gitUntracked,
        warning: ui.gitModified,
    };
}

function mapErp(ui) {
    const fg = ui.windowFg;
    const bg = ui.windowBg;
    const panel = ui.panelBg || bg;
    const surface = ui.surfaceBg || bg;
    return {
        page: bg,
        'page-foreground': fg,
        panel,
        'panel-foreground': fg,
        header: panel,
        toolbar: ui.promptBg || surface,
        link: ui.actionFg,
        button: ui.actionFg,
        'button-foreground': 'hsl(0, 0%, 100%)',
        danger: ui.gitDeleted,
        'danger-foreground': 'hsl(0, 0%, 100%)',
        success: ui.gitUntracked,
        'success-foreground': 'hsl(0, 0%, 100%)',
        warning: ui.gitModified,
        'warning-foreground': fg,
        'table-header': panel,
        'table-border': ui.border || ui.grid,
        'row-alt': ui.intervalBg || ui.selectedBg || panel,
        'form-border': ui.border,
        'form-focus': ui.actionFg,
        'status-neutral': ui.mutedFg,
        'status-active': ui.actionFg,
        'status-done': ui.gitUntracked,
        'status-error': ui.gitDeleted,
        'amount-in': ui.gitUntracked,
        'amount-out': ui.gitDeleted,
    };
}

function deriveTokens(ui) {
    return {
        comment: ui.mutedFg,
        string: ui.intervalFg || ui.weekdayHeader,
        number: ui.actionFg,
        boolean: ui.actionFg,
        keyword: ui.actionFg,
        control: ui.chartHighlight || ui.actionFg,
        operator: ui.mutedFg,
        punctuation: ui.mutedFg,
        type: ui.weekdayHeader || ui.intervalFg,
        typeBuiltin: ui.intervalFg,
        functionDef: ui.listFg || ui.windowFg,
        functionCall: ui.actionFg,
        variable: ui.windowFg,
        parameter: ui.promptFg || ui.windowFg,
        property: ui.weekdayHeader,
        constant: ui.chartHighlight,
        tag: ui.actionFg,
        tagBracket: ui.mutedFg,
        attribute: ui.intervalFg,
        jsxComponent: ui.actionFg,
        cssProperty: ui.gitUntracked,
        cssClass: ui.actionFg,
        cssId: ui.actionFg,
        cssValue: ui.intervalFg,
        escape: ui.quoteGlow || ui.intervalFg,
        heading: ui.actionFg,
        quoteText: ui.quoteText || ui.windowFg,
    };
}

function enforceAll(kind, map, pairs) {
    let fixes = 0;
    const fails = [];
    for (const [name, pal] of Object.entries(map)) {
        const { fixed, failed } = enforcePairs(pal, pairs, MIN_RATIO);
        fixes += fixed.length;
        for (const f of failed) {
            fails.push(`${kind}:${name} ${f}`);
        }
    }
    return { fixes, fails };
}

async function loadPalettes(group, file) {
    const url = pathToFileURL(join(themesRoot, group, file)).href;
    return import(url);
}

function writeGroup(group, ui, tokens, web, erp) {
    const dir = join(themesRoot, group);
    mkdirSync(dir, { recursive: true });
    writeFileSync(
        join(dir, 'ui-palettes.mjs'),
        serializePaletteObject(`${group} UI (UiTheme roles)`, ui, 'uiPalettes'),
    );
    writeFileSync(
        join(dir, 'token-palettes.mjs'),
        serializePaletteObject(`${group} syntax tokens`, tokens, 'tokenPalettes'),
    );
    writeFileSync(
        join(dir, 'web-palettes.mjs'),
        serializePaletteObject(`${group} WorldMan web styleclasses`, web, 'webPalettes'),
    );
    writeFileSync(
        join(dir, 'erp-palettes.mjs'),
        serializePaletteObject(`${group} ERP semantic colors`, erp, 'erpPalettes'),
    );
}

function writeCatalogMd(stem, body) {
    writeFileSync(join(themesRoot, 'catalog', `${stem}.md`), body);
}

function familyMdFromMinor(stem, fam) {
    const lines = [
        `# ${stem}`,
        '',
        `group: ${fam.group}`,
        '',
        '## Meaning / 含义',
        '',
        fam.meaning_en,
        '',
        fam.meaning_zh,
        '',
    ];
    for (const v of fam.variants) {
        const title = v.type === 'dark' ? `${v.label}` : v.label;
        lines.push(`## ${title}`, '', '### English', '', v.en, '', '### 中文', '', v.zh, '');
    }
    lines.push('## Variants', '');
    for (const v of fam.variants) {
        lines.push(
            `- \`${v.id}\` — ${v.label} (\`${v.type}\`), paletteKey \`${v.paletteKey}\` in \`${fam.group}/\``,
        );
    }
    lines.push('');
    return lines.join('\n');
}

function familyMdFromCountry(stem, c) {
    const loc =
        c.locales && c.locales.length
            ? c.locales.join(', ')
            : '(extended; not required by zfr L2)';
    const lines = [
        `# ${stem}`,
        '',
        'group: country',
        `locales: ${loc}`,
        '',
        '## Meaning / 含义',
        '',
        c.meaning_en,
        '',
        c.meaning_zh,
        '',
        `## Country: ${stem} (light) / 浅色`,
        '',
        '### English',
        '',
        c.light_en,
        '',
        '### 中文',
        '',
        c.light_zh,
        '',
        `## Country: ${stem} (dark) / 深色`,
        '',
        '### English',
        '',
        c.dark_en,
        '',
        '### 中文',
        '',
        c.dark_zh,
        '',
        '## Variants',
        '',
        `- \`country-${c.slug}\` — Country: ${stem} (\`light\`), paletteKey \`${c.lightKey}\` in \`country/\``,
        `- \`dark-country-${c.slug}\` — Country: ${stem} (Dark) (\`dark\`), paletteKey \`${c.darkKey}\` in \`country/\``,
        '',
    ];
    return lines.join('\n');
}

console.log('1/4 extract from minor-themes…');
const ex = spawnSync(
    process.execPath,
    [join(__dirname, 'extract-worldman-themes.mjs'), MT],
    { stdio: 'inherit' },
);
if (ex.status !== 0) {
    process.exit(ex.status || 1);
}

console.log('2/4 merge L2 countries + rebuild web/erp with contrast…');
const minorUiMod = await loadPalettes('minor', 'ui-palettes.mjs');
const vibeUiMod = await loadPalettes('vibe', 'ui-palettes.mjs');
const countryUiMod = await loadPalettes('country', 'ui-palettes.mjs');
const minorTok = (await loadPalettes('minor', 'token-palettes.mjs')).tokenPalettes;
const vibeTok = (await loadPalettes('vibe', 'token-palettes.mjs')).tokenPalettes;
const countryTok = (await loadPalettes('country', 'token-palettes.mjs')).tokenPalettes;

const countryUi = {
    ...countryUiMod.uiPalettes,
    ...l2ExtraUiPalettes,
    ...l3ExtraUiPalettes,
};
for (const [k, ui] of Object.entries({ ...l2ExtraUiPalettes, ...l3ExtraUiPalettes })) {
    countryTok[k] = deriveTokens(ui);
}

function buildWebErp(uiMap) {
    const web = {};
    const erp = {};
    for (const [k, ui] of Object.entries(uiMap)) {
        web[k] = mapWeb(ui);
        erp[k] = mapErp(ui);
    }
    return { web, erp };
}

const groups = {
    minor: {
        ui: { ...minorUiMod.uiPalettes },
        tokens: { ...minorTok },
        ...buildWebErp(minorUiMod.uiPalettes),
    },
    vibe: {
        ui: { ...vibeUiMod.uiPalettes },
        tokens: { ...vibeTok },
        ...buildWebErp(vibeUiMod.uiPalettes),
    },
    country: {
        ui: countryUi,
        tokens: countryTok,
        ...buildWebErp(countryUi),
    },
};

const allFails = [];
let totalFixes = 0;
for (const [g, pack] of Object.entries(groups)) {
    let r = enforceAll(`${g}/ui`, pack.ui, UI_FG_BG_PAIRS);
    totalFixes += r.fixes;
    allFails.push(...r.fails);
    /* rebuild web/erp from contrast-fixed ui */
    const we = buildWebErp(pack.ui);
    pack.web = we.web;
    pack.erp = we.erp;
    r = enforceAll(`${g}/web`, pack.web, WEB_FG_BG_PAIRS);
    totalFixes += r.fixes;
    allFails.push(...r.fails);
    r = enforceAll(`${g}/erp`, pack.erp, ERP_FG_BG_PAIRS);
    totalFixes += r.fixes;
    allFails.push(...r.fails);
    writeGroup(g, pack.ui, pack.tokens, pack.web, pack.erp);
}
console.log(`   contrast adjustments: ${totalFixes}`);
if (allFails.length) {
    console.warn('   remaining failures:', allFails);
    process.exitCode = 1;
}

console.log('3/4 write bilingual catalogs…');
mkdirSync(join(themesRoot, 'catalog'), { recursive: true });
/* remove old catalog md then rewrite */
for (const f of readdirSync(join(themesRoot, 'catalog'))) {
    if (f.endsWith('.md')) {
        writeFileSync(join(themesRoot, 'catalog', f), ''); // will overwrite
    }
}

const catalogRows = [
    ['id', 'label', 'type', 'group', 'family', 'paletteKey', 'catalog', 'locales'],
];

for (const [stem, fam] of Object.entries(CATALOG)) {
    writeCatalogMd(stem, familyMdFromMinor(stem, fam));
    for (const v of fam.variants) {
        catalogRows.push([
            v.id,
            v.label,
            v.type,
            fam.group,
            stem,
            v.paletteKey,
            `catalog/${stem}.md`,
            '',
        ]);
    }
}

for (const [stem, c] of Object.entries(COUNTRY_CATALOG)) {
    writeCatalogMd(stem, familyMdFromCountry(stem, c));
    const locs = (c.locales || []).join('|');
    catalogRows.push([
        `country-${c.slug}`,
        `Country: ${stem}`,
        'light',
        'country',
        stem,
        c.lightKey,
        `catalog/${stem}.md`,
        locs,
    ]);
    catalogRows.push([
        `dark-country-${c.slug}`,
        `Country: ${stem} (Dark)`,
        'dark',
        'country',
        stem,
        c.darkKey,
        `catalog/${stem}.md`,
        locs,
    ]);
}

writeFileSync(
    join(themesRoot, 'catalog.tsv'),
    catalogRows.map((r) => r.join('\t')).join('\n') + '\n',
);

console.log('4/4 docs + L2 coverage check…');
const covered = new Set();
for (const c of Object.values(COUNTRY_CATALOG)) {
    for (const loc of c.locales || []) {
        covered.add(loc);
    }
}
const required = [...TIER_I, ...TIER_II, ...TIER_III];
const missing = required.filter((l) => !covered.has(l));
const localeMapLines = Object.entries({ ...L2_LOCALE_TO_COUNTRY, ...L3_LOCALE_TO_COUNTRY })
    .map(([loc, fam]) => `- \`${loc}\` → ${fam}`)
    .join('\n');

writeFileSync(
    join(themesRoot, 'ELEMENTS.md'),
    `# WorldMan theme elements

Color values live in grouped \`.mjs\` palettes (like minor-themes).

\`\`\`text
themes/
  catalog/          # bilingual color-language prose (en + 中文)
  minor/            # soft / pride
  vibe/             # media / retro
  country/          # cultural country palettes (zfr L2 + extended)
  lib/color-utils.mjs
  catalog.tsv       # includes locales column
\`\`\`

Each of \`minor/\`, \`vibe/\`, \`country/\`:

| File | Purpose |
|------|---------|
| \`ui-palettes.mjs\` | UiTheme-style roles |
| \`token-palettes.mjs\` | Syntax tokens |
| \`web-palettes.mjs\` | WorldMan web styleclasses (SOP 011) |
| \`erp-palettes.mjs\` | ERP semantic colors |

## Web primaries

background, foreground, primary, primary-foreground, card, card-foreground,
accent, accent-foreground, muted-foreground, border, ring, destructive, success, warning

## ERP primaries

page, page-foreground, panel, panel-foreground, header, toolbar, link,
button, button-foreground, danger, danger-foreground, success, success-foreground,
warning, warning-foreground, table-header, table-border, row-alt, form-border,
form-focus, status-neutral, status-active, status-done, status-error,
amount-in, amount-out

## Contrast / 色彩可分

Semantic foreground/background pairs **must** meet contrast ≥ ${MIN_RATIO}:1
(WCAG-style ratio; same method as minor-themes \`ensureContrast\`).

Checked pairs (see \`lib/color-utils.mjs\`):

- UI: windowFg/windowBg, windowFg/surfaceBg, listFg/windowBg, promptFg/promptBg, …
- Web: foreground/background, card-foreground/card, primary-foreground/primary, …
- ERP: page-foreground/page, panel-foreground/panel, button-foreground/button, …

\`\`\`bash
node tools/rebuild-worldman-themes.mjs
node tools/check-theme-contrast.mjs
\`\`\`

## zfr L2 country coverage

Tier I + II locales map to country families:

${localeMapLines}

Missing after rebuild: ${missing.length ? missing.join(', ') : '(none)'}.
`,
);

writeFileSync(
    join(themesRoot, 'README.md'),
    `# WorldMan themes

\`\`\`text
themes/
  index.html          # demo: ui / web / token / erp scenes
  demo.js / demo-data.js
  catalog/            bilingual Meaning (en + 中文)
  minor|vibe|country/ ui|token|web|erp-palettes.mjs
  lib/color-utils.mjs
  catalog.tsv
\`\`\`

- \`000\` → \`$PROJECT/sop/themes/\`
- \`011\` may use **all** class sets (ui / web / token / erp) by real semantics
- Country packs cover **zfr L2 + L3** (Tier I–III) locales

Preview:

\`\`\`bash
cd suite/worldman/themes && python3 -m http.server 8765
# open http://127.0.0.1:8765/
\`\`\`

Rebuild:

\`\`\`bash
node tools/rebuild-worldman-themes.mjs [/path/to/minor-themes]
node tools/check-theme-contrast.mjs
\`\`\`
`,
);

/* Embed palette snapshot for the static demo (index.html). */
const demoPayload = {
    generated: new Date().toISOString(),
    catalog: catalogRows.slice(1).map((r) => ({
        id: r[0],
        label: r[1],
        type: r[2],
        group: r[3],
        family: r[4],
        paletteKey: r[5],
        catalog: r[6],
        locales: r[7] || '',
    })),
    groups: {
        minor: {
            ui: groups.minor.ui,
            token: groups.minor.tokens,
            web: groups.minor.web,
            erp: groups.minor.erp,
        },
        vibe: {
            ui: groups.vibe.ui,
            token: groups.vibe.tokens,
            web: groups.vibe.web,
            erp: groups.vibe.erp,
        },
        country: {
            ui: groups.country.ui,
            token: groups.country.tokens,
            web: groups.country.web,
            erp: groups.country.erp,
        },
    },
};
writeFileSync(
    join(themesRoot, 'demo-data.js'),
    `/* Auto-generated by tools/rebuild-worldman-themes.mjs — do not edit. */\n` +
        `export const DEMO = ${JSON.stringify(demoPayload)};\n`,
);

console.log(
    `done: ${catalogRows.length - 1} variants; L2+L3 missing=${missing.length ? missing.join(',') : 'none'}`,
);
if (missing.length) {
    process.exitCode = 1;
}
